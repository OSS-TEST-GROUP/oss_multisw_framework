# 증거 — Autoware 가 우리 데이터로 자율주행 명령을 내고, 그 명령이 vECU 까지 돌아왔다

> 측정 2026-08-11 · **도메인 42 단일** (파티션 Cyclone / SiLS Fast DDS) · headless
> **§1~§5 의 플랜트는 `sils_ego_source`(시험용 스텁)이고, §6 부터는 SS_v2 다.**
> 둘은 플랜트 자리를 교체하는 별개의 것이며 **한 런에서 같이 성립한 적이 없다** — §6 표.
> 용어: [`sils-integration-topology.md` §1.1](../sils-integration-topology.md)
> 선행: [`ss2-clock-survey-2026-08-11.md`](ss2-clock-survey-2026-08-11.md) ·
> [`vehicle-seat-no-simulator-2026-08-11.md`](vehicle-seat-no-simulator-2026-08-11.md)

어제 여기까지였다 — 경로는 세웠지만 **자율주행 전환이 거부**됐고, 원인은
`velocity_status`/`steering_status`/`map_to_base_link` 의 **타임스탬프 timeout** 이었다
(vehicle-seat 문서 §5.5 발견 4). 오늘 그 셋을 고치고 다시 올라갔다.

---

## 1. 결과

| 단계 | 관측 |
|---|---|
| 경로 수립 | `/api/routing/state` = **2 (SET)**, `getMainLanelets: [ids: 103]` |
| 계획 | `/planning/scenario_planning/trajectory` **10.0 Hz** |
| 제어 명령 생성 | `/control/command/control_cmd` **33.3 Hz** |
| 자율주행 가용 | `/system/operation_mode/availability` → **autonomous: true** |
| 자율주행 전환 | `change_to_autonomous` → success, `/api/operation_mode/state` **mode: 2** |
| 명령이 vECU 까지 | `sils/ego/cmd` 에 유효 봉투, 독립 검증기 **verify_pass** |
| 파티션 건강 | 3종 생존, `invalid type object` **0** |

**명령의 내용이 모드에 따라 뒤집히는 것**이 이 시험의 핵심 관측이다 — 배선이 아니라
Autoware 의 판단이 흐르고 있다는 뜻이기 때문이다:

```
STOP 모드      target_received  steering 0.00054592  acceleration -1.5      gear 3
AUTONOMOUS     target_received  steering 0.00055238  acceleration +0.836088 gear 3
```

`-1.5` 는 정지 명령, `+0.836` 은 가속 명령이다. 같은 배선에서 값만 바뀌었다.

---

## 2. 흐른 경로

```
sils_ego_source ──EgoState(정준)──▶ sils/ego/state
                                        │
              ┌─────────────────────────┴─────────────────────────┐
              ▼                                                   ▼
   aw_vehicle_status_bridge                            aw_localization_bridge
   /vehicle/status/*  48.99 Hz                         /localization/*  48.99 Hz + TF
              └─────────────────────────┬─────────────────────────┘
                                        ▼
                              Autoware (파티션 3종, Cyclone)
                          routing SET · trajectory 10 Hz · AUTONOMOUS
                                        │
                          /control/command/control_cmd  33.3 Hz
                                        ▼
                          aw_control_target_bridge   received=495 published=495
                                        │
                          autoware/control_target (정준 ControlTarget)
                                        ▼
                          vehicle_interface (vECU)   state+target → control
                                        │
                              sils/ego/cmd (정준 VehicleControl)
                                        ▼
                                   ❌ 소비자 없음
```

마지막 화살표가 이 시험의 경계다. **루프는 아직 안 닫혔다** — 아래 §4.

---

## 3. 재현

```bash
# 1) 파티션 3종 (Cyclone, 도메인 42)
cd ~/oss/oss_adsw && ./partition/run_partitions.sh \
  --repo ghcr.io/oss-test-group/autoware-partition \
  --map-path "$HOME/autoware_map/sample-map-planning" --domain-id 42 --headless --delay 15

# 2) 제어 파티션만 시뮬레이터 끄고 교체  ★ 재기동할 때마다 다시 해야 한다
tools/sils/ss2/run_control_partition_sils.sh --domain-id 42 \
  --map-path "$HOME/autoware_map/sample-map-planning"

# 3) 상태 브리지 2개 (rclcpp=Cyclone@42, 네이티브=Fast DDS@42)
ROS_DOMAIN_ID=42 VISS_DDS_DOMAIN_ID=42 tools/sils/ss2/run_vehicle_seat_bridges.sh

# 4) 플랜트 — ★ --vmax 를 작게. 아래 "함정" 참조
VISS_DDS_DOMAIN_ID=42 SILS_DURABILITY=transient-local \
LD_LIBRARY_PATH="$PWD/output/x86_64/lib" \
  output/x86_64/Debug/extension/ros2/bin/sils_ego_source --rate-ms 20 --count 90000 \
    --x0 3638.69 --y0 73605.25 --yaw0 -1.4441 --vmax 0.2 &

# 5) 되먹임: Autoware 명령 -> 정준 ControlTarget
docker run -d --name sils-aw-target --net=host --ipc=host \
  -e ROS_DOMAIN_ID=42 -e RMW_IMPLEMENTATION=rmw_cyclonedds_cpp -e ROS_LOCALHOST_ONLY=0 \
  -e VISS_DDS_DOMAIN_ID=42 -e SILS_DURABILITY=transient-local -e SILS_RUN_ID=ss2-wire \
  sils-ss2-adapter:humble ros2 run ss2_ego_bridge aw_control_target_bridge

# 6) vECU — ego-min 이 아니라 vehicle-interface 채널맵이다 (ControlTarget 입력이 있는 쪽)
VISS_DDS_DOMAIN_ID=42 SILS_RUN_ID=ss2-wire SILS_DURABILITY=transient-local \
SILS_CHANNELMAP=service/sils/examples/vehicle-interface/channelmap/vehicle-interface.yaml \
LD_LIBRARY_PATH="$PWD/output/x86_64/lib" \
  output/x86_64/Debug/extension/ros2/bin/vehicle_interface > runs/vehicle-interface.jsonl &

# 7) 경로 + 자율주행 (컨테이너 안에서)
docker exec adsw-control-42 bash -lc 'source /opt/ros/humble/setup.bash &&
  source /opt/autoware/setup.bash &&
  ros2 topic pub --once /planning/mission_planning/goal geometry_msgs/msg/PoseStamped \
    "{header: {frame_id: map}, pose: {position: {x: 3651.808, y: 73502.310, z: 0.0},
      orientation: {z: -0.661018, w: 0.750370}}}"'
# ★ /initialpose3d 를 한 번 찍어야 autonomous 가 열린다 -- 아래 "함정"
#   → §10 이후로는 이 줄이 필요 없다. aw_localization_bridge 가 1 Hz 로 발행한다
#     (브리지 이미지 재빌드 필요). 이 시험 당시에는 수동이었다.
docker exec adsw-control-42 bash -lc '... ros2 topic pub --once /initialpose3d ...'
docker exec adsw-control-42 bash -lc '... ros2 service call \
  /api/operation_mode/change_to_autonomous autoware_adapi_v1_msgs/srv/ChangeOperationMode "{}"'

# 감시
tools/sils/ss2/watch_vehicle_seat.sh --interval 10
```

