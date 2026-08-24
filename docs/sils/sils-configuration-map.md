# 구성 지도 — 어떤 구성에서 어떤 어댑터가 도는가 ★

> 작성 2026-08-12 · 근거는 전부 `evidence/` 의 실측
> **배선의 원리**는 [`sils-integration-topology.md`](sils-integration-topology.md),
> 이 문서는 **"오늘 무엇을 켜고 무엇을 꺼야 하는가"** 만 다룬다.
> **복붙 가능한 전체 실행 명령**은 [`sils-docker-runbook.md`](sils-docker-runbook.md) §3 —
> 아래 구성 이름(C0/C1/C2/C3)이 그대로 그 문서의 절 제목이다.

---

## 0. 왜 이 문서가 필요한가 — 실제로 혼동이 났다

어댑터는 여섯인데, **한 번도 여섯이 같이 돈 적이 없다.** 구성마다 켜는 게 다르고,
그 이유가 문서에 없어서 이런 질문이 반복됐다:

> "기존에 우리가 만든 어댑터들은 왜 불필요해졌지?"

답은 **불필요해진 게 아니라 그 구성에서 자리가 없어진 것**이다. 그리고 자리가 없어진
이유는 딱 하나다 — **누가 `vehicle_interface` 좌석에 앉았느냐.**

```
        Autoware 는 플랜트가 누구인지 모른다.
        아는 것은 이 두 갈래뿐이다.

              ┌──────────── Autoware 파티션 3종 ────────────┐
              │      Perception ─▶ Decision ─▶ Control      │
              └──┬──────────────────────────────────▲──────┘
   control_cmd   │                                  │  /vehicle/status/*
   gear_cmd      ▼                                  │  /localization/*
             ╔══════════════════════════════════════════════╗
             ║        vehicle_interface 좌석 (VCU)           ║
             ║   ★ 이 자리에 누가 앉느냐가 모든 것을 정한다     ║
             ╚══════════════════════════════════════════════╝
                            ▲             ▼
                       (차량 상태)      (구동 명령)
                            └── 플랜트 ───┘
```

**좌석에 앉을 수 있는 후보는 넷이고, 서로를 배제한다:**

| 좌석 주인 | 언제 | 우리 상태 브리지 2개는 |
|---|---|---|
| `simple_planning_simulator` (Autoware 내장) | 벤더 기본 구성 | ❌ 꺼야 한다 (라이터 둘) |
| **`concealer`** (SS_v2 가 데려온다) | 시나리오에 `isEgo` 가 있을 때 | ❌ 꺼야 한다 (라이터 둘) |
| **우리 브리지 + vECU** | 좌석이 비어 있을 때 | ✅ **이게 그 자리다** |
| 실차 CAN 드라이버 | 실차 | (해당 없음) |

> **한 좌석에 둘이 앉으면 DDS 는 막지 않는다.** 같은 토픽에 라이터가 둘이 되고 Autoware 는
> 두 차량의 상태를 표시 없이 번갈아 읽는다. **에러도 경고도 없다.** 이 저장소가 가장 자주
> 밟은 함정이고, 아래 구성표가 존재하는 유일한 이유다.

---

## 1. 어댑터 여섯 — 전수표 ★

전부 [`adapters/ss2/src/`](../../adapters/ss2/src/) 한 패키지, 실행물만 다르다
(`ros2 run ss2_ego_bridge <실행물>`). 각각 **한쪽은 ROS 2(Cyclone), 다른 쪽은 정준
Fast DDS** 인 두 얼굴 프로세스다.

```
        ROS 2 (Autoware / SS_v2)                    정준 SiLS 버스 (Fast DDS)
   ─────────────────────────────────────      ────────────────────────────────

     /entity/status ─────────────────▶ ①ss2_ego_bridge ────▶ sils/ego/state
     /api/vehicle/{kinematics,status} ▶ ⑥aw_ego_state_bridge ▶ sils/ego/state  🟡비계

     /vehicle/status/* (6) ◀────────── ③aw_vehicle_status_bridge ◀── sils/ego/state
     /localization/* + /initialpose3d ◀ ④aw_localization_bridge  ◀── sils/ego/state

     /control/command/{control,gear}_cmd ▶ ②aw_control_target_bridge ▶ autoware/control_target
     /control/command/{control,gear}_cmd ◀ ⑤ss2_control_bridge      ◀ sils/ego/cmd
```

