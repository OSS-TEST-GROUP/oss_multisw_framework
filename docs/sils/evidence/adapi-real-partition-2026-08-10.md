# 진짜 Autoware 파티션 ADAPI 실측 — 3계단 2·3단계

> 2026-08-10 · Ubuntu 22.04 / x86_64 · KATECH Autoware Partition `main` (`c128be8`)
> 대조 도구: [`check_autoware_adapi.sh`](../../../tools/sils/ss2/check_autoware_adapi.sh)
> 계약 기준: concealer `3370c817efc5458fbe69cc3c02a9bc8956cb8a81`
> 서사와 대응은 [sils-stage3-autoware-partition.md](../sils-stage3-autoware-partition.md).

## 구성

```
파티션 (도메인 43, Cyclone DDS, No-CUDA, headless)
  adsw-perception-43 / adsw-decision-43 / adsw-control-43
  ghcr.io/oss-test-group/autoware-partition:adsw-{perception,decision,control}
  런처: partition/run_partitions.sh --domain-id 43 --headless --delay 15
  맵:   ~/autoware_map/sample-map-planning

조회 (도메인 43, Cyclone 으로 맞춤)
  SILS_RMW=rmw_cyclonedds_cpp tools/sils/ss2/check_autoware_adapi.sh --domain 43
```

## 결과 — 20행 계약

| | 기본 기동 | `deprecated_api` 후 |
|---|---|---|
| present | 17 / 20 | **20 / 20** |
| core 실패 | 0 | 0 |
| drive 누락 | **2** | 0 |
| feature 누락 | 1 | 0 |
| state source | `adapi` (3/3, `legacy` 도 존재) | 동일 |

원본 JSON: [`before`](adapi-real-partition-2026-08-10-before.json) ·
[`after`](adapi-real-partition-2026-08-10-after.json)

### 기본 기동에서 빠졌던 3행

```
topic    feature  /api/external/get/emergency        MISSING
service  drive    /api/external/set/engage           MISSING
service  drive    /api/autoware/set/velocity_limit   MISSING
```

전부 **tier4 external API 계열**. 나머지 external 계열(`rtc_commands`, `rtc_auto_mode`,
`get/rtc_status`)은 **있었다** — RTC controller 만 별도로 런치되기 때문이다.

## 원인 — `launch_deprecated_api` 기본값 `false`

`tier4_autoware_api_launch/launch/autoware_api.launch.xml`:

```xml
<arg name="launch_deprecated_api" default="false"/>
...
<group if="$(var launch_deprecated_api)">
  <include file="$(find-pkg-share tier4_autoware_api_launch)/launch/deprecated_api.launch.xml"/>
</group>
```

그 `deprecated_api.launch.xml` 이 `autoware_iv_external_api_adaptor` 를 띄운다.
**패키지는 이미 이미지에 설치돼 있다** — `/opt/autoware/lib/libautoware_iv_external_api_adaptor.so`,
`/opt/autoware/share/autoware_iv_external_api_adaptor/`. 빌드도 상류 patch 도 불필요.

기본 기동 시 실제로 떠 있던 API 노드:

```
/adapi/node/*                              autoware_default_adapi  (모던 ADAPI)
/autoware_api/external/rtc_controller/*    RTC 만
/default_adapi/helpers/*                   adaptors
```

## concealer 가 그 둘을 실제로 쓰는가 — 쓴다

`external/concealer/src/field_operator_application.cpp`:

| 행 | 내용 |
|---|---|
| `:157` | `requestEngage("/api/external/set/engage", *this)` — 클라이언트 생성 |
| `:304` | `case LegacyAutowareState::waiting_for_engage:` 에서 **무조건 호출** 후 `driving` 전이 대기 |
| `:163` | `requestSetVelocityLimit("/api/autoware/set/velocity_limit", *this)` |
| `:604` | `setVelocityLimit()` ← `EgoEntity::setVelocityLimit` ← 시나리오 액션 |