### 함정 셋 (전부 오늘 밟았다)

**① 픽스처가 빠르면 경로를 못 세운다.** `--vmax 8.0` 이면 개루프 픽스처가 **직진으로**
맵을 벗어난다. 20초면 목표점을 지나고, 미션 플래너는

```
Failed to plan route.
/planning/mission_planning/route_selector/main/set_waypoint_route:
  status code 3 'The planned route is empty.'
```

를 낸다. 원인이 "브리지가 이상하다" 로 읽히지만 **차가 차선 밖에 있을 뿐**이다.
`--vmax 0.2` 로 차선 위에 머물게 하면 즉시 `[ids: 103]` 이 나온다.

**② `/initialpose3d` 를 아무도 안 낸다.** `launch_localization=false` 라 pose initializer
가 없다. 다른 모든 진단이 통과해도 이 하나 때문에 `autonomous: false` 다. 한 번 찍으면
열린다. **수동 단계로 남아 있다** — §4.
> ⚠ **원인 설명이 틀렸다 — §10 참조.** 관측(찍으면 열린다)은 맞지만, 그 이유는
> "pose initializer 가 없어서" 가 아니라 **벤더 shim 이 이 토픽을 기다리기 때문**이다.
> 포즈 값은 쓰이지 않는다(불리언 래치). 자동화 방식이 달라지는 차이다.

**③ 파티션을 다시 띄우면 시뮬레이터가 되살아난다.** `run_partitions.sh` 는 stock 제어
파티션을 올리므로 `run_control_partition_sils.sh` 를 **매번 다시** 돌려야 한다. 안 하면
`/vehicle/status/*` 발행자가 둘(우리 브리지 + `simple_planning_simulator`)이 되고,
Autoware 가 두 소스를 섞어 받는데 **아무 오류도 안 난다.**
(`run_vehicle_seat_bridges.sh` 는 이 경우를 거부한다 — `docker run` 으로 직접 띄우면
그 가드를 우회하게 된다.)

---

## 4. 이 시험이 증명하지 않는 것

- **폐루프가 아니다.** `sils/ego/cmd` 를 소비하는 플랜트가 없다. `sils_ego_source` 는
  개루프 픽스처라 명령을 무시하고 자기 궤적을 간다. 차는 Autoware 가 시킨 대로
  움직이지 않았고, 움직였는지 여부는 이 시험의 관측 대상이 아니다.
- **주행 증명이 아니다.** 위 이유로 "Autoware 가 SS_v2 데이터로 주행한다" 는 아직이다.
  남은 것은 플랜트 하나다 — SS_v2 를 Cyclone/42 로 올리고 `sils/ego/cmd` 를
  `ss2_control_bridge` 로 되먹이면 루프가 닫힌다.
- **`/initialpose3d` 는 수동이다.** 자동화하려면 `aw_localization_bridge` 가 같은 샘플에서
  내보내는 것이 자연스럽다 — pose 를 아는 노드가 하나여야 TF 때와 같은 이유로 어긋나지
  않는다(vehicle-seat 문서 발견 3).
  → **§10 에서 그렇게 했다.** 다만 이유가 예상과 달랐고(불리언 래치), 그래서 "한 번" 이
  아니라 **주기 발행**이다. **rig 실측 완료** — 수동 단계 0 으로 AUTONOMOUS 까지.
- **`bad_gear=1`** 이 `aw_control_target_bridge` 에 한 번 찍혔다. 첫 샘플의 기어가 매핑
  밖이었다는 뜻이고, 이후 0 이다. 원인 미확인.

---

## 6. SS_v2 를 플랜트로 붙여봤다 — 절반 성공, 그리고 B1-2 갈림길에 정면으로 닿았다

같은 날 이어서, 픽스처를 SS_v2 로 갈아끼웠다.

### 새로 확인된 것 (둘 다 그동안 미검증이었다)

| | 결과 |
|---|---|
| **SS_v2 가 Cyclone 에서 도는가** | ✅ 도메인 45 격리 시험. `/simulation/entity/status` **19.99 Hz**, `/clock` 정상, 크래시 없음 |
| **파티션이 kashiwanoha 로 뜨는가** | ✅ lanelet id 34378… 확인. `Map is loaded`, 오류 없음. (override 파일의 "투영 원점이 다르면 걸릴 수 있다" 우려 해소) |

