# 증거 — `simple_planning_simulator` 를 끄고 SiLS 브리지만으로 Autoware 구동

> 측정 2026-08-11 · 장비: 리눅스(Ubuntu 22.04) · KATECH Autoware Partition 3종
> ROS 2 도메인 43 (Cyclone) · SiLS DDS 도메인 42 (Fast DDS)

3계단 §10-1 로 남겨뒀던 시험이다. 원래 계획은 **"역브리지(`aw_vehicle_status_bridge`)만으로
Autoware 가 도는가"** 였는데, 실행 전에 이미지 안의 런치 파일을 읽어보니 **그 계획 자체가
성립하지 않았다.** 그 발견과, 보강한 뒤의 실측을 함께 남긴다.

---

## 1. 계획이 틀렸던 이유 — 시뮬레이터는 열 개를 발행한다

`autoware_simple_planning_simulator/launch/simple_planning_simulator.launch.py` 의
리매핑. 이 파티션은 `localization_sim_mode=api` 라서 **`motion_publish_mode=full_motion`**
경로를 탄다:

| 시뮬레이터 출력 | 실제 토픽 | 누가 채우나 |
|---|---|---|
| `output/twist` | `/vehicle/status/velocity_status` | `aw_vehicle_status_bridge` |
| `output/steering` | `/vehicle/status/steering_status` | 〃 |
| `output/gear_report` | `/vehicle/status/gear_status` | 〃 |
| `output/control_mode` | `/vehicle/status/control_mode` | 〃 |
| `output/turn_indicators_report` | `/vehicle/status/turn_indicators_status` | 〃 |
| `output/hazard_lights_report` | `/vehicle/status/hazard_lights_status` | 〃 |
| **`output/odometry`** | **`/localization/kinematic_state`** | **`aw_localization_bridge`** ← 신규 |
| **`output/acceleration`** | **`/localization/acceleration`** | **〃** ← 신규 |
| `output/actuation_status` | `/vehicle/status/actuation_status` | 아무도 (아래) |
| `output/imu` | `/sensing/imu/imu_data` | 아무도 (아래) |

그리고 `obigo_launch/launch/planning_simulator.launch.xml` 이 못박고 있다:

```xml
<arg name="launch_localization" value="false"/>
```

**Autoware 자체 측위가 꺼져 있으므로, 시뮬레이터를 끄면 자차 위치를 말해줄 것이 하나도
남지 않는다.** 역브리지는 필요조건이었지 충분조건이 아니었다.

---

## 2. 구독자 수 실측 — 놓칠 뻔한 쪽이 더 컸다

시뮬레이터를 **켠 채** 측정 (원본: [`sps-topic-inventory-2026-08-11.txt`](sps-topic-inventory-2026-08-11.txt)).
열 개 전부 발행자는 `simple_planning_simulator` **하나뿐**이었다.

```
/vehicle/status/velocity_status          subs=2
/vehicle/status/steering_status          subs=5
/vehicle/status/gear_status              subs=2
/vehicle/status/control_mode             subs=3
/vehicle/status/turn_indicators_status   subs=1
/vehicle/status/hazard_lights_status     subs=1     ──── 소계 14
/vehicle/status/actuation_status         subs=0     ★ 구독자 없음
/sensing/imu/imu_data                    subs=1     (AEB)
/localization/kinematic_state            subs=28    ★
/localization/acceleration               subs=10    ★  ──── 소계 38
                                         합계 53
```

**`/localization/*` 38 : `/vehicle/status/*` 14.** 뒤늦게 발견한 쪽이 2.7배 크다.

QoS 는 열 개 전부 **RELIABLE · KEEP_LAST(1) · VOLATILE**.
`aw_localization_bridge` 의 기본값이 이것과 일치함을 확인했다(구현 시에는 추론이었고,
이 측정으로 확정).

---

## 3. 대조군 — 시뮬레이터만 끄고 브리지 없이

원본: [`sps-off-nobridge-2026-08-11.txt`](sps-off-nobridge-2026-08-11.txt)

```
열 개 토픽 전부  pubs=0
구독 53개 그대로 살아서 굶음
```

파티션 로그에 예측한 그대로의 증상:

```
[control.collision_detector]: waiting for current odometry...
[control.trajectory_follower.lane_departure_checker_node]: waiting for current_twist msg...
[control.autonomous_emergency_braking]: [AEB] waiting for ego velocity
[control.trajectory_follower.controller_node_exe]: Control is skipped since input data is not ready.
```

---

## 4. 실험군 — 두 브리지 투입

```
bin/sils_ego_source (도메인 42, 50 Hz)
    │  canonical EgoState @ sils/ego/state
    ├─▶ aw_vehicle_status_bridge  ──▶ /vehicle/status/*   (도메인 43, Cyclone)
    └─▶ aw_localization_bridge    ──▶ /localization/*
```

원본: [`sps-off-bridged-2026-08-11.txt`](sps-off-bridged-2026-08-11.txt)

