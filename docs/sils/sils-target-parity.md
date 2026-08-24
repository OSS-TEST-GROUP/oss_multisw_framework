# SiLS L4 — Target Parity (Target-like Fidelity)

아키텍처 v3 §11 충실도 사다리의 최상단 **L4 (Target-like Fidelity)** 의 정의와 검증
방법, 그리고 그 스캐폴딩(도구·계약·attestation)을 정의한다. L4 의 대표 검증은
**target parity** 다.

관련: [SiLS 도구 README](../../tools/sils/README.md) · [FMU/L3 가이드](sils-fmu-plant.md) ·
아키텍처 [v3](sils-virtual-ecu-architecture-v3.md) §11.

---

## 1. 사다리에서 L4 의 자리

| 수준 | 명칭 | 보장 | 대표 검증 | 도구 |
|---|---|---|---|---|
| L0 | Interface Stub | type/topic match·기본 sample | endpoint matching | evaluate |
| L1 | Protocol Fidelity | QoS·req/resp·오류 semantics | protocol conformance | evaluate `analyze_protocol` |
| L2 | Behavioral Fidelity | 상태기계·timing·failure | golden scenario | `compare_golden` 전용 (evaluate 는 등급 미발급) |
| L3 | Plant-coupled Fidelity | FMU 기반 폐루프 | model correlation | evaluate `analyze_plant` |
| **L4** | **Target-like Fidelity** | **ARM64·실 gateway/protocol** | **target parity** | **`parity.py`** |

L0·L1·L3 은 **단일 trace** 로 판정된다(그 run 하나의 증거). **L2 는 `fidelity_evidenced` 에
나타나지 않는다** — golden 비교(`compare_golden`)의 `MATCH`/`REGRESSION` 이 유일한 증거이며
등급으로 환산되지 않는다(architecture-v3 §11). **L4 도 단일 trace 로 판정할 수 없다** — 실제
타깃과의 동등성이라, 두 개의 trace(실 타깃 · SiLS)를 교차 비교해야 한다.

---

## 2. L4 는 무엇이고 무엇이 아닌가

> **L4 = "SiLS vECU 가 실제 ECU 처럼 행동한다."**

같은 자극(stimulus)을 **실 타깃 ECU** 와 **SiLS vECU** 양쪽에 주고, 관측 가능한 인터페이스
(behavior 출력·프로토콜 응답·plant 궤적)가 **계약된 tolerance 내에서 일치**하면 L4.
여기서 **실 타깃이 authority** — SiLS 를 타깃에 맞춰 판정한다.

**golden 재현성과 혼동하지 말 것.** golden(§12.4, `compare_golden.py`)은 *SiLS run 이 자기
자신을 재현하는가*(SiLS-vs-SiLS, 결정론)를 본다. 참조 기준이 **이전 SiLS run** 이다.
target parity 는 *SiLS 가 실제 ECU 와 같은가*(SiLS-vs-**타깃**)를 본다. 참조 기준이 **실
하드웨어에서 캡처한 타깃 trace** 다. 값 비교 엔진은 재사용하지만, 의미와 authority 가 다르다.

```text
 golden (§12.4)                         target parity (§11 L4)
 ┌─────────┐   재현성?   ┌─────────┐    ┌──────────┐  동등성?  ┌─────────┐
 │ SiLS #1 │ ─────────▶ │ SiLS #2 │    │ 실 타깃  │ ────────▶ │  SiLS   │
 └─────────┘  (authority=└─────────┘    └──────────┘ (authority=└─────────┘
              이전 SiLS run)             실 ECU trace)
```

---

## 3. L4 를 주장하려면 — 두 조건 모두

`parity.py` 는 **둘 다** 성립해야 PARITY(L4)를 준다. 이게 L4 의 핵심이다.

### 3.1 값/timing 패리티
비교 대상 이벤트가 계약의 **신호별 tolerance** 와 **timing window** 안에서 일치.
- 물리 신호(plant `output`/`command`/`feedback_in`)는 실 시스템이라 bit-exact 가 아니라
  **물리 tolerance**(예: abs 0.05, rel 0.005). golden 의 기본(exact)과 다른 점.
- 이산/프로토콜 필드(응답 `status`)는 default tolerance(exact)로 떨어져 **정확히** 일치해야.
- timing: 타깃은 실시간. SiLS 가 lockstep 이면 sim-time 이 wall-time 과 분리(§8)되므로
  `parity.py`(compare_golden 경유)가 wall-clock window 를 자동 스킵하고 `sim_time_ns`
  동등성이 timing 축을 대신한다.