### SS_v2 → 정준 버스 → Autoware 는 흐른다

```
ss2_ego_bridge          received=400 published=400 skipped_no_ego=0 skipped_no_time=0
aw_vehicle_status_bridge stamp anchor ok: max deviation 5.1 ms, plant restarts ridden through: 3
파티션 3종                생존, invalid type object 0
```

`plant restarts ridden through: 3` 이 §5 의 런 경계 규칙이 실제로 일한 흔적이다 — 픽스처
두 번 + SS_v2 로 교체 한 번, 앵커가 매번 다시 걸렸고 편차는 5.1 ms 로 남았다.

### ★ 막힌 지점 — concealer 가 우리 vECU 의 자리에 앉는다

`isEgo: true` 시나리오(`RoutingAction.AcquirePositionAction`)를 쓰면:

```
$ ros2 topic info -v /vehicle/status/velocity_status
Publisher count: 2
  concealer                  /simulation      ← SS_v2 의 EgoEntity 가 데려온다
  aw_vehicle_status_bridge   /                ← 우리 것
```

**concealer 는 SS_v2 쪽 vehicle interface 다** — Autoware 에 차량 상태를 넣고
`/control/command/control_cmd` 를 받아 시뮬레이터에 적용한다. 그건 정확히 우리 vECU 가
앉기로 한 자리다. 게다가 §5b 대로 `EntityStatus.time` 은 Autoware 가 ego 를 engage 할
때까지 NaN 이고, 그 engage 를 중개하는 것도 concealer 다.

`isEgo` 를 뺀 시나리오(`LateralAction.LaneChangeAction`)로 바꾸면 절반이 풀린다:

| | isEgo 있음 | isEgo 없음 |
|---|---|---|
| concealer | **있다** (발행자 2개 충돌) | **없다** (발행자 1개, 우리 것) |
| 시나리오 시계 | engage 전까지 NaN | **즉시 시작** (실측: `skipped_no_time` 이 멈춤) |
| 엔티티 타입 | EGO(0) | VEHICLE(1) → `SS2_EGO_NAME=ego` 로 이름 선택 필요 |
| **제어 명령을 누가 차에 적용하나** | concealer → simple_sensor_simulator 의 vehicle model | **아무도 안 한다** |

마지막 줄이 벽이다. [`ss2_control_bridge`](../../../adapters/ss2/src/ss2_control_bridge.cpp)
는 정준 VehicleControl 을 **`/control/command/control_cmd` 로** 내보내고 그 소비자는
`concealer::AutowareUniverse` 다. 실제 Autoware 가 이미 그 토픽에 발행하고 있으므로
2계단처럼 쓸 수 없고(발행자 둘), concealer 를 없애면 소비자가 없다.

### ★★ 정정 — "concealer 를 그대로 쓴다" 는 선택지가 **존재하지 않는다**

위를 쓴 뒤 벤더 구성만 깨끗하게 올려 다시 확인했다(우리 브리지·vECU 전부 내리고,
파티션 재기동 → 시뮬레이터 off → external API → 시나리오 순서로). 결과:

```
AutowareError: Requested the service "/api/localization/initialize" 30 times,
               but was not successful.
```

그리고 그 이유가 우리 구성 자체다:

```
/api/localization/initialize 존재:  1     ← ADAPI 가 서비스를 광고한다
pose_initializer 노드:              0     ← 그 일을 실제로 하는 노드가 없다
/localization/* 노드 수:            0
```

**concealer 는 Autoware 의 localization 스택을 통해 자차 초기 위치를 넣는다.** 그런데
SiLS 구성은 `launch_localization=false` 다 — `aw_localization_bridge` 가 위치를 대신
공급하기 때문이다(vehicle-seat 문서 발견 3). 즉

> **concealer 의 초기화 경로와 "우리가 위치를 공급한다" 는 구성은 상호 배타적이다.**

토픽이 겹치는 문제가 아니라(그것도 있지만), **concealer 가 완료하려면 우리가 치운 바로
그 스택이 필요하다.** 그래서 "concealer 는 두고 우리 vECU 만 관측자로 붙인다" 는 절충은
`launch_localization=false` 상태에서는 성립하지 않는다.

concealer 가 도달한 지점은 매번 달라졌고, 그 순서가 곧 요구사항 목록이다:

| 시도 | 막힌 곳 | 원인 |
|---|---|---|
| Autoware 없이 | `/api/operation_mode/change_to_stop` | Autoware 자체가 없음 |
| Autoware 있음 | `/api/autoware/set/velocity_limit` | tier4 external API 미기동 (`launch_deprecated_api=false`) |
| external API 붙임 | `/api/localization/initialize` | **localization 스택 없음** (`launch_localization=false`) |

앞의 둘은 우리가 고칠 수 있었다. 셋째는 우리 설계와 정면으로 충돌한다.

### 그래서 이건 배선 문제가 아니라 B1-2 결정이다

> **SS_v2 의 EgoEntity 를 쓰면 concealer 가 vehicle interface 를 가져간다.
> 우리 vECU 를 그 자리에 앉히려면 concealer 를 우회해야 한다.**

선택지 (둘 다 미착수):

0. ~~벤더 구성 그대로 가고 vECU 는 관측자로 붙인다~~ — **확인 결과 불가.** 아래 §7.

1. **concealer 우회** — SS_v2 의 EgoEntity 가 concealer 대신 우리 경로를 쓰게 한다.
   상류 patch 또는 concealer 가 기대하는 인터페이스를 우리가 흉내낸다
   (`adapters/autoware-stub` 이 그 방향의 선행 작업이다).
   [`ss2-concealer-survey-2026-08-07.md`](ss2-concealer-survey-2026-08-07.md) 참조.