**여덟 개 토픽 전부 `pubs=1`, 발행자가 우리 브리지, QoS 가 시뮬레이터의 것과 정확히 일치.**

| 항목 | 결과 |
|---|---|
| `/localization/kinematic_state` Autoware 수신 | **48.7 Hz** (픽스처 50 Hz 발행) |
| 브리지 유실 | `received=4116 published=4116` / `4125 = 4125` — **0** |
| `bad_gear` | 0 |
| 프레임 | `frame_id: map`, `child_frame_id: base_link` |

### 굶주림 메시지의 종료 시각

브리지 기동 ≈ `1786414195`.

| 메시지 | 마지막 출현 | 판정 |
|---|---|---|
| `waiting for current odometry` | `1786414203` | ✅ **멈춤** |
| `waiting for current_twist msg` | `1786414203` | ✅ **멈춤** |
| `[AEB] waiting for ego velocity` | `1786414203` | ✅ **멈춤** |
| `Control is skipped ... not ready` | 계속 | ⚠ §5 |

---

## 5. 미해결로 남은 것과, 그게 브리지 탓이 아닌 이유

### 5.1 `Control is skipped` 는 계속된다 — 픽스처의 한계

`bin/sils_ego_source` 는 **열린 루프 테스트 픽스처**다. 맵 원점(0,0,0)에서 출발해
vx 를 램프시킬 뿐이고, 그 좌표는 **lanelet 맵 위가 아니다.** 그래서 Autoware 는
경로를 세울 수 없고, 궤적이 없으니 trajectory follower 가 계속 건너뛴다.

**이 시험이 증명한 것**: 53개 구독 중 52개가 우리 브리지와 **매칭되고 데이터가 흐른다**
(RxO 일치, 유실 0, 48.7 Hz). **증명하지 않은 것**: Autoware 가 우리 브리지 위에서
**주행한다**.

→ 픽스처에 맵 좌표를 주고 다시 시험한 결과는 **§5.5**. 경로 수립까지는 갔고,
그 과정에서 구멍 두 개(TF, 타임스탬프)를 더 찾았다.

### 5.2 `/sensing/imu/imu_data` — 막지 않는다 (확인됨)

구현 시 "AEB 가 IMU 없이 막히는지 **미측정**" 으로 남겨뒀던 항목이다. **답이 나왔다.**
AEB 는 `waiting for ego velocity` 를 멈춘 뒤 IMU 를 요구하지 않고 다음으로 넘어갔다:

```
[AEB] waiting for object pointcloud message
```

이건 인지(perception) 쪽이고, 벤더 구성에서도 `launch_simulator_dummy_perception=false`
라 원래 비어 있다. **IMU 는 발행하지 않아도 제어를 막지 않는다.**

### 5.3 `/vehicle/status/actuation_status` — 구독자 0 (확인됨)

정적 분석대로다. `autoware_raw_vehicle_cmd_converter` 만 읽는데, 그 노드는
`vehicle_model_type` 에 `ACTUATION_CMD` 가 있어야 뜬다. `sample_vehicle` 은
`DELAY_STEER_ACC_GEARED` 라 **뜨지 않는다.** 실측 `subs=0`.

### 5.4 공분산은 전부 0

`EgoState` 에 불확실성이 없어서 지어내지 않았다. 이 구성의 소비자들은 걸리지 않았지만,
공분산으로 게이팅하는 소비자가 있다면 그걸 보고하는 진짜 플랜트가 필요하다.

---

## 5.5 2차 시험 — 픽스처에 맵 좌표를 주고 다시 (같은 날 오후)

§5.1 의 한계를 없애려고 `bin/sils_ego_source` 에 `--x0/--y0/--yaw0/--vmax` 를 추가했다
(기본값은 옛 동작 그대로 — `x0=y0=yaw0=0` 이면 위치가 `(s,0,0)` 으로 환원된다).
맵 위의 실제 시작점(`/workspace/log/pose.json`)을 주고 다시 돌렸다.

### 성과 — Autoware 가 **우리 브리지 데이터로 경로를 세웠다**

Decision 파티션 로그:

```
[planning.mission_planning.mission_planner]: initial pose - x: 3640.163330, y: 73593.683458
[planning.mission_planning.mission_planner]: initial orientation - qz: -0.660924, qw: 0.750452
[route_handler]: getMainLanelets: lanelet_sequence = [ids: 103 ]
```

`/api/routing/state` = **2 (SET)**. 저 initial pose 는 `aw_localization_bridge` 가 낸
값이고, `simple_planning_simulator` 는 꺼져 있다. **미션 플래너가 우리 데이터를 읽고
차선 103 위에 경로를 얹었다.**

### 발견 3 — `map → base_link` TF. 토픽 감사로는 안 보이는 구멍 ★

`/tf` 는 퍼블리셔 1, 구독 14 로 **멀쩡해 보인다**(`robot_state_publisher` 가
`base_link → 센서` 를 낸다). 그런데:

