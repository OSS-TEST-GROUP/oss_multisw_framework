# SiLS FMU Plant (FMI Co-Simulation) 가이드

IPlant 포트의 세 번째 구현 **FmuPlant** — 외부 물리 모델(FMU)을 vECU 폐루프에
결합하는 방법·현황·주의사항. 아키텍처 v3 Final **§11 L3(Plant-coupled Fidelity)**,
IPlant 포트 **§6**, 시간 소유권 **§8**, 상태 소유권 **§4.1/§19**.

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

`.fmu`(ZIP) 내부:
```
vehicle_plant.fmu
├─ modelDescription.xml          # 변수·valueReference·causality·단위·초기값
├─ binaries/
│  ├─ x86_64-linux/<model>.so    # 플랫폼별 native 바이너리
│  ├─ aarch64-linux/<model>.so   # ← ARM64 컨테이너에 필수 (아래 §5.1)
│  └─ win64/<model>.dll
├─ resources/
└─ documentation/
```

fmi4c가 하는 일: FMU 로드 → 압축 해제 → `modelDescription.xml` 파싱 → FMI 버전/
인터페이스 확인 → 플랫폼 `.so` dlopen → instance 생성 → init → (매 tick) set input /
doStep / get output → terminate/free. **하지 않는 일**: FMU 저작, 다중 FMU 실행 순서,
DDS/ROS2 연결, 단위·signal 변환, Clock Master, golden 비교, 실패 판정, lifecycle —
이건 전부 **SiLS(우리)** 몫.

---

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
| Descriptor 기반 변수·단위 매핑 | ⬜ 미구현 (현재 raw valueReference, §5.2) |
| ARM64에서 실 FMU 실행 | ⬜ 미검증 (fmi4c 빌드는 arch 중립; FMU 바이너리 별개, §5.1) |

**검증 예시 (Feedthrough, FMI 3.0, p-controller Kp=0.8 setpoint=10):**
출력=입력 통과 → `u_n = 0.8·(10 − u_{n-1})` → `0, 8, 1.6, 6.72, 2.624, 5.9008 … → 4.44` 수렴.

---

## 5. 도입 시 주의사항 (검토 반영)

### 5.1 ARM64 바이너리 — fmi4c 빌드 ≠ FMU 실행
fmi4c가 ARM64로 컴파일돼도 **FMU 안에 `binaries/aarch64-linux/*.so`가 없으면** ARM64
vECU 컨테이너에서 그 FMU를 실행할 수 없다(x86_64 바이너리만 든 FMU는 불가). FMU 공급 시
**대상 아키텍처 바이너리 포함** 또는 **소스 동봉(빌드 가능)**을 계약으로 요구해야 한다.

### 5.2 변수 매핑 — 현재 raw VR, 향후 Descriptor
현재 `FmuPlant`는 입력/출력을 **raw valueReference**로 지정한다:
`SILS_FMU_INPUT_VR` / `SILS_FMU_OUTPUT_VR`. VR은 `modelDescription.xml`에서 확인:
```bash
unzip -p model.fmu modelDescription.xml | grep -iE 'valueReference|<Float64|causality'
```
> **주의:** `FmuPlant`는 `Float64`(`getFloat64`/`setFloat64`)만 다룬다. `Float32`·정수형
> 변수는 그 VR을 쓰면 안 된다.

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
| 필수 output 누락 / sim_time 불일치 | ⬜ 미검사(향후 Descriptor 도입 시) |

### 5.5 MORAI와 상태 소유권 (중복 plant 금지)
FMU와 MORAI가 **동시에 같은 차량 동역학**을 계산하면 안 된다(§4.1/§19). 택일:
- **방식 A (MORAI가 plant)**: `command → MORAI → vehicle status`. 이 경우 FMU 기반 plant는
  중복 → vECU는 `plant:{type:none}` 순수 컨트롤러, MORAI 상태를 DDS로 수신.
- **방식 B (FMU가 plant)**: `command → FmuPlant → vehicle status`, MORAI엔 pose만 주입해
  시각화/센서 렌더링.