| # | 어댑터 | 방향 | 읽는 것 | 내는 것 |
|---|---|---|---|---|
| ① | [`ss2_ego_bridge`](../../adapters/ss2/src/ss2_ego_bridge.cpp) | 플랜트 → 버스 | ROS `/entity/status`<br>(`EntityStatusWithTrajectoryArray`) | DDS `sils/ego/state` (`EgoState`) |
| ② | [`aw_control_target_bridge`](../../adapters/ss2/src/aw_control_target_bridge.cpp) | Autoware → 버스 | ROS `/control/command/control_cmd`<br>`/control/command/gear_cmd` | DDS `autoware/control_target` (`ControlTarget`) |
| ③ | [`aw_vehicle_status_bridge`](../../adapters/ss2/src/aw_vehicle_status_bridge.cpp) | 버스 → Autoware | DDS `sils/ego/state` | ROS `/vehicle/status/*` **6개** |
| ④ | [`aw_localization_bridge`](../../adapters/ss2/src/aw_localization_bridge.cpp) | 버스 → Autoware | DDS `sils/ego/state` | ROS `/localization/kinematic_state`<br>`/localization/acceleration`<br>**`/initialpose3d`** (1 Hz 래치) |
| ⑤ | [`ss2_control_bridge`](../../adapters/ss2/src/ss2_control_bridge.cpp) | 버스 → 플랜트 | DDS `sils/ego/cmd` (`VehicleControl`) | ROS `/control/command/control_cmd` + `gear_cmd` |
| ⑥ | [`aw_ego_state_bridge`](../../adapters/ss2/src/aw_ego_state_bridge.cpp) 🟡 | Autoware → 버스 | ROS `/api/vehicle/kinematics`<br>`/api/vehicle/status` | DDS `sils/ego/state` |

### 셋으로 묶어 보면 외우기 쉽다

| 묶음 | 어댑터 | 무엇을 하나 | 좌석과의 관계 |
|---|---|---|---|
| **상태 상행** | ① (플랜트→버스) | 진짜 차량 상태를 정준 계약으로 | 좌석과 무관 — **항상 필요** |
| **상태 좌석** | ③ + ④ | 그 상태를 Autoware 에게 먹인다 | ★ **좌석 그 자체.** 좌석 주인이 따로 있으면 꺼야 한다 |
| **명령 하행** | ② (Autoware→버스) | Autoware 판단을 vECU 에게 | 좌석과 무관 — **항상 필요** |
| **명령 좌석** | ⑤ (버스→플랜트) | vECU 결과를 차에 반영 | ★ 좌석의 나머지 반쪽 |
| **비계** | ⑥ | 플랜트 없이 개발용 | 🟡 **실 구성 금지** |

> ★ **③④⑤ 는 "좌석 부품" 이고 ①② 는 아니다.** 이 한 줄이 아래 매트릭스 전체를 설명한다.
> 좌석 주인이 바뀌면 ③④⑤ 만 흔들리고 ①② 는 그대로다.

### ⚠ ⑥ `aw_ego_state_bridge` 는 실 구성에 넣지 말 것

Autoware ADAPI 로 `EgoState` 를 만든다. 그런데 그 값은 Autoware 자신의 시뮬레이터가
**자기 자신을 보고하는 것**이라, 여기서 먹인 ECU 는 자기가 읽는 값에 영향을 줄 수 없다 —
**무슨 짓을 해도 개루프**다. ①과 같이 켜면 `sils/ego/state` 에 라이터가 둘이 된다.
존재 이유는 플랜트가 없는 동안 개발자 ECU 를 진짜 Autoware 에 물려 보는 것뿐이다.

---

## 2. 구성 넷 — 그림과 함께

### C0 · 픽스처 (플랜트 없음)

> 목적: **배선만** 검증. 플랜트 변수를 0으로 둔다.
> 증거: [`evidence/vehicle-seat-no-simulator-2026-08-11.md`](evidence/vehicle-seat-no-simulator-2026-08-11.md)

