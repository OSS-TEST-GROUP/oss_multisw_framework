# SiLS 가상 ECU (vECU) 도구 계층

MMF(oss_multisw_framework) 위에서 **SiLS(Software-in-the-Loop) 가상 ECU** 를 컴파일·실행·판정·회귀검사하는 도구 모음이다. 아키텍처 확정본은 **v3 Final** 문서(`docs/sils-virtual-ecu-architecture-v3.md`)이며, 아래 코드의 `§n` 주석은 모두 그 문서의 절 번호를 가리킨다.

> **검증 상태 (중요).** 이 디렉터리의 Python/shell 도구 계층은 **샌드박스에서 실제 실행·테스트까지 완료**되어 있다(아래 "테스트" 참조). `service/sils/` 의 C++ vECU 본체는 FastDDS 툴체인이 있는 Linux(x86_64)에서 **컴파일 + 실제 DDS 왕복이 검증**되었고, fidelity **L0·L2·L3 를 실런타임 trace 로 판정 완료**(2026-07 실측): local·docker 두 백엔드 `run_pass`, 병렬 격리·Fleet(vECU 3개), lockstep 결정론(§8), 입력 스테이징·replay(§7), 행동 상태기계(§11 L2), **플랜트 폐루프(§11 L3, AnalyticPlant)를 두 번 독립 실행해 sim_time·궤적이 소수점까지 재현**(compare_golden MATCH). 실행이라 드러난 결함 3건도 수정: docker `--ipc=host`(SHM), late-join verifier 용 `TRANSIENT_LOCAL`, lockstep golden 의 벽시계 latency 스킵. 도구 계층은 바이너리 없이도 dry-run·사전 캡처 trace 로 전 구간을 검증할 수 있게 설계되었다.

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
| [contracts/lint_catalog.py](contracts/lint_catalog.py) | Contract/QoS Catalog 단독 위생 검사(필수필드·중복·domain_env 오타·dangling qos·IDL 실존·enum). compile 前 게이트 | §9.1/§9.3 | [16](contracts/test_lint_catalog.py) |
| [topology_compiler/compile.py](topology_compiler/compile.py) | Topology+Descriptor+Contract+QoS → `resolved-run.yaml` + `saved-service-list.yaml`. 정적 검증(single-owner, 이름 중복, domain_env 일관성) | §9.5/§11/§13 | [8](topology_compiler/test_compile.py) |
| [topology_compiler/resolved_to_shell.py](topology_compiler/resolved_to_shell.py) | `resolved-run.yaml` → shell-safe `KEY='value'`(코디네이터 `--from-resolved` 급전) | §11 | (compile 테스트에 포함) |
| [run_coordinator/domain_lease.py](run_coordinator/domain_lease.py) | 유한 pool 에서 DDS domain 원자적 임대(TTL+owner). 병렬 run 격리, Coordinator 사망 시 회수 | §12.1/§12.2 | [7](run_coordinator/test_domain_lease.py) |
| [conformance/evaluate.py](conformance/evaluate.py) | trace.jsonl → conformance verdict. §5.2 상태기계(+verifier terminal), 증거기반 pass(L0), lockstep/staging/behavior/plant 판정(L2/L3), lease 무결성, fleet 완전성 | §5/§7/§8/§11/§12 | [40](conformance/test_evaluate.py) |
| [conformance/to_junit.py](conformance/to_junit.py) | verdict → JUnit XML(CI 수집). PASS/FAIL/ABORTED/INCOMPLETE → testcase/failure/error | §16 | [12](conformance/test_to_junit.py) |
| [conformance/capture_golden.py](conformance/capture_golden.py) | run trace → 정규화 golden(식별자 제거·시각 rebase). PASSED+conformant·결정론 모드만 허용 | §12.4 | [8](conformance/test_capture_golden.py) |
| [conformance/compare_golden.py](conformance/compare_golden.py) | candidate vs golden 회귀 판정(FP tolerance·behavior/plant 값 비교=model correlation·ordering·missing/extra 정책·입력모드 게이트·lockstep 시 벽시계 latency 스킵) | §11/§12.4 | [18](conformance/test_compare_golden.py) |
| [ci/sils-ci.sh](ci/sils-ci.sh) | 위 전부를 한 진입점으로: lint→compile→run→evaluate→junit→compare. 단계별 skip | §9~§16 | [12](ci/test_sils_ci.sh) |

Run Coordinator 본체(Phase 1)는 도구가 아니라 서비스라 `service/` 에 있다:

