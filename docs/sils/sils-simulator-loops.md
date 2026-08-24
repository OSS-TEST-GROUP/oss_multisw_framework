# SiLS — 두 검증 루프 (A: MORAI · B: Web.Auto 시나리오)

> ⚠ **용어**: 여기서 "Web.Auto 시나리오" 는 **플랜트가 SS_v2 인 구성**을 뜻한다. Web.Auto
> 는 시나리오를 **저작**하는 클라우드 플랫폼이고 런타임 부품이 아니며, 브링업에 계정도
> 필요 없다. 플랜트 자리에 실제로 교체되어 들어가는 것은 `sils_ego_source`(시험용 스텁) ·
> SS_v2 · MORAI 셋이다 — [`sils-integration-topology.md` §1.1](sils-integration-topology.md).

> **한 줄**: 같은 vECU·같은 게이트로 **두 개의 시뮬레이터 루프**를 병행한다. 스위치 단위는
> **외부 어댑터 컨테이너 하나**이고, `service/sils/` 는 재빌드하지 않는다. 위험한 것은 배선이
> 아니라 **두 루프의 판정 결과를 뒤섞는 것** — 그래서 golden 은 플랜트별로 격리한다.
>
> **"rosbag 으로 돌리면 되지 않나?" 는 §2 를 먼저 읽을 것** — 재생 경로는 이미 있고
> (`ReplayPlant`), 다른 것은 시나리오냐 rosbag 이냐가 아니라 **루프가 닫히느냐**다.
>
> **그림으로 먼저 보려면** → [`sils-simulator-loops.html`](sils-simulator-loops.html) — 두 경로를
> 나란히 놓고 **우리 scope 를 색으로 구분**한 시각 요약 (SiLS 소유 / 우리가 만들 어댑터 /
> 외부 오픈소스 / 외부 상용 / 클라우드). 정확한 내용은 이 `.md` 가 정본.
>
> **상태 (2026-08-07)** — **1계단(배선) 종료.** SS_v2 → 어댑터 → 버스 → vECU 관통,
> V2(Fast DDS 2.6↔3.x) ✅ · V1(프레임) ✅ VEHICLE · V3(클록) ✅ SIM·50 ms 고정 스텝
> ([실측](evidence/ss2-stage1-verdict-2026-08-07.md)). B1-1·B1-2·B1-2b 조사 완료
> ([근거](evidence/ss2-concealer-survey-2026-08-07.md)), Autoware 대역
> [`adapters/autoware-stub`](../../adapters/autoware-stub/README.md) 구현.
> **★ B1-4 종료 (2026-08-10)** — 진짜 `EgoEntity` 를 띄워 **진짜 concealer** 가 stub 을
> `driving` 까지 걸었다 (`run_ego_scenario.sh`). Autoware 없는 ego 가 성립한다, 상류 patch 0.
> **★ 2계단(B-det) 루프 폐쇄 확인 (2026-08-10)** — `run_closed_loop.sh` 로 ego 가
> **10.00 m/s** 로 주행. 그 값이 vECU 의 `SILS_CTRL_SETPOINT` 과 일치한다는 것이 요점이다:
> 상태 경로가 끊겼다면 제어기가 `vx=0` 을 영원히 읽어 속도가 발산했을 것이므로,
> **수렴 자체가 양방향이 살아 있다는 증거**다. 남은 것은 결정론(W1/W2/W3)과 게이트 배선.
> ⚠ 3계단은 **GPU 장비 대기** — adsw 파티션이 nvidia 런타임을 요구한다(§4 말미).
> 관련: [real-bridge-requirements](sils-real-bridge-requirements.md) BR-S / BR-M / BR-A / BR-C ·
> [plant-adapter-design](sils-morai-plant-adapter-design.md)(Option 2) ·
> [vehicle-interface-autoware](sils-vehicle-interface-autoware.md)(VCU)

---

> ★ **배선도는 별도 문서에 있다** — 시뮬레이터·Autoware·MMF·SiLS 네 덩어리가 어떻게 붙고,
> 어떤 브리지가 실제 구성에 남고 무엇이 아직 없는지:
> **[sils-integration-topology.md](sils-integration-topology.md)**.

## 1. 왜 두 개인가

두 루프는 **다른 질문에 답한다**. 하나가 다른 하나를 대체하지 않는다.

| | **루프 A — MORAI** | **루프 B — Web.Auto 시나리오** |
|---|---|---|
| 답하는 질문 | "**실제 도입 대상 시뮬레이터**에서 우리 vECU가 도는가" | "**표준 시나리오 자산**으로 요구를 자동 판정할 수 있는가" |
| 시뮬레이터 | MORAI (상용, 원 요구) | `scenario_simulator_v2` (TIER IV, **오픈소스**) |
| 자극원 | MORAI 시나리오 러너 | Web.Auto Scenario Editor `.xosc` |
| 판정 기준 | 별도 저작 `requirements.yaml` | **같은 `.xosc` 에서 자동 추출** |
| 결정론 전망 | **Sync Mode 존재 확인**(공개 자료) — patch 불필요해 보이나 **ROS 2 가용성 미확인**([BR-M-3](sils-real-bridge-requirements.md)) | **조사 완료 (B1-1)** — 그대로는 L3 불가, **fork/patch 필요**. 아래 |
| 예상 fidelity | L0 ~ L3 | **B-det L3 / B-full L0~L1** (아래) |

**B 를 먼저 한다.** B 의 리스크는 우리가 코드를 읽어 풀 수 있었고(실제로 풀었다 — §7 B1-1),
B 의 선행 작업이 A 의 선행이기도 하며, B 가 서면 **L3 기준선**이 생겨 A 를 그 대비로 평가할 수
있다. 반대 순서면 A 의 단위 변환 버그(§6, 57.3× 함정)와 "시뮬레이터가 원래 다른 것"을 구분할
기준이 없다.

### 루프 B 는 다시 두 갈래다 ★ (B1-1 조사 결과)

`scenario_simulator_v2` 는 **Autoware 를 기다리지 않는다**(`spinSome()` 비블로킹, §7 B1-1). 따라서
Autoware 가 루프에 들어가는 순간 결정론은 성립하지 않는다. 목표를 하나로 잡으면 안 된다:

| | **B-det** — 결정론 확보용 | **B-full** — 통합 검증용 |
|---|---|---|
| 구성 | vECU + SS_v2 (+ `ControlTarget` 은 fixture 가 공급) | Autoware + vECU + SS_v2 |
| 목표 | **L3** — 값 수준 재현, golden 기준선 | **폐루프 동작 검증** (`analyze_vehicle` closed_loop) |
| 결정론 | 시뮬레이터를 barrier 에 넣으면 성립 | **포기** — Autoware 가 비동기 |
| 쓰임 | 회귀 게이트의 기준선 | "실제로 도는가" 의 증거 |

**B-det 가 golden 을 만들고, B-full 이 통합을 증명한다.** 하나로 합치려 하면 둘 다 잃는다 —
이건 [BR-C-1](sils-real-bridge-requirements.md) 의 "지원 O → L3 / 지원 X → L0" 이분법보다
정확한 그림이며, MORAI(루프 A)에도 그대로 적용된다.

---

## 2. 왜 시나리오 "실행"인가 — rosbag 재생으로는 왜 안 되나

자주 나오는 질문이다: *"시나리오를 만들면 어차피 rosbag 으로 돌릴 수 있는 것 아닌가? 기존과 뭐가
다른가?"* — **자극 재생만 보면 맞다.** 그리고 그 경로는 **이미 있다.**

- [`ReplayPlant`](../../service/sils/vecu/src/plant.hpp) — 녹화된 출력을 tick 인덱스로 되먹인다
- `evaluate.py` 의 `mode="replay"` 판정 경로, `input_mode: replay` golden 게이트

새로 만들 것이 아니다. 다른 것은 시나리오냐 rosbag 이냐가 아니라 **루프가 닫히느냐**다.

### 열린 루프 vs 닫힌 루프

```
  rosbag 재생 (열린 루프)                     시나리오 실행 (닫힌 루프)

   녹화된 state ──▶ vECU ──▶ 제어              state ──▶ vECU ──▶ 제어
        │                     │                  ▲                 │
        │                     ✗ 버려짐           └── 시뮬레이터 ◀──┘
        ▼                                            (제어를 반영해
   녹화된 state(다음)                                 다음 state 를 계산)
   ← 내 제어와 무관하게 이미 정해져 있음
```

### 구체적으로 무엇이 깨지나

`.xosc` 에서 뽑은 요구가 "충돌 없음" · "속도 16.7 m/s 이하" · "감속 −3.0 m/s² 이상" 이라고 하자.
**rosbag 으로 돌리면**:

