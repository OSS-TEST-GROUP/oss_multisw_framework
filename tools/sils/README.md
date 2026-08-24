# SiLS 가상 ECU (vECU) 도구 계층

MMF(oss_multisw_framework) 위에서 **SiLS(Software-in-the-Loop) 가상 ECU** 를 컴파일·실행·판정·회귀검사하는 도구 모음이다. 아키텍처 확정본은 **v3 Final** 문서(`docs/sils/sils-virtual-ecu-architecture-v3.md`)이며, 아래 코드의 `§n` 주석은 모두 그 문서의 절 번호를 가리킨다.

> **검증 상태 (중요).** 이 디렉터리의 Python/shell 도구 계층은 **샌드박스에서 실제 실행·테스트까지 완료**되어 있다(아래 "테스트" 참조). `service/sils/` 의 C++ vECU 본체는 FastDDS 툴체인이 있는 Linux(x86_64)에서 **컴파일 + 실제 DDS 왕복이 검증**되었고, `evaluate.py` 가 자동 부여하는 fidelity **L0·L1·L3 를 실런타임 trace 로 판정 완료**(2026-07 실측): local·docker 두 백엔드 `run_pass`, 병렬 격리·Fleet(vECU 3개), lockstep 결정론(§8), 입력 스테이징·replay(§7), 행동 상태기계 동작, **플랜트 폐루프(§11 L3, AnalyticPlant)를 두 번 독립 실행해 sim_time·궤적이 소수점까지 재현**(compare_golden MATCH). 실행이라 드러난 결함 3건도 수정: docker `--ipc=host`, late-join verifier 용 `TRANSIENT_LOCAL`, lockstep golden 의 벽시계 latency 스킵. 도구 계층은 바이너리 없이도 dry-run·사전 캡처 trace 로 전 구간을 검증할 수 있게 설계되었다.
>
> **L2 는 자동 판정되지 않는다.** `evaluate.py` 의 `fidelity_evidenced` 는 L0·L1·L3·L4(parity 경유)만 발급하며 `_bump("L2")` 는 존재하지 않는다. §11 L2 의 대표 검증인 golden scenario 비교는 `gate.py` 밖의 별도 스테이지(`sils-ci.sh` stage 5 → `compare_golden.py`)이고 `MATCH`/`REGRESSION` 만 낸다. 위 행동 상태기계 런이 `MATCH` 로 재현된 것은 **회귀 증거**이지 자동 L2 등급이 아니다. 자세한 경계는 [architecture-v3 §11](../../docs/sils/sils-virtual-ecu-architecture-v3.md) 주의 박스 참조.

---

## 데이터 흐름

```
config/sils/contracts/mmf-endpoints.yaml   (Contract Catalog, §9.1)
config/sils/qos/qos-profiles.yaml          (QoS Catalog, §9.3)
config/sils/topologies/*.yaml              (Topology, §9.4)
service/sils/vecu/descriptors/*.yaml       (ECU Descriptor, §9.2)
        │
        │  lint_catalog.py        ── 카탈로그 위생 검사 (§9.1/§9.3)  ← compile 前 게이트
        ▼
   compile.py (§11/§13)
        │   resolved-run.yaml + saved-service-list.yaml
        │   (+ resolved_to_shell.py: 코디네이터용 shell 변수 추출)
        ▼
   run_phase1.sh  (Run Coordinator, §5/§14.4)
        │   ← domain_lease.py 로 병렬 run 도메인 격리 (§12)
        │   trace.jsonl  (coordinator + probe + verify 병합 스트림)
        ├────────────────────────────────────────────────┐
        ▼                                                  ▼
   evaluate.py (§9/§11/§12)                        capture_golden.py (§12.4)
        │   verdict.json                                   │  golden trace (정규화)
        ▼                                                  ▼
   to_junit.py (§16)                              compare_golden.py (§12.4)
        │   junit.xml → CI                                 │  회귀 판정 (golden 대비)
        └───────────────── sils-ci.sh 가 전체를 오케스트레이션 ─────────────────┘
```

한 줄 요약: **입력 위생(lint) → 컴파일 → 실행 → 판정 → CI 리포트 → 회귀 방어**.

---

## 도구 인덱스