2. **VehicleEntity + 외부 구동** — `isEgo` 없이 가고, 제어 명령을 SS_v2 에 적용할 경로를
   따로 만든다. `simple_sensor_simulator` 의 vehicle model 은 Ego 경로에 묶여 있으므로
   그대로는 안 되고, W3/B2-2(프레임 구동부 patch)와 같은 층의 작업이 된다.

**결정 전까지 폐루프는 닫히지 않는다.** 다른 모든 조각은 오늘 확인됐다.

---

## 5. 다음

| 할 일 | 왜 |
|---|---|
| ~~SS_v2 를 Cyclone/도메인 42 로~~ | ✅ §6. 상태 경로까지 흐른다 |
| ~~**B1-2 결정 — concealer 우회 방식**~~ | ✅ §7 에서 닫혔다. 우회는 선택이 아니라 전제였고 이미 그렇게 돼 있다 |
| ~~`/initialpose3d` 를 `aw_localization_bridge` 로~~ | ✅ **§10. rig 실측 완료** — 수동 단계 0 으로 autonomous 전환 |
| **벤더 요청 — `/localization/initialize` 서비스** | ▶ SS_v2 폐루프의 유일한 블로커. [`sils-autoware-partition-vendor-requests.md`](../sils-autoware-partition-vendor-requests.md) |
| TypeObject 근본 수정 | 지금은 전파를 껐을 뿐이다. `@final` 미시험 |
| AWSIM 이 `vehicle_interface` 를 점유하는지 | 플랜트 후보 판정. 미착수 |
| ~~SS_v2 플랜트로 자율주행까지~~ | ✅ **§11.** 경로 SET·AUTONOMOUS·vECU 수신. 단 차는 안 움직인다(§8) |

---

## 7. 최종 확인 — concealer 는 **이 파티션에서 원리적으로 못 돈다** (우리 탓이 아니다)

§6 이 남긴 마지막 의심은 "우리가 localization 을 끈 탓이니, 상류 경로
(`scenario_simulation:=true`)로 가면 켜진 채로 될 것" 이었다. **아니었다.**

벤더 이미지의 `obigo_launch/launch/planning_simulator.launch.xml`, 75행:

```xml
<group scoped="false" if="$(var is_launch_autoware)">
  <include file="$(find-pkg-share obigo_launch)/launch/$(var launch_file)">
    ...
    <arg name="launch_sensing"      value="false"/>
    <arg name="launch_localization" value="false"/>   ← value=, 덮어쓸 수 없다
    <arg name="launch_perception"   value="false"/>
```

`value=` 이므로 명령줄로 못 바꾼다. `scenario_simulation` 과도 **무관**하다 — 그 변수는
`launch_dummy_vehicle`(101행), `launch_scenario_simulator_v2_adapter`(103행),
`use_sim_time`(49행)만 건드린다.

즉 **이 파티션의 Autoware 는 설계상 localization 이 없다.** sensing 과 perception 도 없다.
planning_simulation 전용 구성이고, 그래서 **자차 위치는 vehicle_interface 자리에 앉은
누군가가 공급해야 한다.**

| 그 자리에 앉는 것 | 위치를 어떻게 주나 | 이 파티션에서 |
|---|---|---|
| 벤더 기본 `simple_planning_simulator` | 자기가 pose + status 를 직접 발행 | ✅ 된다 |
| **우리 브리지 2개** | `aw_localization_bridge` 가 pose + TF 발행 | ✅ **오늘 증명** (§1) |
| **SS_v2 concealer** | `/api/localization/initialize` 로 **localization 스택을 초기화** | ❌ 그 스택이 없다 |

**concealer 는 pose 를 주는 방식이 아니라 localization 을 초기화하는 방식이다.** 이 파티션에는
초기화할 대상이 없으므로, 우리가 무엇을 하든 concealer 는 완료하지 못한다.

### 그래서 B1-2 갈림길은 이렇게 닫힌다

"concealer 를 그대로 쓴다" 는 선택지는 **없다.** 남는 것은 둘뿐이고, 하나는 이미 우리 것이다:

| | 상태 |
|---|---|
| **우리 브리지가 vehicle_interface 를 맡는다** | ✅ **이미 동작한다.** 오늘 Autoware 가 이 위에서 자율주행 명령을 냈다 |
| SS_v2 를 그 구성의 **플랜트**로 쓴다 | ⬜ `sils/ego/cmd` 를 SS_v2 차량 모델에 적용할 길이 필요 (W3/B2-2 층) |
| MORAI 를 플랜트로 쓴다 | ⬜ 벤더 대기. concealer 가 없으므로 이 문제 자체가 없다 |

**결론: concealer 우회는 "선택" 이 아니라 이 파티션의 전제였고, 우리는 이미 그렇게 만들어
놓았다.** 남은 일은 concealer 와 싸우는 것이 아니라 **플랜트 쪽 마지막 화살표**다.

---

## 8. "플랜트 쪽 화살표 하나" 는 틀린 표현이었다 — 그릴 화살표가 없다

§7 을 "이제 `sils/ego/cmd` 를 SS_v2 에 꽂으면 된다" 로 요약했는데, 그 화살표가 **어디에
꽂히는지** 확인하지 않고 쓴 말이었다. 확인했다.

### 명령이 실제로 차를 움직이는 지점

`simple_sensor_simulator/src/vehicle_simulation/ego_entity_simulation.cpp:321`

```cpp
autoware->getVehicleCommand();          // ← 차량 모델의 입력이 여기서 온다
...
input(0) = gear_sign * acceleration + acceleration_by_slope;
input(1) = tire_angle;
vehicle_model_ptr_->setInput(input);
```

그 `autoware` 는 `concealer::AutowareUniverse` 이고, **토픽 레벨 인터페이스일 뿐**이다
(`external/concealer/src/autoware_universe.cpp:28`):