| 내 vECU 의 행동 | 판정 결과 |
|---|---|
| **아무것도 안 함** | 녹화된 ego 가 알아서 감속한다(원본 컨트롤러가 밟았으니) → **하지 않은 일로 통과** |
| **급제동** | 녹화된 state 는 그대로 → 충돌 여부는 **녹화된 대로** |

즉 "충돌 없음"·"속도 16.7 이하" 는 **내 ECU 가 아니라 녹화본을 판정한다.** 내 출력에 걸린 요구
(`control_published.acceleration`)만 실제로 ECU 를 시험하는데, 그것도 **반응하지 않는 입력**에
대해서다.

**시나리오로 돌리면** 제동 → 속도 감소 → NPC 와의 거리 변화 → 충돌 여부가 **내 ECU 행동의
결과**가 된다.

> **한 줄**: rosbag 은 *"내 ECU 가 이 입력열에 대해 말이 되는 명령을 내는가"* 를 시험할 수 있고,
> *"내 ECU 가 결과를 달성하는가"* 는 시험할 수 없다.

### 코드가 이미 이 구분을 강제한다

[`evaluate.py`](../../tools/sils/conformance/evaluate.py) `analyze_plant`:

```python
elif mode == "replay":
    closed = False  # playback, not a live coupled loop
```

그리고 L3 승급 조건이 `mode == "live"` 와 `closed_loop` 을 **둘 다** 요구한다:

```python
if (plant.get("model") != "none" and plant.get("mode") == "live"
        and plant.get("closed_loop") and plant.get("deterministic")):
    _bump("L3")
```

→ **replay 는 L3 에 올라갈 수 없다.** 우연이 아니라 설계다.

### 비교

| | **rosbag 재생** | **시나리오 실행** |
|---|---|---|
| 루프 | **열림** — 입력이 내 제어에 무반응 | **닫힘** — 시뮬레이터가 반영 |
| 판정 가능한 요구 | 내 **출력**에 걸린 것만 | **결과**에 걸린 것 포함(충돌·TTC·정착) |
| fidelity 상한 | **L3 불가**(코드가 막음) | L3 가능 |
| 변형 | **1개로 고정** — 녹화된 그 한 번 | **파라미터 스윕** — `$vmax` 바꿔 N개 (§4) |
| 결정론 | ✅ (그래서 golden 입력 모드로 유효) | lockstep 이면 ✅ |
| 비용 | 매우 쌈 — 시뮬레이터 불필요 | 시뮬레이터 + 어댑터 필요 |
| 데이터 성격 | **실측일 수 있음** | 모델 |

`ParameterDeclarations` 치환을 구현한 이유가 저 "변형" 행이다 — `.xosc` 한 장으로 `$vmax` 를
바꿔가며 N 가지를 돌릴 수 있는데, rosbag 은 녹화된 한 번이 전부다.

### 대체재가 아니라 층이 다르다

**rosbag/replay 가 잘하는 것** — 싸고 결정론적이라 실제로 유용하다:

- 회귀 검사 (golden 입력 모드로 `replay` 가 **허용**되어 있다)
- 스키마·프로토콜 적합성 (L0/L1)
- 시뮬레이터 없는 환경에서 도구 계층 전체 검증 — `sils-ci.sh --traces-dir` 경로가 이것

**rosbag 만 할 수 있는 것도 있다** ⭐ — 시뮬레이터는 **모델**이고 rosbag 은 **실측**이다:

- [target-parity](sils-target-parity.md) 의 **L4** 는 "실 타깃 레퍼런스 trace" 를 authority 로
  요구한다 — 그게 실차 로그다
- [verifiable-digital-twin](sils-verifiable-digital-twin.md) 이 Digital **Shadow** 를
  "값싼 다음 스텝(MF4/rosbag)" 이라고 한 것이 이 경로다

사다리에서 자리가 이렇게 갈린다:

```
  L0 / L1  ─ rosbag 재생으로 충분 (싸고 결정론적)
  L2       ─ golden 비교 (재생이든 실행이든)
  L3       ─ 시나리오 실행 필수 (닫힌 루프)        ◀── 루프 B 가 여기
  L4       ─ 실차 rosbag 이 레퍼런스로 다시 필요
```

### 왜 지금 매핑된 조건 2종만으로는 부족한가

현재 importer 가 매핑하는 `SpeedCondition`·`AccelerationCondition` 은 공교롭게도 rosbag 으로도
어느 정도 판정 가능한 축이다. 정작 시나리오의 값어치가 큰 `CollisionCondition`·
`TimeToCollisionCondition`·`ReachPositionCondition` 은 전부 unmapped 인데 **이유가 두 겹**이다:

1. 트레이스에 신호 자체가 없다 → trace schema v1 변경 필요 (§7 B2-4)
2. 신호가 있어도 **닫힌 루프가 아니면 판정에 의미가 없다** → 루프 B 가 서야 한다

그래서 "importer 조건 확장" 과 "실 시뮬레이터 연동" 은 별개 작업이 아니라 **같은 하나**다.

---

## 3. 전체 그림 — 스위치되는 것과 안 되는 것

```
┌──────────────────────────── 외부 (SiLS 밖) ────────────────────────────┐
│                                                                        │
│   [루프 A]                              [루프 B]                       │
│   MORAI                                 scenario_simulator_v2          │
│   + MORAI 시나리오 러너                  + .xosc (Web.Auto Editor 저작) │
│        │                                      │                        │
│        ▼                                      ▼                        │
│   MORAI 어댑터                          SS_v2 어댑터                    │
│   (deg→rad, km/h→m/s,                  (rad·m/s² 이미 1:1,             │
│    accel/brake 정규화)                   변환 거의 없음)                │
│        │                                      │                        │
└────────┼──────────────────────────────────────┼────────────────────────┘
         │        ▲                             │        ▲
         │        │       ◀── 여기가 스위치 지점 ──▶      │
         ▼        │                             ▼        │
┌─────────────────┴──────────────────────────────────────┴───────────────┐
│  DDS 버스 — 정규 계약 (교체해도 안 바뀜)                                │
│                                                                        │
│    sils/ego/state    ──▶  EgoState        (SI: m, m/s, rad, rad/s)     │
│    sils/ego/cmd  ◀──  VehicleControl  (SI: rad, m/s² signed)       │
│                                                                        │
├────────────────────────────────────────────────────────────────────────┤
│  SiLS vECU  ── 동일 바이너리 · 동일 채널맵 · 재빌드 없음               │
│    RunVehicleProbe → IVehicleBehavior → VehicleWireCodec               │
│    IClockSource (wall | lockstep barrier 참가자)                        │
├────────────────────────────────────────────────────────────────────────┤
│  판정 — 동일 gate                                                       │
│    trace.jsonl → validate_trace → evaluate(analyze_vehicle/lockstep)   │
│                → gate.py → verdict.json / junit.xml                    │
│    회귀: compare_golden  ⚠ golden 은 플랜트별로 격리 (§5)              │
└────────────────────────────────────────────────────────────────────────┘
```

### 왜 채널맵조차 안 바뀌는가

토픽(`sils/ego/state`, `sils/ego/cmd`)이 **우리 것**이다. 어댑터가 우리 쪽으로 맞춰
발행하므로, 시뮬레이터가 바뀌어도 vECU 가 보는 버스는 동일하다.
[`config/sils/channelmap/ego-min.yaml`](../../config/sils/channelmap/ego-min.yaml) 그대로 쓴다.

### 근거 — 코드에 시뮬레이터가 새지 않는다

| 확인 | 결과 |
|---|---|
| `service/sils/` 로직에 `morai`/`awsim` 식별자 | **0건** (주석·CMake 코멘트에만 존재) |
| 토픽 바인딩 | 코드가 아니라 config (`SILS_CHANNELMAP`) |
| 단위 변환 | IDL 주석이 **어댑터 책임**으로 명시 ([EgoState.idl](../../idl/dds/sils/EgoState.idl)) |
| lockstep barrier | 참가자 비종속 — `TickGrant` 구독 + `TickComplete{participant_id}` 응답이면 누구나 |

이미 한 번 스위치해봤다: `sils_ego_source`/`sils_control_source` fixture 가 "제3의 시뮬레이터"로
같은 자리에 꽂혀 lockstep barrier 까지 돌았다(S3). **어댑터가 구현할 계약은 한 문장이다** —
*grant 받으면 sim 한 스텝 전진 → state 발행 → `TickComplete` 응답*.

---

## 4. 루프 B — 자산 체인 (`.xosc` 한 파일이 자극과 판정 둘 다)