```
  bin/sils_ego_source ──▶ sils/ego/state ──┬──▶ ③ ──▶ /vehicle/status/*  ─┐
   (개루프 픽스처 50 Hz)                     └──▶ ④ ──▶ /localization/*    ─┤
                                                                          ▼
                                                                    ┌──────────┐
   vECU ◀── autoware/control_target ◀── ② ◀── /control/command/*  ◀─┤ Autoware │
    │                                                               └──────────┘
    └──▶ sils/ego/cmd ──▶ (아무도 안 받는다 — 픽스처는 명령을 무시한다)
```

| 좌석 주인 | **우리 (③④)** |
|---|---|
| 켜는 것 | ② ③ ④ + `sils_ego_source` |
| 끄는 것 | ① ⑤ ⑥ · `simple_planning_simulator` |
| 증명되는 것 | QoS 매칭 · 구독 매칭 · starvation 로그 정지 · 유실 0 |
| **증명 안 되는 것** | **주행.** 픽스처가 맵 원점에서 출발해 lanelet 위가 아니다 → 경로가 안 선다 |

```bash
tools/sils/ss2/run_control_partition_sils.sh          # 벤더 시뮬레이터를 끈다
tools/sils/ss2/run_vehicle_seat_bridges.sh --with-ego-source
```

---

### C1 · SS_v2 를 플랜트로 (`isEgo` 없음) — 좌석은 우리 것

> 목적: **진짜 맵 좌표를 아는 플랜트**로 좌석 배선을 검증. concealer 가 안 뜬다.
> 증거: [`evidence/autoware-return-path-2026-08-11.md` §11](evidence/autoware-return-path-2026-08-11.md)

```
   ┌── SS_v2 (isEgo 없음 → concealer 안 뜬다) ────────────────┐
   │                                                          │
   │   엔티티 ◀── 시나리오 각본이 움직인다 (LaneChangeAction …)  │  ★ 우리 명령이 아니다
   │      │                                                   │
   │  /entity/status ──▶ ① ──▶ sils/ego/state ──┬──▶ ③ ──▶ /vehicle/status/*  ┐
   │                                            └──▶ ④ ──▶ /localization/*    │
   └──────────────────────────────────────────────────────────┘               ▼
                                                                        ┌──────────┐
                vECU ◀── autoware/control_target ◀── ② ◀── control_cmd ◀┤ Autoware │
                 │                                                      └──────────┘
                 ▼
           sils/ego/cmd  ──▶ ⑤ ──▶ control_cmd ──▶ ✗ 받는 사람이 없다
```

| 좌석 주인 | **우리 (③④) + vECU** |
|---|---|
| 켜는 것 | ① ② ③ ④ |
| 끄는 것 | ⑤ (받는 사람이 없다 — 아래) · ⑥ |
| 도달한 것 | `initialization_state=INITIALIZED` · 경로 SET · **AUTONOMOUS** · vECU `target_received` |
| 차를 움직이는 것 | **시나리오 각본.** Autoware 도 vECU 도 아니다 → **개루프** |
| 남은 것 | 목표 지점 선택 — `Goal's footprint exceeds lane!` (10~30 m 앞은 SET, 40 m 이상 거부) |

### ⚠ C1 은 폐루프가 될 수 없다 — SS_v2 의 구조다

`simple_sensor_simulator.cpp` 가 명령을 소비하는 차량 모델을 **`isEgo` 일 때만** 만든다:

```cpp
if (req.is_ego()) {
    ego_entity_simulation_ = std::make_shared<vehicle_simulation::EgoEntitySimulation>(...);
} else {
    /* 평범한 엔티티 — 시나리오 각본대로 움직인다 */
}
```

그래서 **`isEgo` 없는 엔티티에 우리 명령을 넣을 방법이 없다.** ⑤ 를 띄워도 `control_cmd`
를 받아 갈 것이 SS_v2 안에 존재하지 않는다.

> ★ **그리고 이것이 SS_v2 의 근본 딜레마다.**
>
> | 원하는 것 | 필요한 것 | 대가 |
> |---|---|---|
> | 우리 명령으로 차가 움직인다 | `isEgo` → `EgoEntitySimulation` | **concealer 가 딸려 와 좌석을 가져간다** (→ C2) |
> | 좌석을 우리가 쥔다 | `isEgo` 없음 | **차를 움직일 수 없다** (→ C1) |
>
> **SS_v2 에서는 둘을 동시에 못 한다.** MORAI 에는 이 딜레마가 없다 — concealer 가
> 없으므로 좌석을 쥔 채로 명령을 넣는다. C1 은 그 구성의 **리허설**이고,
> 폐루프 증명은 아니다.