```cpp
getCommand("/control/command/control_cmd", rclcpp::QoS(1), *this),
getGearCommand("/control/command/gear_cmd", rclcpp::QoS(1), *this),
```

AD API 호출은 여기 없다. 그건 traffic_simulator 쪽 EgoEntity 가 만드는 **다른** concealer 의
일이다. 즉 concealer 는 두 개이고 역할이 다르다:

| | 어디 | 하는 일 |
|---|---|---|
| `AutowareUniverse` | `simple_sensor_simulator` 안 | `control_cmd`/`gear_cmd` **구독** → 차량 모델 구동, localization 발행 |
| EgoEntity 쪽 concealer | `traffic_simulator` 안 | Autoware 기동 + **AD API** (localization/initialize, routing, engage) |

### 그런데 둘은 분리할 수 없다

```cpp
// simple_sensor_simulator.cpp:145
assert(ego_entity_simulation_ && "Ego is spawned but ego_entity_simulation_ is nullptr!");
```

**차량 모델(`ego_entity_simulation_`)은 Ego 가 spawn 될 때만 생성된다.** Ego 를 spawn 하면
traffic_simulator 쪽 concealer 가 따라오고, 그것이 `/api/localization/initialize` 를 부른다.

그리고 이 파티션 이미지에는:

```
pose_initializer 패키지: 없음
```

**설치조차 안 돼 있다.** `deprecated_api` 때처럼 "안 띄운 것을 띄우는" 우회가 불가능하다.

### 결론 — 그릴 화살표가 없다

```
EgoEntity 없음  →  차량 모델 없음  →  적용할 명령이 없음  →  꽂을 곳이 없다
EgoEntity 있음  →  AD API concealer  →  pose_initializer 필요  →  이미지에 없다
```

**이 파티션 이미지에서는 어느 쪽으로도 SS_v2 의 차가 명령대로 움직일 수 없다.** 우리 배선
문제가 아니고, 우리가 고칠 수 있는 것도 아니다.

### 그래서 플랜트 쪽 선택지 (전부 우리 밖의 일)

| | 무엇이 필요한가 | 그 뒤에 남는 우리 일 |
|---|---|---|
| **A. localization 포함된 파티션 이미지** | 벤더에게 요청 (최소 `pose_initializer`) | 시뮬레이터의 `control_cmd` **구독을 우리 vECU 출력으로 remap**. 안 하면 Autoware 가 SS_v2 를 직접 몰고 vECU 는 관측자로 남는다 |
| **B. SS_v2 상류 patch** | 차량 모델을 비-Ego 에도 열거나, 명령 토픽을 인자화 | W3/B2-2 와 같은 층 |
| **C. MORAI** | 벤더 대기 | 어댑터가 명령을 적용하는 게 설계라 이 문제가 없다 |

### 두 가지 유용한 부수 사실

- **`simulate_localization` 은 인자다.** `false` 면 `AutowareUniverse` 가 `/localization/*`
  대신 `/simulation/debug/localization/*` 로 발행한다(autoware_universe.cpp:35-44).
  즉 A 로 갔을 때 우리 `aw_localization_bridge` 와의 토픽 충돌은 **patch 없이 플래그로**
  해소된다.
- **A 에서도 "Autoware 가 SS_v2 데이터로 주행" 은 vECU 없이 성립한다.** Autoware →
  `control_cmd` → 시뮬레이터로 직결되고 상태는 우리 브리지가 준다. **vECU 를 제어 경로에
  넣는 것은 그 위에 얹는 별개의 한 걸음**(remap)이다. 두 목표를 섞지 말 것.

---

## 9. `initialize_localization` 로 건너뛸 수 있나 — **없다** (확인 완료)

런치 인자에 `initialize_localization := 10` 이 노출돼 있어 "concealer 의 localization
초기화를 끌 수 있으면 §8 의 A·B 가 다 필요 없다" 는 기대가 있었다. 아니다.

```cpp
// external/concealer/src/field_operator_application.cpp:158
requestInitialPose("/api/localization/initialize", *this,
                   std::chrono::seconds(common::getParameter<int>("initialize_localization")))
```

그 인자는 `concealer::Service` 의 **재시도 간격**이다. 불리언이 아니다.

```cpp
// external/concealer/include/concealer/service.hpp
auto operator()(request, attempts_count, availability_timeout = 180s)
{
  while (ok() and not client->service_is_ready())
    if (now > deadline)
      throw AutowareError("Service ", name, " is not ready after ", 180, " seconds");   // (1)

  for (attempt = 0; attempt < attempts_count; ++attempt, interval.sleep())
    if (... and receive(future.get())) return;

  throw AutowareError("Requested the service ", name, " ", attempts_count,
                      " times, but was not successful.");                              // (2)
}
```

우리가 본 오류 둘이 이 두 throw 지점에 정확히 대응한다:

| 관측한 오류 | throw | 뜻 |
|---|---|---|
| `Service "/api/autoware/set/velocity_limit" is not ready after 180 seconds` | (1) | 서비스가 아예 없었다 (external API 미기동) |
| `Requested the service "/api/localization/initialize" 30 times, but was not successful.` | (2) | 서비스는 있고 응답도 오는데 **`success=false`** |

(2) 의 판정은 `response->status.success` 다. 즉 ADAPI 는 살아서 응답하고, **그 뒤의 구현만
없다.** `pose_initializer` 부재라는 §8 의 진단과 정확히 일치한다.

`initialize_localization` 을 0 으로 줘도 더 빨리 실패할 뿐이고, 실패는 항상 throw 다.
**건너뛰기 경로는 존재하지 않는다.**

### 그래서 벤더 요청은 한 줄로 특정된다

> **제어 파티션 이미지에 `autoware_pose_initializer` 를 포함해 주십시오.**
> (`/api/localization/initialize` 가 `success=false` 를 돌려주는 원인. 패키지가 이미지에
> 설치돼 있지 않아 런치 인자로도 우회할 수 없습니다.)

