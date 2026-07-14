# SiLS 가상 ECU (vECU) 도구 계층

MMF(oss_multisw_framework) 위에서 **SiLS(Software-in-the-Loop) 가상 ECU** 를 컴파일·실행·판정·회귀검사하는 도구 모음이다. 아키텍처 확정본은 **v3 Final** 문서(`docs/sils-virtual-ecu-architecture-v3.md`)이며, 아래 코드의 `§n` 주석은 모두 그 문서의 절 번호를 가리킨다.

> **검증 상태 (중요).** 이 디렉터리의 Python/shell 도구 계층은 **샌드박스에서 실제 실행·테스트까지 완료**되어 있다(아래 "테스트" 참조). `service/sils/` 의 C++ vECU 본체(probe/verify)는 FastDDS 툴체인이 있는 Linux(x86_64)에서 **컴파일 빌드 + 실제 DDS 샘플 왕복이 검증**되었다: `run_phase1.sh` 의 **local·docker 두 백엔드 모두** vECU 가 `/VIN/Vehicle` 로 publish → 독립 verify 가 유효 VISS envelope 수신 → `run_pass`(2026-07 실측). 실런타임 trace 를 `evaluate.py` 가 그대로 판정(PASSED/L0)함도 확인. 도구 계층은 바이너리 없이도 dry-run·사전 캡처 trace 로 전 구간을 검증할 수 있게 설계되었다.

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
| [conformance/evaluate.py](conformance/evaluate.py) | trace.jsonl → conformance verdict. §5.2 상태기계 매핑, 증거기반 pass(L0), lease 무결성, fleet 완전성 | §9/§11/§12 | [14](conformance/test_evaluate.py) |
| [conformance/to_junit.py](conformance/to_junit.py) | verdict → JUnit XML(CI 수집). PASS/FAIL/ABORTED/INCOMPLETE → testcase/failure/error | §16 | [12](conformance/test_to_junit.py) |
| [conformance/capture_golden.py](conformance/capture_golden.py) | run trace → 정규화 golden(식별자 제거·시각 rebase). PASSED+conformant·결정론 모드만 허용 | §12.4 | [8](conformance/test_capture_golden.py) |
| [conformance/compare_golden.py](conformance/compare_golden.py) | candidate vs golden 회귀 판정(FP tolerance·latency window·ordering·missing/extra 정책·입력모드 게이트) | §12.4 | [15](conformance/test_compare_golden.py) |
| [ci/sils-ci.sh](ci/sils-ci.sh) | 위 전부를 한 진입점으로: lint→compile→run→evaluate→junit→compare. 단계별 skip | §9~§16 | [12](ci/test_sils_ci.sh) |

Run Coordinator 본체(Phase 1)는 도구가 아니라 서비스라 `service/` 에 있다:

| 파일 | 역할 |
|---|---|
| [service/sils/run_coordinator/run_phase1.sh](../../service/sils/run_coordinator/run_phase1.sh) | Run Coordinator: `start`(local/docker 백엔드) · `cleanup` · `reap`(고아 run 회수, §14.4) |
| service/sils/vecu/src/viss_probe_main.cc | vECU probe: `VissRequestTopic` 을 `/VIN/Vehicle` 로 publish (C++, **빌드 미검증**) |
| service/sils/vecu/src/viss_verify_main.cc | 독립 one-shot 구독 검증기 (C++, **빌드 미검증**) |

---

## 설정 파일 (`config/sils/`)

| 파일 | 내용 |
|---|---|
| `contracts/mmf-endpoints.yaml` | Contract Catalog. MMF 실측 endpoint(`/VIN/Vehicle`, `VissRequestTopic`, `VISS_DDS_DOMAIN_ID`) |
| `qos/qos-profiles.yaml` | QoS Catalog(viss-default/request-reliable/state-latest) |
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

현재 전부 green (Python 6개 스위트 + shell smoke = 총 92 checks). 파이썬만 필요하며 DDS/도커 툴체인은 필요 없다.

---

## Phase 1 범위와 다음 단계

Phase 1 은 vECU 한 개가 `/VIN/Vehicle` 로 VISS 요청을 publish 하고 독립 검증기가 왕복을 확인하는 최소 수직 슬라이스다(§17 Scope Gate). 도구 계층은 이미 다중 인스턴스(replicas)·domain pool·회귀 루프를 지원한다.

**실검증 완료(x86_64)**: C++ 빌드, local·docker 백엔드 실제 DDS 왕복(`run_pass`), 실 trace 의 `evaluate.py` 판정까지 확인됨. Docker 백엔드는 `--network host`(discovery)에 더해 **`--ipc=host`(SHM 데이터 경로)** 가 필수임을 실측으로 확인해 반영했다.

**남은 것**: arm64(`--aarch64`) 실빌드·실행 검증, 실제 VISS SUT 와의 폐루프(현재는 독립 verify 로 왕복만 확인), 물리적 timing 충실도 실측(§0A Contract Spike), Fleet(다중 vECU 동시 기동) 실런타임. 여기까지 오면 도구 계층이 만들어 온 trace/verdict/golden 파이프라인이 실제 런타임과 완전히 맞물린다.
