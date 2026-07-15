# SiLS 가상 ECU — 이해를 위한 문서 (전체 훑기)

> 이 문서는 SiLS(Software-in-the-Loop) 가상 ECU 시스템 **전체를 하나씩 이해하며 채워나가는
> 학습 문서**다. 챕터 단위로 늘려간다. 각 챕터는 독립적으로 읽히도록 쓰고, 코드 근거는
> `파일:라인` 링크로 건다. 시각 다이어그램(렌더판): [sils-understanding.html](sils-understanding.html).

전체 개요는 [SiLS 도구 README](../tools/sils/README.md), 아키텍처 근거는
[v3](sils-virtual-ecu-architecture-v3.md). 이 문서는 "코드가 실제로 어떻게 도는지"를
차근차근 풀어 쓰는 쪽에 집중한다.

---

## 전체 지도 (챕터 맵 · 채워나가는 체크리스트)

| 장 | 주제 | 상태 |
|---|---|---|
| **1** | **Topology Compiler — 정적 변환·검증** | ✅ 작성됨 |
| 2 | Contract / QoS Catalog — 무엇을 **선언**하나 (계약의 의미·QoS 매칭). *검증(lint)은 1장* | ⬜ |
| 3 | Descriptor → `SILS_*` env — 선언적 포트 배선(§9.2) | ⬜ (1장 §1.3에 일부) |
| 4 | Run Coordinator — 도메인 임대·기동·teardown·reap | ⬜ |
| 5 | vECU 포트 — Clock / Behavior / Plant / Scenario | ⬜ |
| 6 | Conformance 판정 — `evaluate.py` 가 trace 를 어떻게 읽나 | ⬜ |
| 7 | 충실도 사다리 L0–L4 — 각 레벨의 증거 | ⬜ |
| 8 | Golden / 회귀 — capture ↔ compare 재현성 | ⬜ |
| 9 | FMU 트랙 — fmi4c·보안 게이트 (별도: [sils-fmu-plant.md](sils-fmu-plant.md)) | ↗ 링크 |
| 10 | L4 Target Parity (별도: [sils-target-parity.md](sils-target-parity.md)) | ↗ 링크 |
| 11 | Fleet / run 격리 / 재현성 | ⬜ |

이해한 장을 ✅ 로 바꿔가며 진행한다. `↗` 는 이미 전용 문서가 있어 링크로 대체하는 장.

> **"검증"은 한 장이 아니라 시스템 전반에 흩어진 cross-cutting 주제다.** 장을 혼동하지 않도록
> 어디서 무엇을 검증하는지 미리 못 박는다:
>
> | 검증 계층 | 무엇을 | 코드 | 장 |
> |---|---|---|---|
> | 정적 계획 검증 | 카탈로그 위생 + 토폴로지 문맥 | lint_catalog · compile | **1** |
> | 런타임 conformance | run trace → verdict | evaluate.py | 6 |
> | 회귀 · 패리티 | 재현성 · 타깃 동등성 | compare_golden · parity | 8 · 10 |
>
> 따라서 **카탈로그의 *검증*(lint)은 1장 소관**이고, 2장은 카탈로그가 *무엇을 선언하는가*(계약
> 모델)만 다룬다 — 검증을 반복하지 않는다.

---

# 1장. Topology Compiler — 정적 변환·검증

## 1.1 한 문장

Topology Compiler 는 **"무엇을 돌릴지" 선언(YAML 4종)** 을 읽어, 잘못된 조합을 실행 전에
잡고(**검증**) 맞으면 실행 가능한 계획으로 펼치는(**변환**) **순수 정적** 단계다.
DDS·도커·빌드·arm64 없이 돈다.

```
Topology + Descriptor + Contract Catalog + QoS Catalog   (선언)
        │
        ▼  Topology Compiler   (정적 = 아무것도 실행 안 함)
        │
resolved-run.yaml + saved-service-list.yaml              (실행 계획)
        │
        ▼  Run Coordinator (run_phase1.sh) 가 소비 → 실제 기동
```

핵심 코드: [compile.py](../tools/sils/topology_compiler/compile.py) ·
[lint_catalog.py](../tools/sils/contracts/lint_catalog.py) ·
[resolved_to_shell.py](../tools/sils/topology_compiler/resolved_to_shell.py).

