# SiLS — 검증 가능한 디지털 트윈 코어 (Verifiable Digital Twin Core)

> 포지셔닝 문서. SiLS 가상 ECU 프레임워크를 "디지털 트윈" 관점에서 어디에 놓고, 무엇이 차별점인지 정리한다.
>
> 상태: **포지셔닝(신규 구현 없음)** · 2026-07 · 대상 프레임워크 `oss_multisw_framework` (MMF)

---

## 1. 한 줄 주장

디지털 트윈(DT)의 고질적 약점은 기능이 아니라 **신뢰**다 — "이 트윈을 실물 대신 믿어도 되나?"에 대부분의 DT가 답하지 못한다. SiLS는 그 신뢰를 **선언하고 기계로 검증**하는 계층을 이미 갖고 있다.

> **SiLS는 "또 하나의 시뮬레이터"가 아니라 검증 가능한(verifiable) 디지털 트윈 코어다.**
> 충실도 L0~L4(선언+시험) · conformance 게이트(fail-closed, 224 테스트) · 결정론(lockstep) · Tool Confidence 도시에 — "믿어달라"가 아니라 **증거로 검증된 트윈**.

---

## 2. 디지털 트윈 스펙트럼과 우리 위치

DT는 하나가 아니라 동기(sync) 정도에 따른 스펙트럼이다.

| 단계 | 정의 | 동기 방향 | 우리 상태 |
|---|---|---|---|
| **Digital Model** | 실물과 자동 동기 없는 가상 모델 | 없음 | ✅ **이미 우리 것** — vECU + plant + 충실도 L0~L4 |
| **Digital Shadow** | 실물 → 가상 단방향 동기 | 실물→가상 (텔레메트리·로그) | 🟡 **값싼 확장** — MF4/rosbag 로그 재현(replay) |
| **Digital Twin(진짜)** | 실물 ↔ 가상 양방향 + 라이브 + 예지보전 | 양방향 | ❌ **별개 제품** — NFR 상충(실시간·상태추정·수명데이터) |

**핵심**: 우리의 차별점은 "어느 단계에 있느냐"가 아니라, **세 단계 전부를 관통하는 신뢰 계층**을 갖췄다는 점이다. 단계는 수요에 따라 올라가면 되지만, 어느 단계든 "트윈을 믿을 근거"는 똑같이 필요하고 — 그걸 우리가 이미 만든다.

---

## 3. 신뢰 스택 — 왜 믿을 수 있나 (전부 실제 코드에 근거)

| 계층 | 무엇 | 코드 근거 |
|---|---|---|
| **충실도 L0~L4** | 실물과의 동등성을 등급으로 **선언하고 시험으로 증명**. SiLS 상한 L3(폐루프 결정론), L4=실타깃 parity | `evaluate.py` `FIDELITY_RANK{none:0…L4:5}` |
| **Conformance 게이트** | 트윈이 내는 trace = 공개 계약. SCHEMA→FIDELITY→PARITY 3단 **fail-closed**, 224 테스트 | `gate.py` · `validate_trace.py` |
| **결정론 (lockstep)** | 동일 입력 = 동일 trace → 트윈 실행을 **재현·회귀** 가능 | ClockMaster grant/complete · TickInbox |
| **Tool Confidence 도시에** | 판정 도구 자체의 신뢰 근거를 코드로 명시(안전팀이 얹을 기술 근거) | `docs/sils/sils-tool-confidence.md` |

이 네 계층이 합쳐져 **"기계로 검증된 트윈 출력"**을 만든다. 트윈의 충실도가 **주장이 아니라 등급으로 선언되고 게이트로 검사**된다는 것이 요지다.

---

## 4. 파트너에게 주는 것

- **선언되고 기계-검증되는 충실도** — "우리 트윈은 정확합니다"가 아니라 "L3로 선언, 게이트 PASS, 224 테스트 green"으로 제시.
- **CI에 물리는 회귀** — 동일 trace 재현 → 트윈이 조용히 틀어지는 드리프트를 게이트가 차단.
- **표준 접점** — FMI·OpenSCENARIO·(예정)MF4 로 기존 DT 툴체인과 접속, 자산 재작성 없이 이관.

---

## 5. 정직한 경계

- 이건 트윈의 **신뢰 코어**이지 **라이브 operational 트윈이 아니다** (비실시간·per-run·fixture 기반).
- **Shadow(실 로그 재현)** 는 값싼 다음 스텝 — 진입점은 온디맨드 목록의 **MF4/rosbag 임포터**. 구체 수요 시 착수.
- **Operational 라이브 DT**(텔레메트리 동기·상태추정·양방향 액추에이션·예지보전)는 NFR이 정면으로 달라 **별개 제품**. 실차 물리 데이터가 필요하고, 이는 실 브리지(#1, MORAI=시뮬)보다도 먼 얘기.
- **최대 미검증 가정**: 실 MORAI/Autoware 폐루프는 아직 fixture다. 신뢰 스택의 *메커니즘*은 지금 유효하나, 판정의 *값*은 실 플랜트가 붙어야 완성된다(→ 실 브리지 #1).

---

## 6. 관련 문서

- `docs/sils/sils-tool-confidence.md` — 신뢰 스택의 근거 문서(도구 신뢰)
- `docs/sils/sils-functional-verification-interop.md` — conformance≠verification, 표준 상호운용
- `docs/sils/sils-virtual-ecu-architecture-v3.md` — 충실도 L0~L4, 아키텍처
- `docs/sils/sils-reading-order.md` — 문서 인덱스

---
*포지셔닝 문서 — 신규 구현을 수반하지 않는다. Shadow/Operational 확장은 구체 수요가 생길 때 착수.*