```
  Web.Auto Scenario Editor  (웹 저작 · 파라미터화된 시나리오)
            │
            │  TIER IV Scenario Format v2.0  ← YAML 기반. .xosc 가 아니다
            ▼
      변환 (scenario_test_runner 내장)
            │
            ▼
   OpenSCENARIO XML (.xosc)  ─────┬──────────────────────────┐
                                  │                          │
              ┌───────────────────┘                          └────────────────┐
              ▼  자극                                                  판정 ▼
   scenario_test_runner (온프렘, 오픈소스)          import_openscenario.py
   → Autoware 구동 → SS_v2 어댑터 → 버스           → requirements.yaml
                                                    → gate.py --requirements
                                                    → FIDELITY 판정에 융합
```

이것이 [BR-C-3](sils-real-bridge-requirements.md) 의 **"자극과 판정이 한 소스"** 다. 같은 파일이
한쪽에서는 주행을 만들고 다른 쪽에서는 pass/fail 기준이 된다 — 두 벌을 유지하며 어긋날 여지가 없다.

> **주의**: 이 성질은 **루프 B 에서만** 성립한다. MORAI 가 `.xosc` 를 받지 않으면 루프 A 는 요구를
> 별도 저작해야 한다([BR-M](sils-real-bridge-requirements.md) A1-3).

> **위 그림에서 클라우드는 첫 두 칸뿐이다.** `.xosc` 아래는 전부 온프렘·오픈소스다. 나중에 결정론을
> 위해 patch 하게 되는 것도 **Web.Auto 가 아니라 `scenario_simulator_v2`** 다 — §7.2 첫 표 참조.

### vECU 를 어디에 끼우나 — concealer 문제

`scenario_simulator_v2` 는 이미 Autoware 와 폐루프를 닫는다. **concealer**(Autoware 어댑터)와
**simple_sensor_simulator**(차량 동역학)가 그 역할인데, 그 자리가 **정확히 우리 VCU 가 앉으려는
자리**(실차의 `vehicle_interface` 노드 위치)다.

```
 기본 (SS_v2 단독):
   Autoware ──control_cmd──▶ concealer ──▶ simple_sensor_simulator ──┐
      ▲                                                              │
      └───────────────── /vehicle/status/* ◀────────────────────────┘

 우리가 원하는 것 (VCU 삽입):
   Autoware ──control_cmd──▶ VCU (우리 vECU) ──VehicleControl──▶ simulator ──┐
      ▲                            ▲                                        │
      │                       ControlTarget                                 │
      └──────── /vehicle/status/* ◀───── EgoState ◀───────────────────────────┘
```

| 안 | 방식 | 평가 |
|---|---|---|
| (a) concealer 뒤 | simulator 의 ego 동역학 입력을 VCU 출력으로 치환 | 개입 지점이 시뮬레이터 내부 |
| **(b) concealer 우회** | VCU 가 `/control/command/control_cmd` 를 직접 소비 → `ControlTarget` → `VehicleControl` | **권장** — [BR-A-1](sils-real-bridge-requirements.md) 매핑표를 그대로 재사용, 손댈 것 없음 |

#### 선정 기준은 "2계단에서 되는가" 가 아니라 **"3계단에서도 같은 배선인가"** ★

**Autoware 는 최종 구성의 필수 컴포넌트다.** 1·2계단에서 빼는 것은 브링업 편의이지
Autoware 없는 구성이 목적지라는 뜻이 아니다. 따라서 삽입 지점은 **Autoware 가 들어온
뒤에도 그대로 서는 것**이어야 하며, 이 기준으로 (a)는 탈락한다 — Autoware 의 폐루프도
같은 지점을 지나므로 2계단에서 만든 것을 3계단에서 다시 만들게 된다.

#### (b) 를 제대로 그리면 — 우회가 아니라 **중간자(man-in-the-middle)**

"VCU 가 `control_cmd` 를 소비한다" 만으로는 부족하다. concealer 가 **같은 토픽을 계속
듣고 있으면** VCU 가 무엇을 내든 시뮬레이터는 Autoware 명령대로 움직인다 — VCU 는
구경꾼이 된다. 사슬을 **끊고 사이에 들어가야** 한다:

```
  Autoware ──/control/command/control_cmd──▶ [어댑터]
                                                 │ ControlTarget (DDS)
                                                 ▼
                                              [ vECU ]
                                                 │ VehicleControl (DDS)
                                                 ▼
                                              [어댑터]
                                                 │ /sils/control_cmd
                                                 ▼
                                            concealer   ← 입력 토픽만 remap
                                                 ▼
                                     simple_sensor_simulator
```

concealer 는 **손대지 않는다.** ROS 2 토픽 remap(`--ros-args -r`)으로 입력만 우리 쪽으로
돌린다 → **상류 patch 0**.

#### ⚠ 3계단의 실행 전제 — GPU (2026-08-07 확인) → ❌ **정정됨 (2026-08-10)**

> **이 절의 GPU 결론은 틀렸다. 3계단에 GPU 는 필요 없다.**
> 원인은 이미지가 아니라 **런처**였다 — `docker-compose-integration.yaml` 의 `oss-adsw-*` 서비스에는
> `deploy.resources.reservations.devices: nvidia` 가 붙어 있고, 상류가 실제로 권하는
> `partition/run_partitions.sh` 에는 GPU 인자가 아예 없다(실행 시 `CUDA: disabled` 출력).
> **같은 이미지, 다른 런처.** 2026-08-10 에 `run_partitions.sh` 로 세 파티션이 No-CUDA 로
> 기동하고 맵을 읽는 것까지 실측됐다 →
> [sils-stage3-autoware-partition.md §2-6, 1단계 실측 결과](sils-stage3-autoware-partition.md).
> 아래 문단은 compose 경로에서만 유효한 기록으로 남긴다.

우리 Autoware 는 이미 있다 — `ghcr.io/oss-test-group/autoware-partition:adsw-{decision,control,perception}`,
**public 이고 amd64·arm64 둘 다** 있다. 다만 **compose 경로에서는** 세 서비스 모두 nvidia
컨테이너 런타임을 요구한다:

```
Error response from daemon: could not select device driver "nvidia" with capabilities: [[gpu]]
```

| | |
|---|---|
| **GPU 예약을 지워 반쪽만 띄우지 말 것** | 반쪽 Autoware 의 ADAPI 목록은 대조 근거가 되지 못한다 |
| 확인된 것 | `start_script` 에 **KCity 전제가 없다** — `map_path:=/autoware_map`(마운트 지점) · `vehicle_model:=sample_vehicle` · `sensor_model:=sample_sensor_kit`(Autoware 표준 샘플) |
| 맵 | 따로 구할 필요 없다. **kashiwanoha 완본**(lanelet2 + pointcloud 39MB)이 SS_v2 이미지에 있고, 그것을 쓰면 **시나리오/Autoware 맵 정합 문제도 같이 사라진다.** 오버레이: [`docker-compose-sils-autoware.override.yaml`](../../docker/compose/docker-compose-sils-autoware.override.yaml) |
| 대조 도구 | [`check_autoware_adapi.sh`](../../tools/sils/ss2/check_autoware_adapi.sh) — 토픽 10 + 서비스 10, 이름과 **타입까지** |

**Jetson 이 있으면 전부 포팅할 필요는 없다.** `build-ss2.sh` 의 arch 가드가 말하듯 ARM64 는
[L4 타깃 패리티](sils-target-parity.md)라는 **별개 축**이고, 여기서 섞으면 "arm64 라서 다른 건지
시뮬레이터라서 다른 건지" 구분이 안 된다. 나눠 붙이는 편이 낫다:

```
   Jetson (arm64, GPU)              x86 장비
   Autoware 파티션 3종  ◀── DDS ──▶  SS_v2 · 어댑터 · vECU
   (arm64 이미지 존재)     도메인 42
```

교차 빌드가 필요 없고(모두 `network_mode: host`, 도메인 42), **더 현실적인 분산 토폴로지**를
시험하게 된다. 전제는 같은 L2 + 멀티캐스트 discovery + RTPS 포트(UDP 7400번대).
안 되면 Fast DDS Discovery Server 로 전환.

#### 그래서 2계단이 3계단의 부분집합이 된다

```
  3계단:  Autoware ──▶ 어댑터 ──▶ vECU ──▶ 어댑터 ──▶ concealer ──▶ sim
  2계단:  fixture  ──▶         ──▶ vECU ──▶ 어댑터 ──▶ concealer ──▶ sim
          └ 여기만 교체 ┘        └─────────── 동일 ───────────┘
```

2계단에서 만드는 **하류 절반이 3계단에서 그대로 산다.** 이것이 (b)를 택하는 실질적 이유다.