현재 계획대로 MORAI가 차량 상태 authority면 FMU는 **미사용**하거나 MORAI가 소유하지 않는
보조 모델(타이어·브레이크 등)로 범위를 제한한다. (FMU 트랙 vs MORAI 브리지 트랙은 별개 —
[README](../tools/sils/README.md) "남은 것" 참조.)

### 5.6 보안 — FMU는 native code
`.fmu` 안의 `.so`/`.dll`은 실제 native 코드다. **신뢰하지 않는 FMU 로드 = 임의 코드 실행
위험.** 권장: 별도 컨테이너, read-only FS, 네트워크 제한, CPU/메모리/timeout 제한, FMU
**hash·공급자 기록**, **허용된 FMU만** 실행(allowlist).

---

## 6. 실행 레시피

```bash
# 1) FMU 지원 빌드 (fmi4c 정적 링크; 없으면 FetchContent 로 clone → 빌드 컨테이너 네트워크 필요)
SILS_ENABLE_FMU=ON ./build.sh --build-sils          # (+ --aarch64 등)

# 2) 샘플 FMU 확보 (Modelica Association Reference-FMUs)
curl -LO https://github.com/modelica/Reference-FMUs/releases/download/v0.0.40/Reference-FMUs-0.0.40.zip
unzip Reference-FMUs-0.0.40.zip -d ref-fmus
unzip -p ref-fmus/3.0/Feedthrough.fmu modelDescription.xml | grep -iE 'valueReference|causality'
#   → Float64_continuous_input=VR7(input), Float64_continuous_output=VR8(output)

# 3) lockstep 폐루프 실행 (결정론)
RID=fmu; DOM=93; BIN=<out>/bin; FMU=$PWD/ref-fmus/3.0/Feedthrough.fmu
SILS_RUN_ID=$RID VISS_DDS_DOMAIN_ID=$DOM \
  $BIN/sils_clock_master --participants 1 --ticks 20 --dt-ms 100 --tick-interval-ms 40 &
SILS_RUN_ID=$RID VISS_DDS_DOMAIN_ID=$DOM SILS_VECU_INSTANCE_ID=viss-probe-1 \
  SILS_CLOCK_MODE=lockstep SILS_BEHAVIOR=p-controller \
  SILS_PLANT=fmu SILS_FMU_FILE=$FMU SILS_FMU_INPUT_VR=7 SILS_FMU_OUTPUT_VR=8 \
  $BIN/sils_vecu_viss_probe
SILS_RUN_ID=$RID VISS_DDS_DOMAIN_ID=$DOM $BIN/sils_vecu_viss_verify
```
기대: `plant.model="fmu"`, `fmi_version="3.0"`, `closed_loop=true`, `deterministic=true`,
**fidelity=L3**. 두 번 돌려 `capture_golden`→`compare_golden` MATCH로 재현성 봉인.

---

## 7. 후속 우선순위

1. ✅ FMI 3.0 Co-Simulation Reference FMU 실행 · FmuPlant · lockstep dt=FMU time · step 실패/NaN fail-fast · golden 재현
2. ⬜ **ARM64에서 실 FMU 실행** 검증(aarch64 바이너리 포함 FMU)
3. ⬜ **Descriptor 기반 변수·단위·causality 매핑**(§5.2) + 검증
4. ⬜ FMI 2.0 Co-Simulation 실 FMU 실측
5. ⬜ 실 차량 동역학 FMU(Simulink/Dymola/AMESim export)로 상관검증(model correlation)
6. ⬜ FMU 보안 샌드박싱(§5.6)
7. ⬜ (검토 후) FMI 3.0 Scheduled Execution

**요약:** fmi4c는 vECU가 아니라 **vECU가 외부 물리 모델 FMU를 실행하도록 해주는 importer
어댑터**다. `FmuPlant`로 IPlant 뒤에 들어가며, 현재 FMI 3.0 Co-Simulation이 실 FMU로
L3까지 실증됐다.