> ⚠ 재사용 가능한 kashiwanoha 목표 좌표는 evidence §11 에 적어 뒀다.
> `check_footprint_inside_lanes` 는 `ros2 param set` 으로 못 끈다 — 생성 시점에 캐시된다.

---

### C2 · SS_v2 EgoEntity 주행 (2026-08-12, 오늘) — **좌석을 concealer 에게 내줬다**

> 목적: **Autoware 가 SS_v2 차를 실제로 몬다.** 시나리오 판정까지.
> 증거: [`evidence/ss2-autoware-driving-2026-08-12.md`](evidence/ss2-autoware-driving-2026-08-12.md)

```
   ┌────────────── SS_v2 (isEgo 있는 시나리오) ──────────────┐
   │                                                        │
   │   EgoEntity ◀──────── concealer ◀── control_cmd ◀──┐   │   ★ concealer 가 좌석을
   │      │                    │                        │   │      통째로 가져갔다
   │      │                    └──▶ /vehicle/status/*   │   │
   │      │                    └──▶ /localization/*  ───┼───┼──▶ ┌──────────┐
   │      │                                             └───┼────┤ Autoware │
   │      │                                                 │    └────┬─────┘
   │      ▼                                                 │         │
   │  /entity/status ──▶ ① ──▶ sils/ego/state ──┐           │         │
   └────────────────────────────────────────────┼───────────┼─────────┘
                                                ▼           │
                                       ┌─── vECU 5기 ───┐   │
                                       │  (관측자)      │◀──┴── ② ◀── control_cmd
                                       └───────┬────────┘
                                               ▼
                                        sils/ego/cmd  (플랜트에 안 닿는다)

   ※ 초기화 서비스 공백은 우리 shim 이 메운다:
      concealer ──▶ /api/localization/initialize ──ADAPI──▶ /localization/initialize
                                                              └─ localization_initialize_shim
                                                                 └▶ /initialpose3d ▶ 벤더 shim
```

| 좌석 주인 | **`concealer`** |
|---|---|
| 켜는 것 | ① ② + `localization_initialize_shim.py` + `deprecated_api.launch.xml` |
| **끄는 것** | ③ ④ — **concealer 와 같은 토픽에 쓴다** · ⑤ — concealer 가 이미 명령을 받는다 |
| 도달한 것 | mode 2 · 시나리오 **Passed** · 속도 곡선(가속·순항·감속·정지·재출발) |
| **MMF 공존** | ✅ 확인 — VSS 60경로, TypeObject 에러 0 ([evidence](evidence/c2-with-mmf-2026-08-12.md)) |
| ⚠ **런마다 리셋** | **파티션 3종을 매번 재기동해야 한다.** Autoware 상태가 안 씻겨 다음 concealer 가 `WAITING_FOR_ENGAGE` 에서 죽는다. control 만으론 부족 |
| vECU 위치 | **병렬 관측자.** 살아 있는 Autoware 판단을 받아 로직을 돌고 아비트레이션까지 했다 |

> ★ **이 구성에서 ③④⑤ 가 "불필요" 해진 것이 아니다.** concealer 가 그 자리를 도로
> 가져갔을 뿐이다. 근거: 시나리오 컨테이너가 종료되자 `/vehicle/status/velocity_status`
> 의 **발행자가 0** 이 됐다 — 그 자리를 아무도 안 채우면 Autoware 는 다시 눈이 먼다.

**이 구성이 포기하는 것 (정직하게)** — 좌석을 내주면 좌석에 딸린 것도 같이 간다:

| 잃는 것 | 누구 몫이 되나 |
|---|---|
| 스탬프 앵커 정책 ([`aw_stamp_policy.hpp`](../../adapters/ss2/include/aw_stamp_policy.hpp)) | concealer |
| `/initialpose3d` 자동화 (④ 안에 구현) | 우리 shim 이 대신 낸다 |
| `map→base_link` TF 소유 | concealer |
| **vECU 를 좌석에 앉히는 길** | 없어진다 — vECU 가 제어 경로 밖이다 |

---

### C3 · MORAI 본선 (구성 A) — 좌석이 비어 있다

> 목적: **제품 구성.** 벤더 대기 중.

