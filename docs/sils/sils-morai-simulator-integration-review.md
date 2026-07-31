# SiLS–MORAI Simulator 연동 아키텍처 검토

> 대상: `oss_multisw_framework` SiLS와 MORAI 차량 시뮬레이터 연동  
> 작성일: 2026-07-14  
> 문서 성격: 연동 사전 검토 및 단계별 구현 권고  
> 범위: 차량 제어/상태, 센서, 시간 동기화, 시나리오 자동화, 네트워크, 시험·운영

---

## 1. 검토 결론

MORAI 연동은 가능하다. 다만 이를 단순 ROS/UDP 브리지 추가로 정의하면 차량 동역학의 중복 계산, 시간축 불일치, 좌표 변환 오류, 대용량 센서 병목이 뒤늦게 드러날 가능성이 높다.

초기 구현은 다음 구조를 권장한다.

1. MORAI를 **외부 Plant 및 Sensor Simulator**로 둔다.
2. 검증 중인 **한양대 ROS2–MORAI UDP Bridge**를 외부 연동 컴포넌트로 재사용한다.
3. Bridge가 K-Autoware 표준 타입까지 제공하지 않는 endpoint에만 선택적 **MoraiContractAdapter**를 둔다.
4. MORAI가 Ego 차량 동역학을 소유한다.
5. MMF/ADSW는 조향·가속·제동·기어 명령을 보내고 MORAI는 차량 상태를 반환한다.
6. 최초 연동은 현재 SiLS Phase 1과 맞는 `WallClock + live`로 제한한다.
7. 시나리오 제어와 고빈도 데이터를 분리한다.
8. Camera/LiDAR/Radar는 차량 상태 폐루프가 검증된 뒤 추가한다.

> 가장 먼저 확정할 것은 통신 프로토콜이 아니라 **차량 상태의 단일 권위자(authority)**와 **시간 진행 방식**이다.

> **후속(2026-07): 이 검토의 canonical + adapter 방향이 구현으로 이어졌다.** 정규 계약을
> [`EgoState.idl`](../../idl/dds/sils/EgoState.idl)·[`VehicleControl.idl`](../../idl/dds/sils/VehicleControl.idl)
> (SI 단위)로 확정하고, 채널맵 구동 vECU(`sils_vecu_vehicle_probe`)로 버스 결합 폐루프를
> 실증했다(S1 free-run). 여기서 말하는 **MoraiContractAdapter(외부 어댑터)**는 그 정규 타입으로
> 번역하는 anti-corruption layer로 유지된다(단위변환 deg→rad·km/h→m/s 소유). 설계·3안 비교:
> [sils-morai-plant-adapter-design.md](sils-morai-plant-adapter-design.md).

### 1.1 권장 구조

```text
                              Run Coordinator
                    ┌──────────────┴──────────────┐
                    │ 실행/준비/시나리오/종료 제어 │
                    │ MSC · gRPC · ROS service    │
                    └──────────────┬──────────────┘
                                   │ Control Plane
                                   ▼
┌────────────────┐ ROS2/DDS  ┌────────────────────┐  ROS2/DDS  ┌──────────────────┐
│ MMF SUT / ADSW │ ◀────────▶│ 선택적 Contract    │ ◀─────────▶│ 한양대 UDP–ROS2  │
│ K-Autoware     │           │ Adapter/Supervisor │            │ Bridge           │
└────────────────┘           └────────────────────┘            └────────┬─────────┘
                                                                      │ UDP
                                                                      ▼
                                                              ┌──────────────┐
                                                              │  MORAI SIM   │
                                                              │ Plant/Sensor │
                                                              └──────────────┘
```

한양대 Bridge가 MORAI UDP parsing과 ROS2 publication을 소유한다. 우리 SiLS는 이를 재구현하지 않고 외부 runtime component로 편입해 실행·readiness·계약·시험 결과를 관리한다. Bridge가 `morai_msgs`까지만 제공하거나 일부 K-Autoware 타입이 누락된 경우에만 MoraiContractAdapter가 semantic/단위/좌표 변환을 보완한다.

---

## 2. 연동 전에 반드시 결정할 사항

### 2.1 차량 동역학 소유권

가능한 구성은 두 가지다.

| 구성 | 동역학 소유자 | MORAI 역할 | 판단 |
|---|---|---|---|
| A. MORAI Plant | MORAI | 차량 물리, 환경, 센서 | **초기 연동 권장** |
| B. External Plant | MMF FMU/IPlant | 시각화, 환경, 센서 렌더링 | 동역학 FMU 검증이 목적일 때 사용 |

#### A. MORAI가 차량 동역학을 소유

```text
ADSW → steering / accelerator / brake / gear → MORAI
ADSW ← pose / velocity / acceleration / gear ← MORAI
```

장점은 MORAI의 차량 모델과 센서가 동일한 물리 상태를 사용한다는 점이다. 초기 폐루프 검증이 가장 단순하다.

#### B. MMF가 차량 동역학을 소유

MMF의 FMU/IPlant가 pose와 velocity를 계산하고 MORAI에는 계산 결과를 주입한다. 이 경우 MORAI의 Ego Ghost Mode 등 외부 차량 상태 주입 기능을 검토할 수 있다.

주의할 점:

- MORAI 자체 동역학과 외부 Plant를 동시에 활성화하지 않는다.
- pose를 주입하면서 accel/brake 명령도 MORAI에 보내지 않는다.
- 누가 collision response와 wheel state를 계산하는지 명시한다.
- sensor timestamp가 외부 Plant step과 일치하는지 검증한다.

Topology에는 최소한 다음 속성이 필요하다.