> **Autoware 가 필수라고 해서 B-det(2계단)이 임시 발판이 되는 것은 아니다.**
> [BR-S-3](sils-real-bridge-requirements.md): 시뮬레이터를 barrier 에 넣어도 **제어기가
> 비동기면 결정론은 성립하지 않는다.** 즉 **L3 회귀 기준선은 B-det 에만 존재할 수 있다.**
> 제품에는 Autoware 가 들어가고, 회귀 판정은 B-det 이 한다 — 둘은 서로 다른 질문에 답하는
> **영구적인 두 구성**이다.

#### B1-2 는 두 개의 질문이다 — ② 가 2계단을 막고 있다

```
  ① 입력 : ControlTarget 을 누가 주나
      2계단 → fixture          ✅ 결정됨 (Autoware 부재)
      3계단 → Autoware         ← concealer 협상은 여기서만

  ② 출력 : VehicleControl 이 어떻게 ego 를 움직이나
      2계단 → ???              ← 진짜 막힌 곳. 되먹임이 없으면 폐루프가 아니고,
      3계단 → ???                 폐루프가 아니면 L3 도 없다
```

⚠ **현재 구성에는 concealer 가 아예 없다.** [ss2-bringup §6](sils-ss2-bringup.md) 실측:
노드는 `scenario_node` · `simple_sensor_simulator` · `visualizer` **뿐**이고,
`cpp_mock_scenarios` 는 ego 를 C++ API 로 직접 몬다. 따라서 ② 의 후보는:

| 후보 | 미확인 |
|---|---|
| **concealer 도입** — 우리가 `control_cmd` 를 발행, concealer 가 ego 를 움직임 | concealer 가 **Autoware 없이 뜨는가**. §1 의 "concealer 는 Autoware *메시지 패키지*만 요구" 가 근거이나 미검증 |
| API 직접 호출 — mock 처럼 `traffic_simulator` API 로 구동 | 시뮬레이터 안에 우리 코드가 들어간다 → **어댑터 경계 붕괴** |
| `simple_sensor_simulator` 주입 | (a)안. 상류 patch |

첫 번째만이 "어댑터 밖" 원칙과 "3계단에서도 같은 배선" 을 동시에 만족한다.

#### B1-2 조사 결과 (2026-08-07, 소스 근거) ★

근거 전문: [`evidence/ss2-concealer-survey-2026-08-07.md`](evidence/ss2-concealer-survey-2026-08-07.md)
(pin 커밋 `3370c817` 기준).

**핵심: concealer 는 노드가 아니라 라이브러리이고, 두 얼굴을 갖는다.**

| | 클래스 | 링크되는 곳 | 역할 | 우리에게 |
|---|---|---|---|---|
| **데이터 평면** | `AutowareUniverse` | `simple_sensor_simulator` | `control_cmd` → 차량모델, `/vehicle/status/*` 발행. **생명주기 handshake 없음** | ✅ 필요 |
| **제어 평면** | `FieldOperatorApplication` | `traffic_simulator` (`EgoEntity` 의 **기반 클래스**) | Autoware 생명주기 · engage · routing · `waitForAutowareStateToBe()` **블로킹** | ⚠ 피한다 |

**무거운 것은 전부 제어 평면에 있고, 우리는 데이터 평면만 필요하다.**

##### ② 의 답

```cpp
// external/concealer/src/autoware_universe.cpp
getCommand("/control/command/control_cmd", rclcpp::QoS(1), *this),
getGearCommand("/control/command/gear_cmd", rclcpp::QoS(1), *this)

// simple_sensor_simulator EgoEntitySimulation::update()
const auto [speed, acceleration, tire_angle, gear_sign, gear_command]
    = autoware->getVehicleCommand();
vehicle_model_ptr_->setInput(input);
```

> **우리 어댑터가 `/control/command/control_cmd` 와 `/control/command/gear_cmd` 를 발행하면
> ego 차량모델이 움직인다.** 상류 patch 0, 2계단에서는 remap 조차 불필요(경쟁자가 없다).
> 3계단에서는 Autoware 출력을 remap 으로 끊고 우리가 그 자리에 선다 — **하류 배선 동일.**

`/vehicle/status/*` 는 `AutowareUniverse` 가 **발행 측**이므로 우리 몫이 아니다(확인항목 3 해결).

##### 08-07 실측이 여기서 설명된다

`traffic_simulation_demo` 의 `'ego'` 가 type=1(VEHICLE)이었던 이유 — **`EgoEntity` 로 spawn 하면
`launch_autoware` 기본값이 `true` 라 Autoware 가 실제로 뜨기 때문에** mock 이 `VehicleEntity` 로
회피한 것이다. 증상은 "타입" 이었고 원인은 **spawn 함수 선택**이었다.

##### 남은 리스크 하나 — 런타임 시험으로만 판정된다

`EgoEntitySimulation`(차량모델 + `AutowareUniverse`)은 ego 를 spawn 해야 생기는데,
`traffic_simulator` 에서 ego 를 spawn 하면 `FieldOperatorApplication` 도 **함께** 만들어진다.

##### ✅ 답 나옴 — **EgoEntity 는 곧 Autoware 다**

```cpp
// openscenario_interpreter/src/syntax/controller.cpp
auto Controller::isAutoware() const & -> bool
{
  return properties.get<Boolean>("isEgo") or name == traffic_simulator::VehicleBehavior::autoware();
}
```

**메서드 이름이 `isAutoware()`** 다. 상류는 "ego 인가" 와 "Autoware 인가" 를 같은 질문으로
취급한다. `EgoEntity : FieldOperatorApplication`(= *field operator* = 자율주행 스택)인 것도
같은 이유다.

| 근거 | |
|---|---|
| `cpp_mock_scenarios/CMakeLists.txt` | 유일한 EgoEntity 목을 빌드 제외 — *"require EgoEntity and cannot be used for CI"* |
| `scenario_test_runner/scenario/` 79개 `.yaml` | `LateralAction.LaneChangeAction.yaml` 의 `"ego"` 에 **`isEgo` 없음** → VehicleEntity |
| `Controller::isAutoware()` | ego ≡ Autoware |

**상류 자신도 Autoware 없이 EgoEntity 를 돌리지 않는다.**
`launch_autoware:=false` 는 "따로 띄웠다" 는 뜻이지 "없이 돈다" 가 아니다.

##### 그래서 2계단에는 **최소 ADAPI stub** 이 필요하다

§4 의 좋은 소식은 그대로다 — **무거운 것은 전부 제어 평면에 있다:**

| | 필요 | 비용 |
|---|---|---|
| `AutowareUniverse` (control_cmd → 차량모델) | ✅ | **0** — 토픽 2개 발행 |
| `FieldOperatorApplication` 만족 | ✅ | **최소 stub** — 상태 토픽 몇 개, 어댑터 밖 |

stub 의 목적은 Autoware 흉내가 아니라 **블로킹 해제**다.
구현: [`adapters/autoware-stub`](../../adapters/autoware-stub/README.md) (2026-08-07).

##### B1-2b 완료 — stub 은 **토픽 3 + 서비스 10** 짜리 상태 기계다

> 처음엔 서비스 7 개로 적었다. 08-10 실행이 정정했다 — `set_velocity_limit` ·
> `rtc_commands` · `rtc_auto_mode` 3 개는 상태를 하나도 안 건드리지만 **빠지면 안 된다.**
> concealer 는 열 개 전부에 클라이언트를 만들고, 서버가 없을 때의 실패 모드가
> **180 초 침묵 후 throw** 다. 근거와 경위: [`stub_node.py`](../../adapters/autoware-stub/autoware_stub/stub_node.py) 주석.

상수 발행이 아니다. concealer 는 `initialize() → plan() → engage()` 를 부르며 각 단계마다
상태 전이를 **기다린다**. 최신 세대(핀 기본값 `awf/universe/20240605`)에서 상태는
`/autoware/state` 가 아니라 **localization + routing + operation_mode 세 토픽에서 파생**된다.

| 내부 | 기동 시 | `localization/initialize` 후 | `routing/set_route_points` 후 | `set/engage` 후 |
|---|---|---|---|---|
| localization | **UNKNOWN** | **INITIALIZED** | INITIALIZED | INITIALIZED |
| route | **UNKNOWN** | UNSET | **SET** | SET |
| operation_mode | **UNKNOWN** | STOP / off | STOP / off | **AUTONOMOUS / on** |
| → 파생 | `initializing` | `waiting_for_route` | `waiting_for_engage` | **`driving`** |

> 기동 값이 `UNKNOWN` 인 것이 요점이다. `UNINITIALIZED/UNSET/STOP` 으로 시작하면 그 조합이
> 이미 `waiting_for_route` 로 읽혀서, `initialize()` 가 기다리는 `initializing` 전이가
> **concealer 가 구독하기 전에** 지나가 버린다. 건너뛴 상태는 관측되지 않는다.
> (이 문서의 이전 판은 여기가 틀려 있었다 — 2026-08-07 스모크 실행으로 정정.)