```
   ┌── MORAI ──┐                                                        
   │           │                                                        
   │  EgoVehicleStatus ──▶ MORAI 어댑터 ❌ ──▶ sils/ego/state ──┬──▶ ③ ──▶ /vehicle/status/*
   │           ▲              (deg→rad, km/h→m/s)              └──▶ ④ ──▶ /localization/*
   │           │                                                          │
   │       CtrlCmd ◀── MORAI 어댑터 ❌ ◀── sils/ego/cmd ◀── vECU ◀── ② ◀──┘
   └───────────┘                                            (좌석 주인)
```

| 좌석 주인 | **우리 브리지 + vECU** — MORAI 는 concealer 같은 것을 안 데려온다 |
|---|---|
| 켜는 것 | ② ③ ④ + **MORAI 어댑터** |
| 끄는 것 | ① ⑤ ⑥ (SS_v2 전용) |
| 막힌 것 | **MORAI 어댑터 미구현** (설계 문서만) · Sync Mode 가 ROS 2 메시지 집합에 없다 |

> **이것이 MORAI 가 본선인 구조적 이유다.** 편의가 아니라 **좌석이 비어 있는 유일한
> 플랜트**라서다. SS_v2 는 concealer 를, AWSIM 은 자기 인터페이스를 데려온다.

---

## 3. 매트릭스 — 한 장으로 ★

| | ① `ss2_ego` | ② `aw_control_target` | ③ `aw_vehicle_status` | ④ `aw_localization` | ⑤ `ss2_control` | ⑥ `aw_ego_state` |
|---|:---:|:---:|:---:|:---:|:---:|:---:|
| **C0** 픽스처 | — | ✅ | ✅ | ✅ | — | — |
| **C1** SS_v2 (isEgo 없음) | ✅ | ✅ | ✅ | ✅ | ⛔ 소비자 없음 | — |
| **C2** SS_v2 EgoEntity (오늘) | ✅ | ✅ | **⛔ 끈다** | **⛔ 끈다** | **⛔ 끈다** | — |
| **C3** MORAI 본선 | — | ✅ | ✅ | ✅ | — | — |
| 개발용 (플랜트 없음) | — | ✅ | — | — | — | 🟡 |

⛔ = **켜면 조용히 깨진다** (라이터 둘). — = 그 구성에 해당 없음.

**세로로 읽으면 보인다:**

- **② 는 모든 열에 ✅** — Autoware 판단을 vECU 에게 주는 길은 하나뿐이다
- **③④ 는 C2 에서만 ⛔** — 좌석 주인이 따로 있는 유일한 구성이라서
- **① 은 SS_v2 구성에만** — MORAI 에서는 MORAI 어댑터가 그 역할

---

## 4. 어느 구성을 골라야 하나

```
   무엇을 증명하려는가?
        │
        ├─ 배선·QoS·구독 매칭만  ─────────────────────▶  C0  (30초, 플랜트 불필요)
        │
        ├─ 우리 브리지가 좌석을 채울 수 있나  ─────────▶  C1
        │       (= MORAI 리허설. 좌석 소유가 핵심)
        │
        ├─ Autoware 가 실제로 차를 모나 / 시나리오 판정 ─▶  C2
        │       (= 좌석을 내준다. vECU 는 관측자)
        │
        └─ 제품                ─────────────────────▶  C3  (MORAI 어댑터 대기)
```

> **C1 과 C2 는 서로를 대체하지 않고, SS_v2 에서는 합쳐지지도 않는다.** 사다리의
> 위아래가 아니라 **다른 축**이다 — C1 은 "우리가 좌석을 소유한다", C2 는 "Autoware 가
> 실제로 주행한다". 둘을 한 런으로 착각하면 "Autoware 가 우리 브리지로 SS_v2 를 몰았다"
> 는 **틀린 요약**이 나온다. 합칠 수 없는 이유는 취향이 아니라 `isEgo` 분기다(위 C1 주의).
> **MORAI 가 오면 그때 비로소 합쳐진다** — concealer 가 없으니 좌석을 쥔 채 명령을 넣는다.

---

## 5. vECU 쪽 배선 — 다중 ECU 와 VCU

어댑터가 버스에 올려놓은 것을 실제로 소비하는 쪽이다.