```yaml
simulator:
  kind: morai
  vehicle_state_authority: morai   # morai | external-plant
  time_mode: wall                  # wall | synchronous
```

### 2.2 연동 성공 기준

“토픽이 보인다”를 완료 조건으로 잡아서는 안 된다. 최소 성공 기준은 다음과 같다.

- Run Coordinator가 MORAI 준비 상태를 판정할 수 있다.
- 고정 시나리오를 load/reset하고 Ego 초기 상태를 재현할 수 있다.
- ADSW 명령 publisher는 한 개만 존재한다.
- control command timeout 시 안전 상태로 전환한다.
- 차량 상태에 source timestamp, receive timestamp, sequence가 존재한다.
- 좌표, yaw, 속도, 조향 단위의 golden sample 검증을 통과한다.
- MORAI 종료 또는 network 단절을 run failure로 판정하고 정리한다.

---

## 3. Control Plane과 Data Plane 분리

### 3.1 Control Plane

Control Plane은 낮은 빈도의 run orchestration을 담당한다.

- simulator 실행 및 연결 확인
- 맵/시나리오 로드
- Ego 차량 및 센서 설정
- 초기 위치, 날씨, 마찰 계수, traffic 설정
- reset, start, pause, resume, stop
- simulator process/status 확인
- run 종료 및 artifact 수집

MORAI의 MSC API, gRPC API 또는 ROS service 중 실제 제품에서 제공되는 인터페이스를 사용한다. 한 제품 문서에 기능이 존재하더라도 설치 버전과 라이선스에서 사용할 수 있다는 뜻은 아니므로 capability spike가 선행되어야 한다.

### 3.2 Data Plane

Data Plane은 실시간 제어와 상태/센서 전달을 담당한다.

| 방향 | 데이터 | 초기 단계 |
|---|---|---|
| MMF → MORAI | steering, accelerator/throttle, brake, gear | P0 |
| MORAI → MMF | pose, velocity, acceleration, yaw, gear | P0 |
| MORAI → MMF | GPS, IMU | P1 |
| MORAI → MMF | object list, traffic light | P1 |
| MORAI → MMF | Camera, LiDAR, Radar | P2 |

### 3.3 프로토콜 선택

| 프로토콜 | 적합한 용도 | 주요 고려사항 |
|---|---|---|
| ROS/ROS bridge | message 기반 상태·센서 연동 | MORAI 버전의 ROS1/ROS2 지원 범위와 custom `morai_msgs` 확인 |
| UDP | 제어, 상태, 고빈도 센서 | packet schema, sequence, loss, fragmentation, endian, port 관리 필요 |
| MSC/gRPC/API | 시나리오와 simulator lifecycle | 고빈도 센서 전달용으로 사용하지 않음 |
| CAN | ECU 수준 인터페이스 검증 | 지원 장비, DBC, 라이선스, timestamp 및 bus load 확인 후 도입 |

권장 조합은 **MSC/gRPC/ROS service를 Control Plane**, **ROS 또는 UDP를 Data Plane**으로 사용하는 hybrid 구성이다.

### 3.4 2024-12-09 협의 내용의 현재 해석

당시 MORAI 회신은 다음과 같다.

- Camera: 일반/Semantic/Instance는 `sensor_msgs/CompressedImage`, Depth는 `sensor_msgs/Image`
- RADAR: `morai_msgs/RadarDetections`
- LiDAR: `sensor_msgs/PointCloud2`
- GPS: `morai_msgs/GPSMessage`
- IMU: `sensor_msgs/Imu`
- ROS2/DDS: 공식 지원하지 않음

이 내용은 **당시 납품 버전의 기준선**으로 보존해야 하지만 현재 기능으로 단정하면 안 된다. 2026-07 확인 시 MORAI 공식 GitHub에는 `MORAI-ROS2_morai_msgs`와 ROS2 example 저장소가 공개되어 있고 `CtrlCmd`, `EgoVehicleStatus`, `GPSMessage`, `RadarDetections` 등의 ROS2 메시지가 존재한다.

따라서 현재 판단은 다음과 같다.

> 2024년 회신은 역사적 사실이지만, 대상 MORAI build의 ROS2 공식 지원·유지보수·QoS 호환성은 공급사에 다시 확인한다. 공개 ROS2 저장소가 있다는 것만으로 납품 제품이 K-Autoware와 직접 호환된다고 판정하지 않는다.

ROS2가 DDS를 사용하더라도 다음이 모두 맞아야 직접 통신할 수 있다.

- 동일 ROS2 message package와 정확한 schema/version
- 동일한 ROS2 type name/type hash 및 CDR serialization
- topic name과 namespace
- QoS reliability, durability, history, depth
- `ROS_DOMAIN_ID`, discovery 범위 및 RMW 구현 호환성
- timestamp와 frame convention

따라서 native ROS2를 사용할 수 있더라도 계약 검증 경계는 유지한다. 단, 한양대 Bridge가 동일 책임을 이미 검증했다면 별도 Adapter를 중복 구현하지 않고 Supervisor/Contract Validator만 둔다.

### 3.5 Sensor 데이터 포맷 Gap 분석

K-Autoware가 요구하는 센서 타입과 2024년 MORAI 회신을 비교하면 다음과 같다.