`driving` 이 되면 concealer 는 더 기다리지 않고, `AutowareUniverse` 가
`/control/command/control_cmd` 로 차량모델을 돌린다 — **그 토픽은 우리 vECU 가 낸다.**

전체 명세·근거·더 싼 legacy 경로 옵션:
[`evidence/ss2-concealer-survey-2026-08-07.md`](evidence/ss2-concealer-survey-2026-08-07.md) §5.3.
대조 도구: [`tools/sils/ss2/check_autoware_adapi.sh`](../../tools/sils/ss2/check_autoware_adapi.sh)
— 같은 표를 **3계단에서 진짜 Autoware 검증에도** 쓴다.

막히면 fallback 은 **`FieldOperatorApplication` 최소 stub** — 전면 ADAPI 가 아니라
블로킹을 푸는 최소 집합만 흉내낸다(어댑터 밖 유지). 상세: 조사 문서 §5.

---

## 5. ★ 플랜트 격리 — 두 루프 병행의 전제

**이것을 안 하면 두 루프를 동시에 돌릴 수 없다.**

`compare_golden` 은 `control_published`/`state_received` 값을 tick 인덱스로 대조한다.
**시뮬레이터가 다르면 차량 동역학이 달라 값이 당연히 다르다.** 그래서 MORAI golden 을 SS_v2 런과
대조하면:

```
   MORAI golden  vs  SS_v2 후보  →  값 발산  →  "REGRESSION"
                                      ▲
                          코드가 회귀한 건지 플랜트가 바뀐 건지
                          구분할 단서가 트레이스 어디에도 없다
```

회귀 게이트가 막으라고 있는 바로 그 **거짓 신호**다.

### 구현된 가드 (2026-08-06)

```
  capture_golden.py --plant morai
        └─▶ golden_meta 에 {"plant": "morai"} 기록

  compare_golden.py --golden G --candidate C --plant scenario_simulator_v2
        └─▶ gate_plant() 가 golden 의 plant 와 대조
              ├─ 일치            → 정상 비교 진행
              ├─ 불일치          → REJECTED, exit 2  ← 회귀가 아니라 정책 거부
              └─ 한쪽이라도 미선언 → 경고 + 통과 (게이트 미적용)
```

- **불일치는 `exit 2`(정책 거부)이지 `exit 1`(REGRESSION)이 아니다.** 플랜트가 다르면 필드 diff
  자체에 의미가 없으므로, 비교를 아예 수행하지 않는다(읽을 여지를 주지 않는다).
- **미선언은 경고로 통과한다.** 기존 `input_mode` 게이트와 같은 정책이다 — 이 필드가 생기기 전에
  캡처된 golden 을 전부 하드 실패로 바꾸면 기존 자산이 죽는다.
- `sils-ci.sh --plant ID` 로 capture·compare 두 스테이지에 동시 전달된다. 미선언 시 CI 로그에
  경고를 남긴다.

### 운영 규칙

> **두 루프를 병행하는 순간부터 `--plant` 는 필수다.** golden 디렉터리도 분리한다:
> `goldens/morai/` · `goldens/scenario_simulator_v2/`

플랜트 식별자 예: `morai` · `scenario_simulator_v2` · `awsim` · `fmu` · `fixture`

---

## 6. 스위치해도 따라오지 않는 것 4가지

배선은 스위처블이지만 **판정의 의미는 자동으로 옮겨가지 않는다.**

| # | 항목 | 내용 |
|---|---|---|
| **1** | **fidelity 등급이 갈린다** | MORAI 가 외부 스텝 제어를 지원하지 않으면 wall-free-run → `analyze_lockstep` 실패 → `fidelity_evidenced` 가 L3 에서 **L0** 으로. 버그가 아니라 사다리가 정상 작동하는 것이지만, **두 루프의 결과를 같은 급으로 읽으면 안 된다** |
| **2** | **golden 이 갈린다** | §5. 가드로 강제됨 |
| **3** | **종방향 명령 충실도가 갈린다** | 정규 계약은 `acceleration` m/s² 단일 모드. Autoware/SS_v2 는 네이티브 m/s² 라 **1:1 무손실**. MORAI 가 `longlCmdType=acceleration` 을 지원하지 않으면 accel/brake 0~1 정규화 + **차량 파라미터 튜닝**이 들어가 손실이 생긴다([BR-M-2](sils-real-bridge-requirements.md)) |
| **4** | **"자극·판정 한 소스"가 갈린다** | §4 주의 참조 |

### 루프 A 의 단위 변환 함정 ⚠

[EgoState.idl](../../idl/dds/sils/EgoState.idl) 주석이 명시적으로 경고한다 — MORAI 어댑터는
**반드시** 변환해야 한다:

| MORAI 원본 | 정규 | 누락 시 |
|---|---|---|
| `front_steer` deg | rad | **조향·yaw 루프가 ~57.3× 틀어짐** |
| angular vel deg/s | rad/s | 〃 |
| velocity km/h | m/s | 속도 판정 전면 오류 |

**변환은 어댑터에만 산다.** vECU·Autoware 브리지는 정규값만 읽는다.

---

## 7. 작업 목록

### 루프 B

| # | 작업 | 상태 |
|---|---|---|
| **B0-1** | `compare_golden` 플랜트 격리 가드 (`gate_plant`) | ✅ **완료** |
| **B0-2** | `capture_golden --plant` → `golden_meta.plant` | ✅ **완료** |
| **B0-3** | importer `ParameterDeclarations` 치환 (`$name`, 체인·순환·미선언·`${expr}` 처리) | ✅ **완료** |
| **B0-4** | [공통] CI 스테이지 연결(P0) — 현재 `Jenkinsfile*`·`.github/workflows` 에 sils 참조 **0건** | ⬜ |
| **B1-1** | [공통] BR-C-1 답: SS_v2 스텝 제어 — **소스로 확인** | ✅ **완료 — 아래 §7.1** |
| **B1-2** | concealer 삽입 지점 — 어댑터가 `/control/command/{control_cmd,gear_cmd}` 를 발행하면 ego 가 움직인다(**상류 patch 0**). ★ **EgoEntity ≡ Autoware** 이므로 2계단에는 대역이 필요 | ✅ **완료 — §4 · [근거](evidence/ss2-concealer-survey-2026-08-07.md)** |
| **B1-2b** | Autoware 대역 최소 집합 — 토픽 3 + 서비스 7 상태 기계로 확정, [`adapters/autoware-stub`](../../adapters/autoware-stub/README.md) 구현. [`smoke_autoware_stub.sh`](../../tools/sils/ss2/smoke_autoware_stub.sh) 로 9/9 PASS — `driving` 관통 + 서비스 7개 전부 응답 + 멱등성(2026-08-07) | ✅ **완료** — concealer 수용 여부만 B1-4 대기 |
| **B1-3** | [공통] BR-C-2a 실측 | 🔴 **3계단의 최우선 블로커 (08-10)**. ① 벤더 교차: Fast DDS ↔ Cyclone 은 discovery·pub/sub 은 되고 **서비스 왕복은 실패**(`RTPS_READER_HISTORY` payload 크기) → concealer ↔ AD API 가 전부 서비스라 Fast DDS 로는 3계단 불가. ② 그래서 Cyclone 으로 전환해보니 **2계단이 깨진다** — 어댑터가 `ddsi_xt_type_add_typeobj with invalid type object` 후 **segfault**, ego STATIONARY. XCDR/XTypes 칸은 이제 "미측정"이 아니라 **"실패, 원인 미상"**. [실측·다음 진단 순서](evidence/adapi-real-partition-2026-08-10.md) |
| **B1-4** | ego 시나리오 자산. ★ **변환기는 필요 없었다** — `scenario_test_runner` 가 `.yaml` 을 **내부에서** `.xosc` 로 변환해 받는다. 상류 82개 중 **29개가 `isEgo: true`** 이고, 그중 `RoutingAction.AcquirePositionAction.yaml` 을 골랐다(ego 1개·맵은 `find-pkg-share` 로 자체 해석). 실행: [`run_ego_scenario.sh`](../../tools/sils/ss2/run_ego_scenario.sh) | ✅ **완료 (2026-08-10)** — 진짜 concealer 가 stub 을 `driving` 까지 걸었다. 가는 길에 stub 결함 2건이 드러났다(서비스 10개 필요 · tier4 `status.code`) — §7.5 |
| **B1-5** | **SS_v2 브링업 (1계단 배선)** — 빌드·기동·실측·**vECU 관통 판정 완료** | ✅ **1계단 종료** [ss2-bringup §6.2](sils-ss2-bringup.md) · [실측](evidence/ss2-stage1-verdict-2026-08-07.md) |
| **B2-1** | SS_v2 → DDS 어댑터 [`adapters/ss2`](../../adapters/ss2/README.md) — 매핑 61 checks + **실런 관통**. V2(2.6↔3.x interop) ✅ · V1(twist 프레임) ✅ **VEHICLE** · V3(클록) ✅ **SIM·50 ms 고정 스텝** | ✅ **완료** |
| **B2-2** | **(W3)** SS_v2 frame 구동부 patch → barrier 참가자화. 어댑터만으로는 불충분(§7.1·§7.2 ①) | ⬜ |
| **B2-3** | VCU 배선 — `VehicleControl` → `/control/command/{control_cmd,gear_cmd}` 되먹임 어댑터. 구현: [`ss2_control_bridge`](../../adapters/ss2/README.md) | ✅ **완료 (2026-08-10)** — 폐루프에서 ego 가 **10.00 m/s 주행**, vECU setpoint 와 일치 |
| **B2-4** | importer 조건 확장 — `ReachPosition`/`Collision`/`TTC`. ⚠ **선행**: 트레이스에 position 신호가 없어 **schema v1 변경** 필요 | ⬜ |
| **B2-5** | **(W1)** `ExternalClockSource` (`mode="external"`) + fidelity 매핑에서 L3 차단 — "가짜 lockstep" 방지(§7.2) | ⬜ **범위 축소** — §7.4 |
| **B2-6** | **(W2)** 어댑터 `/clock` → tick 브리지 (SiLS 밖, B2-5 종속). ⚠ **실측상 `/clock` 은 BEST_EFFORT** — 유실 탐지 필수, 무시하고 중계하면 "가짜 lockstep" | ⬜ **범위 축소** — §7.4 |

