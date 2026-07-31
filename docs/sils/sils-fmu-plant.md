# SiLS FMU Plant (FMI Co-Simulation) 가이드

IPlant 포트의 세 번째 구현 **FmuPlant** — 외부 물리 모델(FMU)을 vECU 폐루프에
결합하는 방법·현황·주의사항. 아키텍처 v3 Final **§11 L3(Plant-coupled Fidelity)**,
IPlant 포트 **§6**, 시간 소유권 **§8**, 상태 소유권 **§4.1/§19**.

> **FMU vs MORAI/AWSIM — 다른 결합.** FmuPlant 는 **in-process** 폐루프다(tick 루프가 직접
> `Step`). 시뮬레이터(MORAI/AWSIM) 연동은 `IPlant` 가 아니라 **버스 결합**(Option 2)으로 별도
> 경로다 — [sils-morai-plant-adapter-design.md](sils-morai-plant-adapter-design.md) 참조.

> 📊 **시각 요약**: [sils-fmu-plant.html](sils-fmu-plant.html) — FMI/FMU/fmi4c 구분,
> IPlant 드롭인, 폐루프 다이어그램, Feedthrough 수렴 궤적, 현황·주의를 한 페이지로.

---

## 1. 용어: FMI / FMU / fmi4c

| 이름 | 정체 | 이 프로젝트에서 |
|---|---|---|
| **FMI** | Functional Mock-up **Interface** — 모델 실행 표준 API 규격(1.0/2.0/3.0) | 우리가 준수하는 규격 |
| **FMU** | FMI 규격으로 포장된 실제 모델 파일(`.fmu`, ZIP) | **외부 산출물** — 우리가 만들지 않음 |
| **fmi4c** | FMU를 C/C++에서 로드·실행하게 해주는 경량 C 라이브러리("FMI **for** C") | `FmuPlant`가 쓰는 importer |

> fmi4c는 "FMI 4"가 아니라 "FMI-for-C". FMI 1/2/3의 Model Exchange / Co-Simulation /
> Scheduled Execution import를 지원하는 오픈소스 C 라이브러리.