concealer 의 미존재 서버 실패 모드는 **180 초 침묵 후 throw** 다. 08-10 이전에
`RoutingAction.AcquirePositionAction` 이 정확히 `velocity_limit` 으로 한 번 죽었다
([`stub_node.py`](../../../adapters/autoware-stub/autoware_stub/stub_node.py) 주석 참조).

## 검증한 대응

```bash
docker exec -d adsw-control-43 bash -lc \
  'source /opt/ros/humble/setup.bash && source /opt/autoware/setup.bash &&
   exec ros2 launch tier4_autoware_api_launch deprecated_api.launch.xml \
     > /workspace/log/deprecated_api.txt 2>&1'
```

→ 재실행 결과 **20/20, drive 누락 0**.

## 도메인 분리 (2단계)

| 확인 | 결과 |
|---|---|
| 도메인 42 에서 본 `/api/` 서비스 | **0** |
| 도메인 43 에서 본 `/api/` 서비스 | 47 |
| 컨테이너 이름 | `adsw-*-43` — `sils-*` 와 충돌 없음 |

**공존 판정** — 파티션 3개를 43 에서 돌린 채 2계단 폐루프를 42 에서 통째로 재실행:

```
samples 401   sim-time span 20.00s
vx first +10.000 · vx last +10.000 · vx max +10.000 m/s
VERDICT: MOVING -- the ego reached 10.00 m/s.
ss2-adapter  received=698 published=697 skipped_no_ego=1 write_failed=0
ss2-control  forwarded=858 published=858 stale_holds=0 bad_gear=0 matched=1
```

## ★ 벤더 교차 실측 (B1-3 Cyclone 칸) — 같은 날 추가 측정

`sils-ss2:humble` 을 도메인 43(파티션, Cyclone)에 붙이고 **RMW 만 바꿔가며** 3단으로 측정.
20행 계약 대조와 달리 이번엔 **실제 데이터와 실제 왕복**을 시험했다.

| 우리 쪽 RMW | discovery | 토픽 데이터 (`--once` 수신) | **서비스 왕복** |
|---|---|---|---|
| `rmw_fastrtps_cpp` | 토픽 514 / 서비스 1010 | ✅ 도착 | ❌ **실패** |
| `rmw_cyclonedds_cpp` | 토픽 514 / 서비스 1010 | ✅ 도착 | ✅ `InterfaceVersion_Response(major=1, minor=6, patch=0)` |

- 토픽: `/api/operation_mode/state`
- 서비스: `/api/interface/version` (`autoware_adapi_version_msgs/srv/InterfaceVersion`) —
  읽기 전용이라 Autoware 상태를 바꾸지 않는다

Fast DDS 실패 시그니처:

```
[RTPS_READER_HISTORY Error] Change payload size of '28' bytes is larger than
the history payload size of '15' bytes and cannot be resized.
```

### 양방향 확인 — 5단계가 쓸 바로 그 타입으로

위 표는 Cyclone → Fast DDS(수신) 한 방향이다. 반대 방향도 **`autoware_control_msgs/msg/Control`**
(5단계에서 vECU 가 받을 `control_cmd` 의 타입)으로 따로 확인했다.

| 방향 | 시험 | 결과 |
|---|---|---|
| Cyclone(파티션) → Fast DDS(우리) | `/api/operation_mode/state` echo | ✅ 수신 |
| **Fast DDS(우리) → Cyclone(파티션)** | `/sils_interop_test`, `autoware_control_msgs/msg/Control`, 5 Hz | ✅ **파티션 컨테이너 안에서 정상 수신** |

⚠ 첫 시도는 "토픽 안 보임"으로 나왔는데 **측정 실수**였다 — `autoware_control_msgs` 는
`/opt/ros/humble` 이 아니라 SS_v2 워크스페이스에 있어서 `local_setup.bash` 를 소싱하지 않으면
`ros2 topic pub` 이 아예 발행을 못 한다. 벤더 문제로 오독하기 쉬운 자리다.