**B 완료 기준**: `.xosc` 하나로 자극·판정 구동 → `gate.py` PASS · **B-det** 2회 실행
`compare_golden` **MATCH** + `fidelity_evidenced = L3` · **B-full** `analyze_vehicle` closed_loop

### 7.1 B1-1 조사 결과 — SS_v2 의 시간 전진 (2026-08-06, 소스 근거) ★

`tier4/scenario_simulator_v2` master 소스를 읽어 확인한 것. **각 항목에 근거 파일을 붙였다.**

| # | 확인 사항 | 결과 | 근거 |
|---|---|---|---|
| 1 | 시뮬레이션 시간이 논리적인가 wall 유도인가 | **논리 고정 증분** ✅ | `simulation_clock.cpp`: `seconds_since_the_simulator_started_ += realtime_factor / frame_rate_` — wall 경과와 무관 |
| 2 | frame 진행을 무엇이 구동하나 | **내부 wall 타이머** ⚠ | `openscenario_interpreter.cpp`: `timer = create_wall_timer(currentLocalFrameRate(), evaluate_storyboard)` (기본 30 Hz) |
| 3 | 외부 스텝 제어 API 가 있나 | **없음** ⚠ | interpreter 노드에 step/pause/tick 서비스·토픽 없음 |
| 4 | 프레임 안에 동기 barrier 가 있나 | **있으나 자기 센서 sim 에만** | `api.cpp updateFrame()`: `updateEntitiesStatusInSim()`/`updateTimeInSim()` 이 ZeroMQ **blocking req/resp** |
| 5 | 제어기(Autoware)를 기다리나 | **아니오** ❌ | `ego_entity.cpp`: `updateFieldOperatorApplication()` → `spinSome()` — **비블로킹**, 최신 수신값만 읽음 |
| 6 | `/clock` 권위 | traffic_simulator 가 발행 | `updateFrame()` 말미 `clock_pub_->publish(...)`. 단 `use_sim_time` **기본 false** |

**판정**: **그대로는 L3 불가.** 시간 *값*은 결정론적이지만(1), 프레임 진행이 wall 타이머이고(2)
제어기가 barrier 밖이라(5) 프레임당 수신되는 제어 명령 수·시점이 실행마다 흔들린다. 이는 우리
S3 문서가 기술한 갭과 **정확히 같은 구조이며 방향만 반대**다 — S3 에서는 시뮬레이터가 barrier
밖이었고, 여기서는 제어기가 밖이다.

> ⚠ **웹 검색 요약을 믿지 말 것.** 검색 엔진이 "scenario_simulator_v2 는 synchronous mode 로
> 클라이언트 명령과 lockstep 진행한다" 는 요약을 냈는데 **소스와 상충한다**(CARLA 의 synchronous
> mode 와 혼동한 것으로 보인다). 위 표는 전부 소스 직접 확인이다.

**그러나 개조 비용은 낮다** — 세 조건이 유리하다:

1. 시간이 **이미** 논리 고정 증분이다(wall 유도가 아니다) → 시간 모델을 갈아엎을 필요가 없다
2. frame 진행이 `updateFrame()` **함수 하나**로 캡슐화돼 있다
3. 이미 **동기 req/resp(ZeroMQ)** 로 진행하는 구조라 blocking 진행이 낯설지 않다

**필요한 작업**: `create_wall_timer` 를 대체해 우리 `TickGrant` 를 받아 `updateFrame()` 을 호출하고
`TickComplete{participant_id}` 로 응답하는 구동부. 두 경로:

- **(i) 포크/패치** — interpreter 의 timer 를 grant 구독으로 교체. 빠르지만 상류 추적 부담
- **(ii) 상류 기여** — "external clock source" 옵션을 제안. 느리지만 유지보수가 없음

우리 IDL·barrier 는 **변경 0** 이다(참가자 집합만 늘어난다, §3).

**Autoware 는 별개 문제다.** 시뮬레이터를 barrier 에 넣어도 Autoware 자체는 여전히 비동기다 —
그래서 §1 의 **B-det / B-full** 분리가 나온다. Autoware 를 barrier 에 넣는 것은 이 작업 범위 밖이고,
현실적으로는 B-full 에서 결정론을 포기하는 것이 맞다.

### 7.2 그래서 우리 쪽은 어떻게 하나 (B1-1 대응) ★

#### 먼저 — "Web.Auto 코드를 고치나?" **아니다**

자주 혼동되는 지점이라 못박는다. **고치는 대상은 Web.Auto 가 아니라 `scenario_simulator_v2` 다.**

| | 무엇 | 우리 관계 | 고치나 |
|---|---|---|---|
| **Web.Auto** | TIER IV **클라우드 SaaS** (Scenario Editor·CI/CD·Evaluator) | 산출물 **`.xosc` 파일만** 가져온다 | **불가·불필요** |
| **`scenario_simulator_v2`** | TIER IV **오픈소스** (Apache-2.0) | **우리 온프렘에서 우리가 띄운다** | **여기를 patch** |

즉 자산(`.xosc`)은 Web.Auto 에서 오고, 그것을 **실행하는 엔진은 우리가 셀프호스팅하는 오픈소스**다.
클라우드 실행에 의존하지 않는다 — §3 의 "스위치되는 것" 안에 들어오는 이유가 이것이다.

#### "wall clock 을 쓴다" 는 절반만 맞다

| | SS_v2 실제 | 우리가 원하는 것 |
|---|---|---|
| 시간 **값** | `+= realtime_factor / frame_rate_` — 논리 고정 증분, wall 경과 무관 | ✅ **이미 일치** |
| 프레임 **진행 트리거** | `create_wall_timer(30 Hz)` | ❌ **여기만 어긋난다** |

*"시계가 벽시계"* 가 아니라 ***"벽시계가 시뮬레이션을 떠민다"*** 다. 시간 값은 이미 우리가 원하는
형태이고 **누가 다음 프레임을 명령하느냐만** 어긋나 있다 — 위 "개조 비용이 낮은" 첫 번째 이유.

#### 선택지 3개

| | 무엇을 고치나 | 결정론 | fidelity | 쓰임 |
|---|---|---|---|---|
| **①** SS_v2 를 barrier 참가자로 | **SS_v2 구동부** patch (남의 코드) | ⭕ | **L3** | **B-det** |
| **②** 우리가 SS_v2 `/clock` 을 따라감 | 어댑터 + 우리 clock source 1개 | ❌ | L1 (시간축만 정렬) | **B-full** |
| **③** 각자 free-run | 없음 | ❌ | L0 | — |

**①·② 를 둘 다 한다.** §1 의 B-det / B-full 갈래가 정확히 이 두 선택지에 대응한다. ③ 은 기본
상태이지 선택지가 아니다.

#### 우리 쪽 실제 작업