| Sensor | K-Autoware 요구 | MORAI 제공 | 판정 | Adapter 요구사항 |
|---|---|---|---|---|
| Camera | `sensor_msgs/msg/Image` | 일반/Semantic/Instance: `CompressedImage`, Depth: `Image` | **부분 불일치** | 압축 영상 decode 및 `Image` republish. encoding, step, endian, resolution과 timestamp 보존 |
| RADAR | `radar_msgs/msg/RadarTracks` | `morai_msgs/RadarDetections` | **의미·구조 불일치** | detection→track 변환 또는 tracker 필요. UUID, velocity, acceleration, size, classification, covariance 정책 확정 |
| LiDAR | `sensor_msgs/msg/PointCloud2` | `sensor_msgs/PointCloud2` | **타입 일치** | fields layout, frame, timestamp, intensity/ring, endian, dense 여부 검증 |
| GNSS | `sensor_msgs/msg/NavSatFix` | `morai_msgs/GPSMessage` | **타입 불일치** | latitude/longitude/altitude 변환, `NavSatStatus`, covariance 및 altitude datum 정책 필요 |
| IMU | `sensor_msgs/msg/Imu` | `sensor_msgs/Imu` | **타입 일치** | frame, 단위, gravity 포함 여부, orientation/각속도/가속도 covariance 검증 |

#### Camera

K-Autoware가 raw `Image`만 구독한다면 일반 RGB/Semantic/Instance 영상에는 decompression node가 필요하다. Depth만 `Image`라고 해서 일반 영상 요구가 충족되는 것은 아니다.

추가 계약:

- `encoding`: `rgb8`, `bgr8`, `mono8`, `16UC1`, `32FC1` 등
- compression format과 decode 색상 순서
- `CameraInfo` 제공 여부 및 calibration URL/hash
- optical frame convention
- source timestamp를 decode 시각으로 덮어쓰지 않기
- semantic/instance label palette와 class ID version

`CameraInfo`가 요구 목록에는 없지만 실제 perception pipeline이 intrinsic/extrinsic calibration을 사용한다면 필수 endpoint로 추가해야 한다.

#### RADAR

`RadarDetections[]`를 `RadarTracks[]`로 배열 이름만 바꾸는 변환은 잘못이다. `radar_msgs/RadarTrack`은 stable UUID, position, velocity, acceleration, size, classification과 covariance를 요구한다. MORAI detection에 이 정보가 모두 없거나 프레임 간 ID가 안정적이지 않다면 다음 중 하나를 선택해야 한다.

1. MORAI가 tracking된 object와 stable ID/covariance를 제공하도록 요청한다.
2. Bridge 또는 선택적 MoraiContractAdapter 뒤에 별도 radar tracker를 둔다.
3. K-Autoware 입력을 detection-compatible 타입으로 변경한다.

알 수 없는 covariance를 임의의 0으로 채우면 “오차 없음”으로 해석될 수 있다. unknown 정책을 K-Autoware 소비자와 합의해야 한다.

#### LiDAR

타입은 일치하지만 wire compatibility가 보장되는 것은 아니다. 최소한 다음 golden capture를 비교한다.

- `header.frame_id`와 TF tree
- `height`, `width`, `point_step`, `row_step`
- `fields`: x/y/z, intensity, ring, return type, timestamp offset
- float datatype과 byte order
- raw MORAI 좌표축과 ROS 변환 후 좌표축
- NaN 및 `is_dense` 처리

#### GNSS

공개 MORAI ROS2 `GPSMessage`에는 latitude, longitude, altitude와 map origin 기준 east/north offset이 있다. `NavSatFix`로 변환할 때 offset은 별도 local pose 계산에 사용하고 위경도 필드에 섞지 않는다.

주의할 점은 altitude 기준이다. MORAI 공개 메시지 설명은 sea level 기준이고 `NavSatFix`는 일반적으로 WGS84 ellipsoid altitude를 기대한다. geoid separation 처리 여부를 합의하지 않으면 수십 m 수준의 수직 오차가 생길 수 있다.

필수 정책:

- MORAI `status` → `NavSatStatus.status/service` mapping
- covariance 제공 여부와 `position_covariance_type`
- altitude datum: MSL 또는 WGS84 ellipsoid
- source timestamp와 GNSS frame
- east/north offset 및 map origin의 별도 전달 방식

#### IMU

타입이 같아도 다음 의미가 같아야 한다.

- angular velocity: rad/s
- linear acceleration: m/s²
- 중력 포함/제거 여부
- orientation quaternion의 축과 normalization
- covariance unknown 표현
- `imu_link`와 base_link 사이 static TF

### 3.6 Controller 및 Vehicle Status 포맷 Gap 분석

> 제공된 내용에는 controller 구조를 설명하는 스크린샷 파일명만 있고 이미지 자체는 현재 저장소에서 확인되지 않았다. 아래 mapping은 전달된 메시지 목록과 공개 MORAI/Autoware message definition을 기준으로 작성했으며, 스크린샷에만 존재하는 topic·port·제어 mode는 이미지 확보 후 보완해야 한다.

#### K-Autoware → Vehicle/MORAI

| K-Autoware 요구 | MORAI 측 후보 | 변환/정책 |
|---|---|---|
| `autoware_control_msgs/msg/Control` | `morai_msgs/CtrlCmd` | lateral tire angle → `front_steer`; longitudinal velocity/acceleration 중 MORAI `longl_cmd_type` 하나를 선택 |
| `autoware_vehicle_msgs/srv/ControlModeCommand` | MORAI control/event service | service request를 simulator control ownership으로 변환하고 실제 성공 확인 후 응답 |
| `tier4_vehicle_msgs/msg/VehicleEmergencyStamped` | adapter safety policy + `CtrlCmd` | emergency=true 시 기존 command를 차단하고 brake/stop safe command 적용 |
| `autoware_vehicle_msgs/msg/GearCommand` | MORAI event/gear control | enum mapping과 실제 gear state acknowledgment 필요 |
| `autoware_vehicle_msgs/msg/HazardLightsCommand` | MORAI lamp/event control | hazard 지원 여부와 report feedback 확인 |
| `autoware_vehicle_msgs/msg/TurnIndicatorsCommand` | MORAI lamp/event control | left/right/disable enum mapping과 report feedback 확인 |