표준과 도구 지원 범위는 [FMI 공식 사양](https://fmi-standard.org/docs/3.0.2/)과
[FMI 공식 도구 목록의 fmi4c 항목](https://fmi-standard.org/tools/)을 기준으로 한다. 단,
도구 목록의 지원 표시는 라이브러리 차원의 기능 범위이며 이 프로젝트의 검증 완료 범위를
의미하지 않는다. 실제 승인 범위는 §4의 실측 표로 한정한다.

`.fmu`(ZIP) 내부:
```
vehicle_plant.fmu
├─ modelDescription.xml          # 변수·valueReference·causality·단위·초기값
├─ binaries/
│  ├─ <platform-tuple>/<model>   # FMI 버전별 디렉터리 명명은 §5.1 참조
│  └─ ...
├─ resources/
└─ documentation/
```

fmi4c가 하는 일: FMU 로드 → 압축 해제 → `modelDescription.xml` 파싱 → FMI 버전/
인터페이스 확인 → 플랫폼 `.so` dlopen → instance 생성 → init → (매 tick) set input /
doStep / get output → terminate/free. **하지 않는 일**: FMU 저작, 다중 FMU 실행 순서,
DDS/ROS2 연결, 단위·signal 변환, Clock Master, golden 비교, 실패 판정, lifecycle —
이건 전부 **SiLS(우리)** 몫.

---

> **FMU는 역할 중립 소켓** — 안에 무엇을 넣느냐로 역할이 갈린다. 이 문서는 **FMU = 물리 모델(plant)**,
> 즉 `IPlant` 뒤의 `FmuPlant`(입력 = 제어 u, 출력 = 상태 y). FMU에 **ECU 제어로직**을 넣으면(V-ECU)
> `IVehicleBehavior` 뒤의 `FmuBehavior`가 되어 **버스결합 경로에서 vECU 자체로 동작**한다
> (개발자 연동 Case 3, 입력 = 상태, 출력 = 제어명령). 즉 "ECU를 FMU로"와 "플랜트가 FMU"는
> 방향이 반대인 **다른 역할 · 다른 포트**다.

## 2. SiLS에서의 위치 — IPlant 뒤 어댑터

fmi4c는 최상위 컴포넌트가 아니라 **IPlant 뒤 구현체**다. tick 루프·evaluate·golden은
전혀 바뀌지 않는다.

```
[IBehavior / 제어 알고리즘]
        │ command
        ▼
     [IPlant]
        ├─ AnalyticPlant  (손수 쓴 1차 모델, live)      ✅
        ├─ ReplayPlant    (녹화 재생)                    ✅
        └─ FmuPlant       (fmi4c → 실 .fmu, FMI 2.0/3.0) ✅  ← 이 문서
```

포트가 FMI Co-Simulation과 1:1 대응하므로 실 FMU가 드롭인된다:

| IPlant | FMI 2.0 | FMI 3.0 |
|---|---|---|
| `SetInputs(command)` | `fmi2_setReal` | `fmi3_setFloat64` |
| `Step(dt_ns)` | `fmi2_doStep(t, dt, …)` | `fmi3_doStep(t, dt, …)` |
| `output()` | `fmi2_getReal` | `fmi3_getFloat64` |

매 tick:
```
TickGrant(sim_time, dt) 또는 wall-clock tick
  → 제어 알고리즘 실행(feedback = 직전 output)
  → FmuPlant.SetInputs(command)
  → FmuPlant.Step(dt)          # doStep(time_, dt); time_ += dt
  → FmuPlant.output()          # 다음 tick feedback
  → plant_step{feedback_in,command,output} 방출 → TickComplete
```
`plant_step` 계약은 AnalyticPlant와 **동일**하므로 `analyze_plant`(L3 판정)·
`compare_golden`(model correlation, FP tolerance)이 그대로 판정한다.

---

## 3. 실행 방식: Co-Simulation 채택

FMI 실행 방식 3종 중 **Co-Simulation**을 쓴다:

| 방식 | Solver 소유 | SiLS 적용 | 현황 |
|---|---|---|---|
| Model Exchange | importer(우리) | solver 직접 관리 → 무거움 | 미지원 |
| **Co-Simulation** | **FMU 내부** | `IPlant::Step(dt)`와 최적 | **채택·구현** |
| Scheduled Execution | 외부 scheduler | FMI 3.0 task/runnable에 적합, 복잡 | 미지원(후속) |

`FmuPlant`는 `fmi2_instantiate(…, fmi2CoSimulation, …)` / `fmi3_instantiateCoSimulation(…)`
으로 CS 인스턴스만 만든다.

---

## 4. 현황 — 무엇이 검증됐나 (2026-07, `feature/sils-vecu-fmu-plant`)

> ⚠️ **"fmi4c가 FMI 3.0을 지원한다" ≠ "우리가 필요한 FMI 3.0 기능이 검증됐다".**
> 아래는 실제로 실행·판정한 범위만 적는다.

| 항목 | 상태 |
|---|---|
| FmuPlant 어댑터 (IPlant 3번째 구현) | ✅ 구현 (`plant.hpp`, `SILS_HAVE_FMU` 가드) |
| FMI 버전 자동 감지(2.0/3.0) | ✅ 구현 |
| **FMI 3.0 Co-Simulation 실 FMU 실행** | ✅ **실증** (Reference-FMUs `Feedthrough.fmu`) |
| L3 폐루프 판정 | ✅ PASSED, `closed_loop=true`, `deterministic=true` |
| 재현성 | ✅ 2회 독립 실행 궤적 bit 동일 + `compare_golden` model-correlation MATCH |
| 빌드 | ✅ `-DSILS_ENABLE_FMU=ON`, fmi4c **정적 링크**(런타임 `.so` 의존 없음) |
| FMI 2.0 Co-Simulation | ⬜ 코드 경로 있음, **실 FMU 미실측** |
| Model Exchange / Scheduled Execution | ⬜ 미지원 |
| 변수 이름→VR 해석·검증 (fmu_map.py) | ✅ FMI 2.0/3.0, causality·type·unit 검증 (§5.2) |
| Descriptor 통합 (계약이 이름으로 바인딩) | ⬜ 미구현 (도구는 있음; 스키마 배선 남음) |
| ARM64에서 실 FMU 실행 | ⬜ 미검증 (fmi4c 빌드는 arch 중립; FMU 바이너리 별개, §5.1) |
| FMI set/get 반환 status 검사 | ⬜ 미구현 (`doStep`만 status 검사) |
| FMU reset/re-initialize | ⬜ 미구현 (`FmuPlant::Reset()`은 현재 no-op) |
| FMI 3.0 Event Mode / early return | ⬜ 미지원(기본 CS step만 검증) |
| fmi4c dependency pinning | ⬜ 미구현(`GIT_TAG main`, 재현 가능한 commit/tag 고정 필요) |
| FMU 보안 게이트 (allowlist·hash) | ✅ `fmu_gate.py` — 로드 전 sha256·allowlist 검증(§5.6), Run Coordinator 프리플라이트 |
| FMU/fmi4c provenance manifest | 🟨 부분 — 게이트가 file·sha256·fmi_version·model_identifier 기록(§5.10); binary/parameter 하위 hash·importer commit은 미구현 |

**검증 예시 (Feedthrough, FMI 3.0, p-controller Kp=0.8 setpoint=10):**
출력=입력 통과 → `u_n = 0.8·(10 − u_{n-1})` → `0, 8, 1.6, 6.72, 2.624, 5.9008 … → 4.44` 수렴.

---

## 5. 도입 시 주의사항 (검토 반영)

### 5.1 ARM64 바이너리 — fmi4c 빌드 ≠ FMU 실행
fmi4c가 ARM64로 컴파일돼도 **FMU 안에 `binaries/aarch64-linux/*.so`가 없으면** ARM64
vECU 컨테이너에서 그 FMU를 실행할 수 없다(x86_64 바이너리만 든 FMU는 불가). FMU 공급 시
**대상 아키텍처 바이너리 포함** 또는 **소스 동봉(빌드 가능)**을 계약으로 요구해야 한다.

플랫폼 디렉터리 이름은 FMI 버전에 따라 다르므로 FMU 공급 계약에서 버전과 tuple을 함께
확인한다.

| FMI 버전 | Linux x86-64 예 | Linux ARM64 예 | 비고 |
|---|---|---|---|
| FMI 2.0 | `binaries/linux64/` | exporter별 지원 확인 필요 | legacy platform tuple 사용 |
| FMI 3.0 | `binaries/x86_64-linux/` | `binaries/aarch64-linux/` | `<machine>-<system>` tuple 사용 |

따라서 `aarch64-linux` 디렉터리 확인만으로 FMI 2.0 FMU까지 판정하면 안 된다. 최종 검증은
fmi4c가 선택한 바이너리를 ARM64 컨테이너에서 실제 `instantiate → initialize → doStep`하여
확인한다.

### 5.2 변수 매핑 — 이름으로 해석(fmu_map.py), 향후 Descriptor 통합
`FmuPlant`는 입력/출력을 **raw valueReference**(`SILS_FMU_INPUT_VR`/`SILS_FMU_OUTPUT_VR`)로
받지만, XML을 손으로 읽어 정수를 붙이는 대신 **`tools/sils/fmu/fmu_map.py`가 변수
이름→VR을 해석·검증**한다(causality/type/unit). FMI 2.0(ScalarVariable/Real)·3.0
(Float64) 모두 지원하고, `Float64/Real`이 아니거나(Float32·정수 등) causality가 안 맞으면
거부한다:
```bash
# .fmu 안 modelDescription 을 읽어 이름으로 VR 해석 → 그대로 env 에 eval
eval "$(python3 tools/sils/fmu/fmu_map.py --fmu model.fmu \
          --input Float64_continuous_input --output Float64_continuous_output)"
#   -> RESOLVED_FMU_INPUT_VR=7  RESOLVED_FMU_OUTPUT_VR=8  RESOLVED_FMU_FMI_VERSION='3.0'
SILS_FMU_INPUT_VR=$RESOLVED_FMU_INPUT_VR SILS_FMU_OUTPUT_VR=$RESOLVED_FMU_OUTPUT_VR ...
# 단위 assertion 도 가능: --expect-output-unit m/s  (불일치 시 fail)
```
> **주의:** `FmuPlant`는 `Float64`/`Real`(`getFloat64`/`setFloat64`)만 다룬다. `Float32`·
> 정수형 변수는 fmu_map 이 거부한다.

**향후(Descriptor 매핑)**: DDS Contract ↔ FMU 변수를 이름으로 연결하고 아래를 검증:
변수명·VR, input/output causality, 데이터 타입, **단위**, 초기값, min/max, sample period,
**좌표계·부호 방향**. (목표 스키마 예:)
```yaml
plant:
  type: fmu
  fmi_version: "2.0"        # 또는 3.0
  fmu: models/vehicle_dynamics.fmu
  step_size_ms: 20
  inputs:  { brake_command: Vehicle.Brake.Command }
  outputs: { velocity: Vehicle.Speed, brake_pressure: Vehicle.Brake.Pressure }
```

### 5.3 시간 소유권 — sim_time == FMU time
FMU가 임의 wall-clock을 쓰게 두면 안 된다. `FmuPlant.Step`은 lockstep이 준 `dt`로
`doStep(time_, dt)`를 호출하고 `time_ += dt`로 누적 → **SiLS sim_time과 FMU
current time 일치**. lockstep(§8)에서 결정론·재현성이 성립하는 이유.

### 5.4 실패 정책 (fail-fast, §14) — 현재 커버
| 상황 | 현재 처리 |
|---|---|
| FMU load 실패 / platform 바이너리 누락 | `fmi4c_loadFmu` 실패 → 에러, probe exit 2 |
| FMI 버전 불일치(2.0/3.0 외) | `unsupported FMI version` → probe exit 2 |
| instantiate / 초기화 실패 | 에러 → probe exit 2 |
| doStep 오류 / dt 거부 | 비-OK status → **출력 NaN** → `analyze_plant` non-finite fail |
| NaN/Inf 출력 | NaN → `analyze_plant` fail-fast |
| 미컴파일 상태에서 `SILS_PLANT=fmu` | 명확한 에러로 fail-fast |
| set input / get output 오류 | ⬜ 현재 반환 status 미검사 — 반드시 보강 필요 |
| FMI 3 early return / terminate 요청 | ⬜ 현재 결과 flag 미처리 — 지원 전 명시적 거부 필요 |
| 필수 output 누락 / sim_time 불일치 | ⬜ 미검사(향후 Descriptor 도입 시) |

### 5.5 MORAI와 상태 소유권 (중복 plant 금지)
FMU와 MORAI가 **동시에 같은 차량 동역학**을 계산하면 안 된다(§4.1/§19). 택일:
- **방식 A (MORAI가 plant)**: `command → MORAI → vehicle status`. 이 경우 FMU 기반 plant는
  중복 → vECU는 `plant:{type:none}` 순수 컨트롤러, MORAI 상태를 DDS로 수신.
- **방식 B (FMU가 plant)**: `command → FmuPlant → vehicle status`, MORAI엔 pose만 주입해
  시각화/센서 렌더링.

현재 계획대로 MORAI가 차량 상태 authority면 FMU는 **미사용**하거나 MORAI가 소유하지 않는
보조 모델(타이어·브레이크 등)로 범위를 제한한다. (FMU 트랙 vs MORAI 브리지 트랙은 별개 —
[README](../../tools/sils/README.md) "남은 것" 참조.)

### 5.6 보안 — FMU는 native code
`.fmu` 안의 `.so`/`.dll`은 실제 native 코드다. **신뢰하지 않는 FMU 로드 = 임의 코드 실행
위험.** 권장: 별도 컨테이너, read-only FS, 네트워크 제한, CPU/메모리/timeout 제한, FMU
**hash·공급자 기록**, **허용된 FMU만** 실행(allowlist).

**구현: FMU 보안 게이트 (`tools/sils/fmu/fmu_gate.py`).** allowlist·hash 검증을
FmuPlant가 파일을 열기 **전에** Run Coordinator가 수행한다(프리플라이트).

- **allowlist**: `config/sils/fmu/allowlist.yaml` — 검증된 FMU만 등록. 각 항목은
  `id`·`sha256`(파일 전체 바이트)·`supplier`(출처/검증자)·`fmi_version`·`model_identifier`·
  `platforms`. 등록·해시는 `--record`로 stanza를 생성해 붙여넣는다.
- **fail-closed**: allowlist에 없거나(해시 불일치·변조·파일 없음), allowlist가 아예 없으면
  **거부(exit 1)** → Run Coordinator가 launch 하지 않는다.
- **이름 바인딩(`--expect-id`)**: 토폴로지가 특정 항목을 지정하면, 그 id에 **승인된 바이트와
  정확히 일치**해야 통과 — allowlist된 다른 FMU가 대신 실행되는 것을 차단.
- **provenance(§5.10)**: 통과 시 `file`·`sha256`·`fmi_version`·`model_identifier`·`size`·
  allowlist `id`/`supplier`를 기록(`--manifest`/`--json`).
- **드리프트 경고**: allowlist 메타데이터가 (해시 검증된) modelDescription과 다르면
  **경고만**(바이트가 실행 대상이므로 차단하지 않음).

```bash
# 신뢰하는 FMU 등록용 stanza 생성 → allowlist.yaml 에 붙여넣기
python3 tools/sils/fmu/fmu_gate.py --fmu Feedthrough.fmu --record
# 로드 전 게이트(Run Coordinator 가 SILS_FMU_FILE 설정 시 자동 실행)
python3 tools/sils/fmu/fmu_gate.py --fmu Feedthrough.fmu --expect-id reference-feedthrough
```

Run Coordinator는 `SILS_FMU_FILE`가 설정된 run에서 이 게이트를 자동 실행한다
(`SILS_FMU_ALLOWLIST`/`SILS_FMU_ALLOWLIST_ID`로 override). 거부 시 `fmu_gate_denied`
trace 후 teardown. 컨테이너 격리·read-only FS·리소스 제한은 배포/오케스트레이션 계층의
별도 과제로 남는다(게이트는 "어떤 바이트를 실행할지"를 통제).

### 5.7 Co-Simulation step의 의미와 수치 안정성

`doStep(t, dt)`의 `dt`는 단순 호출 주기가 아니라 FMU 사이에 값을 교환하는
**communication step size**다. Co-Simulation FMU는 보통 두 communication point 사이를
내부 solver로 진행하며, SiLS가 solver 내부 계산 순서를 직접 통제하지 않는다. FMI 표준도
Co-Simulation 전체 orchestration algorithm은 표준 범위 밖으로 둔다.

```text
t=N                     t=N+dt
 command set → doStep(dt) → output read
       FMU 내부 solver/상태 진행
```

`dt`가 너무 크면 빠른 상태 변화가 누락되거나 폐루프가 불안정해질 수 있고, 너무 작으면 실행
비용이 커진다. 따라서 Golden 승인 전에 다음을 고정·검증한다.

- FMU가 허용하는 minimum/recommended step size
- Controller tick과 FMU communication step의 비율(기본은 1:1)
- 고정 `dt` 반복 실행의 궤적과 수치 오차
- `dt/2`, `dt`, `2dt` 민감도 시험
- direct feedthrough 및 event가 있는 모델의 입력/출력 시점

Wall-clock 모드의 실행 속도와 FMU의 simulation time은 별개다. 느리게 실행되더라도
`doStep`에는 SiLS가 소유한 `sim_time`과 `dt`만 전달한다.

### 5.8 Lifecycle, Reset, 다중 인스턴스

정상 lifecycle은 다음 순서를 지켜야 한다.

```text
load/unzip → instantiate → initialize → repeated doStep
           → terminate → freeInstance → freeFmu/temp cleanup
```

현재 구현은 생성 시 한 번 초기화하고 `FmuPlant::Reset()`은 아무 작업도 하지 않는다. 따라서
같은 프로세스에서 run을 재시작하며 초기 상태로 복귀하는 기능은 아직 보장하지 않는다. 현재처럼
run마다 vECU 프로세스를 새로 만드는 경우에는 새 instance가 생성되지만, 장기 실행 프로세스나
checkpoint 재시작을 도입하려면 다음 중 하나를 구현해야 한다.

- `reset` 지원 FMU: FMI reset 후 initialization sequence 재실행
- reset 미지원 FMU: instance를 해제하고 새로 instantiate
- rollback 필요 시: FMU capability 확인 후 get/set FMU state 사용

각 vECU instance는 독립 FMU instance와 독립 extraction directory를 소유해야 한다. 현재 다중
vECU는 프로세스가 분리돼 충돌 가능성이 낮지만, 한 프로세스에 여러 `FmuPlant`를 넣는 최적화는
동일 FMU 다중 instance, fmi4c thread-safety와 임시 경로 충돌을 별도 시험하기 전 허용하지 않는다.

### 5.9 FMI 3.0 고급 기능 경계

FMI 3.0 지원이라는 표현을 다음처럼 구분한다.

| 기능 | 현재 범위 |
|---|---|
| 기본 Float64 Co-Simulation `set → doStep → get` | 검증 완료 |
| array/Float32/integer variable | 미지원 |
| Event Mode | 미지원 |
| early return / intermediate update | 미지원 |
| Clocks / clocked variable | 미지원 |
| Scheduled Execution / model partition | 미지원 |

현재 `FmuPlant`는 FMI 3.0 instantiate 시 Event Mode와 early return을 사용하지 않는 기본 경로다.
FMU의 `modelDescription.xml` capability가 현재 지원 범위를 요구하는지 startup validation에서
확인하고, 지원하지 않는 기능을 요구하면 실행 전에 실패시켜야 한다. Hopsan의
[fmi4c 기반 호환성 자료](https://hopsan.github.io/documentation/fmi.html)도 FMI 3.0 Reference
FMU 중 Clock 등 일부 항목을 미통과로 표시하므로 “3.0” 버전 문자열만으로 호환을 승인하지 않는다.

### 5.10 재현성을 위한 Provenance

Golden trace는 FMU 파일만 같다고 재현되는 것이 아니다. run manifest에 다음을 함께 기록한다.

```yaml
fmu:
  file: vehicle_dynamics.fmu
  sha256: "..."
  fmi_version: "3.0"
  model_identifier: "..."
  platform: aarch64-linux
  binary_sha256: "..."
  parameters_sha256: "..."
  input_mapping_sha256: "..."
  step_size_ns: 20000000
importer:
  name: fmi4c
  git_commit: "..."
vecu:
  image_digest: "sha256:..."
```

FMU, parameter, mapping, step size, importer 또는 compiler가 달라지면 기존 Golden과의 차이를
버그로 단정하지 않고 변경 승인 절차를 거친다.

### 5.11 의존성 고정과 공급망

현재 CMake fallback은 fmi4c의 `GIT_TAG main`을 FetchContent로 가져온다. 같은 SiLS commit도
빌드 시점에 따라 서로 다른 importer를 포함할 수 있으므로 release/CI에서는 허용하면 안 된다.

- 검증된 fmi4c commit SHA 또는 release tag로 고정
- dependency source와 license를 SBOM에 기록
- 가능하면 사내 artifact/cache에서 검증된 source archive 공급
- 변경 시 Reference FMU, 실제 Plant FMU, ARM64 실행을 다시 검증
- network 없는 재현 빌드를 지원

---

## 6. 실행 레시피

```bash
# 1) FMU 지원 빌드 (fmi4c 정적 링크; 없으면 FetchContent 로 clone → 빌드 컨테이너 네트워크 필요)
SILS_ENABLE_FMU=ON ./build.sh --build-sils          # (+ --aarch64 등)

# 2) 샘플 FMU 확보 (Modelica Association Reference-FMUs)
curl -LO https://github.com/modelica/Reference-FMUs/releases/download/v0.0.40/Reference-FMUs-0.0.40.zip
unzip Reference-FMUs-0.0.40.zip -d ref-fmus

# 3) 변수 이름으로 VR 해석 (하드코딩 대신, §5.2)
RID=fmu; DOM=93; BIN=<out>/bin; FMU=$PWD/ref-fmus/3.0/Feedthrough.fmu
eval "$(python3 tools/sils/fmu/fmu_map.py --fmu "$FMU" \
          --input Float64_continuous_input --output Float64_continuous_output)"
#   → RESOLVED_FMU_INPUT_VR=7  RESOLVED_FMU_OUTPUT_VR=8

# 4) lockstep 폐루프 실행 (결정론)
SILS_RUN_ID=$RID VISS_DDS_DOMAIN_ID=$DOM \
  $BIN/sils_clock_master --participants 1 --ticks 20 --dt-ms 100 --tick-interval-ms 40 &
SILS_RUN_ID=$RID VISS_DDS_DOMAIN_ID=$DOM SILS_VECU_INSTANCE_ID=viss-probe-1 \
  SILS_CLOCK_MODE=lockstep SILS_BEHAVIOR=p-controller \
  SILS_PLANT=fmu SILS_FMU_FILE=$FMU \
  SILS_FMU_INPUT_VR=$RESOLVED_FMU_INPUT_VR SILS_FMU_OUTPUT_VR=$RESOLVED_FMU_OUTPUT_VR \
  $BIN/sils_vecu_viss_probe
SILS_RUN_ID=$RID VISS_DDS_DOMAIN_ID=$DOM $BIN/sils_vecu_viss_verify
```
기대: `plant.model="fmu"`, `fmi_version="3.0"`, `closed_loop=true`, `deterministic=true`,
**fidelity=L3**. 두 번 돌려 `capture_golden`→`compare_golden` MATCH로 재현성 봉인.

---

## 7. 후속 우선순위

1. ✅ FMI 3.0 Co-Simulation Reference FMU 실행 · FmuPlant · lockstep dt=FMU time · step 실패/NaN fail-fast · golden 재현
2. ⬜ **status/lifecycle 보강**: set/get 반환값, terminate, reset/re-initialize, FMI 3 unsupported capability 거부
3. ⬜ **fmi4c commit 고정과 provenance manifest**(§5.10~§5.11)
4. ⬜ **ARM64에서 실 FMU 실행** 검증(aarch64 바이너리 포함 FMU)
5. 🟡 변수 이름→VR 해석·검증은 `tools/sils/fmu/fmu_map.py`로 완료(§5.2, FMI 2.0/3.0·
   causality·type·unit); **Descriptor/계약이 이름으로 바인딩**하는 스키마 통합은 남음
6. ⬜ communication step 민감도와 controller/FMU rate 계약(§5.7)
7. ⬜ FMI 2.0 Co-Simulation 실 FMU 실측
8. ⬜ 실 차량 동역학 FMU(Simulink/Dymola/AMESim export)로 상관검증(model correlation)
9. 🟡 FMU 보안(§5.6): allowlist·hash 게이트 `fmu_gate.py` 완료(로드 전 검증·provenance 기록·
   Run Coordinator 프리플라이트); **컨테이너 격리·read-only FS·리소스 제한**(배포 계층 샌드박싱)은 남음
10. ⬜ (검토 후) FMI 3.0 Scheduled Execution

**요약:** fmi4c는 vECU가 아니라 **vECU가 외부 물리 모델 FMU를 실행하도록 해주는 importer
어댑터**다. `FmuPlant`로 IPlant 뒤에 들어가며, 현재 FMI 3.0 Co-Simulation이 실 FMU로
L3까지 실증됐다.

---

## 8. 참고 자료

- [FMI 3.0.2 공식 사양](https://fmi-standard.org/docs/3.0.2/)
- [FMI 공식 Tools 목록](https://fmi-standard.org/tools/)
- [fmi4c 소스 저장소](https://github.com/robbr48/fmi4c)
- [Modelica Association Reference FMUs](https://github.com/modelica/Reference-FMUs)
- [Hopsan FMI/fmi4c 호환성 자료](https://hopsan.github.io/documentation/fmi.html)
