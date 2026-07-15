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
| **2** | **Contract / QoS Catalog — 무엇을 선언하나 (계약의 의미·QoS 매칭). *검증(lint)은 1장*** | ✅ 작성됨 |
| **3** | **Descriptor → `SILS_*` env — 선언적 포트 배선(§9.2)** | ✅ 작성됨 |
| **4** | **Run Coordinator — 도메인 임대·기동·ready·verify·teardown·reap** | ✅ 작성됨 |
| **5** | **vECU 포트 — Clock / Behavior / Plant / Scenario (헥사고날)** | ✅ 작성됨 |
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

# 2장. Contract / QoS Catalog — 무엇을 선언하나 (계약 모델)

> 이 장은 **선언(declaration)** 만 다룬다. 카탈로그의 *검증*(lint)은 1장 §1.4.A 소관 — 여기선
> 반복하지 않는다.

## 2.1 한 문장

**IDL 은 타입만 정의한다.** "이 타입을 *어느 topic·domain 에서, 어떤 QoS 로, 누가 발행/구독하며,
누가 소유하는가*"는 IDL 에 없다. 그 **기계 판독 가능한 합의**를 한곳에 모은 게 Contract Catalog
([mmf-endpoints.yaml](../config/sils/contracts/mmf-endpoints.yaml))와 QoS Catalog
([qos-profiles.yaml](../config/sils/qos/qos-profiles.yaml))다.

```
IDL (VissRequest.idl)      "타입"    → struct VissRequestTopic { string payload; }
        +
Contract Catalog           "합의"    → 이 타입을 /VIN/Vehicle, VISS domain, viss-default QoS,
                                        single-owner 로, vECU=publish / SUT=subscribe
        +
QoS Catalog                "품질"    → viss-default = RELIABLE · VOLATILE · KEEP_LAST 10
```

## 2.2 계약이 선언하는 것 — 필드

한 계약([viss-request](../config/sils/contracts/mmf-endpoints.yaml)) 기준:

| 필드 | 값(예) | 의미 |
|---|---|---|
| `id` | `viss-request` | 계약 이름. Descriptor/Topology 가 이 id 로 참조 |
| `semantic_version` | `0.1.0` | 계약 변경 추적(x.y.z) |
| `transport` | `dds` | 전송 |
| `domain_env` | `VISS_DDS_DOMAIN_ID` | **어느 DDS domain env 에 바인딩**(★ MMF 아닌 VISS, §3.4) |
| `idl` / `struct` / `registered_type` | `VissRequest.idl` / `VissRequestTopic` | 타입 출처·이름 |
| `topic` | `/VIN/Vehicle` | wire 상의 topic 이름(per-instance 면 `{instance_id}` 템플릿) |
| `qos_profile` | `viss-default` | QoS Catalog 참조 |
| `ownership` | `single` | 소유권 모델(§2.5) |
| `payload` | `{payload: string, json}` | 페이로드 필드·인코딩 |
| `late_sample_policy` | `next_tick` | tick 경계 이후 도착 입력 처리(§7.3, evaluate 강제) |
| `roles` | `sut: subscribe / vecu: publish` | **방향 합의**(§2.3) |

## 2.3 왜 "계약"인가 — SUT ↔ vECU 합의

DDS 는 topic·type·QoS 가 **양쪽에서 맞아야** 통신이 성립한다. 계약은 그 합의를 명문화한다:

```
        contract: viss-request
        topic /VIN/Vehicle · type VissRequestTopic · qos viss-default
   ┌──────────────┐                                   ┌──────────────┐
   │  vECU (probe) │ ── publish ──▶  /VIN/Vehicle  ──▶ │  SUT (VISS)  │ subscribe
   │  roles.vecu  │                                    │  roles.sut  │
   └──────────────┘                                   └──────────────┘
        같은 topic + 같은 type + 호환 QoS  ⇒  match. 하나라도 어긋나면 침묵(no data).
```

- `roles` 가 **방향**을 고정한다: Phase 1 slice 에서 vECU 는 클라이언트라 요청을 **publish**,
  VISS(SUT)가 **subscribe**. (lint 는 "둘 다 publish → 구독자 없음"을 경고한다 — 검증은 1장.)
- 계약이 깨지면(topic 오타·타입 불일치·QoS 비호환) endpoint 가 **match 안 되고 조용히 실패**한다.
  그래서 이 합의를 사람 기억이 아니라 **파일로 고정**하고 정적 검증한다.

## 2.4 QoS 매칭 (RxO) — 왜 TRANSIENT_LOCAL 이 중요한가

### 먼저: QoS 는 "하나"가 아니라 **정책(policy)의 묶음**이다
DDS 에서 QoS 는 여러 정책의 세트다. `TRANSIENT_LOCAL` 은 QoS 자체가 아니라 그중
**Durability 정책의 한 값**이다.

```
QoS (묶음)
├─ Reliability   : { BEST_EFFORT, RELIABLE }
├─ Durability    : { VOLATILE, TRANSIENT_LOCAL, TRANSIENT, PERSISTENT }   ← TRANSIENT_LOCAL 은 이 정책의 한 값
├─ History       : { KEEP_LAST(depth), KEEP_ALL }
└─ Deadline · Liveliness · Ownership · ResourceLimits …                    (그 외 다수)
```

그래서 QoS Catalog 의 한 "프로파일"은 정책 여러 개를 묶은 것이다
([qos-profiles.yaml](../config/sils/qos/qos-profiles.yaml)):

```yaml
viss-sils-verifiable:              # ← 하나의 QoS 프로파일(묶음)
  reliability: RELIABLE            #   Reliability 정책 값
  durability:  TRANSIENT_LOCAL     #   Durability  정책 값
  history: { kind: KEEP_LAST, depth: 256 }   # History 정책 값
```

### QoS 는 "협상(nego)"이 아니다 — **RxO 호환성 판정**이다
흔한 오해: "통신하면서 서로 QoS 를 맞춰간다". DDS 는 그렇게 하지 않는다.

- Writer 는 자기 QoS 를 **독립적으로 '제공(Offered)'**, Reader 는 **독립적으로 '요청(Requested)'** —
  통신 *전에* 앱이 각자 **미리 정한다**.
- discovery 때 DDS 는 **호환성만 판정**한다 (RxO: Requested vs Offered). **중간값 타협도, 값 변경도 없다.**

```
Writer.offered ──┐   제공이 요청을 만족?
                 ├── YES → 매칭, 데이터 흐름
Reader.requested─┘   NO  → 매칭 실패, 침묵(데이터 0). incompatible_qos 이벤트만. (아무도 값 안 바꿈)
```

즉 **양쪽이 미리 각자 선언 → DDS 는 "붙일지 말지"만 판정**. 이게 §2.6 "왜 카탈로그로 미리
고정하나"의 이유다 — 자동으로 안 맞춰주니, 애초에 호환되게 선언돼 있어야 한다.

### 만족 규칙 (제공 ⊒ 요청 — "제공이 같거나 세야")

| 정책 | Offered 가 Requested 를 만족하려면 |
|---|---|
| reliability | Writer `RELIABLE` ⊒ Reader `BEST_EFFORT`/`RELIABLE` (BEST_EFFORT writer 는 RELIABLE reader 못 만족) |
| durability | Writer `TRANSIENT_LOCAL` ⊒ Reader `VOLATILE`/`TRANSIENT_LOCAL` |