현재 공개 MORAI ROS2 `CtrlCmd`는 다음 특성이 있다.

- `longl_cmd_type`: throttle(1), velocity(2), acceleration(3)
- `accel`, `brake`: 0~1
- `front_steer`, `rear_steer`: rad
- `velocity`: km/h
- `acceleration`: m/s²

따라서 `Control`의 velocity m/s를 MORAI velocity mode로 보낼 경우 km/h 변환이 필요하다. 반면 acceleration mode를 택하면 velocity target을 함께 적용할 수 있다고 가정해서는 안 된다. longitudinal mode는 topology/contract에 하나로 고정한다.

#### Vehicle/MORAI → K-Autoware

| K-Autoware 요구 | MORAI source 후보 | 변환/정책 |
|---|---|---|
| `tier4_vehicle_msgs/msg/ActuationStatusStamped` | `EgoVehicleStatus.accel/brake` | pedal ratio와 timestamp 변환. 실제 actuation인지 command echo인지 확인 |
| `autoware_vehicle_msgs/msg/ControlModeReport` | control service/status | 요청값 echo가 아니라 simulator가 수락한 실제 mode를 report |
| `tier4_api_msgs/msg/DoorStatus` | 별도 vehicle/event status | MORAI가 미지원하면 endpoint를 unsupported로 선언. 임의로 CLOSED 고정 금지 |
| `autoware_vehicle_msgs/msg/GearReport` | MORAI gear/event status | 실제 gear enum mapping |
| `autoware_vehicle_msgs/msg/HazardLightsReport` | MORAI lamp status | 실제 적용 상태를 report |
| `autoware_vehicle_msgs/msg/SteeringReport` | `EgoVehicleStatus.front_steer` | deg→rad 변환 및 steering tire/wheel 의미 확인 |
| `autoware_vehicle_msgs/msg/TurnIndicatorsReport` | MORAI lamp status | 실제 적용 상태를 report |
| `autoware_vehicle_msgs/msg/VelocityReport` | `EgoVehicleStatus.velocity/angular_velocity` | vehicle frame 투영, angular velocity deg/s→rad/s |

현재 공개 메시지에는 특히 주의할 단위 비대칭이 있다.

> `CtrlCmd.front_steer` 입력은 rad이지만 `EgoVehicleStatus.front_steer` 출력은 deg다. 또한 Ego angular velocity는 deg/s이고 Autoware `VelocityReport.heading_rate`는 rad/s다.

이 변환을 빠뜨리면 조향 폐루프와 yaw rate가 약 57.3배 틀어진다. 단위 변환은 코드 상수가 아니라 endpoint contract와 자동 golden test로 검증한다.

또한 제공된 `VehicleEmergencyStamped` 링크가 `ControlModeCommand.srv`를 가리키고 있으므로 문서 링크를 실제 TIER IV 메시지 경로로 교정해야 한다. 공개 메시지는 `stamp`와 `bool emergency`로 구성된다.

### 3.7 권장 Bridge 배치

대상 MORAI build가 ROS2를 공식 지원하는지에 따라 transport 전단만 바뀐다.

```text
[MORAI ROS1 build]
MORAI ROS1 topics → ROS1 ingress → Contract Adapter → ROS2 K-Autoware topics

[MORAI ROS2 build]
MORAI ROS2 topics ────────────→ Contract Adapter → ROS2 K-Autoware topics

[MORAI UDP build]
MORAI UDP packets → 검증된 UDP–ROS2 Bridge → [선택 Adapter] → K-Autoware
```

세 경우 모두 sensor/controller schema 변환, 단위·좌표 변환, freshness, watchdog과 diagnostics의 최종 소유자를 계약으로 명시한다. 한양대 Bridge가 담당하는 기능을 중복 구현하지 않되 SiLS가 종단 결과를 독립 검증한다.

### 3.8 한양대 UDP–ROS2 Bridge 적용 결론과 SiLS 역할

제공된 MORAI Network Settings 화면은 다음을 보여준다.

- Ego-0 network가 `Connected` 상태
- Command Control transport가 `UDP`
- Target Frame이 `50 Hz`
- Host/Destination IP가 개발 환경의 `127.0.0.1`
- 데이터 종류별 Host/Destination port 설정 구조
- 화면에 보이는 Collision Data 예시는 host `9091`, destination `9092`

따라서 검증 중인 실제 경로는 “MORAI native ROS2/DDS”가 아니라 다음 구조로 판단한다.

```text
MORAI Simulator ── UDP ──▶ 한양대 UDP–ROS2 Bridge ── ROS2/DDS ──▶ K-Autoware
                 ◀─────── controller command ◀──────────────────
```

화면의 localhost와 port는 개발 캡처의 예시이지 운영 계약값이 아니다. 분리 호스트나 container에 배치할 때는 topology가 실제 IP, port block, firewall 및 host networking을 생성해야 한다.

#### 역할 분담

| 구성요소 | 소유 책임 |
|---|---|
| MORAI | Ego 차량 동역학, 환경, collision, sensor 생성, UDP endpoint |
| 한양대 Bridge | UDP packet codec, ROS2 pub/sub, 연결/재연결, 지원하는 메시지 변환 |
| K-Autoware | perception/planning/control 및 K-Autoware vehicle interface |
| SiLS Run Coordinator | 전체 start/readiness/run/stop/cleanup과 pass/fail 판정 |
| SiLS Topology Compiler | host IP, UDP port, `ROS_DOMAIN_ID`, deployment, contract binding 검증 |
| SiLS Contract Catalog | topic/type/QoS/rate/frame/unit/timestamp/ownership 계약 |
| Bridge Supervisor | 외부 Bridge health, publisher/subscriber match, freshness, watchdog |
| 선택적 MoraiContractAdapter | Bridge가 제공하지 않는 K-Autoware 표준 타입과 semantic 변환만 보완 |
| Recorder/Diagnostics | rate/drop/latency/stale, rosbag/log/config/result artifact 수집 |