| 파일 | 역할 |
|---|---|
| [service/sils/run_coordinator/run_phase1.sh](../../service/sils/run_coordinator/run_phase1.sh) | Run Coordinator: `start`(local/docker 백엔드) · `fleet`(다중 vECU) · `cleanup` · `reap`(고아 run 회수, §14.4) |
| [service/sils/vecu/src/viss_probe_main.cc](../../service/sils/vecu/src/viss_probe_main.cc) | vECU probe: tick 루프를 IClockSource/IBehavior/IPlant 포트로 구동, `/VIN/Vehicle` 로 publish (C++, **빌드+실런타임 검증**) |
| [service/sils/vecu/src/viss_verify_main.cc](../../service/sils/vecu/src/viss_verify_main.cc) | 독립 one-shot 구독 검증기 (C++, **빌드+실런타임 검증**) |
| [clock_source.hpp](../../service/sils/vecu/src/clock_source.hpp) · [behavior.hpp](../../service/sils/vecu/src/behavior.hpp) · [plant.hpp](../../service/sils/vecu/src/plant.hpp) | 헥사고날 포트: IClockSource(wall/lockstep) · IBehavior(viss-request/accumulate/p-controller) · IPlant(AnalyticPlant/ReplayPlant) |

---

## 설정 파일 (`config/sils/`)

| 파일 | 내용 |
|---|---|
| `contracts/mmf-endpoints.yaml` | Contract Catalog. MMF 실측 endpoint(`/VIN/Vehicle`, `VissRequestTopic`, `VISS_DDS_DOMAIN_ID`) |
| `qos/qos-profiles.yaml` | QoS Catalog(viss-default/viss-sils-verifiable/request-reliable/state-latest) |
| `topologies/viss_probe_smoke.yaml` | Phase 1 스모크 토폴로지(vECU 1개, docker 백엔드) |
| `regression/golden-rules.yaml` | §12.4 회귀 규칙 참조 세트(그대로 사용 가능) |

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

각 도구는 pytest 무의존의 자체 실행 테스트를 가진다(assert 기반). 전체 실행:

```bash
for t in tools/sils/topology_compiler/test_compile.py \
         tools/sils/run_coordinator/test_domain_lease.py \
         tools/sils/conformance/test_evaluate.py \
         tools/sils/conformance/test_to_junit.py \
         tools/sils/conformance/test_compare_golden.py \
         tools/sils/conformance/test_capture_golden.py \
         tools/sils/contracts/test_lint_catalog.py; do
    python3 "$t" || echo "FAILED: $t"
done
bash tools/sils/ci/test_sils_ci.sh
```

현재 전부 green: Python 7개 스위트(evaluate 40 · compare_golden 18 · capture 8 · to_junit 12 · lint 16 · compile 8 · domain_lease 7 = 109) + shell smoke(fleet 17 · ci 12). 파이썬만 필요하며 DDS/도커 툴체인은 필요 없다.

---

## Phase 1 범위와 다음 단계

Phase 1 은 vECU 한 개가 `/VIN/Vehicle` 로 VISS 요청을 publish 하고 독립 검증기가 왕복을 확인하는 최소 수직 슬라이스다(§17 Scope Gate). 도구 계층은 이미 다중 인스턴스(replicas)·domain pool·회귀 루프를 지원한다.

**실검증 완료(x86_64)**: C++ 빌드; `run_phase1.sh` local·docker 단일 왕복(`run_pass`); 병렬 격리(§12, 동시 2런 각자 도메인); **Fleet(§11/§13, vECU 3개 동시 기동·도메인 공유·`run_pass`)**; 위 모든 실 trace 를 `evaluate.py` 가 그대로 PASSED 로 판정(fleet 은 instance_id 3개 나열). Docker 백엔드는 `--network host`(discovery)에 더해 **`--ipc=host`(SHM 데이터 경로)** 가 필수임을 실측으로 확인해 반영했다.

**모드 전환(§8) + 입력 스테이징(§7) 확인**: `SILS_CLOCK_MODE=lockstep` 로 ClockMaster
+probe+verify 왕복 시 evaluate.py 가 `clock.deterministic=true`(tick 1..N +1 단조,
sim_time dt 전진, barrier 닫힘) 판정. `SILS_STAGE_INPUT=1` + sils_input_source 로
입력이 tick 에 **수신 순서대로 분할 소비**(§7.2 "소비 순서 고정")됨을 실측 —
`staging.deterministic=true`. IClockSource 포트(wall/lockstep 어댑터) + TickInbox 로
구현.

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

**arm64 빌드 확인**: `./build.sh --aarch64 --build-sils` 로 aarch64 크로스빌드 성공(코드 수정 없이 — SiLS 는 arch 중립, core 와 동일 FastDDS/IDL 경로). arm64 **실행** 검증은 arm64 호스트(또는 qemu/binfmt) 필요.

**남은 것**: L1 C++ 실런타임 trace 판정(도구 계약은 완료, 빌드 후 왕복 실측), arm64 실행 검증(arm64 머신/에뮬), 실제 VISS SUT 와의 폐루프(현재는 responder 스탠드인으로 왕복 확인), 물리적 timing 충실도 실측(§0A Contract Spike), §9.5-유효한 다중 topic/multi-owner contract 기반 fleet(현재 `--replicas` 는 single-owner 우회 스모크), FmuPlant 의 **실 `.fmu` 실런타임 검증**(어댑터·빌드옵션 완료 — fmi4c + Reference-FMU 로 실측 남음)과 **MORAI 등 외부 시뮬 UDP–ROS2 브리지 트랙**(별도 Topology, §4.1), 그리고 L4(target parity) fidelity.