**실제로 물린 defect**(코드 주석에 기록): lockstep/plant run 은 tick 을 **짧게 몰아 publish 후
idle** 한다. 독립 기동한 verifier(reader)가 마지막 샘플 *이후에* match 하면, **VOLATILE writer 는
히스토리를 안 남겨** → endpoint 는 match 됐는데 **샘플 0개**(matched-but-no-data).

→ 해결: probe writer/verify reader 가 [viss-sils-verifiable](../config/sils/qos/qos-profiles.yaml)
(`TRANSIENT_LOCAL`, depth 256)을 기본으로 쓴다(코드상 `SILS_DURABILITY=transient-local` 기본).
`TRANSIENT_LOCAL` 은 **superset offer** 라 VOLATILE reader 와도 RxO 호환 → late-join 이어도
히스토리를 받는다. `SILS_DURABILITY=volatile` 로 원래 `viss-default` 동작을 강제할 수도 있다.

> QoS 는 "장식"이 아니라 **누가 언제 데이터를 받는지를 좌우하는 계약 조항**이다. 그래서 카탈로그가
> 관리하고, 실 RxO 는 Phase 0A Contract Spike 에서 실측·확정한다(현재 값은 후보).

## 2.5 Ownership 모델

`ownership` 은 "한 topic 을 누가 소유(발행)할 수 있나"를 선언한다. 1장 §1.4-B 의 §9.5 검사가
이걸 **RESOLVED 토픽 기준**으로 강제한다.

| ownership | 의미 | §9.5 |
|---|---|---|
| `single` | 단일 owner. 한 topic 에 producer 정확히 1 | producer 2+ → 위반 |
| `per-instance` | replica 마다 자기 topic 단독 소유. `topic` 에 `{instance_id}` 필수 | N 인스턴스 = N topic = 각 1 owner → valid |
| `shared` / `multi` | co-producer 허용 | 다중 producer 면제 |

[fleet-example.yaml](../config/sils/contracts/fleet-example.yaml) 의 `wheelspeed` 가
`per-instance` + `/VIN/WheelSpeed/{instance_id}` 예: replica 4개가 `/VIN/WheelSpeed/wheelspeed-1…4`
로 각자 다른 topic 을 소유 → 같은 topic 다중 producer(single 위반)가 아니라 §9.5-valid.

## 2.6 왜 IDL 인라인이 아니라 카탈로그로 빼나

- **IDL 은 타입 전용** — topic/QoS/ownership/버전은 IDL 문법에 없다.
- **단일 출처**: 여러 Descriptor 가 같은 계약을 `contract_id` 로 참조(재사용). 정의가 한 곳.
- **단일 소유·추적**: `(topic, domain_env)` 유일성·`semantic_version` 으로 "누가 이 topic 을
  소유하고 계약이 언제 바뀌었나"를 기계가 추적(1장 §1.4 lint 가 강제).
- **QoS 재사용**: endpoint 마다 QoS 를 반복하지 않고 `qos_profile` 로 참조(§9.3).

## 2.7 검증은 어디서?

이 장은 **선언**만 설명했다. 이 카탈로그가 *올바른지*(필수필드·중복 id·dangling qos·IDL 실존·
domain_env 오타·enum·per-instance `{instance_id}`…)는 **1장 §1.4.A 의 lint_catalog** 가,
토폴로지 문맥의 §9.5 소유권은 **1장 §1.4.B 의 compile** 이 검증한다. → 6장(런타임 conformance)·
8·10장(회귀·패리티)은 또 다른 검증 계층(전체 지도의 cross-cutting 표 참조).

---

# 3장. Descriptor → `SILS_*` env — 선언적 포트 배선 (§9.2)

> 1장 §1.3-④ 를 확장한다. 2장이 **wire 계약**(topic·QoS·ownership)을 다뤘다면, 이 장은 **vECU
> 내부 동작**(어떤 clock·behavior·plant·scenario 로 돌지)을 Descriptor 선언에서 실행 env 로
> 잇는 다리다.

## 3.0 먼저 — "컴파일"이 두 개다 (헷갈리기 쉬움)

`SILS_*` 는 vECU 를 *컴파일*하는 입력이 **아니다**. **실행 때 주입되는 런타임 환경변수**다.
vECU 는 descriptor 와 무관하게 미리 빌드된 **범용 C++ 바이너리 하나**이고, 실행 때 `std::getenv`
로 `SILS_*` 를 읽어 포트를 고른다(`SILS_PLANT`→어떤 IPlant, `SILS_BEHAVIOR`→어떤 IBehavior).

| 무엇 | 도구 | 언제 | 산출물 | `SILS_*` |
|---|---|---|---|---|
| **vECU 빌드** | `build.sh` (CMake·C++) | 한 번, 미리 | probe 바이너리·도커 이미지 (**범용**) | ✗ 모름 |
| **Topology 컴파일** | `compile.py` | 설정마다 | resolved-run.yaml (**+ SILS_\* env**) | ✓ 생성 |
| **실행** | `run_phase1.sh` | run 마다 | 바이너리 + SILS_* → 동작 | ✓ 주입·읽음 |

즉 **descriptor 마다 재컴파일하지 않는다** — 같은 이미지에 **env 만 바꿔** 다양한 vECU 를 만든다.
(이 문서의 "Topology 컴파일"[1장]과 "vECU 빌드"는 서로 다른 것.)

## 3.1 한 문장 — "정문(선언)"으로 포트를 켠다

vECU 의 포트(clock/behavior/plant/scenario/attestation)는 **Descriptor 에 선언**하면 파이프라인이
probe 가 읽는 `SILS_*` 환경변수로 **자동 배선**한다. 손으로 `export SILS_PLANT=fmu …` 할 필요 없이,
Descriptor 가 `plant: {type: fmu}` 라고 쓰면 그게 실행까지 이어진다.

```
Descriptor (선언)              compile.py                resolved_to_shell.py         run_phase1.sh        probe (C++)
clock/behavior/plant/    →  resolve_runtime_env  →   export SILS_* / --env-args  →  local:상속       →  SILS_* 읽고
scenario/attestation        (SILS_* 로 변환·         (셸 주입 형태 2가지)          docker/fleet:-e      동작
                             resolved-run 에 실음)
```

### Descriptor 파일은 어디 있나 (실제 경로)

Descriptor 는 레포의 손으로 쓴 YAML 이다 — `service/sils/vecu/descriptors/*.yaml`:

```
service/sils/vecu/descriptors/
├─ viss_probe.yaml            # Phase 1 스모크(behavior: stub, plant: none)
├─ viss_scenario_probe.yaml   # L2 accumulate + scheduled 결함주입 예
└─ wheelspeed_probe.yaml      # per-instance fleet 멤버
```