#### Bridge 재사용 Gate

Bridge가 다음 중 어디까지 제공하는지 인수 자료로 확정한다.

| Bridge 제공 범위 | 우리 SiLS 구현 범위 |
|---|---|
| K-Autoware 표준 Sensor/Controller/Status 전체 | Supervisor + Contract Validator |
| `morai_msgs` ROS2 topic까지만 제공 | Supervisor + MoraiContractAdapter |
| Sensor 표준 변환, Controller 미지원 | 별도 Vehicle Interface Adapter 추가 |
| 일부 endpoint만 지원 | 누락 endpoint만 선택적 adapter로 보완 |

Bridge 인수 필수 자료:

- 지원 MORAI build 및 ROS2 distribution/RMW
- UDP packet schema와 전체 port map
- ROS2 topic/type/direction/QoS/rate 목록
- frame/TF, 단위, timestamp 변환표
- sensor/controller/status 지원 매트릭스
- reconnect, sequence/drop, timeout, safe-stop 동작
- source/binary/container와 실행·종료 방법

#### 우리 SiLS의 핵심 역할

> 우리 SiLS는 MORAI UDP codec을 새로 만드는 시스템이 아니다. 검증된 Bridge를 외부 컴포넌트로 편입하고, 구성 생성, 계약 일치, readiness, 장애 감시, 시나리오 실행, 결과 판정과 재현 가능한 artifact를 책임진다.

초기 runtime 순서는 다음과 같다.

```text
Topology compile 및 IP/port/domain 검증
 → Bridge 기동 또는 외부 Bridge 연결 확인
 → MORAI UDP connected 확인
 → Bridge ROS2 필수 endpoint match/freshness 확인
 → K-Autoware 기동 및 control/status 폐루프 확인
 → 고정 scenario 실행과 pass/fail 판정
 → rosbag/Bridge log/MORAI config/result flush
 → K-Autoware/Bridge/MORAI stop 및 orphan cleanup
```

Bridge가 외부 업체 소유 프로세스여서 SiLS가 직접 기동할 수 없다면 `deployment: external`로 선언하고 Coordinator는 attach/readiness/health/abort notification만 수행한다.

---

## 4. 현재 MMF와의 접점 검토

### 4.1 `/VIN/Vehicle`은 MORAI 센서 입력 경로가 아님

현재 `config/sils/contracts/mmf-endpoints.yaml`의 `/VIN/Vehicle`은 VISS 클라이언트에서 서버로 보내는 `VissRequestTopic`이다. MORAI의 vehicle status나 sensor sample을 이 토픽에 publish해서는 안 된다.

```text
vECU VISS client → /VIN/Vehicle request → Vehicle Info Manager
```

MORAI 입력은 다음과 같이 별도 provider 경계로 들어와야 한다.

```text
MORAI UDP
      │
      ▼
한양대 UDP–ROS2 Bridge
      │ morai_msgs 또는 canonical K-Autoware ROS2
      ▼
[선택적 MoraiContractAdapter]
      │ canonical Autoware DDS/ROS message
      ▼
Vehicle Info Manager DDSProvider
      │
      ▼
VSS/VISS data
```

### 4.2 기존 DDSProvider 재사용 범위

현재 `service/core/vehicle_info_mgr/src/providers/DDSProvider.cpp`는 다음 Autoware 계열 토픽을 이미 구독한다.

- `rt/api/vehicle/kinematics`
- `rt/api/vehicle/status`
- `rt/api/perception/objects`
- `rt/perception/object_recognition/detection/objects`
- `rt/perception/traffic_light_recognition/traffic_signals`
- `rt/autoware_orientation`

따라서 Bridge가 canonical Autoware 타입을 직접 제공하면 기존 경로를 재사용한다. `morai_msgs`까지만 제공하는 endpoint는 선택적 MoraiContractAdapter에서 변환한다.

단, Camera/LiDAR/IMU 원본은 현재 DDSProvider가 모두 수용하는 구조가 아니다. VISS 변환이 필요한 semantic data와 ADSW가 직접 소비할 raw sensor data를 분리해야 한다.

### 4.3 권장 컴포넌트 구조

```text
service/sils/morai_integration/
  ├─ BridgeSupervisor         # external/managed bridge readiness/health
  ├─ ContractValidator        # topic/type/QoS/rate/frame 검증
  ├─ MoraiContractAdapter     # 선택: 누락된 canonical 변환만 구현
  ├─ CoordinateTransformer    # 선택: bridge 미지원 frame 변환
  ├─ TimeAndFreshnessMonitor  # timestamp/sequence/stale
  └─ MoraiHealthReporter      # run diagnostics
```

권장 설정 파일:

```text
config/sils/contracts/morai-endpoints.yaml
config/sils/topologies/morai-ego-smoke.yaml
config/sils/qos/morai-qos-profiles.yaml
```

---

## 5. 시간 동기화와 결정성

### 5.1 초기 단계: WallClock/live

현재 SiLS Phase 1과 맞는 초기 구성이다.

```yaml
run:
  input_mode: live

clock:
  mode: wall

simulator:
  determinism: best-effort
```