### 3.2 Attestation (환경 증명)
값이 맞는 것만으로는 부족하다. **SiLS run 이 실제 target-like 구성요소를 썼음**을 증명해야
한다 — ARM64 · 실 gateway · 실 protocol. SiLS trace 의 `target_attestation` 이벤트를 계약의
`attestation` 절과 대조한다.

> §11: **"ARM64 는 L4 구성 요소 중 하나이며 L4 전체를 자동으로 충족하지 않는다."**

x86 에서 stub gateway 로 우연히 값이 맞은 run 은 target parity 가 **아니다**. 이 경우
`parity.py` 는 **PARITY_UNATTESTED(최대 L3)** 로 강등한다. 즉 attestation 은 "arm64 면 L4"
라는 착각을 코드로 막는 게이트다.

```text
 값 패리티?  ── no ──▶ DIVERGENCE            (below-L4)
     │yes
 attestation? ── no ─▶ PARITY_UNATTESTED     (값은 맞지만 L4 주장 불가 → L3)
     │yes
     ▼
   PARITY                                     (L4 evidenced)
```

---

## 4. 계약 (`config/sils/parity/viss-target-parity.yaml`)

L4 를 *선언하고 검증*하기 위한 계약. 주요 필드:

| 필드 | 의미 |
|---|---|
| `attestation.{arch,gateway,protocol}` | L4 게이트. SiLS run 이 증명해야 하는 실 구성요소 |
| `compare_events` | 비교할 관측 인터페이스 이벤트(behavior_output·response_received·plant_step) |
| `key_fields` | 정렬 키(tick_seq/request_id 로 타깃·SiLS 이벤트를 짝지음) |
| `signals` | 신호별 numeric tolerance(물리 신호는 non-exact) |
| `default_tolerance` | 위에 없는 필드의 기본(이산 필드는 exact) |
| `timing_window_ms` | 타깃-SiLS per-event wall 오프셋 허용치(lockstep 시 스킵) |

`attestation` 절이 없으면 값만으로 판정하되, `parity.py` 는 그 사실을 결과에 명시한다
(조용히 통과시키지 않음).

---

## 5. 실행 플로우

```bash
# 1) 실 타깃 ECU 에 자극 → 타깃 레퍼런스 trace 캡처 (실측; arm64 실 하드웨어)
#    target-trace.jsonl  (authority)

# 2) 같은 자극을 SiLS vECU 에, 실 구성요소로 실행하며 attestation 을 truthful 하게 선언
SILS_ATTEST_ARCH=aarch64 SILS_ATTEST_GATEWAY=real SILS_ATTEST_PROTOCOL=real \
  <run coordinator로 SiLS run>          # → sils-trace.jsonl (target_attestation 포함)

# 3) target parity 판정
python3 tools/sils/conformance/parity.py \
    --target target-trace.jsonl --sils sils-trace.jsonl \
    --contract config/sils/parity/viss-target-parity.yaml
#   PARITY (fidelity L4) → exit 0
#   PARITY_UNATTESTED / DIVERGENCE → exit 1
```

probe 는 매 run 시작에 `target_attestation`{arch,gateway,protocol} 을 방출한다. 값은
env(`SILS_ATTEST_*`)로 self-declare — **신뢰할 수 있는 attestation 은 배포/CI 가 truthful
하게 설정**해야 하며, 기본값(arch=unknown, gateway/protocol=sim)은 sim run 을 뜻해 L4 를
주지 않는다.

---

## 6. 현재 상태와 남은 것

| 항목 | 상태 |
|---|---|
| L4 정의(사다리·golden 과의 구분) | ✅ 본 문서 |
| target parity 판정 도구 `parity.py` | ✅ 값/timing 비교 + attestation 게이트, 테스트 12 |
| parity 계약 스키마 | ✅ `config/sils/parity/viss-target-parity.yaml` |
| probe `target_attestation` 방출 | ✅ `SILS_ATTEST_*` env |
| evaluate L4 rank + attestation 기록 | ✅ 단일 trace 는 attestation 만 기록(교차판정은 parity.py) |
| **실 타깃 ECU 레퍼런스 trace 캡처** | ⬜ 실측 — 실 하드웨어 필요 |
| **arm64 실 하드웨어에서 SiLS 실행** | ⬜ 실측 — arm64 호스트/실 gateway 필요 |
| 실 gateway/protocol(vCAN·SOME-IP·CAN-FD) 배선 | ⬜ 실측 트랙 |

즉 **L4 의 판정 계약(도구·스키마·attestation 게이트)은 완료**됐고, 남은 것은 실 타깃·arm64
환경에서의 **실측**이다(§0A Contract Spike 계열과 함께).