Topology([config/sils/topologies/*.yaml](../config/sils/topologies/))가 `vecus[].descriptor` 로
**경로를 참조**하고, compile.py 가 그 경로로 읽는다([load](../tools/sils/topology_compiler/compile.py#L140)):

```yaml
# config/sils/topologies/viss_scenario.yaml
vecus:
  - name: viss-scenario
    descriptor: service/sils/vecu/descriptors/viss_scenario_probe.yaml   # ← 이 파일을 읽음
```

실제 참조 3건:
| Topology | → Descriptor |
|---|---|
| [viss_probe_smoke.yaml](../config/sils/topologies/viss_probe_smoke.yaml) | `service/sils/vecu/descriptors/viss_probe.yaml` |
| [viss_scenario.yaml](../config/sils/topologies/viss_scenario.yaml) | `service/sils/vecu/descriptors/viss_scenario_probe.yaml` |
| [wheelspeed_fleet.yaml](../config/sils/topologies/wheelspeed_fleet.yaml) | `service/sils/vecu/descriptors/wheelspeed_probe.yaml` |

## 3.2 무엇이 변환되나 — Descriptor 필드 → `SILS_*`

[resolve_runtime_env](../tools/sils/topology_compiler/compile.py#L118) 의 매핑(발췌):

| Descriptor | `SILS_*` env | 비고 |
|---|---|---|
| `clock.mode` | `SILS_CLOCK_MODE` | wall / lockstep |
| `behavior.type` | `SILS_BEHAVIOR` | `stub`→`viss-request`, accumulate, p-controller |
| `behavior.spec.period_ms` | `SILS_VISS_PROBE_PERIOD_MS` | |
| `behavior.spec.kp` / `setpoint` | `SILS_CTRL_KP` / `SILS_CTRL_SETPOINT` | p-controller |
| `protocol.mode` / `response_topic` | `SILS_PROTOCOL` / `SILS_RESPONSE_TOPIC` | reqresp |
| `plant.type` (≠none) | `SILS_PLANT` | first-order / replay / fmu |
| `plant.spec.drag` / `init` / `replay_file` | `SILS_PLANT_DRAG` / `_INIT` / `SILS_PLANT_REPLAY_FILE` | |
| `plant.fmu.file` / `input_vr` / `output_vr` | `SILS_FMU_FILE` / `SILS_FMU_INPUT_VR` / `_OUTPUT_VR` | |
| `plant.fmu.allowlist` / `allowlist_id` | `SILS_FMU_ALLOWLIST` / `_ALLOWLIST_ID` | 5장·9장 보안 게이트 |
| `scenario.type` / `faults` | `SILS_SCENARIO` / `SILS_SCENARIO_FAULTS` | `3:drop,7:corrupt` |
| `attestation.{arch,gateway,protocol}` | `SILS_ATTEST_ARCH` / `_GATEWAY` / `_PROTOCOL` | 10장 L4 |

이 env dict 는 resolved-run.yaml 의 각 container 에 `env:` 로 실린다(1장 §1.3 변환 출력).

## 3.3 세 갈래 주입 경로 — local / docker / fleet

[resolved_to_shell.py](../tools/sils/topology_compiler/resolved_to_shell.py) 가 **두 가지 형태**로
방출하고, [run_phase1.sh](../service/sils/run_coordinator/run_phase1.sh) 가 백엔드별로 주입한다:

| 경로 | 방출 형태 | 주입 방식 |
|---|---|---|
| **local** | `export SILS_*='...'` (eval 용, 따옴표) | coordinator 가 eval → 자식 probe 가 **상속** |
| **docker** | 위 export + `RESOLVED_ENV_KEYS` | 키 목록으로 `-e NAME=${!NAME}` 재조립 |
| **fleet** | `--env-args` (raw `KEY=VALUE`) | `env`(local)·`-e`(docker) 로 멤버별 주입 |

- **왜 두 형태인가**: eval 용 `export` 는 따옴표가 붙어 `env`/`-e` 인자로 못 쓴다. 그래서 docker·fleet
  용으로 따옴표 없는 raw `KEY=VALUE` 를 주는 `--env-args` 모드를 따로 둔다.
- **local 이 export 를 쓰는 부수 효과**: coordinator 가 `SILS_FMU_FILE` 을 상속받아 launch 전
  **fmu_preflight**(보안 게이트, 9장)가 그 값을 볼 수 있다.

## 3.4 Descriptor 소유 vs Coordinator 소유

`resolve_runtime_env` 는 **포트 설정만** 만든다. 아래는 **넣지 않는다** — Run Coordinator 가 소유:

| Coordinator 가 주입 | 이유 |
|---|---|
| `SILS_RUN_ID` | run 마다 유일(추적·teardown) |
| `SILS_VECU_INSTANCE_ID` | 인스턴스 식별(fleet) |
| `SILS_TOPIC` | per-instance 해석된 topic(§9.5, 1장 §1.3-③); resolved endpoint 에서 옴 |
| `VISS_DDS_DOMAIN_ID` | run.domain_id (도메인 임대, 4장) |

즉 **"무엇을 하는 vECU인가"(포트)는 Descriptor**, **"이번 run 의 정체성"(누가·어디서)은 Coordinator**.

## 3.5 왜 오프라인으로 검증되나

이 배선이 보장할 것은 단 하나: **"Descriptor 가 만드는 env 문자열 = 손으로 넣던 env 문자열."**
이 문자열 동등성은 DDS·빌드 없이 순수 파이썬/셸 테스트로 단언된다(1장 §1 도구들과 같은 방식).
probe(C++)는 그 env 를 읽을 뿐이라, 문자열이 같으면 런타임 동작은 자동으로 따라온다 — 그래서
이미 실측한 L1/L3/scenario 런타임 결과가 그대로 성립한다.
검증: [test_compile.py](../tools/sils/topology_compiler/test_compile.py) 의 포트→env·smoke·예제·
export/env-args 케이스.

## 3.6 실제 예 — 같은 바이너리, env 만 다르게 (실측 출력)

**① vECU 는 한 번만 빌드**한다(descriptor 무관, 범용):
```bash
./build.sh --build-sils            # → sils_vecu_viss_probe (범용 바이너리·이미지)
```

**② 같은 바이너리에 두 다른 토폴로지를 컴파일**하면 `SILS_*` env 만 달라진다. 아래는 실제 출력:

```console
# A) viss_probe_smoke — behavior: stub
$ python3 tools/sils/topology_compiler/compile.py \
      config/sils/topologies/viss_probe_smoke.yaml --out-dir /tmp/smoke
$ python3 tools/sils/topology_compiler/resolved_to_shell.py \
      /tmp/smoke/resolved-run.yaml --index 0 --env-args
SILS_CLOCK_MODE=wall
SILS_BEHAVIOR=viss-request                 # stub → viss-request
SILS_VISS_PROBE_PERIOD_MS=500
SILS_VISS_PROBE_RESPONSE_TOPIC_PREFIX=sils/viss_probe/response
SILS_VISS_PROBE_REQUEST_ACTION=get
SILS_VISS_PROBE_REQUEST_PATH=Vehicle.Speed

# B) viss_scenario — behavior: accumulate + 결함주입
$ python3 tools/sils/topology_compiler/compile.py \
      config/sils/topologies/viss_scenario.yaml --out-dir /tmp/scen
$ python3 tools/sils/topology_compiler/resolved_to_shell.py \
      /tmp/scen/resolved-run.yaml --index 0 --env-args
SILS_CLOCK_MODE=wall
SILS_BEHAVIOR=accumulate                   # ← 다름
SILS_VISS_PROBE_PERIOD_MS=200              # ← 다름
SILS_SCENARIO=scheduled                    # ← 결함주입 포트 켜짐
SILS_SCENARIO_FAULTS=3:drop,7:duplicate
SILS_ATTEST_ARCH=x86_64
SILS_ATTEST_GATEWAY=sim
SILS_ATTEST_PROTOCOL=sim
```

**③ 실행 — 같은 바이너리를 env 만 바꿔** 띄운다:
```bash
service/sils/run_coordinator/run_phase1.sh start \
    --from-resolved /tmp/scen/resolved-run.yaml --bin-dir <build-dir> --backend local
# run_phase1 이 위 SILS_* 를 주입 → 같은 sils_vecu_viss_probe 가
#   accumulate behavior + scheduled 결함(tick 3 drop, tick 7 duplicate) 으로 동작
```

**A vs B: 바이너리는 동일**, resolved-run 의 env 만 다르다. A 는 `viss-request` 를 500ms 마다,
B 는 `accumulate` + tick 3 drop·tick 7 duplicate. **재빌드 없음** — §3.0 의 "같은 이미지, env 만"
그대로다. (선언 원본: [viss_scenario_probe.yaml](../service/sils/vecu/descriptors/viss_scenario_probe.yaml))

---

# 4장. Run Coordinator — 실행 오케스트레이터

> 1~3장은 **실행 전(정적)** 이었다. 이 장부터 **실행**이다. Run Coordinator
> ([run_phase1.sh](../service/sils/run_coordinator/run_phase1.sh))는 검증된 계획(resolved-run)과
> SILS_* env 를 받아 **실제로 vECU 를 띄우고**, 도메인 격리·생명주기·정리를 책임지는 호스트측
> bash 오케스트레이터다.

## 4.1 한 문장

Run Coordinator 는 한 run 을 **도메인 확보 → (FMU 게이트) → 기동 → ready 대기 → verify → 판정 →
teardown** 으로 몰고 가며, 병렬 run 이 서로 간섭하지 않게 **DDS domain 을 격리**하고, 죽어도 자원이
새지 않게 **lease·state 파일로 회수 가능**하게 만든다.

## 4.2 네 서브명령 ([main](../service/sils/run_coordinator/run_phase1.sh) dispatch)

| 명령 | 하는 일 |
|---|---|
| `start` | vECU 1개 run: 기동→ready→verify→판정→teardown |
| `fleet` | vECU N개, **한 도메인 공유**, 동시 기동(§11/§13) |
| `cleanup` | 특정 run 을 명시적으로 teardown(idempotent) |
| `reap` | 고아 run 회수 + 만료 domain lease 정리(§14.4) |

## 4.3 한 run 의 생명주기 (`start`)

```
domain 확보 ─▶ fmu_preflight ─▶ 기동 ─────▶ ready 대기 ─▶ verify ────▶ 판정 ─────▶ teardown
(lease/inject)  (§5.6 게이트)   local/docker  (probe "ready"  (독립 one-shot  run_pass/    (컨테이너·PID 정리
                              백엔드         trace 폴링,     구독자로 샘플   run_fail      + domain 반환)
                                             timeout 시 실패) 검증)          terminal
   trace:  domain_leased        process_running   vecu_ready    verify_invoke   run_pass     domain_released
                                                  /ready_timeout /verify_pass                cleanup_done
```

- **ready 대기**: "VecuReady" 대용 — probe 가 스스로 찍는 `"event":"ready"` trace 를 폴링한다
  (local 은 trace 파일, docker 는 `docker logs`). `ready_timeout_sec` 넘으면 `ready_timeout` +
  teardown + 실패.
- **verify**: Coordinator 가 **독립 one-shot 구독자**([sils_vecu_viss_verify](../service/sils/vecu/src/viss_verify_main.cc))를
  띄워 실제 샘플이 오는지 본다. 이게 §11 L0(엔드포인트 match + 샘플 수신)의 런타임 증거.
- 모든 단계가 **trace.jsonl 한 줄씩** 남고, 그 스트림이 6장 evaluate 의 입력이 된다.

## 4.4 도메인 임대 — 병렬 run 격리 (§12, [domain_lease.py](../tools/sils/run_coordinator/domain_lease.py))

DDS 는 **domain id** 로 통신을 격리한다. 같은 domain 이면 서로 샘플이 섞이므로, **병렬 run 은
서로 다른 domain** 을 써야 한다. 그런데 domain 은 유한하다 → 유한 pool 에서 **임대(lease)**:

- **atomic 획득**: per-domain lease 파일을 `O_CREAT|O_EXCL` 로 만든다 → 두 러너가 같은 domain 을
  동시에 잡는 race 를 OS 가 막는다. pool 소진 시 `allocate` 는 **exit 3**.
- **TTL + owner_pid**: lease 에 만료시각·소유 pid 를 기록. Coordinator 가 release 없이 죽어도
  (crash) 만료되면 **reap** 이 회수 → domain 누수 방지(§14.4).
- **release 는 idempotent**: 같은 run_id lease 를 지운다. teardown 이 항상 호출.
- **pool 분리**: `--lease-dir`(env `SILS_DOMAIN_LEASE_DIR`)로 러너별 pool 을 나눠 CI 간섭 차단(§12.2).
  이 도구는 **메커니즘**이고, "어느 러너가 어느 pool 을 쓰나"는 CI 정책.

> domain 은 `--domain` 으로 **직접 주입(inject)** 하거나 `--domain-pool LO-HI` 로 **임대(allocate)**
> 한다. 주입은 SUT 의 실제 domain 에 붙일 때(§3.4), 임대는 병렬 격리가 필요할 때.

## 4.5 상태·추적 파일 — 두 개의 사이드카

Coordinator 는 run 마다 `STATE_DIR` 에 두 파일을 둔다
([state_file / trace_file](../service/sils/run_coordinator/run_phase1.sh#L82)):

| 파일 | 역할 |
|---|---|
| `{run_id}.state` | **teardown 근거**. `BACKEND`·`PROBE_PID`/`CONTAINER_NAME`·`DOMAIN`·`DOMAIN_LEASED` 등. 죽어도 이 파일로 정리 가능 |
| `{run_id}.trace.jsonl` | **판정 입력**. 생명주기 이벤트 한 줄씩 → 6장 evaluate 가 읽음 |

state 파일에 `CONTAINER_NAME` 을 `docker run` **전에** 먼저 적는 이유: run 과 append 사이에서
죽어도 teardown 이 컨테이너를 찾아 지울 수 있게 하기 위함(§14.4 crash 안전).

## 4.6 teardown & reap — 새지 않게

- **[do_teardown](../service/sils/run_coordinator/run_phase1.sh#L127)** (idempotent): state 를 읽어
  컨테이너(`docker stop/rm`)·프로세스(`TERM`→안 죽으면 `KILL`)를 정리하고, `DOMAIN_LEASED` 면 lease
  release, `domain_released`·`cleanup_done` trace. 이미 정리됐으면 `cleanup_noop`(성공).
- **[cmd_reap](../service/sils/run_coordinator/run_phase1.sh#L198)**: Coordinator 가 mid-run 에 죽어
  teardown 을 못 한 **고아 run** 을 state 파일 기준으로 회수 + state 파일이 이미 사라진 **만료 lease**
  도 정리. 주기적으로 돌리는 안전망(§14.4).

## 4.7 두 백엔드 — local vs docker

| | local | docker |
|---|---|---|
| 실행 형태 | 호스트 프로세스(`&`, PROBE_PID) | 컨테이너(`docker run -d`, CONTAINER_NAME) |
| env 주입 | export 상속 + inline(3장) | `-e` 플래그(3장) |
| 네트워크 | 그대로 | `--network host` + `--ipc=host`(SHM 데이터 경로) |
| verify | **둘 다 동일** — 호스트측 one-shot `sils_vecu_viss_verify` |

`--ipc=host` 는 장식이 아니다 — 없으면 컨테이너가 자기 `/dev/shm` 을 써서 Fast DDS SHM 전송이
호스트 reader 에 안 보인다 → **endpoint 는 match 인데 샘플 0**(2026-07 실측). 호스트 IPC 공유로
SHM 데이터 경로를 연다.

## 4.8 실행 환경 — 컨테이너 안에서 probe 가 어떻게 뜨나

SiLS vECU 이미지 `runtime-mmf-sils-vecu`([Dockerfile.rel](../docker/Dockerfile.rel#L147))는 probe
바이너리를 `/oss/bin/` 에 넣고 `ENTRYPOINT ["/oss/entrypoint.sh"]`
([sils_vecu_entrypoint.sh](../docker/entrypoint/sils_vecu_entrypoint.sh))를 둔다. 엔트리포인트가
하는 건 딱 하나 — **probe 를 PID 1 로 `exec`**:

```
docker run -d -e SILS_*=… -e VISS_DDS_DOMAIN_ID=90 <image>
   └─▶ entrypoint.sh ──exec──▶ sils_vecu_viss_probe (PID 1)
                                   └─ getenv(SILS_*) 로 포트 선택 → DDS publish
```

- **컨테이너 수명 = probe 프로세스 수명** (멀티서비스 fan-out 없음 — MMF core 컨테이너와 다름).
- probe 가 PID 1 이라 `docker stop` → **SIGTERM 이 probe 에 직접** → wall-clock 루프 배수 + participant
  정리(graceful). 4장 §4.6 teardown 이 이걸 노린다.
- local 백엔드는 컨테이너 없이 **호스트 프로세스로 같은 바이너리**를 돌린다(§4.7). 실행 형태만 다르고
  바이너리·env 계약은 동일.
- `VISS_DDS_DOMAIN_ID` 는 일부러 기본값을 안 준다 → 누락 시 probe 로그로 드러남(§3.4).

### entrypoint·이미지는 범용 — vECU 정보를 담지 않는다

[sils_vecu_entrypoint.sh](../docker/entrypoint/sils_vecu_entrypoint.sh) 전문에는 vECU 를 특정하는
값이 **하나도 없다**:

- ✅ 하는 일: `LD_LIBRARY_PATH` 세팅 → 바이너리 존재 확인 → `exec /oss/bin/sils_vecu_viss_probe`
- ❌ 없는 것: behavior/plant/scenario 값·instance_id·domain — 전부 없음

모든 vECU 인스턴스(viss-request든 accumulate+결함이든 wheelspeed든)가 **동일한 entrypoint·동일한
이미지**를 쓴다. core 컨테이너([core_entrypoint.sh](../docker/entrypoint/core_entrypoint.sh), ~4KB,
다중 서비스 supervised fan-out)와 달리 sils 는 **probe 하나만 exec**(~1KB) — 그래서 "컨테이너 수명 =
probe".

### 그럼 "실 vECU 정보"는 어디에? — 세 군데로 나뉜다

"vECU 정보 = env" 는 반쪽이다. 실제 vECU 를 이루는 것은 세 군데에 있다:

| 무엇 | 어디 | 언제 | 예 |
|---|---|---|---|
| **① 로직(구현)** | **probe 바이너리** (컴파일 baked) | 빌드 | behavior/plant/scenario.hpp 의 실제 C++ — 범용 바이너리에 **모든 포트 구현**이 들어있고 env 가 하나를 고름 |
| **② 선택·파라미터·경로** | **`SILS_*` env** (런타임 주입) | 실행 | `SILS_BEHAVIOR=accumulate` · `SILS_CTRL_KP=0.8` · `SILS_SCENARIO_FAULTS=3:drop` · `SILS_FMU_FILE=/models/vehicle.fmu`(경로) |
| **③ 모델 아티팩트(데이터)** | **컨테이너 파일시스템** (파일) | 이미지 bake / mount | 실제 `.fmu` 파일 · replay 데이터 — env 는 **PATH 만** 주고 파일 자체는 FS 에 있어야 함 |

- **로직**은 바이너리에 컴파일된다([viss_probe_main.cc 가 behavior/plant/scenario.hpp include](../service/sils/vecu/CMakeLists.txt#L85))
  — 범용 바이너리가 4개 포트의 **모든 구현**을 담고, env(②)가 그중 하나를 선택. 그래서 "어떤 vECU인가"의
  **행동 로직 자체는 이미 바이너리 안**에 있다(env 는 스위치일 뿐).
- **모델 데이터**(FMU 등)는 파일이라 **컨테이너 FS 에 실제로 존재**해야 한다. `SILS_FMU_FILE` 은
  경로일 뿐 — 파일이 없으면 로드 실패(fmu_gate 가 sha256 로 검증, §5.6/9장). 현재 local 백엔드는
  호스트 FS 에서 읽고, docker 로 FMU 를 쓰려면 이미지에 bake 하거나 `-v` mount 필요
  (**자동 마운트 미구현** — FMU 는 지금까지 local 로 검증).

즉 **"실 vECU 정보"는 env 에만 있는 게 아니다** — 로직은 바이너리(①), 모델 데이터는 파일(③),
env(②)는 그 둘을 **고르고 파라미터를 주는 스위치**다.

### 실행 중 컨테이너가 무슨 vECU 인지 확인

entrypoint·이미지가 범용이라 "이 컨테이너가 무슨 vECU 인지"는 **주입된 env 를 봐야** 안다:

```bash
docker inspect -f '{{range .Config.Env}}{{println .}}{{end}}' <container> | grep '^SILS_'
#   SILS_BEHAVIOR=accumulate
#   SILS_SCENARIO=scheduled
#   SILS_SCENARIO_FAULTS=3:drop,7:duplicate
#   ...
```

## 4.9 MMF 통합 — 두 경로 (기동을 누가 하나)

vECU 를 실제로 띄우는 주체는 두 가지다:

**(A) Phase 1 직접 경로** — [run_phase1.sh](../service/sils/run_coordinator/run_phase1.sh) 가 직접
`docker run`(또는 로컬 프로세스). **현재 구현·검증된 런타임**. MMF 서비스 제어를 우회하는 **부트스트랩**
(vECU 런타임 자체를 먼저 세우는 최소 경로).

**(B) MMF 통합 경로** (아키텍처 목표) — compile 이 낸
[saved-service-list.yaml](../tools/sils/topology_compiler/compile.py#L336)(MMF ServiceRegistry 포맷)을
MMF 의 기존 컨테이너 제어가 소비한다:

```
saved-service-list.yaml   (name · image · container-type:docker · extra_args(-e SILS_*) · services[])
        │
adsw_manager   "the yaml owner" (service/extension/{ros2,agl}/adsw_manager)
        │        saved-service-list.yaml + ServiceRegistry 소유, Save() 영속
lifecycle_mgr  DDS 제어 (service/core/lifecycle_mgr)
        │        StartContainer/StopContainer · ContainerType={DOCKER, DOCKER_COMPOSE, K3S} · 회복정책
        ▼
   컨테이너 기동 — MMF 자기 서비스와 동일 경로
```

- `extra_args` 에 **`-e SILS_*`(3장 포트 env)**가 그대로 들어가 MMF 가 **올바른 포트로** 컨테이너를 띄운다.
- MMF 는 DOCKER 외 **DOCKER_COMPOSE·K3S** 도 지원([ContainerType](../service/core/lifecycle_mgr/include/ContainerType.hpp)).

| | (A) Phase 1 직접 | (B) MMF 통합 |
|---|---|---|
| 기동 주체 | run_phase1.sh `docker run` | adsw_manager + lifecycle_mgr |
| 소비 산출물 | `resolved-run.yaml` | **`saved-service-list.yaml`** |
| 상태 | 구현·검증됨 | 포맷 방출됨, MMF 제어 배선은 목표 |
| 용도 | 런타임 부트스트랩·CI | 실제 MMF 플릿 운용 |

→ compile 이 **두 산출물**을 내는 이유: `resolved-run.yaml`(A용) + `saved-service-list.yaml`(B용, MMF 정문).

## 4.10 멀티 인스턴스 — "N 컨테이너 = N 인스턴스"

토폴로지 `vecus[]` 는 여러 vECU 를, 각각 `replicas` 로 여러 복제본을 가질 수 있고, compile 의
[expand_replicas](../tools/sils/topology_compiler/compile.py#L72) 가 이를 **N 개 container 로 전개**한다
(resolved-run·saved-service-list **둘 다 N개**). 하지만 **한 컨테이너/프로세스 = 정확히 1 인스턴스**:

```
topology.vecus[]  ──expand──▶  c1 (instance_id=ws-1) → probe(PID1)   ┐
                               c2 (instance_id=ws-2) → probe(PID1)   ├ fleet: 도메인 1개 공유,
                               c3 (instance_id=ws-3) → probe(PID1)   │ 동시 기동
                               c4 (instance_id=ws-4) → probe(PID1)   ┘
```

- entrypoint 가 probe **하나**만 exec 하고 "컨테이너 수명 = 그 probe" 라, **한 프로세스에 인스턴스 여러
  개는 안 들어간다.**
- 여러 인스턴스 = **분리된 N 개 컨테이너/프로세스**(각자 instance_id), `fleet` 명령이 도메인 1개를 공유해
  동시 기동(§4.2).
- 한 프로세스에 여러 vECU 를 넣는 in-process 다중화는 [plant.hpp](../service/sils/vecu/src/plant.hpp) 주석대로
  fmi4c thread-safety·임시경로 충돌 시험 전 **미허용** — 지금은 "프로세스=인스턴스" 모델.

즉 "vECU 패키지/플릿"엔 인스턴스가 여러 개 **들어가지만**, 그건 **컨테이너 N개**로 실현된다.

## 4.11 다음으로

Coordinator 가 남긴 **trace.jsonl** 은 그 자체로 판정되지 않는다. 6장 **evaluate.py** 가 이 스트림을
상태기계·증거로 읽어 verdict 로 환원한다(런타임 conformance — 전체 지도 cross-cutting 표의 ②).

---

# 5장. vECU 포트 — Clock / Behavior / Plant / Scenario

> §4.8 ①에서 "행동 로직은 바이너리 안"이라 했다. 그 바이너리 안이 이 장이다. probe 는 **4개
> 헥사고날 포트**로 짜여 있고, tick 루프가 이들을 조립한다. env(3장)는 각 포트의 **어느 구현을
> 쓸지 고르는 스위치**일 뿐 — 구현 자체는 여기 헤더들에 컴파일돼 있다.

## 5.1 한 문장

vECU 내부는 **IClockSource·IBehavior·IPlant·IScenario** 네 포트로 나뉘고, [tick 루프](../service/sils/vecu/src/viss_probe_main.cc)
가 매 tick 이들을 순서대로 호출해 하나의 vECU 동작을 만든다. 각 포트는 **≥2개 구현**을 갖고 env 로 선택된다.

## 5.2 헥사고날 포트 + "earned abstraction"

포트(=인터페이스)로 빼는 건 **구현이 2개 이상 생겼을 때만** 한다(투기적 추상화 금지). clock_source.hpp
주석 그대로: *"clock 은 실제 구현이 둘이라 추상화가 earned 됐다. IBehavior/IPlant/IScenario 도 두 번째
구현이 나올 때 같은 모양을 따른다 — 미리 하지 않는다."* 그래서 probe 본체는 **포트만 알고 구현은 모른다** —
`SILS_*` 가 `Make*()` 팩토리에서 구현을 고른다.

## 5.3 네 포트 지도

| 포트 | 인터페이스 | 구현 (≥2) | env 선택 | 파일 |
|---|---|---|---|---|
| **Clock** | `IClockSource` | WallClockSource · LockstepClockSource | `SILS_CLOCK_MODE=wall\|lockstep` | [clock_source.hpp](../service/sils/vecu/src/clock_source.hpp) |
| **Behavior** | `IBehavior` | VissRequestBehavior · AccumulatorBehavior · PControllerBehavior | `SILS_BEHAVIOR=viss-request\|accumulate\|p-controller` | [behavior.hpp](../service/sils/vecu/src/behavior.hpp) |
| **Plant** | `IPlant` | AnalyticPlant · ReplayPlant · FmuPlant | `SILS_PLANT=first-order\|replay\|fmu\|none` | [plant.hpp](../service/sils/vecu/src/plant.hpp) |
| **Scenario** | `IScenario` | NominalScenario · ScheduledFaultScenario | `SILS_SCENARIO=nominal\|scheduled` | [scenario.hpp](../service/sils/vecu/src/scenario.hpp) |

## 5.4 tick 루프 — 4포트가 한 tick 에서 협력

[viss_probe_main.cc](../service/sils/vecu/src/viss_probe_main.cc) 의 루프가 포트들을 조립한다:

```
while (clock->WaitForNextTick(tick)) {              # ⟨Clock⟩ 다음 tick 까지 블록 (wall: 주기 / lockstep: grant 대기)
    staged = <replay / staging 로 이번 tick 입력 확정>   #        (§7.3 입력 결정론)
    ctx = { tick.seq, tick.sim_time_ns, staged }
    if (plant) ctx.plant_feedback = plant->output()  # ⟨Plant⟩ 지난 tick 출력을 피드백으로
    act = scenario->OnTick({tick.seq, ...})          # ⟨Scenario⟩ 이번 tick 결함 주입할까?
    out = publish_one(ctx, act)                      # ⟨Behavior⟩ 명령 계산 → (fault 적용) → DDS publish
    if (plant) { plant->SetInputs(out.value)          # ⟨Plant⟩ 그 명령으로
                 y = plant->Step(tick.dt_ns) }        #         한 dt 적분 → 다음 tick 피드백
    clock->TickDone(tick)                            # ⟨Clock⟩ lockstep 이면 TickComplete 응답
}
```

한 tick = **Clock 이 깨우고 → Plant 피드백 읽고 → Scenario 가 결함 결정 → Behavior 가 명령 만들어 publish
→ Plant 가 그 명령으로 진행 → Clock 에 완료 통보.** 이 협력이 L1~L3 을 만든다(§5.5).

## 5.5 포트별 요약

- **IClockSource (§8)** — 시간의 원천. `WallClock`(free-running, `period_ms`, 비결정) vs
  `Lockstep`(ClockMaster 의 TickGrant 를 기다렸다 TickComplete 응답 → sim-time 이 wall-time 과 분리,
  결정론). 자세히는 7장(충실도)·§7/§8.
- **IBehavior (§11 L2)** — 매 tick 무엇을 낼지. `viss-request`(무상태 VISS 요청) · `accumulate`(스테이징
  입력 누적, 상태기계) · `p-controller`(플랜트 피드백으로 `u=Kp(setpoint−feedback)`, L3 제어).
- **IPlant (§11 L3)** — 폐루프의 물리 모델. `AnalyticPlant`(1차 지연, live) · `ReplayPlant`(사전기록
  재생) · `FmuPlant`(fmi4c 로 실제 `.fmu` co-sim). `none` 이면 개루프. 자세히는 [FMU 문서](sils-fmu-plant.md)·9장.
- **IScenario (§11 negative path)** — tick-indexed 결함 주입. `nominal`(무주입) · `scheduled`
  (`3:drop,7:corrupt` 스케줄). 결정론이라 결함 run 도 golden 재현. 자세히는 §11 negative path.

## 5.6 결정론 — 왜 포트를 이렇게 짰나

포트의 목적은 **결정론적 조립**이다: 같은 (clock, behavior, plant, scenario, 입력)이면 같은 tick-indexed
출력이 나와야 golden 재현(8장)·충실도 판정(7장)이 성립한다. 그래서 각 구현은 tick·sim_time 기준으로만
동작하고 wall-clock·RNG 에 의존하지 않는다(scenario 결함조차 tick 스케줄). **env 는 스위치, 로직은
바이너리**(§4.8 ①) — 같은 바이너리가 env 조합만 바꿔 L0~L3 을 만든다.

## 5.7 실전 — 새 ECU 를 포트로 추가하기 (brake · temp 예)

실제로 **brake ECU** 와 **온도조절(temp) ECU** 를 만든다고 하자.

### (a) 두 ECU = 두 vECU = 두 컨테이너

서로 다른 ECU(다른 로직·신호·물리)라 **각자 Descriptor → 각자 인스턴스 → 각자 컨테이너**
(§4.10). 한 Topology 의 `vecus[]` 에 둘 다 넣으면 compile 이 2 컨테이너로 전개:

```yaml
# config/sils/topologies/chassis.yaml
vecus:
  - name: brake-ecu
    descriptor: service/sils/vecu/descriptors/brake_probe.yaml
  - name: temp-ecu
    descriptor: service/sils/vecu/descriptors/temp_probe.yaml
# → 컨테이너 2개, fleet 로 동시 기동 (brake 휠별 이중화면 replicas + per-instance → FL/FR/RL/RR)
```

### (b) 각 ECU 를 4포트에 매핑 — "실제 구현부"의 자리

| 포트 | brake ECU | temp ECU |
|---|---|---|
| **IBehavior** (제어 로직) | `BrakeBehavior` — 요청 감속도 → 유압 명령 | `TempControlBehavior` — 목표온도 PID (또는 p-controller 재사용) |
| **IPlant** (물리 모델) | `BrakePlant`(유압·휠·감속) 또는 `brake.fmu` | `ThermalPlant`(열질량 RC) 또는 `thermal.fmu` |
| **IClockSource** | lockstep(차량 동기) / wall(단독) | 동일 |
| **IScenario** | 센서 드롭·명령 손실 | 센서 고착·지연 |
| **Contract** (신호) | brake 요청/상태 topic·type·QoS | temp topic·type·QoS |

### (c) 구현부가 물리적으로 어디 가나 (§4.8 세 군데 재확인)

| 무엇 | 어디에 |
|---|---|
| 제어 로직(brake/temp 알고리즘) | **IBehavior 구현 클래스** → `behavior.hpp`(또는 새 파일), **바이너리에 컴파일**(§4.8 ①). 고충실도면 실제 ASW 를 IBehavior 어댑터로 감쌈 |
| 물리 모델 | **IPlant 구현 클래스**(analytic) 또는 **`.fmu` 파일**(§4.8 ③, 컨테이너 FS) |
| 신호/타입/QoS | **IDL** + **Contract Catalog**(2장) |
| 선택·파라미터 | **Descriptor → `SILS_*`**(3장) |

### (d) 연결·구현 절차 (brake 추가)

```
① IDL:        BrakeCommand.idl (타입)                                    [2장]
② Contract:   catalog 에 brake-command (topic /VIN/Brake · domain · QoS) [2장]
③ IBehavior:  class BrakeBehavior : public IBehavior {                  [5장 — 로직!]
                 BehaviorOutput OnTick(ctx){ /* 유압 명령 계산 */ } };
              MakeBehavior 에 "brake" 분기 추가