⚠ 두 시험 모두 `ros2 topic pub/echo` 의 **기본 QoS** 다. 실제 `control_cmd` 의 QoS 에서
RxO 가 맞는지는 별도 문제다(이 스택이 이미 한 번 당한 실패 유형).

**해석**

- **discovery 와 pub/sub 은 벤더를 넘는다 — 양방향으로.** Fast DDS 로도 파티션이 다 보이고,
  메시지가 양쪽으로 흐른다.
- ★ **그래서 데이터 평면(5단계)은 벤더 교차로 성립한다.** vECU ↔ Autoware 는
  `control_cmd` 와 `/vehicle/status/*` 뿐이고 전부 토픽이다. **RMW 를 통일하지 않아도 된다.**
- **서비스는 못 넘는다.** `concealer` ↔ AD API 는 전부 서비스이므로,
  **SiLS 기본 구성(Fast DDS)으로는 3계단 4단계가 성립하지 않는다.**

> 범위: 실패는 `/api/interface/version` 한 건에서 관측했고 원인은 Fast DDS 리더의
> history payload 크기 불일치다. **모든 서비스가 반드시 실패한다**까지는 증명하지 않았다.

## ⛔ 그래서 Cyclone 으로 전환해봤다 — 2계단이 깨졌다

위 결론의 자연스러운 대응(SS_v2 를 Cyclone 으로)을 실제로 시험했고 **실패**했다.

### (1) 어댑터 이미지에 Cyclone 부재 — 수정 완료

`sils-ss2:humble` 에는 `librmw_cyclonedds_cpp.so` 가 있지만
**`sils-ss2-adapter:humble` 에는 없었다.** 노드가 기동 즉시 종료:

```
[rcl] RMW implementation not installed (expected identifier of 'rmw_cyclonedds_cpp')
... failed to load shared library 'librmw_cyclonedds_cpp.so'
```

수정: `docker/sils/Dockerfile.ss2-adapter` 의 **adapter 스테이지**에
`ros-humble-rmw-cyclonedds-cpp` 추가 후 재빌드. (toolchain 스테이지의 apt 블록에 넣으면
그 뒤 fastddsgen gradle 빌드 레이어가 매번 무효화된다.)

### (2) ★ 미해결 — Cyclone XTypes 에서 어댑터 segfault

이미지 수정 후 재실행:

```
dq.builtin: type [COMPLETE 750d3e2cba7c...]: ddsi_xt_type_add_typeobj with invalid type object
   ... 수백 줄 ...
dq.builtin: type [INVALID 0000000000000000000000000000]: ddsi_xt_type_init_impl with invalid type object
[ros2run]: Segmentation fault
```

| 관측 | 값 |
|---|---|
| 판정 | `VERDICT: STATIONARY` — `vx first/last/max +0.000 m/s` |
| concealer | `initializing` 에서 정지 (`driving` 미도달) |
| 어댑터·컨트롤 카운터 | **공백** |
| 같은 `invalid type object` 로그 | 어댑터 로그 **및 stub 로그**(순수 rclpy)에 동시 출현 |

**배제한 원인** — 어댑터 단독 실행 (Cyclone, 40초):

| ROS_DOMAIN_ID | VISS_DDS_DOMAIN_ID | 결과 |
|---|---|---|
| 44 | 44 (같은 도메인) | running · segfault 0 · `invalid type object` **0 줄** |
| 44 | 45 (분리) | running · segfault 0 · `invalid type object` **0 줄** |

→ 원인은 "한 프로세스 안의 rclcpp(Cyclone) + 네이티브 Fast DDS" 구조 자체가 **아니다.**
상대 참가자가 있어야 재현된다.

**미완의 진단** — 다음에 할 것:

1. 도메인 42 의 Fast DDS 참가자를 하나씩 빼며 이분 탐색(vECU / 어댑터 네이티브 writer /
   컨트롤 브리지) → `invalid type object` 를 처음 유발하는 참가자 특정