[`clock_source.hpp`](../../service/sils/vecu/src/clock_source.hpp) 의 `IClockSource` 가 이미
`wall` / `lockstep` 두 구현으로 갈려 있어 확장 지점은 열려 있다.

| # | 작업 | 어디 | 선택지 |
|---|---|---|---|
| **W1** | `ExternalClockSource` 추가 (`mode() == "external"`) — `/clock` 종속, **barrier 없음**을 명시. `run_loop` 무수정 | SiLS 안 | ② |
| **W2** | 어댑터에 `/clock` → tick 브리지 | **SiLS 밖** (기존 ROS2↔DDS 어댑터 컨테이너) | ② |
| **W3** | SS_v2 patch — wall timer → `TickGrant` 구독 → `updateFrame()` → `TickComplete` | **외부 저장소** (fork or 상류) | ① |
| **W4** | golden plant id 에 B-det / B-full 구분 반영 | `--plant` 값 규약 (§5) | ①② |

W4 는 코드 변경이 아니다 — 위 표 **B0-1** 의 `gate_plant()` 가 이미 격리해 주므로 **id 문자열만**
나누면 된다(예: `ss2-det` / `ss2-full`).

#### ⚠ 여기서 제일 위험한 함정 — "가짜 lockstep"

② 를 *"어댑터가 `/clock` 을 `TickGrant` 로 바꿔 쏘면 우리 코드 0 수정"* 으로 처리하고 싶어진다.
**그러면 안 된다.**

`LockstepClockSource` 의 `GrantQueue` 는 **무한 FIFO** 이고 QoS 가 `RELIABLE + KEEP_LAST(256)` 이다.
free-run 퍼블리셔가 grant 를 쏟으면 vECU 는 **최신으로 점프하지 않고 뒤처진 채 계속 소비**하면서,
그 동안 `tick_complete` 를 정상적으로 찍는다. `analyze_lockstep` 은 이것을 **건강한 barrier 로 판정**한다.

> barrier 없는 시간 종속은 **반드시 `lockstep` 이 아닌 별도 mode 이름**을 달아야 하고, fidelity
> 매핑에서 **L3 부여를 막아야** 한다. W1 이 "어댑터만으로 되지 않나" 를 뚫고 필요한 진짜 이유다.

이는 §5 의 플랜트 격리와 같은 종류의 방어다 — **판정이 조용히 거짓말하는 경로를 막는 것**.

### 7.3 계단 나누기 — 1계단(배선)은 B1-2·W3 없이 간다 ★

W3(시뮬레이터 patch)와 B1-2(concealer)는 무겁다. **둘 다 필요 없는 계단이 앞에 하나 있다**:

| 계단 | 무엇 | 루프 | 선행 | fidelity | 상태 |
|---|---|---|---|---|---|
| **1 · 배선** | SS_v2 → 어댑터 → `EgoState` → vECU → trace → gate | **열림** (제어 되먹임 없음) | [ss2-bringup](sils-ss2-bringup.md) 만 | L0 | ✅ **완료 2026-08-07** |
| 2 · B-det | 제어 되먹임 + barrier | 닫힘 | + B1-2, W3 | **L3** | ✅ **완료 2026-08-10** — ego 10.00 m/s |
| 3 · B-full | Autoware 투입 | 닫힘 | + 실물 Autoware | L0~L1 | 🟡 진행 — [3계단 문서](sils-stage3-autoware-partition.md) |

1계단은 §2 기준으로 "열린 루프라 rosbag 재생과 다를 게 없는" 상태다 — **판정용으로는 맞는 말이다.**
하지만 여기서 얻으려는 것은 판정이 아니라 **배선 사실**(RMW·type name·QoS·도메인·discovery)이므로
별개다. **1계단 결과로 fidelity 를 주장하지 말 것.**

소스로 확인된 두 가지 (근거·절차: [ss2-bringup §1](sils-ss2-bringup.md)):

1. **Autoware 실물은 1·2계단에 불필요** — `concealer` 는 Autoware *메시지 패키지*만 요구하고
   `dependency_humble.repos` 는 전부 메시지·유틸 저장소다. 21.7GB CUDA 이미지도 GPU 도 3계단에서만.
2. ⚠ **상류 기본 RMW 는 Cyclone DDS** — 상류 `Dockerfile` runtime 스테이지에
   `ENV RMW_IMPLEMENTATION=rmw_cyclonedds_cpp`. [BR-C-2a](sils-real-bridge-requirements.md) 가
   가정으로 적어둔 Fast DDS ↔ Cyclone 문제가 **기본 구성**이라는 뜻이다.

---

### 7.4 1계단 실측이 §7.1 을 수정한다 — SS_v2 에는 이미 스텝이 있다 ★

**1계단 완료 (2026-08-07).** 원측정값: [`evidence/ss2-stage1-verdict-2026-08-07.md`](evidence/ss2-stage1-verdict-2026-08-07.md).

| 검증 | 결과 | 근거 |
|---|---|---|
| **V2** Fast DDS 2.6(어댑터) ↔ 3.x(vECU) | ✅ **PASS** | distinct 513 샘플, `src_seq` 역행 0 |
| **V1** `velocity` 프레임 | ✅ **VEHICLE** | 헤딩 0.166 rad 편차 지점의 `\|vy\|/\|v\| = 0` (지도 프레임이면 0.165) |
| **V3** `stamp_ns` | ✅ **SIM · 50 ms 고정 스텝** | epoch 아님 + 전 stamp 가 50 ms 배수 |

#### §7.1 결론의 정정