```
$ ros2 run tf2_ros tf2_echo map base_link
Could not find a connection between 'map' and 'base_link' because they are not part of
the same tree. Tf has two or more unconnected trees.
```

`simple_planning_simulator` 는 odometry 를 낼 때 **TF 도 함께 방송**한다. 실센서 구성에서는
EKF localizer 가 그 일을 하는데 `launch_localization=false` 라 아무도 안 한다.
증상은 `[control.collision_detector]: Failed to get transform from object frame to base_link`
이고, 자율주행 전환이 거부된다.

**`aw_localization_bridge` 가 같은 샘플에서 TF 도 방송하도록 고쳤다**(`publish_tf`, 기본 참).
한 노드에서 pose 와 transform 을 함께 만들어야 둘이 어긋날 수 없다. 수정 후:

```
- Rotation: in RPY (radian) [0.000, 0.000, -1.444]   ← 시작 yaw 그대로
  0.126  0.992  0.000 3671.691
 -0.992  0.126 -0.000 73346.173
```

### 발견 4 — 그래도 자율주행은 안 걸린다. 원인은 **타임스탬프** ★

`/system/operation_mode/availability` → `autonomous: false`.
Decision 파티션의 진단이 정확히 짚어준다:

```
[system.topic_state_monitor_vehicle_status_velocity_status]: topic is timeout. Set ERROR
[system.topic_state_monitor_vehicle_status_steering_status]: topic is timeout. Set ERROR
[system.topic_state_monitor_transform_map_to_base_link]: /tf topic is timeout. Set ERROR
```

**"not received" 가 아니라 "timeout"** 이다 — 받고는 있는데 낡았다고 판정한다. 확인:

```
$ ros2 topic echo --once /vehicle/status/velocity_status
  stamp: {sec: 119, nanosec: 540000000}      ← 픽스처의 sim-relative 시간
$ date +%s
  1786415534                                  ← Autoware 의 벽시계
```

**Autoware 가 보기에 56년 묵은 메시지다.**

이건 브리지 결함이 아니다. 두 브리지는 **의도적으로 소스의 `stamp_ns` 를 그대로 쓴다** —
도착 시각으로 찍으면 플랜트 지연이 감춰지고 여러 리포트가 서로 어긋나기 때문이다.
문제는 `sils_ego_source` 가 **0 에서 시작하는 sim 상대시간**을 찍는다는 것이다.

> ### ★ MORAI / Web.Auto 어댑터에 대한 요구사항
> **플랜트는 Autoware 와 공유하는 시계로 `EgoState.stamp_ns` 를 찍어야 한다.**
> 실무적으로는 epoch(ns) 이거나, Autoware 를 `use_sim_time` 으로 돌리고 플랜트가
> `/clock` 을 내야 한다. 둘 다 아니면 **모든 값이 정확해도** `topic_state_monitor` 가
> 전부 ERROR 로 막고, 증상은 "데이터가 안 온다" 가 아니라 "자율주행 모드를 쓸 수 없다"
> 로 나타난다 — 원인과 한참 떨어진 자리에서.

### 남은 것 (이 시험 범위 밖)

- `[behavior_velocity_planner]: Waiting for pointcloud data` — 인지 입력. 벤더 구성에서도
  `launch_simulator_dummy_perception=false` 라 원래 비어 있다.

---

## 6. 재현

```bash
# 1) 파티션 3종 (벤더 러너 그대로)
cd "$HOME/oss/oss_adsw"
./partition/run_partitions.sh --repo ghcr.io/oss-test-group/autoware-partition \
  --map-path "$HOME/autoware_map/sample-map-planning" --domain-id 43 --headless --delay 15

# 2) 제어 파티션만 시뮬레이터 끄고 교체
tools/sils/ss2/run_control_partition_sils.sh --domain-id 43

# 3) 두 브리지 + 픽스처
ROS_DOMAIN_ID=43 VISS_DDS_DOMAIN_ID=42 \
  tools/sils/ss2/run_vehicle_seat_bridges.sh --with-ego-source --duration 300
```

`--simulator-on` 을 주면 벤더 동작을 그대로 재현해 A/B 비교를 할 수 있다.

**함정**: 시뮬레이터를 켠 채 브리지를 돌리면 토픽마다 발행자가 둘이 되어 Autoware 가
두 소스를 섞어 받는다. `run_vehicle_seat_bridges.sh` 가 그 경우를 **거부**한다.

---

## 7. 다음

| 할 일 | 왜 |
|---|---|
| `sils_ego_source` 에 epoch 타임스탬프 옵션 추가 | §5.5 발견 4 — 자율주행 전환을 픽스처만으로 넘겨볼 수 있는 마지막 관문 |
| 맵 좌표 + 공유 시계를 가진 플랜트 확보 (MORAI / SS_v2) | 주행 증명의 최종 경로. **시계 요구사항을 어댑터 설계에 반영할 것** |
| vECU 를 `vehicle_interface` 좌석에 넣고 폐루프 | `aw_control_target_bridge` 와 맞물리는 반대편 |