④ IPlant:     class BrakePlant : public IPlant {...}  또는 brake.fmu     [5장/9장]
              (fmu 면 allowlist 등록 §5.6)
⑤ Descriptor: brake_probe.yaml
                 behavior: { type: brake, spec: {...} }
                 plant:    { type: fmu, fmu: { file: /models/brake.fmu, ... } }
                 clock:    { mode: lockstep }
                 buses:    [ endpoints: [ { contract_id: brake-command, direction: publish } ] ]
⑥ Topology:   vecus[] 에 brake-ecu + temp-ecu → compile → 컨테이너 2개
```

tick 루프(§5.4)는 **안 건드린다** — `MakeBehavior`/`MakePlant` 팩토리에 **분기 하나** 추가하면
probe 본체가 그대로 새 포트를 조립한다(earned-abstraction, §5.2).

### (e) 정직한 현재 경계 ★

- **포트(로직·물리·결함·클록)는 지금 바로 재사용** — `BrakeBehavior`/`ThermalPlant` 는 위처럼 꽂힌다.
- **하지만 현재 probe 는 wire 타입이 `VissRequestTopic` 하나로 고정**(VISS 슬라이스). 두 길:
  - **(지금 가능)** brake/temp 가 `VissRequestTopic` 엔벨로프(JSON payload)를 **다른 topic**
    (`/VIN/Brake`, `/VIN/Temp`)에 publish — 새 IDL 없이 2 vECU 구성 가능(입력 스테이징·fleet 예제가
    이미 이 방식).
  - **(확장 필요)** 네이티브 `BrakeCommand` IDL 타입으로 가려면 probe 의 transport 바인딩을 **계약 타입
    구동**으로 일반화(또는 타입 계열별 probe 바이너리) — 아직 미구현.
- 즉 **로직·물리·결함은 포트로 지금**, **네이티브 신규 DDS 타입은 transport 일반화가 남은 작업**.

```
brake ECU                              temp ECU
 ├ IBehavior: BrakeBehavior (로직→바이너리)   ├ IBehavior: TempControlBehavior
 ├ IPlant:    brake.fmu (물리→파일)           ├ IPlant:    thermal.fmu
 ├ Contract:  /VIN/Brake                      ├ Contract:  /VIN/Temp
 └ Descriptor: brake_probe.yaml → SILS_*      └ Descriptor: temp_probe.yaml → SILS_*
        └────── compile → 컨테이너 2개, 각자 probe(PID1) ──────┘