| 도구 | 역할 | § | 테스트 |
|---|---|---|---|
| [contracts/lint_catalog.py](contracts/lint_catalog.py) | Contract/QoS Catalog 단독 위생 검사(필수필드·중복·domain_env 오타·dangling qos·IDL 실존·enum). compile 前 게이트 | §9.1/§9.3 | [18](contracts/test_lint_catalog.py) |
| [topology_compiler/compile.py](topology_compiler/compile.py) | Topology+Descriptor+Contract+QoS → `resolved-run.yaml` + `saved-service-list.yaml`. 정적 검증(single-owner, 이름 중복, domain_env 일관성) + **Descriptor 포트 선언(clock/behavior/plant/scenario/attestation)→SILS_\* env 배선**(§9.2) | §9.2/§9.5/§11/§13 | [16](topology_compiler/test_compile.py) |
| [topology_compiler/resolved_to_shell.py](topology_compiler/resolved_to_shell.py) | `resolved-run.yaml` → shell-safe `KEY='value'`(코디네이터 `--from-resolved` 급전) | §11 | (compile 테스트에 포함) |
| [run_coordinator/domain_lease.py](run_coordinator/domain_lease.py) | 유한 pool 에서 DDS domain 원자적 임대(TTL+owner). 병렬 run 격리, Coordinator 사망 시 회수 | §12.1/§12.2 | [7](run_coordinator/test_domain_lease.py) |
| [conformance/validate_trace.py](conformance/validate_trace.py) | trace.jsonl ↔ `config/sils/trace-schema/v1.json` 적합성. 미등록 event 는 **기본 실패**(forward-compat 는 `--allow-unknown-events`) | §계약 | [12](conformance/test_validate_trace.py) |
| [conformance/evaluate.py](conformance/evaluate.py) | trace.jsonl → conformance verdict. §5.2 상태기계(+verifier terminal), 증거기반 pass(L0), lockstep/staging/behavior/plant/vehicle/scenario 판정, 요구 판정(`--requirements`), L4 attestation 기록, lease 무결성, fleet 완전성. **`fidelity_evidenced` 는 L0·L1·L3 만 발급 — L2 없음** | §5/§7/§8/§11/§12 | [80](conformance/test_evaluate.py) |
| [conformance/gate.py](conformance/gate.py) | 판정 진입점: schema → fidelity → (선택)parity 를 fail-closed 로 묶고 `--junit` 방출. **선언 fidelity 와 `fidelity_evidenced` 를 대조하지는 않는다** | §11/§16 | [10](conformance/test_gate.py) |
| [conformance/import_openscenario.py](conformance/import_openscenario.py) | `.xosc` → `requirements.yaml`. 매핑은 `SpeedCondition`·`AccelerationCondition` **2종만**, 미매핑 8종은 리포트. `ParameterDeclarations` `$name` 치환 지원(체인·순환·미선언·`${expr}` 명시 처리). 시나리오를 **실행하지 않으며** StopTrigger polarity 는 리뷰 대상 | §요구 | [14](conformance/test_import_openscenario.py) |
| [conformance/to_junit.py](conformance/to_junit.py) | verdict → JUnit XML(CI 수집). PASS/FAIL/ABORTED/INCOMPLETE → testcase/failure/error | §16 | [12](conformance/test_to_junit.py) |
| [conformance/capture_golden.py](conformance/capture_golden.py) | run trace → 정규화 golden(식별자 제거·시각 rebase). PASSED+conformant·결정론 모드만 허용. **`--plant`** 로 플랜트 정체를 `golden_meta` 에 기록 | §12.4 | [12](conformance/test_capture_golden.py) |
| [conformance/compare_golden.py](conformance/compare_golden.py) | candidate vs golden 회귀 판정(FP tolerance·behavior/plant/scenario 값 비교=model correlation·ordering·missing/extra 정책·입력모드 게이트·lockstep 시 벽시계 latency 스킵). **`--plant` 플랜트 격리** — golden 과 다른 시뮬레이터면 REGRESSION 이 아니라 **정책 거부(exit 2)** | §11/§12.4 | [27](conformance/test_compare_golden.py) |
| [conformance/parity.py](conformance/parity.py) | **L4 target parity** 판정: SiLS trace vs 실-타깃 레퍼런스 trace 비교(신호별 tolerance·timing) + attestation 게이트(arch/gateway/protocol). PARITY=L4 / PARITY_UNATTESTED=L3 / DIVERGENCE. golden(SiLS-vs-SiLS 재현성)과 달리 타깃이 authority | §11 | [12](conformance/test_parity.py) |
| [ci/sils-ci.sh](ci/sils-ci.sh) | 위 전부를 한 진입점으로: lint→compile→run→evaluate→junit→compare→**parity(L4)→capture-golden**. 단계별 skip(`--parity-target/-contract`, `--capture-golden-dir`) | §9~§16 | [18](ci/test_sils_ci.sh) |