이 모드에서는 다음을 측정해야 한다.

- 실제 sensor publish rate
- command-to-state latency
- source/receive timestamp 차이
- packet/topic sequence gap
- real-time factor 저하
- stale 및 timeout 발생 횟수

### 5.2 synchronous/lockstep 도입 조건

MORAI 제품에서 다음 기능이 모두 검증된 경우에만 SiLS의 TickGrant/TickAck에 연결한다.

1. simulator를 확실히 pause할 수 있다.
2. 정확히 한 step 또는 지정한 `dt`만 진행할 수 있다.
3. 해당 step의 차량 상태와 모든 필수 센서 생성 완료를 알 수 있다.
4. 외부 계산 완료까지 다음 step이 진행되지 않는다.
5. 모든 출력에 동일한 simulation time 기준이 적용된다.
6. timeout 이후 reset/abort가 가능하다.

단순히 pause/resume API가 있다는 이유만으로 lockstep을 보장한다고 판단하면 안 된다. 센서 완료 barrier가 없다면 차량 상태와 Camera/LiDAR가 서로 다른 step에 속할 수 있다.

### 5.3 replay의 제한

MORAI 입력을 replay하더라도 live ADSW의 출력까지 동일해지는 것은 아니다. replay는 다음 용도로 범위를 명확히 한다.

- 입력 어댑터와 소비자 검증
- open-loop 알고리즘 회귀 시험
- 장애 재현
- SUT 출력까지 함께 기록했을 때의 완전 재생

---

## 6. 좌표계, 단위 및 데이터 계약

### 6.1 좌표계 계약

다음 값을 시나리오 설정과 함께 고정해야 한다.

- 지도 CRS 및 EPSG 코드
- UTM zone 또는 local ENU origin
- map, odom, base_link, sensor frame 관계
- X/Y/Z 축 방향
- yaw 0 기준, 증가 방향 및 범위
- quaternion ordering
- 좌표 변환의 책임 컴포넌트

MORAI 센서 출력은 센서 또는 전송 방식에 따라 축 해석이 달라질 수 있다. 예를 들어 raw Velodyne LiDAR 좌표와 ROS driver를 통과한 좌표를 같은 것으로 가정해서는 안 된다.

### 6.2 단위 계약

| 항목 | 반드시 확정할 내용 |
|---|---|
| 속도 | km/h 또는 m/s |
| 각도 | degree 또는 radian |
| 조향 | steering wheel angle 또는 road wheel angle, 좌/우 부호 |
| 가감속 | throttle/brake 비율 또는 acceleration target |
| pose | rear axle, CoG, base_link 중 기준점 |
| gear | P/R/N/D 및 수동 gear enum mapping |
| 시간 | Unix epoch, ROS time, simulation-relative time |

### 6.3 Endpoint Contract 필수 필드

```yaml
endpoints:
  - id: morai-ego-status
    direction: morai-to-mmf
    transport: udp
    source_schema: MoraiEgoVehicleStatus
    target_schema: autoware_adapi_v1_msgs/VehicleKinematics
    frame_id: base_link
    units:
      velocity: m/s
      yaw: rad
    timestamp: simulation-time
    expected_period_ms: 20       # 예시값, 실측 후 확정
    stale_after_ms: 100          # 예시값, 요구사항에 따라 확정
    late_policy: drop
    ownership: single
```

주기와 timeout은 문서 예제값을 그대로 복사하지 말고 실제 MORAI 설정, ADSW 요구사항, 네트워크 실측으로 확정한다.

---

## 7. 네트워크, 성능 및 병렬 실행

### 7.1 외부 호스트 연동

MORAI가 GPU가 있는 별도 호스트나 VM에서 실행되는 경우 다음을 topology 입력으로 관리한다.

- simulator host/destination IP
- NIC 및 routing
- UDP inbound/outbound port
- VM bridge/NAT 여부
- firewall rule
- ROS master/bridge 주소 또는 ROS2 discovery 범위
- MTU와 fragmentation 정책

MORAI 예제의 고정 포트를 여러 run이 공유하면 충돌한다. 병렬 실행을 지원하려면 run별 port block을 할당하고 회수해야 한다.

### 7.2 대용량 센서

Camera/LiDAR/Radar는 VISS JSON이나 범용 string payload로 운반하지 않는다.

| 데이터 | 권장 경로 |
|---|---|
| 차량 속도, 기어, 일반 신호 | DDS/ROS2 또는 VSS/VISS |
| 객체 및 신호등 semantic result | typed DDS/ROS2 |
| Camera, LiDAR, Radar raw | native ROS/UDP, 필요 시 shared memory/전용 recorder |
| run 결과, 설정, 로그 | artifact 저장소 |

성능 시험에는 평균 rate뿐 아니라 p95/p99 latency, drop, out-of-order, CPU, GPU, NIC 대역폭을 포함한다.

### 7.3 CI와 라이선스

다음은 기술 문제와 별도로 공급사 확인이 필요하다.

- headless 실행 지원 여부
- 하나의 license로 가능한 동시 simulator 수
- CI runner/VM에서 license 활성화 방식
- GPU 요구사항과 최소 사양
- 자동화 API 사용 가능 edition
- map/scenario asset의 배포 및 형상관리 권한

이 제약이 확인되기 전에는 domain ID나 container 수만으로 CI 병렬도를 산정할 수 없다.

---

## 8. 안전, 장애 처리 및 진단

### 8.1 제어 안전 규칙