그리고 그 요청의 목적을 오해하지 않도록 함께 적을 것: **Autoware 가 localization 이 필요해서가
아니다.** 우리 구성은 `aw_localization_bridge` 가 pose 와 TF 를 직접 발행해서 없이도
동작한다(§1 에서 경로 수립·자율주행 전환까지 확인). 필요한 유일한 이유는 **SS_v2 의
EgoEntity(→ 차량 모델)를 쓰려면 concealer 의 초기화가 통과해야** 한다는 것이다.

---

## 10. `/initialpose3d` 자동화 — 그리고 함정 ②의 **원인이 틀렸다**

§5 의 "수동 단계 제거" 를 하려고 열어봤더니, 먼저 정정할 것이 나왔다. 함정 ② 는 이렇게
썼다:

> **② `/initialpose3d` 를 아무도 안 낸다.** `launch_localization=false` 라 pose initializer
> 가 없다.

앞 문장은 맞고 **뒤 문장은 틀렸다.** 정확히는 *pose initializer 가 없어서* 가 아니라
**벤더가 그 자리에 넣어둔 shim 이 그 토픽을 기다리고 있어서**다. 이번에는 실행 중인 rig 를
관찰한 게 아니라 **이미지를 정적으로 읽어서** 확인했다 — 로컬에 이미지가 있으므로 파티션을
띄우지 않고도 되는 일이었다.

```bash
docker run --rm --entrypoint bash ghcr.io/oss-test-group/autoware-partition:adsw-control -lc '
  grep -rl initialpose3d /opt/autoware/lib /opt/autoware/share'
```

### 실제 소비자는 하나뿐이고, 포즈를 안 본다

`/opt/autoware/lib/obigo_launch/planning_sim_localization_state.py` (벤더 추가분):

```python
self.create_subscription(PoseWithCovarianceStamped, "/initialpose3d", self._on_initial_pose, 1)
...
def _on_initial_pose(self, _msg):                      # ← 내용을 안 쓴다
    self._state = LocalizationInitializationState.INITIALIZED
    self._publish()                                    # /localization/initialization_state
```

perception 파티션이 띄운다(`perception_simulator_component.launch.xml:88`,
`launch_localization_state_helper` 기본 true). ADAPI 가 그 상태를
`/api/localization/initialization_state` 로 중계하고, **그게 autonomous 게이트다.**

| 확인 | 결과 |
|---|---|
| `autoware_pose_initializer` | 세 이미지 **모두 없음** (§8 과 일치) |
| `/initialpose3d` 를 실제로 구독하는 것 | **넷** — 아래 표 |

> ⚠ **여기서 한 번 틀렸다.** 처음엔 control 이미지만 뒤져보고
> "`component_state_monitor` 도 없다 → 그 설정은 죽었다" 고 썼는데, **decision 이미지에
> 있었다.** 실행 중인 rig 에서 `ros2 topic info -v` 로 잡힌다. **이미지 하나에 없는 것이
> 없는 것이 아니다** — 파티션 셋이 한 ROS 그래프를 공유한다. 정적 분석의 한계를 그대로
> 밟았고, 실측이 고쳤다.

실측(도메인 42, 시뮬레이터 ON 상태에서 잡은 목록):

| 구독자 | 이미지 | 게이트 관여 |
|---|---|---|
| `planning_sim_localization_state` | perception | ✅ shim → `initialization_state` |
| `topic_state_monitor_initialpose3d` | **decision** | ✅ `warn/error_rate·timeout = 0.0` → "한 번이라도 오면 OK" |
| `planning_sim_empty_occupancy_grid` | perception | ❌ |
| `simple_planning_simulator` | control | ❌ (우리 구성에선 OFF) |

> **둘 다 값을 안 본다. 즉 `/initialpose3d` 는 포즈 입력이 아니라 불리언 래치다.**

### 그래서 설계가 바뀐다 — "한 번 찍기" 가 아니라 "계속 재무장"

shim 의 구독은 depth 1 · RELIABLE · **VOLATILE** 이고, 래치는 shim 프로세스 안에 있다.

- 한 번만 쏘면 **그 순간 떠 있던 shim 만** 받는다. 브리지가 먼저 뜨면 아무도 못 받는다.
- perception 파티션이 재기동하면 래치는 `UNINITIALIZED` 로 돌아간다.

그래서 **주기 발행**(기본 1 Hz)이다. 값은 shim 이 버리지만 **진짜 포즈를 보낸다** — 언젠가
pose initializer 가 들어오거나 시뮬레이터를 켠 채 돌리면 placeholder 는 그 순간
"틀린 초기위치" 가 되기 때문이다. 이미 아는 값을 보내는 데 드는 비용이 없다.

### 반영

[`aw_localization_bridge.cpp`](../../../adapters/ss2/src/aw_localization_bridge.cpp)

| 파라미터 | 기본 | 뜻 |
|---|---|---|
| `publish_initialpose` | `true` | 끄면 예전처럼 수동 |
| `initialpose_topic` | `/initialpose3d` | |
| `initialpose_period_ms` | `1000` | `0` 이면 첫 샘플에 한 번만 |

첫 샘플은 타이머를 기다리지 않고 즉시 나가고, **앵커가 latch 되기 전에는 아무것도 안 나간다**
(포즈를 모르는 상태에서 래치를 켜지 않기 위해). `Report()` 에 `initialpose=` 카운터 추가.

**플랜트와 무관하다.** 이 래치는 파티션 속성(`planning_simulation` +
`launch_localization=false`)이므로 **MORAI 로 갈아도 그대로 필요하다.** 필요 없는 유일한
구성은 `launch_localization=true` 인 실센서 Autoware 인데, 거기서는 이 노드 자체를 꺼야 한다.

