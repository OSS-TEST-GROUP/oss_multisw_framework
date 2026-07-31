# SiLS 문서 읽는 순서 (인덱스)

> SiLS 문서가 많아 **읽는 순서**를 잡기 위한 인덱스.
> `.md` = 본문(정확), `.html` = 시각 요약(다이어그램). **빠른 파악은 html, 정확한 내용은 md.**

---

## 0. 한눈에 — 무엇을 읽나

| 문서 | 성격 | 한 줄 |
|---|---|---|
| `sils-vecu-dev-environment.html` | 시각·오리엔테이션 | 전체 생태계 큰 그림 (제일 먼저 보기 좋음) |
| `sils-virtual-ecu-architecture-v3` (.md/.html) | **개념·기준** | 충실도 L0~L4, 포트 5개, 경계 — **권위 있는 설계서** |
| `sils-understanding` (.md/.html) | 현행·레퍼런스 | 실제 동작(DDS·tick·trace·conformance) 전체 훑기(밀도 높음) |
| `sils-extension-model.md` | ADR·경계 | 왜 이 확장 경계인가(core 무수정 원칙) |
| `sils-vecu-development-guide` (.md/.html) | **실무** | 내 vECU 작성→패키징→배포 (§7 = 플랜트 결합 모드) |
| `sils-trace-schema.md` | 계약 | vECU 가 내는 trace = 공개 conformance 경계(v1) |
| `sils-clock-master` (.md/.html) | 심화 | lockstep 시간 권위 — TickGrant/TickComplete barrier |
| `sils-fmu-plant` (.md/.html) | 심화 | FMU(in-process) 폐루프 결합 |
| `sils-target-parity.md` | 심화 | L4 ARM64 타깃 동등성 |
| `sils-morai-autoware-topology.html` | 시각 | MORAI/rosbag ↔ Autoware ↔ vECU 데이터 흐름 |
| `sils-morai-simulator-integration-review` (.md/.html) | 연동 검토 | MORAI 연동 authority·시간·프로토콜 사전검토 |
| `sils-morai-plant-adapter-design` (.md/.html) | **최신 설계** | MORAI/AWSIM 버스 결합(Option 2) — 채널맵·정규 계약 |
| `sils-agent-automation` (.md/.html) | 제안·운영 | LLM 에이전트로 검증 자동화 — 판정은 kit, 에이전트는 앞뒤(트리아지·리포트) |

**구버전(참고만, v3 로 대체됨):** `sils-virtual-ecu-architecture.md/.html`(v2), `sils-virtual-ecu-architecture-review.md`.

---

## 1. 온보딩 순서 (처음 오는 사람)

1. **`sils-vecu-dev-environment.html`** — 큰 그림부터. 컴포넌트가 어디에 앉는지.
2. **`sils-virtual-ecu-architecture-v3.md`** — 개념·충실도·포트·경계. 정신 모델을 여기서.
   (급하면 `-v3.html` 로 다이어그램만 먼저)
3. **`sils-understanding.md`** — 실제로 어떻게 도는가(현행). 길지만 **레퍼런스**.
   특히 6장(trace→verdict) 은 검증 이해의 핵심.
4. **`sils-extension-model.md`** — 확장 경계와 이유. vECU 만들기 전에.
5. **`sils-vecu-development-guide.md`** — 내 vECU 만들기. 앞 내용을 실무로 엮음.

> 여기까지가 "SiLS 가 뭐고 어떻게 만드나"의 기본 5개. 이후는 목적별.

---

## 2. 목적별 빠른 경로

### 🔧 vECU 개발자
`architecture-v3`(개념) → **`vecu-development-guide`** → `trace-schema`(내가 낼 계약)
→ 폐루프면 `fmu-plant` → 타깃 검증이면 `target-parity`

### 🚗 MORAI/AWSIM·시뮬레이터 연동
`architecture-v3`(개념) → **`morai-autoware-topology.html`**(데이터 흐름)
→ `morai-simulator-integration-review`(authority·시간·프로토콜)
→ **`morai-plant-adapter-design`**(버스 결합 Option 2, 채널맵·정규 계약)
→ `vecu-development-guide.md §7`(실제 만드는 법)

### ✅ conformance/검증
**`trace-schema.md`**(계약) → `understanding.md` 6장(trace→verdict) → **`clock-master`**(lockstep 시간 권위) → `target-parity.md`(L4)

### 🤖 검증 자동화·CI
`tool-confidence.md` §1(판정 경계) → **`agent-automation.md`**(에이전트 경계·단계별 계획)
→ `tools/sils/README.md`(도구 인덱스·`sils-ci.sh`)

### 🏛️ 아키텍트/의사결정 리뷰
`architecture-v3` → `extension-model`(ADR) → `morai-plant-adapter-design`(3안 비교)

---

## 3. 읽을 때 주의

- **plant 결합은 두 형태**: FMU/코드 = **in-process `IPlant`**(fmu-plant.md), MORAI/AWSIM =
  **버스 결합**(morai-plant-adapter-design.md, Option 2). 섞지 말 것 — architecture-v3 L3 노트 참고.
- **MORAI 어댑터(ROS2→DDS)는 외부 컴포넌트**(SiLS 밖). SiLS 는 정규 계약(EgoState/VehicleControl)만 소유.
- 버스 결합 trace 이벤트(`state_received` 등)는 아직 **v1 스키마 밖**(conformance 편입은 S4 예정).
- `.md`/`.html` 짝은 내용 동기화됨 — html 은 다이어그램 위주, 세부는 md.

---
*문서 추가/구조 변경 시 갱신.*