```
      sils/ego/state (EgoState)          autoware/control_target (ControlTarget)
             │  ①또는 MORAI 어댑터                    │  ②
      ┌──────┼────────┬──────────┬──────────┐        │
      ▼      ▼        ▼          ▼          ▼        ▼
   steer  adaptive  classic   plainc    ┌─────────────────────┐
      │      │        │          │      │  VCU                │
      │      │        │          │      │ (vehicle-interface) │
      │      ▼        ▼          ▼      │                     │
      │  cmd_adaptive cmd_classic cmd_plainc ──▶ (요청 3개) ──▶│
      │                                  └──────────┬──────────┘
      ▼                                             ▼
  sils/ego/steer_cmd                          sils/ego/cmd  ──▶ ⑤ 또는 MORAI 어댑터
```

| 채널 | 타입 | 라이터 |
|---|---|---|
| `sils/ego/state` | `EgoState` | 플랜트 어댑터 **하나만** |
| `autoware/control_target` | `ControlTarget` | ② 하나만 |
| `sils/ego/cmd` | `VehicleControl` | 단일 ECU 구성이면 그 ECU · **다중이면 VCU 만** |
| `sils/ego/steer_cmd` | `VehicleControl` | `steer-min` 채널맵 ECU |
| `sils/ego/cmd_{adaptive,classic,plainc}` | `VehicleControl` | 각 ECU (`config/sils/channelmap/multi/`) |

> ★ **예제 ECU 넷은 전부 `sils/ego/cmd` 에 쓰도록 돼 있다** — 서로의 *대안*이지 동시
> 구성이 아니기 때문이다. 같이 올리면 라이터가 넷이 되고 승자는 발행 순서가 정한다.
> `config/sils/channelmap/multi/` 의 사본이 out 토픽만 갈라 놓은 것이고 **코드는 안 바뀐다.**
> 통합 규칙: [`sils-vecu-arbitration.md`](sils-vecu-arbitration.md) ·
> 실측: [`evidence/multi-vecu-bus-2026-08-12.md`](evidence/multi-vecu-bus-2026-08-12.md)

실행:

```bash
tools/sils/ss2/run_multi_vecu.sh --duration 60     # 호스트 프로세스 5기
docker/sils/multi-vecu-entrypoint.sh               # 컨테이너 하나에 5기
```

---

## 6. 조용히 실패하는 것들 — 전부 실제로 밟았다 ★

**공통점: 아무도 에러를 안 낸다.** 그래서 목록으로 남긴다.

| # | 함정 | 증상 | 방지 |
|---|---|---|---|
| 1 | **좌석에 둘** (③④ 를 concealer/sps 와 같이) | Autoware 가 두 차량을 번갈아 본다 | 위 매트릭스. `ros2 topic info -v` 로 발행자 수 확인 |
| 2 | **`sils/ego/cmd` 에 라이터 넷** | 승자를 발행 순서가 정한다 | `channelmap/multi/` 사본 |
| 3 | **durability RxO 불일치** | TRANSIENT_LOCAL 리더 ↔ VOLATILE 라이터 = **매칭 자체가 안 된다** | `SILS_DURABILITY` 를 양쪽 동일하게 |
| 4 | **도메인 불일치** | 증상이 3번과 똑같다 — 조용히 아무 일도 안 일어난다 | 단일 도메인 42. **도메인을 가르지 않는다** |
| 5 | **`ss2_topic` 네임스페이스** | `mock_test` → `/entity/status`, `scenario_test_runner` → `/simulation/entity/status` | 런처에 맞춰 `-p ss2_topic:=` |
| 6 | **`/initialpose` ≠ `/initialpose3d`** | 초기화가 영영 안 걸린다 | `3d` 쪽이다 |
| 7 | **`/initialpose3d` 가 VOLATILE** | 늦게 뜬 구독자는 과거 샘플을 못 받는다 | ④ 는 1 Hz 반복 · shim 은 3회 반복 |
| 8 | **`ros2 param set` 이 안 먹는 파라미터** | `check_footprint_inside_lanes` — 생성 시점 캐시 | 목표를 10~30 m 앞으로 |
| 9 | **어댑터 이미지에 옛 토픽 기본값** | `matched=0` | `docker/sils/build-ss2-adapter.sh` 재빌드 |
| 10 | **`unzip` 없는 vECU 이미지** | `Failed to unzip FMU: status = 32512` | `oss-mmf-sils-vecu` 에 `unzip` 추가 (미반영) |
| 11 | **파티션 3종은 ROS 그래프 하나** | 한 이미지에 패키지가 없다고 "부재" 로 결론내면 틀린다 | `ros2 topic info -v` 로 그래프에서 확인 |

