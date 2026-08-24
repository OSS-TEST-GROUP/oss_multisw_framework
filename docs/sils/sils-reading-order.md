# SiLS 문서 읽는 순서 (인덱스)

> SiLS 문서가 많아 **읽는 순서**를 잡기 위한 인덱스.
> `.md` = 본문(정확), `.html` = 시각 요약(다이어그램). **빠른 파악은 html, 정확한 내용은 md.**
>
> 모든 SiLS 문서는 **`docs/sils/`** 에 모여 있다 — 아래 파일명은 이 디렉터리 기준이다.

---

## 0. 한눈에 — 무엇을 읽나

| 문서 | 성격 | 한 줄 |
|---|---|---|
| `sils-configuration-map.md` | **★ 실행 직전 필독** | **구성 지도** — 어댑터 6종 전수표 + 구성 4종(C0 픽스처 / C1 SS_v2 좌석소유 / C2 SS_v2 EgoEntity 주행 / C3 MORAI) 플로우 그림 + **ON/OFF 매트릭스**. "왜 어떤 어댑터는 꺼야 하나" 의 답 = **좌석(`vehicle_interface`) 주인이 구성마다 바뀐다**. 조용히 실패하는 함정 11개 |
| `sils-docker-runbook.md` | **★ 실행(복붙)** | **도커 런북** — 이미지 5종 빌드 · 컨테이너 11종 카탈로그 · **구성별 전체 실행 명령(C0/C1/C2)** · 어댑터 개별 `docker run` · 확인/정지 명령 · env 한 장 · 실패→원인. 명령은 전부 `docker inspect` 로 실측한 것. **⚠ compose 이름(`oss-adsw-control-container`) ≠ 실제 이름(`adsw-control-42`)** |
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
| `sils-tool-confidence.md` | 신뢰 근거 | 판정 도구 자체의 근거 자료집(dossier) — ISO 26262 qualification 아님 |
| `sils-simulator-loops` (.md/.html) | **운영 구조** | **두 검증 루프**(A: MORAI · B: SS_v2 = 문서상 "Web.Auto 시나리오" — 용어는 `sils-integration-topology` §1.1) — 스위치 지점·플랜트 격리·작업목록, **rosbag 재생 vs 시나리오 실행**(§2), **SS_v2 시간 전진 조사와 대응**(§7.1·§7.2). **html = 두 경로 아키텍처 그림 + scope 색 구분**. 시뮬레이터 붙이기 전 필독 |
| `sils-tieriv-evaluator-review.md` | **외부 자료 검토** | TIER IV 자신의 E2E 구성(**AWSIM + SS_v2**, Autoware 는 SS_v2 가 띄운다)과 **우리가 갈리는 세 지점**. `TimeScale 0.4` 가 우리 시계 결정(epoch 앵커)의 성립 조건을 드러낸다. **기성 시뮬레이터는 전부 자기 vehicle_interface 를 갖고 온다** — MORAI 가 본선인 구조적 이유 |
| `sils-morai-vendor-questions.md` | **발송용** | MORAI 벤더 확인 요청 — Q1(시간)~Q5(배포). **§7 한 장 표만 채워도 되게** 구성. 루프 A 착수 전 필수 |
| `sils-autoware-partition-vendor-requests.md` | **발송용** | Autoware 파티션 이미지 요청 **1건** — `/localization/initialize` 서비스 서버(기존 shim 에 15줄). 근거는 전부 이미지 정적 분석. ⚠ **더 이상 블로커가 아니다** — 08-12 에 `/localization/initialize` 이름이 비어 있다는 것을 확인하고 우리가 서버를 세웠다(`tools/sils/ss2/localization_initialize_shim.py`). **요청은 그대로 유효**하되 임시방편으로 우회 중 |
| `sils-vecu-arbitration.md` | **설계·구현·실측** | 여러 vECU 가 낸 명령을 하나로 만드는 규칙 — 축 소유권 + **단조 안전 덮어쓰기**(min/clamp, 순서 무관). **§0 이 먼저다: 스펙이 없다는 사실**을 정면으로 다루고, 스펙 없이 잡을 수 있는 것(불변식·differential)과 없는 것을 가른다. 불변식 I1 은 단위 테스트 30건 + **주행 3975 tick** 에서 확인됨(`evidence/ss2-autoware-driving-2026-08-12.md` §5). 미결: `requirements.yaml` 게이트 등록 |
| `sils-integration-topology.md` | **★ 배선 원리(정본)** | 시뮬레이터·Autoware·MMF·SiLS 가 **어떻게 붙나**. §1.1 용어(fixture/SS_v2/MORAI/Web.Auto) · §3 **`vehicle_interface` 좌석** · §4 브리지 인벤토리 · §5 채널·QoS 실측 · §8 실측 제약 3가지(서비스는 벤더 교차 못 넘는다). **"왜 이렇게 붙나" 는 여기, "오늘 뭘 켜나" 는 `configuration-map`** |
| `sils-stage3-autoware-partition.md` | **실무·절차** | 3계단(실 Autoware 파티션) 기동 절차와 실측 |
| `evidence/*.md` | **원측정값** | 모든 주장의 근거. 문서가 실측과 어긋나면 **evidence 가 이긴다**. 최신: `ss2-autoware-driving-2026-08-12.md`(주행) · `multi-vecu-bus-2026-08-12.md`(vECU 5기) |
| `sils-ss2-bringup.md` | **실무·브링업** | 루프 B **1계단(배선)** 절차 — SS_v2 이미지 빌드·실행, **B1-3 interop 실측**. Linux amd64 필요. ⚠ Autoware 실물은 불필요하다는 근거가 여기 |
| `sils-real-bridge-requirements` (.md/.html) | **요구서** | 실 브리지 #1 요구 — BR-M(MORAI)/BR-S(scenario_simulator_v2)/BR-A(Autoware)/BR-C(공통). **BR-C-1(clock)이 최우선 미결** |
| `sils-functional-verification-interop.md` | 요구 판정·표준 | `requirements.yaml` assert 엔진 + OpenSCENARIO/FMI/MF4/ReqIF 수용 범위 |
| `sils-vehicle-interface-autoware.md` | 연동 상세 | Autoware `control_cmd`/`gear_cmd` ↔ 정규 `ControlTarget` 매핑 |
| `sils-verifiable-digital-twin.md` | 포지셔닝 | "검증 가능한 디지털 트윈 코어" — Model/Shadow/Twin 대비 |
| `sils-implementation-brief.pptx` | **파트너 공유** | 구현 브리핑 슬라이드(26p). 정본은 각 .md — 충돌 시 .md 가 이긴다 |