⚠ **시뮬레이터를 켠 채로는 끄고 써야 한다.** `simple_planning_simulator` 는 이 토픽을
`input/initialpose` 로 실제 적용하므로 주기 발행이 매초 차를 순간이동시킨다.
(`run_vehicle_seat_bridges.sh` 가 그 조합을 원래 거부한다.)

### ★ rig 실측 — 수동 단계 0 으로 §1 이 재현된다 (2026-08-11 17:36~17:40, 도메인 42)

`/initialpose3d` 를 **사람이 한 번도 치지 않았다.** 그 줄만 빼고 §3 그대로.

| 단계 | 관측 |
|---|---|
| `/initialpose3d` 발행자 | **`aw_localization_bridge` 1개** (`Publisher count: 1`) |
| `/localization/initialization_state` | **3 = INITIALIZED** |
| 경로 수립 | `/api/routing/state` **2 (SET)** |
| 계획 | `/planning/scenario_planning/trajectory` **9.79 Hz** |
| 자율주행 가용 | `/system/operation_mode/availability` → **autonomous: true** |
| 전환 | `change_to_autonomous` **success**, `/api/operation_mode/state` **mode: 2** |
| 제어 명령 | `/control/command/control_cmd` **33.2 Hz** |
| 브리지 | `received=23275 published=23149 initialpose=464`, 앵커 편차 4.7 ms |

기동 배너로 새 실행물임을 확인했고(`also publishing /initialpose3d every 1000 ms`),
어댑터 이미지는 `docker/sils/build-ss2-adapter.sh` 로 재빌드했다.

### ★★ 주기 발행이어야 하는 이유도 실측했다

설계 근거였던 "한 번이 아니라 재무장" 을 직접 깨봤다 — 런 도중
`docker restart adsw-perception-42` (= shim 이 죽고 래치가 `UNINITIALIZED` 로 리셋).

```
17:39:16  perception 재기동
17:39:25  shim 재등장
17:39:30  /localization/initialization_state = 3     ← 사람 개입 0
```

**publish-once 였으면 여기서 못 돌아온다.** (중간의 `UNINITIALIZED` 순간은 샘플링을
못 잡았다 — 관측한 것은 "사람 없이 3 으로 돌아왔다" 까지다.)

### 이 실측이 남긴 부작용 하나 — 파티션 재기동은 공짜가 아니다

위 재기동 뒤 `mode` 는 2 로 남았지만 `availability.autonomous` 가 **false 로 내려가 5분 뒤에도
안 올라왔다.** 래치는 정상 복귀했으므로 이 변경과는 무관하고, 원인은 미확인이다
(`/system/component_state_monitor/component/autonomous/*` 를 잡으려 했으나 응답 없음).

> 기록해 둘 사실: **런 도중 파티션을 재기동하면 래치는 자동 복구되지만 다른 게이트는
> 그렇지 않다.** 재기동은 시험 시작 전에 끝내는 것이 맞다.

| 검증 | |
|---|---|
| 이미지 정적 분석 | ✅ (단, 위 ⚠ 처럼 한 건 틀렸다) |
| 컨테이너 빌드 Release | ✅ 경고 0 |
| 오프라인 매핑 테스트 4종 | ✅ 176 checks, 0 failures |
| **rig 실측 — 수동 단계 0** | ✅ **위 표** |
| **재기동 후 자동 재무장** | ✅ 위 |
| 플랜트를 SS_v2 로 바꿔서도 성립하는가 | ⬜ 미실시 (래치는 플랜트 무관이므로 우선순위 낮음) |

### 남은 절반은 우리 것이 아니다

같은 shim 이 **서비스 경로(`/localization/initialize`)는 안 덮는다.** concealer 는 토픽이
아니라 그 서비스로 초기화하므로 §9 의 `success=false` 는 그대로다. 그래서 벤더 요청은
"`pose_initializer` 를 넣어달라" 보다 훨씬 작아졌다 — **이미 가지고 계신 shim 에 15 줄**:
[`sils-autoware-partition-vendor-requests.md`](../sils-autoware-partition-vendor-requests.md).

---

## 11. SS_v2 를 플랜트로 두고 **자율주행 명령까지** 갔다 (2026-08-11 18:0x~18:21)

§6 은 SS_v2 로 **상태 경로까지**였다. 오늘 그 위에 경로 수립·자율주행 전환·vECU 수신을
얹었다. `/initialpose3d` 자동화가 플랜트와 무관하다는 §10 의 주장도 여기서 확인된다.

```
SS_v2 시나리오 (kashiwanoha, isEgo 없음)
  └─/simulation/entity/status─▶ ss2_ego_bridge      received=15005 published=14998 skipped_no_ego=7
      └─sils/ego/state─▶ aw_vehicle_status_bridge   received=16090 published=16024 bad_gear=0
                       └▶ aw_localization_bridge    received=16094 published=16028 initialpose=1271
                          앵커 편차 13.1 ms, plant restarts ridden through: 5
          └─▶ Autoware   initialization_state 3 · routing SET · trajectory 10.0 Hz · mode 2
              └─control_cmd─▶ aw_control_target_bridge  received=6471 published=6471 bad_gear=0
                  └─▶ vECU  target_received=244  gear [0, 3]
```

**`/initialpose3d` 는 여기서도 수동 0 이다** — 발행자는 `aw_localization_bridge` 하나,
`initialization_state = 3`. 래치가 파티션 속성이지 플랜트 속성이 아니라는 §10 의 서술이
서로 다른 플랜트 두 개(픽스처·SS_v2)에서 성립했다.

### 구성 (§3 과 다른 부분만)