2. `CYCLONEDDS_URI` 로 타입 디스커버리(XTypes) 비활성화 → 원인 확정 + 우회로
3. 그래도 안 되면 ROS 쪽만 파티션 도메인에 두고 vECU 쪽 네이티브 Fast DDS 는 다른 도메인으로.
   어댑터가 `ROS_DOMAIN_ID` 와 `VISS_DDS_DOMAIN_ID` 를 이미 따로 받으므로 코드 수정 불필요.

> 참고: `run_ego_scenario.sh` 로 핸드셰이크만 분리해보려 했으나
> `malformed launch argument 'vehicle_model:='` 로 시나리오가 13초에 죽어 **판정 불가**였다.
> 이 경로는 Cyclone 하에서 vehicle-model 프로브가 빈 값을 돌려준 것으로 보이며, 별도 건이다.

## ★★ control_cmd 실증 — 진짜 Autoware 를 주행 상태로 올리고 Fast DDS 로 받았다

앞의 시험들은 인공 토픽이었다. 이번엔 **진짜 Autoware 가 스스로 만들어내는 제어 명령**이다.

### 먼저: 기본 상태에서는 control_cmd 가 흐르지 않는다

```
/control/command/control_cmd : Publisher 1, Subscription 6, 타입 일치
ros2 topic hz  →  12초간 출력 없음        ← 발행 0
operation_mode: 1 (STOP) · is_autonomous_mode_available: false · routing state: 1 (UNSET)
```

토픽이 존재하고 타입이 맞는 것과 **데이터가 흐르는 것은 다른 문제**다. Autoware 를
초기위치 → 경로 → engage 로 올려야 비로소 발행한다.

### 유효 좌표 뽑기 — 맵이 MGRS 라 계산이 필요하다

`sample-map-planning` 은 MGRS(54SVE) 투영이고 `.osm` 노드는 lat/lon 만 갖는다.
**Autoware 의 map frame = UTM 좌표를 100 km 격자로 나눈 나머지**다:

```python
tf = pyproj.Transformer.from_crs("EPSG:4326", "EPSG:32654", always_xy=True)  # UTM 54N
e, n = tf.transform(lon, lat)
map_x, map_y = e % 100000, n % 100000
```

검산: 이 식으로 구한 전체 노드의 centroid 가 **(3749.71, 73724.27)** 로,
perception 로그의 `vector_map_tf_generator: x:3749.71, y:73724.3` 과 일치한다.

가장 긴 차선(id=103, 108.8 m)의 중심선 양 끝:

| | x | y | yaw |
|---|---|---|---|
| START | 3638.693 | 73605.251 | −1.4441 |
| GOAL | 3652.439 | 73497.350 | −1.4443 |

### 기동 순서 (컨테이너 안, Cyclone 네이티브 — concealer 불필요)

| # | 무엇 | 주의 |
|---|---|---|
| 1 | `/initialpose3d` 에 시작 포즈 발행 | ⚠ **`/initialpose` 가 아니다.** `/initialpose` 는 pose_initializer(NDT/GNSS) 경로라 플래닝 시뮬레이터에서는 끝나지 않는다. `simple_planning_simulator` 가 구독하는 것은 `/initialpose3d` 다 |
| 2 | `/planning/mission_planning/goal` 에 목표 포즈 발행 | `autoware_routing_adaptor` 가 받아 `set_route_points` 로 바꾼다 |
| 3 | `/api/operation_mode/change_to_autonomous` 서비스 호출 | Cyclone 네이티브 호출 |

각 단계 후 관측값:

```
1 후 : localization initialization_state = 3 (INITIALIZED) · /localization/kinematic_state 발행 시작
2 후 : routing state = 2 (SET) · /planning/scenario_planning/trajectory 10.0 Hz
       is_autonomous_mode_available: false → true
3 후 : ChangeOperationMode_Response(success=True) · operation_mode = 2 (AUTONOMOUS)
       /control/command/control_cmd → 33.2 Hz  ★
```

### ★ 그리고 우리 Fast DDS 쪽에서 그대로 받힌다