- 제어 명령 publisher는 run당 하나만 허용한다.
- 수동 입력, MORAI 내부 제어, ADSW 제어의 우선순위를 명시한다.
- command timeout 시 throttle 해제와 brake 적용 등 합의된 safe command를 전송한다.
- adapter가 stale vehicle state를 정상 데이터로 전달하지 않는다.
- simulator reset 중에는 ADSW command를 차단한다.
- control mode가 변경되면 명시적인 상태 이벤트를 발생시킨다.

### 8.2 Bridge Supervisor/Adapter 진단 항목

```text
연결 상태            simulator/API/ROS/UDP connected
시나리오 상태        loaded/running/paused/failed
제어 상태            control owner, last command, command timeout
데이터 freshness     endpoint별 last source/receive timestamp
전송 품질            sequence gap, drop, out-of-order, decode failure
변환 상태            unit/frame/schema conversion error
시간 상태            clock skew, sim time regression, duplicate timestamp
성능 상태            actual rate, latency, queue backlog
```

Diagnostics Manager에는 raw 센서 데이터를 보내지 않고 위 상태의 요약과 원인을 표준 Component Health 계약으로 보고하는 것이 적절하다.

### 8.3 Coordinator 장애와 고아 simulator

Run Coordinator가 죽으면 MORAI가 실행된 채 남을 수 있다. 따라서 다음이 필요하다.

- run lease/heartbeat
- adapter의 coordinator-loss watchdog
- idempotent stop/reset
- 재시작 후 orphan run 탐색
- simulator process와 port allocation 회수
- 종료 실패를 별도 artifact 및 diagnostics로 기록

---

## 9. 시나리오 재현성과 형상관리

재현 가능한 run을 위해 아래 정보를 manifest와 artifact에 남긴다.

- MORAI 제품명, version/build, license/edition 식별 정보
- map 및 scenario ID/hash
- vehicle model과 동역학 설정 hash
- sensor configuration hash
- initial pose와 route
- traffic, weather, road friction
- random seed
- adapter build와 contract version
- MMF/ADSW image digest
- clock mode, `dt`, real-time factor
- endpoint IP/port/topic 설정

비결정적 live run은 동일 결과를 보장한다고 표현하지 말고 허용 오차 기반으로 판정한다.

---

## 10. 단계별 개발 권고

### Phase 0A — Capability Spike

코드를 본격 개발하기 전에 MORAI 실환경에서 다음을 증명한다.

- 설치된 제품/버전/edition 확인
- ROS1, ROS2, rosbridge, UDP, MSC, gRPC 실제 사용 가능 여부
- scenario load/reset/status 자동화
- simulator timestamp와 sequence 확인
- synchronous single-step 및 sensor barrier 가능 여부
- headless/병렬 실행과 license 조건
- Ego Ghost Mode 적용 범위
- 한양대 Bridge의 UDP packet/port map과 ROS2 topic/type/QoS/rate inventory
- Bridge가 K-Autoware canonical 타입까지 변환하는지 또는 `morai_msgs`까지만 제공하는지
- Bridge reconnect, stale, command watchdog 및 safe-stop 동작

산출물:

- MORAI capability matrix
- 샘플 packet/topic capture
- 좌표·단위 golden sample
- 확정된 초기 transport 결정서

### Phase 1 — Ego Control/Status Vertical Slice

범위:

- 차량 한 대
- 고정 맵과 고정 시나리오 한 개
- MORAI가 차량 동역학 소유
- steering/accelerator/brake/gear
- pose/velocity/gear
- WallClock/live
- 단일 run, 단일 simulator
- 한양대 Bridge 1개를 `external` 또는 managed runtime component로 등록
- Bridge Supervisor readiness 및 필수 ROS2 endpoint match

완료 조건:

- Coordinator가 ready/run/stop을 판정
- 직진, 정지, 좌/우 조향 golden scenario 통과
- 좌표와 단위 검증 통과
- command timeout과 simulator disconnect 시험 통과
- 동일 조건 반복 시 허용 오차 내 결과

### Phase 2 — Scenario 자동화 및 Semantic Data

- 시나리오 load/reset 자동화
- GPS/IMU
- object list와 traffic light
- recorder 및 artifact manifest
- adapter diagnostics
- port allocator 및 orphan cleanup

### Phase 3 — Raw Sensor 및 결정성 확장

- Camera/LiDAR/Radar
- 대역폭과 storage 최적화
- 다중 Ego/다중 simulator
- fault injection
- MORAI step API가 검증된 경우에만 synchronous/lockstep

---

## 11. 공급사에 확인할 질문

다음 질문은 MORAI 업체와의 기술 협의 초기에 확정해야 한다.

1. 사용할 정확한 제품명, version/build, edition은 무엇인가?
2. 해당 버전이 ROS2를 native로 지원하는가, ROS1/rosbridge가 필요한가?
3. 차량 제어·상태·각 센서의 권장 transport와 schema는 무엇인가?
4. MSC/gRPC/API의 사용 권한과 지원 범위는 무엇인가?
5. UI 없이 scenario load/reset/run/stop이 가능한가?
6. 외부 clock으로 정확히 한 simulation step을 진행할 수 있는가?
7. 한 step의 모든 센서 생성 완료를 확인할 API/event가 있는가?
8. Ego Ghost Mode에서 충돌, wheel, sensor pose는 어떻게 계산되는가?
9. map CRS, origin, EPSG 및 yaw convention을 API로 얻을 수 있는가?
10. simulator crash 또는 network 재연결 절차는 무엇인가?
11. 한 license에서 허용되는 병렬 instance 수는 몇 개인가?
12. CI/headless/VM/GPU 환경에 대한 공식 지원 범위는 무엇인가?

---

## 12. 주요 리스크 및 의사결정 Gate