| | 값 | 왜 |
|---|---|---|
| 파티션 맵 | `~/autoware_map/kashiwanoha` | SS_v2 는 자기 이미지 안 kashiwanoha 만 갖는다. 선택지가 없다 |
| 시나리오 | **이미지 내장** `scenario_test_runner/scenario/LateralAction.LaneChangeAction.yaml` | `isEgo` 없음 → concealer 안 뜸(§7) |
| 어댑터 | `ss2_topic:=/simulation/entity/status`, `SS2_EGO_NAME=ego`, Cyclone@42 | 네임스페이스는 런처가 정한다(compose 주석) |
| SS_v2 RMW | `rmw_cyclonedds_cpp` | 파티션과 같은 도메인 |

**`scenarios/ss2/ego-lane-change.xosc` 는 쓰면 안 된다.** `make_ego_scenario.sh` 가 상류
예제의 `LogicFile` 만 kashiwanoha 로 바꿔놨을 뿐 레인 id 는 상류 그대로(178/180/879)여서
`InternalError: Failed to lookup element with id 178` 로 죽는다. kashiwanoha 의 id 는
34378…34693 이다([ss2-clock-survey §](ss2-clock-survey-2026-08-11.md) 에 이미 기록돼 있었다).
`isEgo` 만 뺀 변형을 만들어봤지만 같은 이유로 죽는다 — 자산 자체가 그 맵의 것이 아니다.

### ★ 경로가 안 서던 진짜 이유 — 좌표가 아니라 **차폭**

`The planned route is empty` 만 보고 세 번 헛짚었다. 셋 다 틀렸다:

| 가설 | 반증 |
|---|---|
| 파티션 맵과 SS_v2 맵이 다르다 | **md5 동일** (`7daf6e62…`) |
| 목표점이 차선 밖이다 | **자차가 실제로 지난 점**을 목표로 줘도 실패 |
| 목표 z 가 안 맞는다 (자차 z ≈ −2.89) | z 를 맞춰도 실패 |

decision 파티션 로그가 답을 갖고 있었다:

```
route_handler: getMainLanelets: lanelet_sequence = [ids: 34462 34465 34468]   ← 차선은 찾았다
mission_planner: Goal's footprint exceeds lane!
mission_planner: Goal is not valid! Please check position and angle of goal_pose
```

`mission_planner.param.yaml` 의 **`check_footprint_inside_lanes: true`** 와 kashiwanoha 의
좁은 캠퍼스 차선, `sample_vehicle` 차폭의 조합이다. **목표를 가까이 잡으면 통과한다.**

| 자차 궤적 기준 거리 | goal (x, y) | routing |
|---|---|---|
| 10 m | 3722.2533, 73772.9452 | ✅ **2 (SET)** |
| 20 m | 3731.5301, 73776.8370 | ✅ 2 |
| 30 m | 3740.4306, 73781.4109 | ✅ 2 |
| 40 m | 3749.2930, 73786.0441 | ❌ footprint |
| 종점 | 3800.8326, 73812.5382 | ❌ footprint |

> **kashiwanoha 목표점 규칙: 자차 앞 10~30 m.** 그 이상은 차폭이 안 들어가는 차선에서
> 끝나 거부된다. sample-map 때(§1)는 §1 이 맵에서 뽑아둔 lanelet 103 양 끝을 썼기 때문에
> 이 문제를 만나지 않았다.

`ros2 param set … check_footprint_inside_lanes false` 는 **효과 없다** — mission_planner 가
기동 시 값을 캐시한다. 끄려면 decision 파티션을 파라미터 오버라이드로 재기동해야 한다.

### 목표점을 뽑은 방법 (투영 없이)

lanelet2 python 이 **MGRS projector 를 안 갖고 있다**(파티션·SS_v2 이미지 둘 다
`UtmProjector`/`LocalCartesianProjector` 뿐). 그래서 OSM 을 파싱하는 대신 **자차 궤적을
녹화**했다 — 자차가 지난 점은 정의상 차선 위이고, 시나리오는 결정적이라 재기동하면 같은
길을 간다.

```bash
docker exec adsw-control-42 bash -lc 'source /opt/ros/humble/setup.bash && source /opt/autoware/setup.bash
  timeout 45 ros2 topic echo /localization/kinematic_state > /tmp/traj.yaml'
# 누적 거리로 10/20/30 m 지점을 뽑고, 연속 두 점으로 yaw 를 만든다
```

### 함정 — 시나리오는 **90초짜리**다

`LateralAction.LaneChangeAction` 은 `global_timeout` 과 무관하게 **자기 스토리를 마치고
종료**한다(`Exited (0)`). 목표를 늦게 발행하면 자차가 이미 종점에 서 있고, 목표가 뒤에
있으므로 경로가 빈다. 같은 좌표가 소수점 12자리까지 반복되면 그 신호다.
**시나리오 재기동 → pose 나오자마자 목표 발행**이 맞는 순서다.

### 이 시험이 증명하지 않는 것 — §8 은 그대로다

- **차는 여전히 명령대로 안 움직인다.** `isEgo` 가 없으므로 EgoEntity 도, 차량 모델도 없고,
  `sils/ego/cmd` 를 적용할 곳이 없다. vECU 는 병렬 관측자다.
- `target_received=244` 의 내용도 그 사실과 일치한다 — **acceleration 고유값 3개**
  (−1.5 / −1.11455 / −1), steering 2개. §1 의 픽스처 런(고유값 178/219)과 달리 차가 실제로
  주행하지 않아 명령이 정지 유지 대역에 머문다.
- `control_cmd` 가 **63.9 Hz** 로 잡혔다(픽스처 런은 33.2 Hz). 발행자 수는 1로 확인했고,
  원인 미확인. 기록만 해둔다.

**결론: 플랜트 자리에 SS_v2 를 두고 Autoware 를 자율주행까지 올리는 경로가 성립한다.
남은 것은 여전히 §8 의 "명령을 차에 적용할 길" 하나이고, 그건 벤더 요청 건이다.**