`sils-ss2:humble` · `RMW_IMPLEMENTATION=rmw_fastrtps_cpp` · 도메인 43:

```
average rate: 33.360        (Cyclone 쪽 33.222 와 동일)
steering_tire_angle: -8.71e-08
velocity: 0.0
acceleration: -1.5
```

**5단계의 데이터 평면이 실증됐다** — 진짜 Autoware 가 만든 `control_cmd` 가 벤더를 넘어
우리 쪽에 초당 33 번 도착한다. RMW 통일도, 라우터도, concealer 도 필요 없었다.

> 범위: 위 제어값은 **정지 유지**(velocity 0, accel −1.5)다. 전송이 성립한다는 것까지이며
> "ego 가 주행한다"는 별개다. 또 QoS 는 `ros2 topic echo` 기본값으로 맞았다 —
> 우리 브리지가 쓸 QoS 에서도 RxO 가 맞는지는 브리지를 붙일 때 다시 봐야 한다.

## ★★★ 5단계 배선 — 진짜 Autoware 의 제어 목표가 vECU 에 도착한다

브리지를 만들어 붙였다. **concealer 없이, RMW 통일 없이, 서비스 한 개도 쓰지 않고.**

```
[Autoware Control 파티션]  Cyclone · 도메인 43 · AUTONOMOUS
   /control/command/control_cmd  33 Hz  +  /control/command/gear_cmd  10 Hz
                  │
                  ▼   aw_control_target_bridge   (Fast DDS 양쪽 — 벤더 교차)
   autoware/control_target  ·  canonical ControlTarget  ·  도메인 42
                  │
                  ▼
        vECU (bin/vehicle_interface, vehicle-interface.yaml)
```

신규 코드: [`aw_control_target_bridge.cpp`](../../../adapters/ss2/src/aw_control_target_bridge.cpp) ·
[`aw_target_mapping.hpp`](../../../adapters/ss2/include/aw_target_mapping.hpp)
(+ `adapters/ss2/CMakeLists.txt`, `docker/sils/Dockerfile.ss2-adapter` 에 `ControlTarget.idl`).

### 측정 — ego 가 실제로 달리는 동안

기동: `/api/simulator/set/pose` 로 차선 시작점 리셋 → 목표 → `change_to_autonomous`.
ego 실측 속도 **4.02 → 3.70 → 3.22 → 2.65 m/s** (목표 접근 중 감속).

| 항목 | 값 |
|---|---|
| 브리지 | `received=126 published=126 write_failed=0` |
| vECU | `target_received=396` · `control_published=398` |
| acceleration | **고유값 178 개**, −3.4000 ‥ −0.2628 |
| steering | **고유값 219 개**, −0.000121 ‥ +0.000017 |
| gear | `[0, 3]` — 주행 중 **3(DRIVE)**, 도착 후 0(PARK) |

값이 상수가 아니라 **178/219 개의 서로 다른 값**이라는 것이 요점이다. 파이프가 살아 있다는
증거를 카운터가 아니라 **내용의 변화**로 잡았다 — 이 문서가 계속 요구하는 그 기준이다.

정지 상태에서 잡으면 `accel=-1.5, gear=0` 한 값만 나온다(고유값 1). 실제로 처음 두 번의
측정이 그랬고, 원인은 파이프가 아니라 **Autoware 가 목표에 이미 도착해 있었던 것**
(`routing state: 3 = ARRIVED`)이었다. `/initialpose3d` 재발행으로는 ego 가 안 움직이고
`/api/simulator/set/pose` 서비스라야 리셋된다 — 이 서비스는 `deprecated_api` 를 띄웠기에
존재한다(§2-8).

### 소비율 — 뒤처지지 않는다