```

### (f) 실제 파일 내용 (워크드 예제)

> 아래는 **예시 스니펫**이다. 실 소스(`behavior.hpp`/`plant.hpp`)엔 넣지 않는다 — VISS 슬라이스에
> 투기적 코드를 두지 않기 위함(earned-abstraction, §5.2). 현재 IBehavior/IPlant 시그니처에 맞춘 형태.

**① `behavior.hpp` — `BrakeBehavior`** (제어 로직 → 바이너리, §4.8①). `PControllerBehavior` 와 같은 꼴:

```cpp
// 목표 감속도(target)와 피드백(실제 감속도)으로 유압 명령 u=Kp*(target-actual) 계산(0..max 클램프).
// value 가 명령이라 probe 가 그대로 플랜트에 먹인다(§5.4 폐루프).
class BrakeBehavior : public IBehavior {
public:
    BrakeBehavior(double kp, double target_decel, double max_pressure)
        : kp_(kp), target_decel_(target_decel), max_pressure_(max_pressure) {}
    BehaviorOutput OnTick(const BehaviorContext& ctx) override {
        double pressure = kp_ * (target_decel_ - ctx.plant_feedback);   // P 제어
        if (pressure < 0.0)           pressure = 0.0;                   // 브레이크는 음수 불가
        if (pressure > max_pressure_) pressure = max_pressure_;
        std::ostringstream inner;
        inner << R"({"action":"set","path":"Vehicle.Brake.Pressure","value":)" << pressure
              << R"(,"requestId":"sils-)" << ctx.tick_seq << R"("})";
        return {inner.str(), pressure};
    }
    const char* name() const override { return "brake"; }
private:
    double kp_, target_decel_, max_pressure_;
};
```
```cpp
// MakeBehavior 팩토리에 분기 (SILS_BEHAVIOR=brake 로 선택)
if (kind == "brake") return std::make_unique<BrakeBehavior>(kp, setpoint, /*max*/100.0);
```

**② 물리 모델 — (A) `plant.hpp` `BrakePlant`** (해석, FMU 불필요). `AnalyticPlant` 와 같은 꼴:

```cpp
// 브레이크+휠 물리(단순): 압력 u → 감속도 x. x' = b*u - a*x (forward-Euler).
class BrakePlant : public IPlant {
public:
    BrakePlant(double gain, double drag) : b_(gain), a_(drag) {}
    void Reset(double initial) override { x_ = initial; }
    void SetInputs(double pressure) override { u_ = pressure; }
    double Step(int64_t dt_ns) override {
        double dt = static_cast<double>(dt_ns) / 1e9;
        x_ += (b_ * u_ - a_ * x_) * dt;
        return x_;
    }
    double output() const override { return x_; }
    const char* name() const override { return "brake-plant"; }
    const char* mode() const override { return "live"; }
private:
    double b_, a_, x_ = 0.0, u_ = 0.0;
};
// MakePlant 에: if (kind == "brake") return std::make_unique<BrakePlant>(gain, drag);
```

**② 물리 모델 — (B) `brake.fmu`** (실제 FMU, 외부 툴 export). `.fmu` 는 바이너리 ZIP 이라 직접 못 쓰고,
안의 `modelDescription.xml` 이 인터페이스다:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<fmiModelDescription fmiVersion="3.0" modelName="BrakePlant">
  <CoSimulation modelIdentifier="BrakePlant"/>
  <ModelVariables>
    <Float64 name="brake_pressure" valueReference="1" causality="input"  start="0"/>
    <Float64 name="deceleration"   valueReference="2" causality="output"/>
  </ModelVariables>
</fmiModelDescription>
```
```bash
# 변수 이름→VR 해석 + 보안 allowlist 등록 (9장·§5.6)
python3 tools/sils/fmu/fmu_map.py  --fmu brake.fmu --input brake_pressure --output deceleration
#   RESOLVED_FMU_INPUT_VR=1  RESOLVED_FMU_OUTPUT_VR=2  RESOLVED_FMU_FMI_VERSION='3.0'
python3 tools/sils/fmu/fmu_gate.py --fmu brake.fmu --record   # 출력 stanza 를 allowlist.yaml 에
```
```yaml
# config/sils/fmu/allowlist.yaml
  - id: brake-plant
    sha256: "…(실제 해시)…"
    fmi_version: "3.0"
    model_identifier: "BrakePlant"
    supplier: "chassis 팀 Simulink export, 2026-07 검증"
    platforms: [aarch64-linux]
```