Run Coordinator 본체(Phase 1)는 도구가 아니라 서비스라 `service/` 에 있다:

| 파일 | 역할 |
|---|---|
| [service/sils/run_coordinator/run_phase1.sh](../../service/sils/run_coordinator/run_phase1.sh) | Run Coordinator: `start`(local/docker 백엔드) · `fleet`(다중 vECU) · `cleanup` · `reap`(고아 run 회수, §14.4) |
| [service/sils/vecu/src/viss_probe_main.cc](../../service/sils/vecu/src/viss_probe_main.cc) | vECU probe: tick 루프를 IClockSource/IBehavior/IPlant 포트로 구동, `/VIN/Vehicle` 로 publish (C++, **빌드+실런타임 검증**) |
| [service/sils/vecu/src/viss_verify_main.cc](../../service/sils/vecu/src/viss_verify_main.cc) | 독립 one-shot 구독 검증기 (C++, **빌드+실런타임 검증**) |
| [clock_source.hpp](../../service/sils/vecu/src/clock_source.hpp) · [behavior.hpp](../../service/sils/vecu/src/behavior.hpp) · [plant.hpp](../../service/sils/vecu/src/plant.hpp) · [scenario.hpp](../../service/sils/vecu/src/scenario.hpp) | 헥사고날 포트: IClockSource(wall/lockstep) · IBehavior(viss-request/accumulate/p-controller) · IPlant(AnalyticPlant/ReplayPlant/FmuPlant) · IScenario(nominal/scheduled 결함주입) |

---

## 설정 파일 (`config/sils/`)

| 파일 | 내용 |
|---|---|
| `contracts/mmf-endpoints.yaml` | Contract Catalog. MMF 실측 endpoint(`/VIN/Vehicle`, `VissRequestTopic`, `VISS_DDS_DOMAIN_ID`) |
| `qos/qos-profiles.yaml` | QoS Catalog(viss-default/viss-sils-verifiable/request-reliable/state-latest) |
| `topologies/viss_probe_smoke.yaml` | Phase 1 스모크 토폴로지(vECU 1개, docker 백엔드) |
| `topologies/viss_scenario.yaml` | 선언적 포트 예제(behavior=accumulate + scheduled 결함주입 + attestation, §9.2) |
| `regression/golden-rules.yaml` | §12.4 회귀 규칙 참조 세트(그대로 사용 가능) |
| `parity/viss-target-parity.yaml` | §11 L4 target-parity 계약(비교 이벤트·신호별 tolerance·timing·attestation 게이트) |
| `fmu/allowlist.yaml` | §5.6 FMU 보안 allowlist(sha256·공급자, `fmu_gate.py` 가 로드 전 대조) |

> **domain env 주의(§3.4).** MMF 는 서비스군마다 DDS domain env 가 다르다: 코어 `MMF_DDS_DOMAIN_ID`, VISS `VISS_DDS_DOMAIN_ID`, ROS2 `ROS_DOMAIN_ID`. vECU 가 SUT 와 같은 domain 에 붙으려면 **그 서비스의 env** 를 써야 한다. 린터가 오타를 잡는다.

---

## 사용 예