## 1.2 언제 검증되는가 — 실행(launch) 전, 컴파일 시점

검증은 **vECU 실행보다 앞선 compile 시점에 한 번** 일어난다. Run Coordinator 는 launch 시
재검증하지 않는다 — 이미 검증된 계획을 신뢰하고 소비만 한다.

```
작성·PR ──▶  CI: lint → compile  ──▶  resolved-run.yaml  ──▶  Run Coordinator  ──▶  trace → evaluate
             ◆ 검증 시점 (여기)        (검증된 실행 계획)      --from-resolved: 읽기만
             실패 시 fail-closed                              재검증 안 함 (신뢰)
```

- **앞에서 한 번**: `sils-ci.sh` 기준 `stage 0 lint → stage 1 compile → stage 2 run`.
  compile 이 실패하면 **아무것도 launch 되지 않는다**(shift-left).
- **실행은 신뢰**: [run_phase1.sh](../service/sils/run_coordinator/run_phase1.sh) 는
  `--from-resolved <resolved-run.yaml>` 로 [resolved_to_shell.py](../tools/sils/topology_compiler/resolved_to_shell.py)
  를 써서 계획을 **읽기만** 한다. `compile.py` 를 다시 호출하지 않는다.
- **순수 정적**: 아무 때나(에디터·pre-commit·PR·CI) 돌릴 수 있다.

> 한 줄: **작성/PR/CI 시점에 검증→`resolved-run.yaml` 산출, 실행 시점엔 산출물을 신뢰(재검증 X).**

## 1.3 어떻게 — 변환 (transform)