vECU 는 50 ms 틱(20 Hz)이고 소스는 33 Hz 다. 20 초 동안 `src_seq` 가 7050 → 7711 (661개,
= 33/s) 진행하는 사이 `target_received` 는 396 건(= 19.8/s)이었다. 즉 vECU 는 매 틱
**최신 샘플을 집고 나머지를 건너뛴다** — 백로그가 쌓여 옛 명령을 읽는 일은 없다.
(측정 전에 그 반대를 의심했으나 데이터가 부정했다.)

### ⚠ 아직 열린 루프다

vECU 의 출력은 **아무 플랜트에도 닿지 않는다.** Autoware 의
`autoware_simple_planning_simulator` 가 여전히 `control_cmd` 를 직접 먹고 차를 움직인다.
지금 vECU 는 그 옆에서 **병렬 관측자**이지 루프 안에 있지 않다.
닫으려면 플랜트 결정이 필요하다 — 파티션의 시뮬레이터를 끄고 우리 플랜트를 넣거나,
vECU 를 `raw_vehicle_cmd_converter` 자리로 밀어넣거나.

## ★ MMF(VISS) ↔ Autoware — 관측 평면 실측

`service/core/vehicle_info_mgr` 의 `viss-server` 를 도메인 43(파티션)에 붙였다.
DDSProvider 는 rclcpp 없이 **네이티브 Fast DDS** 로 ROS 맹글링 토픽(`rt/...`)을 직접 읽는다.

### 주행 중 30초, VSS 경로 50개 이상이 채워졌다

| VSS 경로 | n | 고유값 | 범위 |
|---|---|---|---|
| `Vehicle.Speed` | 309 | 126 | 0 ‥ **15.0525** (km/h) |
| `Vehicle.CurrentLocation.Latitude` | 309 | 112 | 35.9011 ‥ 35.9014 |
| `Vehicle.CurrentLocation.Heading` | 309 | 124 | 277.25° 부근 |
| `Vehicle.Acceleration.Longitudinal` | 309 | 126 | −0.687 ‥ +0.011 |

그 밖에 `ADS.Perception.*`, `ADS.Planning.*`, `ADS.Safety.HazardStatus.*`,
`ADS.OperationMode.Availability.*`, `ADS.Diagnostics.*` 등이 함께 채워진다.
발견한 Autoware 토픽은 총 1628개.

**단위 교차검증**: ego 실측 4.18 m/s × 3.6 = **15.05 km/h** — VSS 가 km/h 로 변환해
내보낸다는 것이 수치로 확인된다(캐노니컬 `EgoState` 는 m/s. 두 평면의 단위가 다르다).

### 발견한 결함 3건과 수정

| # | 결함 | 증상 | 수정 |
|---|---|---|---|
| 1 | `rt/api/vehicle/status` 를 RELIABLE 로 구독 | Autoware 는 **BEST_EFFORT** 제공 → RxO 위반 → **조용히 미매칭**. 토픽은 발견되는데 `Subscription count: 0`, 데이터 0 | `best_effort` 플래그 `false`→`true` |
| 2 | 조향각을 VSS 로 안 내보냄 | `VehicleStatus.steering_tire_angle` 은 원래 수신 중. 받을 자리(`Vehicle.Chassis.Axle.Row1.SteeringAngle`, degrees)도 이미 존재 | `handleVehicleStatus()` 에 push 추가 (**rad→deg**) |
| 3 | `viss-server` 가 **소스에서 빌드되지 않음** | fastddsgen 4.3.0 이 `@unit (value="m")` 을 `unit_altitude = m;` 로 생성(따옴표 누락) → `idl_types` 타깃 실패 | 3개 IDL 의 `@unit` 주석 제거 |

1번의 대가가 컸다: **기어와 조향각이 둘 다 `VehicleStatus` 에서 오므로 메시지 하나가
죽으면서 둘 다 비어 있었다.**

3번은 `build.sh` 의 `make -k … || true` 가 실패를 삼켜서 **빌드가 매번 "성공"으로 끝나고
3주 전(2026-07-21) 바이너리가 그대로 남아 있었다.** 처음 "VISS ↔ Autoware 동작 확인" 이라고
기록한 실측도 그 옛 바이너리로 한 것이다(결과 자체는 유효하나 그 사실을 모르고 있었다).
**산출물 타임스탬프를 확인하지 않으면 몇 주 전 바이너리를 시험하게 된다.**