---

## 7. 아직 없는 것

| 항목 | 상태 |
|---|---|
| **MORAI 어댑터** | ❌ 미구현 — C3 의 유일한 블로커 |
| `/localization/initialize` 서비스 서버 | 벤더 요청 발송 대기 ([문서](sils-autoware-partition-vendor-requests.md)). 우리 shim 은 **임시방편** |
| vECU 를 C2 의 제어 경로에 삽입 | ⏸ **보류 (2026-08-12 결정)** — 아래 |
| 아비트레이션 불변식 I1~I4 → `requirements.yaml` | 단위 테스트 + 주행 실측은 통과, 게이트 미등록 |
| ⑥ `aw_ego_state_bridge` 철거 | C1/C3 가 서면 존재 이유가 사라진다 |

### ⏸ vECU 를 구동 경로에 넣는 remap — **MORAI 답변까지 보류** (2026-08-12)

세 조각 중 둘은 이미 섰다:

| 조각 | 상태 |
|---|---|
| vECU 가 명령을 만든다 | ✅ `arbitrated` · `control_published` |
| `sils/ego/cmd` → ⑤ → concealer → 차량모델 | ✅ **B2-3 완료 (08-10)** — 폐루프에서 ego **10.00 m/s**, vECU setpoint 와 일치 |
| **진짜 Autoware 가 도는 중에** | ⏸ 보류 |

**막는 것은 구조가 아니라 이름 충돌이다.** 3계단에서는 `vehicle_cmd_gate` 가 이미
`/control/command/control_cmd` 를 쓰고 있어서, ⑤ 를 켜면 라이터가 둘이 되고 concealer 가
누구 말을 듣는지는 발행 순서가 정한다. (C1 의 `isEgo` 문제 — 차량 모델이 아예 없는 것 —
과는 성질이 다르다.)

풀 방법은 remap 하나이고, **그 자리는 우리 것이다**
([`tools/sils/ss2/sils_adsw_control_run.launch.xml`](../../tools/sils/ss2/sils_adsw_control_run.launch.xml)):

```
vehicle_cmd_gate ─▶ /control/command/control_cmd_raw ─▶ ② ─▶ vECU ─▶ ⑤ ─▶ /control/command/control_cmd ─▶ concealer
```

**그런데 안 넣기로 했다.** 이유:

- **MORAI 에는 이 문제가 없다.** concealer 가 없어 좌석이 비어 있고, vECU 가 **remap 없이**
  구동 경로에 들어간다. SS_v2 전용 우회를 먼저 만들면 본선에서 안 쓰는 코드가 된다.
- MORAI 벤더 답변(특히 Q1 Sync Mode)이 **결정론 등급을 가른다**. 절단면 설계가 그 답에
  달려 있는데, 답이 오기 전에 배선을 굳히면 다시 뜯는다
  ([`sils-morai-vendor-questions.md`](sils-morai-vendor-questions.md) §9).

**하기로 하면 같이 정해야 할 것 둘:**

| | |
|---|---|
| `mrm_emergency_stop_operator` | 같은 토픽에 쓴다. **비상정지를 vECU 뒤로 보내면 안전 경로를 우리가 가로챈다** — remap 대상에서 빼야 한다 |
| 절단면 정의 | `vehicle_cmd_gate` **하류**를 자르는 것이라 gate 의 필터링(속도 제한·모드 게이팅)은 이미 지나온 뒤다. [`sils-vehicle-interface-autoware.md`](sils-vehicle-interface-autoware.md) 에 명시 필요 |

---

*관련: [`sils-docker-runbook.md`](sils-docker-runbook.md) (컨테이너별 전체 실행 명령) ·
[`sils-integration-topology.md`](sils-integration-topology.md) (배선 원리) ·
[`sils-vecu-arbitration.md`](sils-vecu-arbitration.md) (다중 vECU 통합 규칙) ·
[`sils-simulator-loops.md`](sils-simulator-loops.md) (사다리·백로그) ·
[`evidence/`](evidence/) (원측정값)*