```bash
# 0) 카탈로그 위생 검사
python3 tools/sils/contracts/lint_catalog.py

# 1) 토폴로지 컴파일
python3 tools/sils/topology_compiler/compile.py \
    config/sils/topologies/viss_probe_smoke.yaml --out-dir /tmp/run1

# 2) 전체 파이프라인 dry-run (컴파일 실제 + 코디네이터 계획만, 미기동)
bash tools/sils/ci/sils-ci.sh \
    --topology config/sils/topologies/viss_probe_smoke.yaml \
    --out-dir /tmp/run1 --dry-run

# 3) 사전 캡처 trace 로 판정+CI 리포트+회귀 (바이너리/도커 불필요)
bash tools/sils/ci/sils-ci.sh \
    --topology config/sils/topologies/viss_probe_smoke.yaml \
    --out-dir /tmp/run2 --traces-dir <traces> \
    --golden-dir <goldens> --rules config/sils/regression/golden-rules.yaml

# 4) known-good run 을 golden 으로 등록
python3 tools/sils/conformance/capture_golden.py \
    --trace <run.trace.jsonl> --out <goldens>/<name>.trace.jsonl \
    --input-mode replay --meta catalog_version=0.1.0

# (실제 DDS 왕복: FastDDS 툴체인 필요)
# --build-sils 는 "SiLS 를 빌드 대상에 포함"만 정하고, 아키텍처는 --aarch64 가 정한다(직교).
./build.sh --build-sils              # x86_64 (amd64, 기본)
./build.sh --aarch64 --build-sils    # arm64 (aarch64 크로스 빌드)
bash service/sils/run_coordinator/run_phase1.sh start --backend local --bin-dir <bindir>
```

`sils-ci.sh` 종료코드: `0` OK(fleet PASS·회귀 없음) · `1` stage 실패/FAIL/REGRESSION · `2` usage.

---

## 테스트

### 의존성 (먼저)

```bash
python3 -m pip install -r tools/sils/requirements.txt   # PyYAML + pytest
```

**PyYAML 은 선택이 아니다.** `topology_compiler/compile.py` 와 `resolved_to_shell.py` 는
top-level `import yaml` 이라 없으면 즉시 실패하고, `sils-ci.sh` 는 compile 스테이지를 타므로 전 구간이
막힌다. `evaluate.py --requirements`, `gate.py --requirements/--rules`, `compare_golden --rules`,
`parity --contract`, `import_openscenario`, `lint_catalog`, `fmu_gate/fmu_map` 도 같다.
**예외는 trace-only 경로 하나** — `gate.py <trace.jsonl>` 와 `validate_trace.py <trace.jsonl>` 은
추가 인자 없이 쓰면 stdlib 만으로 동작한다(Phase 2 Tier A 판정 경로). 항목별 표는
[requirements.txt](requirements.txt) 주석 참조.

### 전체 실행 (정식 경로)

```bash
python3 -m pytest tools/sils -q          # 13개 스위트 · 242 tests
bash tools/sils/ci/test_sils_ci.sh       # shell smoke · 23 tests
```

`pytest` 가 정식 경로다 — 대부분의 스위트는 `python3 <test_file>` 자체 실행도 지원하지만
(각 파일의 `if __name__ == "__main__"`), 그 경로는 pytest 의 수집·리포트를 대체하지 않는다.
자체 실행 루프를 쓸 때는 **실패를 삼키지 말 것**:

```bash
rc=0
for t in $(find tools/sils -name 'test_*.py' | sort); do
    python3 "$t" || { echo "FAILED: $t"; rc=1; }
done
exit "$rc"      # `|| echo` 만 쓰면 실패해도 shell 종료코드가 0 이 된다
```

### 현재 수치 (2026-08-06 실측, 전부 green)

| 스위트 | tests | | 스위트 | tests |
|---|---:|---|---|---:|
| `conformance/test_evaluate` | 80 | | `conformance/test_capture_golden` | 12 |
| `conformance/test_compare_golden` | 27 | | `conformance/test_validate_trace` | 12 |
| `contracts/test_lint_catalog` | 18 | | `conformance/test_parity` | 12 |
| `topology_compiler/test_compile` | 16 | | `conformance/test_gate` | 10 |
| `fmu/test_fmu_gate` | 14 | | `fmu/test_fmu_map` | 8 |
| `conformance/test_import_openscenario` | 14 | | `run_coordinator/test_domain_lease` | 7 |
| `conformance/test_to_junit` | 12 | | **합계** | **242** |

여기에 shell smoke(`test_sils_ci.sh` 23 · fleet 17)가 더해진다. DDS/도커 툴체인은 필요 없다.

---

## Phase 1 범위와 다음 단계