### 수정 후 재실측 (18:04 빌드, 주행 중 30초)

| VSS 경로 | 수정 전 | 수정 후 |
|---|---|---|
| `Vehicle.Powertrain.Transmission.SelectedGear` | **없음** | n=309, 고유 2 — `127`(DRIVE) → `126`(PARK) |
| `Vehicle.Chassis.Axle.Row1.SteeringAngle` | **없음** | n=309, 고유 **118** |
| `Vehicle.Speed` / `CurrentLocation.Latitude` | 309 / 309 | 309 / 309 — **회귀 없음** |

`CurrentLocation.Latitude` 는 `@unit` 을 걷어낸 `GeoPoint.idl` 에서 나오는 값이다. 수정 후에도
93개 고유값이 계속 들어온다는 것은 **그 IDL 변경이 Autoware 와의 타입 매칭에 무해**하다는 뜻이다.

### QoS 실측표 (19개 구독 전수 대조)

| 토픽 | Autoware 제공 |
|---|---|
| `/control/command/control_cmd` | RELIABLE · KEEP_LAST(1) · TRANSIENT_LOCAL |
| `/api/vehicle/kinematics`, `/api/vehicle/status` | **BEST_EFFORT** · VOLATILE |
| `/api/system/diagnostics/status` | BEST_EFFORT · VOLATILE |
| `/api/routing/{state,route}`, `/api/operation_mode/state`, `/api/fail_safe/mrm_state`, `/api/autoware/get/emergency`, `/api/system/diagnostics/struct` | RELIABLE · TRANSIENT_LOCAL |
| 나머지(trajectory, perception, planning factors, hazard_status, operation_mode/availability) | RELIABLE · VOLATILE |

전수 대조 결과 **실제 불일치는 `api/vehicle/status` 1건뿐**이었다. `diagnostics/struct` 는
BEST_EFFORT 리더가 RELIABLE 제공자와 매칭되므로 정상이다(요구가 제공보다 약한 쪽은 매칭됨).

## ⚠ 이 실측이 증명하지 않은 것

- **20행 계약 대조 자체는 Cyclone↔Cyclone 이었다.** 조회 쪽을 맞춰서 본 것이다.
  벤더 교차의 답은 위의 별도 측정에 있다.
- **20행 대조는 discovery 수준이다** — `ros2 topic/service list -t` 는 이름과 타입만 본다.
  메시지 왕복은 위 표에서 따로 시험했다.
- **SS_v2 와 파티션이 실제로 연동된 적은 아직 없다.** 2계단 폐루프는 도메인 42 에서
  여전히 `autoware-stub` 을 상대로 돌았다. 오늘 증명한 것은 **격리(공존)**이지 연동이 아니다.
- **ego 가 진짜 Autoware 로 주행한다는 뜻이 아니다.** 이름과 타입이 맞다는 것까지다.
- 파티션은 초기 위치를 안 넣은 headless 상태였다 — decision 의
  `map`→`base_link` lookupTransform ERROR 는 그 때문이며 정상이다.

## 부수 발견

- `run_partitions.sh` 의 `LOG_DIR` 은 저장소 루트 고정 — **도메인이 달라도 같은 로그를
  덮어쓴다.** 도메인 42 실행 기록은 43 실행에 지워졌다.
- `run_partitions.sh` 는 `trap cleanup INT TERM EXIT` + 말미 `tail -F` 다.
  프로세스가 죽으면 컨테이너 3개도 (`--rm` 이므로) 삭제된다.
- 호스트에서 `bin/sils_vecu_*` 를 직접 돌리려면
  `LD_LIBRARY_PATH=$PWD/output/x86_64/lib` 가 필요하다. 없으면 1초 만에
  `libfastdds.so.3.2: cannot open shared object file` 로 죽는다.