| 등급 | 리스크 | 대응/Gate |
|---|---|---|
| Critical | MORAI와 MMF가 차량 동역학을 동시에 소유 | Phase 0A에서 authority 확정, topology validation 추가 |
| Critical | synchronous 기능을 추정해 lockstep 설계 | single-step + sensor barrier 실증 전 lockstep 금지 |
| High | ROS1/ROS2 및 custom message 호환성 | 설치 버전 기준 packet/topic capture와 mapping PoC |
| High | 좌표/yaw/조향 변환 오류 | golden pose 및 직진/회전 scenario 자동 검증 |
| High | Camera/LiDAR가 제어 경로를 압박 | raw sensor data plane 분리, rate/bandwidth budget |
| High | simulator/API 단절 시 제어가 유지 | watchdog, safe command, run abort/cleanup |
| Medium | 포트와 라이선스로 병렬 run 제한 | port allocator와 license concurrency 상한 명시 |
| Medium | vendor schema 변경이 core로 전파 | Bridge 계약 및 선택적 versioned mapper로 격리 |

### 최종 Gate

다음 네 가지가 확인되기 전에는 전체 센서 연동 개발로 확대하지 않는다.

1. 차량 동역학 authority 확정
2. WallClock 폐루프 안정성 증명
3. 좌표·단위 golden test 통과
4. scenario reset 및 장애 cleanup 자동화

### 12.1 SiLS v3 아키텍처 반영 필요사항

기존 `sils-virtual-ecu-architecture-v3`의 Topology Compiler, Run Coordinator, Contract Catalog, WallClock/live, readiness와 artifact 원칙은 그대로 유효하다. MORAI 연동 때문에 핵심 구조를 재설계할 필요는 없지만 다음 보강은 필요하다.

| v3 영역 | 보강 내용 |
|---|---|
| 전체 아키텍처 | `External Simulator`와 `External Bridge`를 SiLS Run의 1급 구성요소로 추가 |
| Topology | `managed | external` 배치, host IP, UDP port profile, ROS domain, simulator/bridge version 추가 |
| Plant 모델 | MORAI를 vECU 내부 `IPlant`가 아닌 external plant로 표시하고 `vehicle_state_authority` 명시 |
| Contract Catalog | UDP source packet ↔ Bridge ROS2 topic ↔ K-Autoware canonical type mapping과 변환 소유자 추가 |
| Readiness | process alive 외에 UDP connected, ROS2 endpoint match, 최초 fresh sample 조건 추가 |
| 격리 예산 | DDS domain뿐 아니라 run별 UDP port block과 외부 host 동시성 포함 |
| Coordinator | managed component는 start/stop, external component는 attach/health/abort notification 수행 |
| Clock | 초기 MORAI run은 `WallClock + live + 50 Hz target`이며 lockstep participant에서 제외 |
| 관측성 | Bridge packet drop/sequence, topic rate, source/receive time, command latency 추가 |
| Artifact | MORAI build, Bridge build, packet/port map, ROS2 inventory, sensor config hash 저장 |
| 단계 계획 | 기존 Phase 1 VISS vertical slice는 유지하고 MORAI는 후속 `External Simulator Vertical Slice`로 추가 |

수정하지 않아도 되는 v3 영역:

- TickInbox와 Relay Gateway의 DDS interposition 구분
- replay/open-loop 경계
- Coordinator lease와 orphan cleanup 원칙
- backend generated artifact 원칙
- 기존 Phase 1 scope gate

특히 한양대 Bridge가 있다는 이유로 v3의 `Relay Gateway`로 분류하면 안 된다. 한양대 Bridge는 기존 DDS peer 사이에 개입하는 relay가 아니라 **MORAI UDP protocol을 ROS2 endpoint로 변환하는 외부 protocol bridge**다.

---

## 13. 참고 자료

- [MORAI SIM Standard Manual](https://help-morai-sim-en.scrollhelp.site/morai-sim-standard-en/)
- [MORAI Simulator Control API](https://help-morai-sim-en.scrollhelp.site/morai-sim-standard-en/morai-simulator-control-api)
- [MORAI ROS Network Settings](https://help-morai-sim-en.scrollhelp.site/morai-sim-standard-en/ros-network-settings)
- [MORAI UDP Example Tutorial](https://help-morai-sim-en.scrollhelp.site/morai-sim-standard-en/morai-sim-udp-example-code-tutorial)
- [MORAI Sensors](https://help-morai-sim-en.scrollhelp.site/morai-sim-standard-en/sensors)
- [MORAI Sensor Coordinate System](https://help-morai-sim-en.scrollhelp.site/morai-sim-standard-en/sensor-coordinate-system)
- [MORAI Ego Ghost Mode](https://help-morai-sim-en.scrollhelp.site/morai-sim-standard-en/ego-ghost-mode)
- [MORAI ROS2 message repository](https://github.com/MORAI-Autonomous/MORAI-ROS2_morai_msgs)
- [MORAI ROS2 example repository](https://github.com/MORAI-Autonomous/MORAI-Example-ROS2)
- [Autoware Control message](https://github.com/autowarefoundation/autoware_msgs/blob/main/autoware_control_msgs/msg/Control.msg)
- [ROS 2 radar_msgs/RadarTracks](https://github.com/ros-perception/radar_msgs/blob/ros2/msg/RadarTracks.msg)
- [TIER IV VehicleEmergencyStamped](https://github.com/tier4/tier4_autoware_msgs/blob/tier4/universe/tier4_vehicle_msgs/msg/VehicleEmergencyStamped.msg)

> 외부 인터페이스 지원 여부는 MORAI 제품 버전과 라이선스에 따라 달라질 수 있으므로, 본 문서의 API 선택은 설치 대상 환경에서 다시 검증해야 한다.