Phase 1 은 vECU 한 개가 `/VIN/Vehicle` 로 VISS 요청을 publish 하고 독립 검증기가 왕복을 확인하는 최소 수직 슬라이스다(§17 Scope Gate). 도구 계층은 이미 다중 인스턴스(replicas)·domain pool·회귀 루프를 지원한다.

**실검증 완료(x86_64)**: C++ 빌드; `run_phase1.sh` local·docker 단일 왕복(`run_pass`); 병렬 격리(§12, 동시 2런 각자 도메인); **Fleet(§11/§13, vECU 3개 동시 기동·도메인 공유·`run_pass`)**; 위 모든 실 trace 를 `evaluate.py` 가 그대로 PASSED 로 판정(fleet 은 instance_id 3개 나열). Docker 백엔드는 `--network host`(discovery)가 필수이고, `--ipc=host` 는 SHM 전송이 살아 있는 구성에서 "matched-but-no-data" 를 막기 위해 추가했다(2026-07 실측). **단 현재 canonical 이미지는 SHM 을 쓰지 않는다** — [fastdds-profile.xml](../../docker/build_environment/fastdds-profile.xml) 이 UDPv4-only + `useBuiltinTransports=false` + `data_sharing OFF` 이고 [Dockerfile.rel](../../docker/Dockerfile.rel) 의 `runtime` 스테이지가 이를 `FASTDDS_DEFAULT_PROFILES_FILE` 로 고정(SiLS 스테이지가 상속)하므로, `--ipc=host` 는 프로파일 미적용 커스텀 이미지에 대한 **방어적 유지**다. 경계 설명은 [understanding §4.7.1](../../docs/sils/sils-understanding.md).

**모드 전환(§8) + 입력 스테이징(§7) 확인**: `SILS_CLOCK_MODE=lockstep` 로 ClockMaster
+probe+verify 왕복 시 evaluate.py 가 `clock.deterministic=true`(tick 1..N +1 단조,
sim_time dt 전진, barrier 닫힘) 판정. `SILS_STAGE_INPUT=1` + sils_input_source 로
입력이 tick 에 **수신 순서대로 분할 소비**(§7.2 "소비 순서 고정")됨을 실측 —
`staging.deterministic=true`. IClockSource 포트(wall/lockstep 어댑터) + TickInbox 로
구현. **late-sample 정책(§7.3)** 은 계약의 `late_sample_policy`(next_tick/drop/fail,
lint enum·컴파일러 전파·probe `SILS_LATE_SAMPLE_POLICY` 방출)로 선언하고, evaluate 가
정책대로 판정한다: next_tick 은 지연 소비(드롭이면 위반)·drop 은 폐기 허용·fail 은 늦은
입력 불허. 어떤 정책이든 **스테이징도 드롭도 안 된 입력**은 손실(위반).

**행동(§11 L2) + 입력 replay(§7.4) 확인**: `SILS_BEHAVIOR=accumulate` 는 tick 별
staged 입력 수를 누적하는 상태기계 — 같은 tick-indexed 입력이면 같은 `value` 수열을
재생산. `SILS_REPLAY_FILE` 로 입력을 파일 구동하면(§7.4) tick→입력 귀속이 파일로
고정돼 벽시계 도착 경합 없이 **run 간 재현 가능**(evaluate `input_mode=replay`,
compare_golden MATCH). IBehavior 포트(viss-request/accumulate/p-controller 어댑터).