**③ `service/sils/vecu/descriptors/brake_probe.yaml`** (선언 → env, 3장):

```yaml
ecu: brake_probe
instance_id: brake-1
fidelity: L3                       # Plant-coupled (폐루프)
clock: { mode: lockstep }
behavior:
  type: brake                      # → SILS_BEHAVIOR=brake
  spec: { kp: 2.5, setpoint: 6.0 } # Kp, 목표 감속도 6 m/s² → SILS_CTRL_KP/SETPOINT
plant:
  type: fmu                        # → SILS_PLANT=fmu   (해석 모델이면 type: brake)
  fmu:
    file: /models/brake.fmu        # → SILS_FMU_FILE (컨테이너 FS 에 존재)
    input_vr: 1                    # → SILS_FMU_INPUT_VR   (fmu_map 결과)
    output_vr: 2                   # → SILS_FMU_OUTPUT_VR
    allowlist_id: brake-plant      # → SILS_FMU_ALLOWLIST_ID (게이트 sha256 확인)
buses:
  - name: chassis-bus
    type: dds
    endpoints: [ { contract_id: brake-command, direction: publish } ]
scenario: {}
```

**무엇이 어디에**:

| 조각 | 파일 | 장 |
|---|---|---|
| 제어 로직 `BrakeBehavior` | behavior.hpp (바이너리) | §4.8① · 5장 |
| 물리 `BrakePlant` / `brake.fmu` | plant.hpp 클래스 / `.fmu` 파일 | §4.8③ · 9장 |
| FMU 변수·보안 | fmu_map.py · allowlist.yaml · fmu_gate.py | 9장 · §5.6 |
| 신호·타입 | BrakeCommand.idl · Contract Catalog | 2장 |
| 선택·값 | brake_probe.yaml → SILS_* | 3장 |

## 5.8 다음으로

이 포트들이 매 tick 남기는 trace(behavior_output·plant_step·scenario_fault·tick_grant…)가 6장
**evaluate** 의 입력이고, 그 조합이 7장 **충실도 사다리 L0–L4** 로 판정된다.

---

# 6장~11장 (채우는 중)

아래는 아직 stub. 이해하는 대로 각 장을 위 1~5장처럼 채운다.

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