[§7.1](#71-b1-1-조사-결과--ss_v2-의-시간-전진-2026-08-06-소스-근거-) 은 "SS_v2 는 tick 이 아니라
wall clock 으로 전진한다" 였다. **절반만 맞다.**

| | §7.1 (소스 기준) | 1계단 실측 |
|---|---|---|
| 시간의 출처 | wall clock | **50 ms 고정 스텝**을 wall clock 에 맞춰 굴림 |
| "한 스텝" 개념 | 없다고 가정 | **이미 있다** |

둘은 모순이 아니다. SS_v2 는 **고정 스텝 적분기를 실시간 속도로 돌리는** 구조다 —
스텝은 존재하고, 다만 **외부가 그 진행을 결정하지 못할 뿐**이다.

#### W1/W2 범위 축소

| | 정정 전 | 정정 후 |
|---|---|---|
| W3 (SS_v2 patch) | 스텝 개념을 **새로 도입** | **이미 있는 스텝의 트리거를 외부로** 돌림 |
| W1 (`ExternalClockSource`) | 변경 없음 | 변경 없음 — dt 를 **50 ms 로 알고 시작**할 수 있음 |
| W2 (`/clock` 브리지) | 변경 없음 | 변경 없음 |

##### ★ 그 50 ms 는 상수가 아니라 **런치 인자**다 (2026-08-07 소스 확인)

```cpp
// traffic_simulator/include/.../simulation_clock.hpp
auto getStepTime() const { return realtime_factor / frame_rate_; }
```

`global_real_time_factor / global_frame_rate` 이고 **둘 다 런치 인자**다.
우리가 50 ms 를 본 이유는 `cpp_mock_scenarios` 의 `global_frame_rate` 기본값이
**20.0** 이기 때문이다. 반면 `scenario_test_runner` 의 기본값은 **30.0**(33.3 ms)이다.

| | 기본 frame rate | 스텝 |
|---|---|---|
| `cpp_mock_scenarios` (1계단에서 쓴 것) | 20 Hz | **50 ms** ← V3 가 측정한 값 |
| `scenario_test_runner` (2·3계단) | 30 Hz | 33.3 ms |
| 상류 CI | `global_frame_rate:=20` 로 **덮어씀** | 50 ms |

따라서 그냥 두면 **1계단과 2계단의 스텝이 달라져** 트레이스를 비교할 수 없게 된다.
`docker-compose-sils-ss2.yaml` 의 `ss2-scenario` 가 `global_frame_rate:=20` 을 기본으로
넘기도록 고정했다 — 상류 CI 와도 같은 값이다.

**W3 범위가 한 번 더 준다.** 스텝의 *크기*는 이미 외부가 정한다. 남은 것은 *진행*뿐이다.

**"가짜 lockstep" 경고는 그대로 유효하다.** 스텝이 존재한다는 것과 그 스텝을 우리가
구동한다는 것은 다르다. `mode="external"` 은 여전히 L3 를 부여받지 못한다.

#### V1 이 VEHICLE 인 것의 의미

어댑터에 좌표 변환을 넣을 필요가 없다 → **2계단 되먹임 경로가 지금 코드 위에 그대로 얹힌다.**
만약 MAP 이었다면 곡선 구간마다 조용히 틀린 횡속도가 vECU 로 들어갔을 것이고,
직선 회귀시험으로는 영원히 잡히지 않았을 것이다.

---

### 7.5 B1-4 — 변환기는 필요 없었다 (2026-08-07, 핀 `3370c817` 소스 조사) ★

B1-4 는 "TIER IV Scenario Format v2.0 → `.xosc` 변환 경로를 찾아라"로 적혀 있었다.
**만들 것이 없다.** `scenario_test_runner` 가 입구에서 이미 변환한다:

```python
# test_runner/scenario_test_runner/scenario_test_runner/scenario_test_runner.py:45
if scenario.path.suffix == ".xosc":
    ...                                            # 그대로 통과
else:  # == '.yaml' or == '.yml'
    for path in convert(scenario.path, ...):       # openscenario_utility 로 변환
```

→ B1-4 는 **변환 작업이 아니라 자산 선택**이다.

#### 상류 자산 실태

| | |
|---|---|
| 시나리오 `.yaml` | **82개** (`scenario_test_runner/scenario/`) |
| 그중 `isEgo: true` | **29개** |
| 상류 CI(`config/workflow.txt`)가 도는 것 | 32개 |
| **그중 `isEgo: true`** | **0개** ⚠ |

CI 목록에서 `isEgo` 로 잡히는 두 파일은 값이 전부 `'false'` 였다.
즉 **상류도 EgoEntity 시나리오를 CI 로 돌리지 않는다.** `cpp_mock_scenarios` 가
*"require EgoEntity and cannot be used for CI"* 로 제외한 것과 **같은 공백**이며,
`.xosc` 경로라고 사정이 다르지 않다. 여기서 실패하면 **상류 쪽 문제일 가능성이 우리 쪽과 비슷하다.**

#### 고른 자산 — `RoutingAction.AcquirePositionAction.yaml`

| 조건 | 왜 |
|---|---|
| 엔티티 **1개**, `isEgo: true` | NPC·센서·신호등이 없어 실패 원인이 핸드셰이크로 좁혀진다 |
| `$(find-pkg-share kashiwanoha_map)/map/lanelet2_map.osm` | 이미지 안에서 **런타임 해석**. `lane_change.xosc`(`filepath="./"`, 맵 없음)와 달리 고칠 경로도, 마운트할 것도 없다 |
| `AcquirePositionAction` | concealer 의 `plan()` 이 이걸 `/api/routing/set_route_points` 로 바꾼다 — **우리 stub 이 답하는 그 서비스** |
| 도착해야 성공 (`exitFailure` @180 s) | B2-3 이 생기면 **같은 자산이 그대로 2계단 목표**가 된다 |

#### `launch_autoware:=false` 로도 concealer 는 돈다

이 플래그는 **프로세스 기동만** 건너뛴다:

```cpp
// traffic_simulator/src/entity/ego_entity.cpp:72
return getParameter<bool>(node_parameters, "launch_autoware", true)
         ? concealer::ros2_launch(...)   // Autoware 프로세스를 띄운다
         : 0;                            // ← false 면 여기. 그게 전부다
```

`EgoEntity` 는 여전히 `FieldOperatorApplication` 을 상속하고, 그 생성자가
`initialize_duration`(기본 30 s) 타이머를 걸고
`requestInitialPose("/api/localization/initialize", ...)` 를 부른다.
**답하는 쪽이 우리 stub 이다.**

#### 실행

```bash
tools/sils/ss2/run_ego_scenario.sh
```

stub 을 **먼저** 띄운다(순서가 중요하다 — concealer 는 EgoEntity 생성 즉시 서비스를 부르기
시작하고, 없으면 `initialize_duration` 예산을 없는 서비스에 태운다. 1계단에서 같은 이유로
런 하나를 날렸다). 판정 근거는 **stub 로그**다 — 진짜 concealer 가 우리 서비스를 불렀다면
전이가 찍힌다.

> **PASS 여도 시나리오는 실패한다.** `control_cmd` 를 내는 쪽이 아직 없어서 ego 가 제자리에
> 선다. 이 시험의 합격 기준은 "**concealer 가 `driving` 까지 왔는가**"이지 시나리오 성공이
> 아니다. 나머지는 B2-3 이다.

---

### 루프 A

| # | 작업 | 상태 |
|---|---|---|
| **A0-1** | B0-1~B0-4 전제 (B 에서 완료됨, 재작업 없음) | 🟡 부분 |
| **A1-0** | 🔴 **벤더 질의서 작성** — A1-1~A1-4 를 발송 가능한 형태로 통합 | ✅ [morai-vendor-questions](sils-morai-vendor-questions.md) |
| **A1-1** | 🔴 **벤더 확인** — Sync Mode. ★ 공개 자료로 **존재 확인**(`WaitForTick`/`SyncModeCmd`, [BR-M-3](sils-real-bridge-requirements.md)) → 남은 질문은 **ROS 2 가용성**과 동작 세부 | 🟡 질의서 Q1·Q5-0 |
| **A1-2** | 🔴 **벤더 확인** — `longCmdType` 에 acceleration(m/s²) 모드? → 무손실 vs 정규화 | ⬜ 질의서 Q2-1 |
| **A1-3** | 🔴 **벤더 확인** — MORAI 시나리오 러너가 `.xosc` 를 받나? | ⬜ 질의서 Q4-1 |
| **A1-4** | 🔴 **벤더 확인** — `CtrlCmd` 조향 단위 (IDL 주석에 `[확인]` 으로 남아 있음) | ⬜ 질의서 Q2-2 |
| **A2-1** | MORAI ↔ DDS 어댑터 — ⚠ 단위 변환 (§6) | ⬜ |
| **A2-2** | 종방향 명령 매핑 (A1-2 종속) | ⬜ |
| **A2-3** | [BR-A-2](sils-real-bridge-requirements.md) 결정 — Autoware 상태 피드백 경로 | ⬜ |
| **A2-4** | lockstep 참가자화 (**A1-1 = O 일 때만**) | ⬜ |

**A 완료 기준**: 폐루프 왕복 + `analyze_vehicle` PASS · A1-1=O 면 `analyze_lockstep` PASS + **L3**,
A1-1=X 면 **L0 + 폐루프 유효**로 명시 보고 · golden 은 **MORAI 전용 세트**

---

## 8. 사용법 요약

```bash
# --- 루프 B: Web.Auto 시나리오 ------------------------------------------------
# 1) .xosc → 요구 (파라미터 치환 포함)
python3 tools/sils/conformance/import_openscenario.py scenario.xosc -o requirements.yaml

# 2) 판정
python3 tools/sils/conformance/gate.py runs/b/traces/ --requirements requirements.yaml

# 3) 기준선 등록 — 플랜트 반드시 선언
python3 tools/sils/conformance/capture_golden.py --trace runs/b/traces/run.trace.jsonl \
    --out goldens/scenario_simulator_v2/run.trace.jsonl \
    --plant scenario_simulator_v2 --input-mode full-lockstep

# 4) 회귀 (CI)
bash tools/sils/ci/sils-ci.sh --topology <T> --out-dir runs/b \
    --golden-dir goldens/scenario_simulator_v2 --plant scenario_simulator_v2

# --- 루프 A: MORAI ------------------------------------------------------------
bash tools/sils/ci/sils-ci.sh --topology <T> --out-dir runs/a \
    --golden-dir goldens/morai --plant morai

# --- 섞으면 막힌다 (의도된 동작) ----------------------------------------------
python3 tools/sils/conformance/compare_golden.py \
    --golden goldens/morai/run.trace.jsonl --candidate runs/b/traces/run.trace.jsonl \
    --plant scenario_simulator_v2
#   → REJECTED (exit 2): plant mismatch — 회귀가 아니라 정책 거부
```

---

*갱신 이력: 2026-08-06 B1-1 조사 완료 → §1 에 B-det/B-full 분리, §7.1 근거 표 추가.
2026-08-06 §7.2 추가 — 대응 선택지 ①②③, 우리 쪽 작업 W1~W4(→ B2-5·B2-6), "가짜 lockstep" 함정,
그리고 **patch 대상은 Web.Auto 가 아니라 `scenario_simulator_v2`** 임을 명시.*

*다음 갱신 시점: **B1-2**(concealer 삽입 지점 (a)/(b) 결정)가 정해졌을 때 — §4 의 표와 §7 B1-2·B2-3 을
함께 고칠 것. 또는 **B1-3**(RMW 실측)이 나오면 [BR-C-2a](sils-real-bridge-requirements.md) 매트릭스를 채울 것.
**B2-5**(W1)를 착수하면 §8 의 `--plant scenario_simulator_v2` 예시를 `ss2-det`/`ss2-full` 로 나눌 것.*