**플랜트 폐루프(§11 L3) 확인**: `SILS_PLANT=first-order` + `SILS_BEHAVIOR=p-controller`
로 컨트롤러가 매 tick 플랜트 출력을 피드백 받아 명령을 계산하고, 플랜트가 그 명령으로
한 dt 적분하는 **실 폐루프**를 구성(IPlant 포트는 §6 정의). evaluate `analyze_plant` 가
`plant_step` 스트림에서 루프 연속성(feedback_in[N]==output[N-1]) + live + 결정론을 확인해
**fidelity=L3** 판정, compare_golden 이 `plant_step` 출력을 model correlation(§11)으로
비교. `SILS_PLANT=replay`(playback)는 결정론적 golden 이지만 명령을 무시하므로 L3-live
아님(입력 replay 와 동일 구분). IPlant 어댑터(AnalyticPlant=live / ReplayPlant=playback /
**FmuPlant**=FMI 2.0/3.0 co-sim). **FmuPlant**(`SILS_PLANT=fmu SILS_FMU_FILE=vehicle.fmu
SILS_FMU_INPUT_VR/OUTPUT_VR`)는 fmi4c 로 실제 `.fmu`(Simulink/Dymola/… export, 또는
Reference-FMU)를 로드해 `SetInputs→fmiX_setReal`, `Step→fmiX_doStep`, `output→fmiX_getReal`
로 구동 — **같은 plant_step 계약**을 방출하므로 analyze_plant/compare_golden 이 그대로 L3
판정. doStep 실패/비유한 출력은 NaN 으로 §14 fail-fast. 빌드 옵션 `-DSILS_ENABLE_FMU=ON`
(없으면 fmi4c FetchContent), 미빌드 시 `SILS_PLANT=fmu` 는 런타임에 명확히 실패.
FMI/FMU/fmi4c 개념·현황·주의사항(ARM64 바이너리·변수매핑·실패정책·보안·MORAI 상태
소유권)은 [docs/sils/sils-fmu-plant.md](../../docs/sils/sils-fmu-plant.md) 참조.
**x86 실런타임에서 두 번 독립 실행 → sim_time·궤적 소수점까지 동일(compare_golden MATCH)로
재현성 확인**; FMI co-sim 이 3번째 구현.

**프로토콜 충실도(§11 L1) 확인**: `SILS_PROTOCOL=reqresp` 로 probe 가 요청을 보내고
응답 토픽을 구독, `sils_vecu_viss_responder`(SUT 스탠드인)가 요청을 받아 `requestId` 를
echo 한 상관 응답을 발행. probe 는 `request_sent`, 응답 수신 시 `response_received`
{request_id,status,error_code} 를 방출하고, evaluate `analyze_protocol` 이 request_id 로
**왕복을 상관**한다: unanswered(orphan)/phantom/duplicate/ill-formed error/QoS(RELIABLE)
위반 시 fail, 매칭된 왕복 ≥1 + 위반 0 이면 **fidelity=L1**. `SILS_RESPONDER_ERROR_EVERY=k`
로 매 k 번째 응답을 well-formed error 로 주입해 오류 semantics 를 결정론적으로 시험.
응답도 TRANSIENT_LOCAL(late-join). *(도구 계약 샌드박스 검증 완료; C++ 실런타임은 빌드 후.)*

**결함 주입(§11 negative path) 확인**: `SILS_SCENARIO=scheduled SILS_SCENARIO_FAULTS="3:drop,7:corrupt,9:duplicate"`
로 IScenario 포트가 **tick-indexed 결정론 스케줄**로 결함을 주입한다 — drop(요청 미발행),
corrupt(파싱 불가 페이로드), duplicate(같은 requestId 2회). RNG·wall-clock 이 아니라 tick
기준이라 **결함 run 자체가 golden-reproducible**(compare_golden 이 `scenario_fault` 를 비교).
probe 는 `scenario_start`{name,scheduled_faults}·`scenario_fault`{tick,fault,request_id} 를
방출하고, evaluate `analyze_scenario` 가 (1) 선언한 결함 수만큼 실제 주입됐는지 확인(안 되면
fail — 결함이 조용히 미주입되면 'fault run' 이 거짓 PASS), (2) 주입된 결함 id 를
`analyze_protocol` 의 unanswered/duplicate 집계에서 **제외**(주입 결함은 자극이지 프로토콜
결함이 아님; 다른 요청의 왕복이 결함 때문에 깨지면 그건 진짜 위반). `SILS_SCENARIO=scheduled`
인데 스펙이 비면 §14 fail-fast. Run Coordinator 가 없어도 도구 계약은 샌드박스 검증 완료.