[compile_topology](../tools/sils/topology_compiler/compile.py#L385) 가 입력 4종을 읽고
[resolve_topology](../tools/sils/topology_compiler/compile.py#L117) 가 **선언 → 구체적 인스턴스
목록**으로 펼친다. 네 단계:

| 단계 | 함수/위치 | 하는 일 |
|---|---|---|
| ① replica 전개 | [expand_replicas](../tools/sils/topology_compiler/compile.py#L72) | `replicas:4`+`params` → `(container_name, instance_id, index)` N개 |
| ② 엔드포인트 해석 | [L157-L184](../tools/sils/topology_compiler/compile.py#L157) | `contract_id` → catalog 의 `topic·domain_env·qos·ownership·late_sample_policy` 치환 |
| ③ per-instance 토픽 | [L197](../tools/sils/topology_compiler/compile.py#L197) | `{instance_id}`/`{index}` → 실제 인스턴스로 치환(각 replica 고유 토픽) |
| ④ 포트→env | [resolve_runtime_env](../tools/sils/topology_compiler/compile.py#L118) | Descriptor 의 `clock/behavior/plant/scenario/attestation` → `SILS_*` env |

**출력 2종** + **재현성 해시**:
- [render_resolved_run](../tools/sils/topology_compiler/compile.py#L275): `resolved-run.yaml` — `containers[]`(각 인스턴스의 endpoints·env) + `input_hashes`.
- [render_saved_service_list](../tools/sils/topology_compiler/compile.py#L242): MMF 컨테이너 제어 포맷(`-e` env·`--network host`).
- [input_hashes](../tools/sils/topology_compiler/compile.py#L401): topology·contracts·qos·각 descriptor 의 sha256 을 박아 "어떤 입력에서 나온 계획인지" 봉인.

## 1.4 어떻게 — 검증 (validate), 2계층

### A. 사전 게이트 — [lint_catalog.py](../tools/sils/contracts/lint_catalog.py) (카탈로그 단독)
컴파일 *전에* 카탈로그 자체 위생 검사(토폴로지가 안 쓰는 항목의 오류까지):
필수필드·**중복 id**·dangling qos·**IDL 파일 실존**(디스크 stat)·domain_env 오타·
enum(transport/ownership/reliability/durability/history)·`KEEP_LAST` depth>0·
중복 `(topic,domain_env)`·semver·per-instance ⊃ `{instance_id}`·late_sample_policy.

### B. 문맥 검증 — [resolve_topology](../tools/sils/topology_compiler/compile.py#L117) (토폴로지 안에서만 보임)

| 검사 | 위치 |
|---|---|
| `run.domain_id` 필수(SUT 도메인 일치, §3.4) | [L135](../tools/sils/topology_compiler/compile.py#L135) |
| 알 수 없는 `contract_id` | [L162](../tools/sils/topology_compiler/compile.py#L162) |
| `qos_profile` 미존재 | [L169](../tools/sils/topology_compiler/compile.py#L169) |
| 이름 중복 | [L188](../tools/sils/topology_compiler/compile.py#L188) |
| **§9.5 단일 소유권** (RESOLVED 토픽 기준) | [L217-L231](../tools/sils/topology_compiler/compile.py#L217) |
| `replicas`↔`params` 길이 | [L82](../tools/sils/topology_compiler/compile.py#L82) |

**§9.5 검사의 묘미**: **RESOLVED 토픽**(③ 템플릿 치환 후) 기준으로 producer 를 센다. 그래서
per-instance fleet(replica 마다 고유 토픽)은 유효, `{instance_id}` 안 박은 경우만 충돌로 잡힌다.

### Fail-closed
에러가 하나라도 있으면 [compile_topology 가 `(None, None, errors)` 반환](../tools/sils/topology_compiler/compile.py#L398)
→ **아티팩트를 아예 안 쓴다**([main](../tools/sils/topology_compiler/compile.py#L448), exit 1).
"부분적으로 잘못된 실행 계획"이 하류로 새지 않는다.

## 1.5 어디서 도는가 — 두 개의 면(plane)

검증은 **arm64 타겟이나 vECU 도커 컨테이너 안에서 도는 게 아니다.** 두 면을 분리해야 한다.

```
┌─ 오케스트레이션 호스트 (CI 러너 / 개발 머신 / Coordinator) ─ Python 3 ─────┐
│  ① compile.py + lint_catalog.py   ← 정적 변환·검증 (여기!)                 │
│        YAML→resolved-run.yaml. DDS·도커·arm64 불필요.                       │
│  ② run_phase1.sh (start): resolved_to_shell·domain_lease·fmu_gate (Python) │
│        검증된 계획 읽기 + 런타임 게이트 → 컨테이너 기동                       │
└────────────────────────────────────┬───────────────────────────────────────┘
                                      │ docker run -e SILS_*=…
                                      ▼
┌─ vECU 컨테이너 (arm64 또는 x86) ─ C++ 만 ─────────────────────────────────┐
│  sils_vecu_viss_probe : SILS_* env 읽고 DDS publish. Python 없음. 검증 안 함.│
└──────────────────────────────────────────────────────────────────────────┘
```

| 시점 | 어디서 | Python 이 하는 일 | 검증? |
|---|---|---|---|
| **compile** (CI/PR) | 호스트 | compile + lint → resolved-run.yaml | **정적 검증 (풀)** |
| **start** | 호스트(Coordinator) | resolved_to_shell(읽기)·domain_lease·fmu_gate | 토폴로지 재검증 X, 런타임 게이트만 |
| **실행 중** | vECU 컨테이너(arm64) | 없음 (C++ 바이너리) | — |

- **검증(compile)은 arch 중립** — x86 맥에서든 CI 러너에서든 같은 판정. arm64 불필요.
- **arm64 가 필요한 건 vECU 실행(런타임)** — probe C++ 바이너리를 arm64 로 빌드·실행하는 것.
  이게 L4 attestation(`arch=aarch64`)이 증명하려는 부분(→ 10장).
- **vECU 컨테이너엔 Python 이 없어도 된다** — probe 는 C++ 이고 `SILS_*` env 만 읽는다.

## 1.6 `resolved-run.yaml` 이 있으면 validated 인가?

**맞다. 단, "언제 기준으로" 검증됐는지가 핵심.**

- ✅ **존재 자체가 통과의 증거**: fail-closed 라 에러가 있으면 파일을 안 쓴다. 디스크에 있다
  = 그게 만들어진 순간의 입력에 대해 모든 정적 검사를 통과했다. 반쪽 아티팩트는 없다.
- ⚠️ **단, 그 순간의 입력(스냅샷)에 대해서만**: 컴파일 *후에* 카탈로그/descriptor 를 고치면
  resolved-run.yaml 은 stale(옛 입력 기준). 파일은 유효하지만 지금 소스와 안 맞는다.
- 그래서 [input_hashes](../tools/sils/topology_compiler/compile.py#L401) 가 있다 —
  재해시해서 현재 소스와 일치하는지 **검증할 수단**. 다만 **Coordinator 는 launch 시 이 해시를
  자동 대조하지 않는다**(신뢰 모델). → drift 체크는 미구현(아래 "다음 후보").

## 1.7 구체 예 — smoke 토폴로지 한 번 통과

[viss_probe_smoke.yaml](../config/sils/topologies/viss_probe_smoke.yaml)(vECU 1개, docker) 컴파일 시:

1. **lint**: 카탈로그 위생 통과
2. **① 전개**: `viss-probe` × replicas 1 → 인스턴스 `viss-probe-1`
3. **② 해석**: `viss-request` 계약 → topic `/VIN/Vehicle`, domain_env `VISS_DDS_DOMAIN_ID`
4. **§9.5**: `/VIN/Vehicle` producer 1개 → OK
5. **④ 포트→env**: `clock:wall, behavior:stub…` → `SILS_CLOCK_MODE=wall SILS_BEHAVIOR=viss-request …`
6. **출력**: `resolved-run.yaml`(1 container + hashes) + `saved-service-list.yaml` → `validation OK: 1 container(s) resolved`

전 과정이 [test_compile.py](../tools/sils/topology_compiler/test_compile.py) 16개 + lint 18개
테스트로 **DDS 없이** 검증된다.

---

# 2장~11장 (채우는 중)

아래는 아직 stub. 이해하는 대로 각 장을 위 1장처럼 채운다.

- **2장 Contract / QoS Catalog** — `config/sils/contracts/*.yaml`·`qos/qos-profiles.yaml` 가
  topic·domain·type·QoS·ownership 을 **선언**하는 단일 출처. 왜 이게 "계약"인가(SUT↔vECU 합의),
  QoS 매칭(RxO) 의미론(RELIABLE/TRANSIENT_LOCAL 이 late-join 검증에 왜 중요한지), 카탈로그로
  빼는 이유(재사용·단일 소유·`semantic_version`). **검증(lint)은 1장 §1.4.A 참조 — 여기선 반복 X.**
- **3장 Descriptor → `SILS_*` env** — 선언적 포트 배선(§9.2). 1장 §1.3-④ 를 확장.
- **4장 Run Coordinator** — 도메인 임대([domain_lease.py](../tools/sils/run_coordinator/domain_lease.py))·
  local/docker 기동·teardown·고아 reap.
- **5장 vECU 포트** — Clock(wall/lockstep)·Behavior(viss-request/accumulate/p-controller)·
  Plant(analytic/replay/fmu)·Scenario(nominal/scheduled). earned-abstraction 원칙.
- **6장 Conformance 판정** — [evaluate.py](../tools/sils/conformance/evaluate.py) 가 trace 를
  상태기계·증거로 어떻게 verdict 로 환원하나.
- **7장 충실도 사다리 L0–L4** — 각 레벨이 요구하는 증거와 판정.
- **8장 Golden / 회귀** — capture ↔ compare, 결정론 모드 게이트.
- **9장 FMU 트랙** → [sils-fmu-plant.md](sils-fmu-plant.md).
- **10장 L4 Target Parity** → [sils-target-parity.md](sils-target-parity.md).
- **11장 Fleet / run 격리 / 재현성** — 다중 vECU·도메인 격리·재현성 봉인.

---

## 확장 가이드 (이 문서를 어떻게 늘리나)

- **새 장 채우기**: 위 "전체 지도" 표에서 ⬜ 인 장을 골라 1장 형식(개념→코드 링크→예)으로 쓰고 ✅.
- **코드 근거는 링크로**: 설명마다 `[file](../path#L###)` 로 정확한 위치를 건다(라인 이동 시 갱신).
- **다이어그램**: 텍스트 ASCII 는 이 .md 에, 렌더판은 [sils-understanding.html](sils-understanding.html) 에
  장별 섹션으로 추가.
- **다음 후보**:
  - 1장 심화: §9.5 소유권 판정 로직을 실패 케이스와 함께.
  - drift 체크 설계 — 실행 전 `input_hashes` 를 현재 소스와 재대조해 stale 이면 거부(미구현).