**구버전 문서는 저장소에 남아 있지 않다** — v3 가 유일한 아키텍처 설계서다(과거 v2
`sils-virtual-ecu-architecture.md/.html` 와 `sils-virtual-ecu-architecture-review.md` 는 삭제됨).

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
**`configuration-map.md`**(★ 무엇을 켜고 끄나 — 여기서 시작) →
**`docker-runbook.md`**(★ 그래서 명령이 뭐냐 — 복붙) →
**`simulator-loops.md`**(두 루프 구조) → **`ss2-bringup.md`**(브링업 상세) → `architecture-v3`(개념)
→ **`morai-autoware-topology.html`**(데이터 흐름)
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
→ `tool-confidence.md`(판정 신뢰 경계) → `verifiable-digital-twin.md`(포지셔닝)

### 🤝 파트너·외부 협의
`sils-implementation-brief.pptx`(개요) → **`real-bridge-requirements.md`**(맞춰야 할 항목)
→ `vehicle-interface-autoware.md`(Autoware 매핑) → `functional-verification-interop.md`(시나리오 자산 수용 범위)

---

## 3. 읽을 때 주의

- **plant 결합은 두 형태**: FMU/코드 = **in-process `IPlant`**(fmu-plant.md), MORAI/AWSIM =
  **버스 결합**(morai-plant-adapter-design.md, Option 2). 섞지 말 것 — architecture-v3 L3 노트 참고.
- **MORAI 어댑터(ROS2→DDS)는 외부 컴포넌트**(SiLS 밖). SiLS 는 정규 계약(EgoState/VehicleControl)만 소유.
- 버스 결합 trace 이벤트(`state_received` 등)는 **S4 에서 v1 스키마에 등록 완료**(producer `vecu`/`sim_fixture`).
  버스 결합 판정은 `analyze_vehicle`/`analyze_lockstep`/`analyze_staging`, 결함 주입은 `analyze_fault`.
- **`fidelity_evidenced` 는 누적 사다리가 아니다** — 자동 부여 레벨은 **L0·L1·L3** 뿐이고 **L2 는
  발급되지 않는다**(golden 비교는 gate 밖 스테이지). architecture-v3 §11 주의 박스 참조.
- **`.md`/`.html` 은 동기화되어 있지 않다.** html 은 손으로 만든 시각 요약이고 생성 스크립트가 없어
  일부 문구가 뒤처져 있다(예: `sils-fmu-plant.html` 의 fmu_map 관련 서술). **정본은 항상 `.md`** —
  충돌하면 md 를 믿을 것. 각 html 하단에 기준 commit 을 표기한다.
- **`.pptx` 도 정본이 아니다** — 파트너 공유용 요약이며 각 슬라이드의 상태 배지
  (구현완료/스캐폴딩/실측미완/목표)를 함께 읽어야 한다.

---
*문서 추가/구조 변경 시 갱신. 마지막 정합성 점검: 2026-08-12
(`configuration-map` 신설 · `integration-topology`/`stage3`/`evidence` 누락분 추가).*