**선언적 포트 배선(§9.2) 확인**: 위 포트들(clock/behavior/plant/scenario/attestation)은 이제
**Descriptor 로 선언**하면 파이프라인이 probe 의 `SILS_*` env 로 자동 배선한다 — 손으로
export 할 필요 없음. `compile.py:resolve_runtime_env` 가 Descriptor 의 `plant:{type:fmu}` /
`scenario:{type:scheduled,faults:[…]}` 등을 `SILS_PLANT`/`SILS_SCENARIO`/… 로 변환해
resolved-run 에 싣고, `resolved_to_shell.py` 가 local 용 `export SILS_*`(coordinator 상속 +
fmu_preflight 가 `SILS_FMU_FILE` 확인) 와 docker/fleet 용 `--env-args`(raw `KEY=VALUE`)를
방출, `run_phase1.sh` 가 local(상속)·docker(`-e`)·fleet(`env`/`-e`) 로 주입한다. 변환·방출은
전부 오프라인 테스트로 단언(빌드/DDS 불필요) — Descriptor→env 문자열이 손으로 넣던 값과
동일함을 보장하므로 런타임 동작은 기존 실측(L1/L3/scenario)이 그대로 성립. 예:
`config/sils/topologies/viss_scenario.yaml`(behavior=accumulate + scheduled 결함주입).

**arm64 빌드 확인**: `./build.sh --aarch64 --build-sils` 로 aarch64 크로스빌드 성공(코드 수정 없이 — SiLS 는 arch 중립, core 와 동일 FastDDS/IDL 경로). arm64 **실행** 검증은 arm64 호스트(또는 qemu/binfmt) 필요.

**타깃 패리티(§11 L4) 정의·스캐폴딩**: L4 는 "SiLS vECU 가 실제 ECU 처럼 행동한다" — 같은
자극을 **실 타깃 ECU**(레퍼런스/authority)와 SiLS vECU 에 주고 관측 인터페이스가 계약된
tolerance 내에서 일치해야 한다. golden(SiLS-vs-SiLS **재현성**, §12.4)과 근본적으로 다르다:
타깃이 authority. `parity.py --target t.jsonl --sils s.jsonl --contract c.yaml` 가 판정하며
**두 조건을 모두** 요구한다 — (1) 값/timing 패리티(compare_golden 엔진 재사용, 신호별
tolerance·timing window; `config/sils/parity/viss-target-parity.yaml`), (2) **attestation**:
SiLS run 이 실제 target-like 구성요소(arch=aarch64·real gateway·real protocol)를 썼다는
`target_attestation` 이벤트(probe: `SILS_ATTEST_ARCH/GATEWAY/PROTOCOL`)가 계약과 일치.
값이 맞아도 x86·stub gateway 면 **PARITY_UNATTESTED(최대 L3)** — §11 "ARM64 는 L4 구성
요소이지 L4 전체를 자동 충족하지 않는다" 를 코드로 강제. 판정: PARITY=L4 / PARITY_UNATTESTED /
DIVERGENCE. evaluate 는 단일 trace 로 L4 를 주지 않고 attestation 만 기록(교차-trace 판정은
parity.py 의 몫). 정의·플로우는 [docs/sils/sils-target-parity.md](../../docs/sils/sils-target-parity.md).
**남은 것**: 실 타깃 ECU 레퍼런스 trace 캡처 + arm64/real-gateway 실행(실측 계열).

**남은 것**: L1 C++ 실런타임 trace 판정(도구 계약은 완료, 빌드 후 왕복 실측), arm64 실행 검증(arm64 머신/에뮬), 실제 VISS SUT 와의 폐루프(현재는 responder 스탠드인으로 왕복 확인), 물리적 timing 충실도 실측(§0A Contract Spike), per-instance fleet 의 **x86 실런타임 실측**(컴파일 검증 + 런타임 배선 완료 — `ownership: per-instance` + `{instance_id}` 템플릿으로 replica 가 distinct topic 을 소유해 §9.5-valid; `resolved_to_shell`→`SILS_TOPIC`→probe 로 각 인스턴스가 자기 topic 에 publish; C++ 빌드 후 다중 topic 왕복 실측 남음), FmuPlant 의 **실 `.fmu` 실런타임 검증**(어댑터·빌드옵션 완료 — fmi4c + Reference-FMU 로 실측 남음)과 **MORAI 등 외부 시뮬 UDP–ROS2 브리지 트랙**(별도 Topology, §4.1). L4(target parity)는 **판정 도구·계약·attestation 게이트 완료**(`parity.py`) — 실 타깃 ECU 레퍼런스 trace 캡처 + arm64/real-gateway 실행만 남음(실측 계열).
