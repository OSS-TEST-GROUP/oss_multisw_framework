# `adapters/ss2` — scenario_simulator_v2 ↔ SiLS 부패 방지 계층 (양방향)

> **한 줄**: SS_v2 와 우리 정규 DDS 계약 사이를 **양쪽으로** 번역한다. 실행 파일 두 개:
>
> | | 방향 | 항목 |
> |---|---|---|
> | `ss2_ego_bridge` (§1–8) | SS_v2 상태 → 정규 `EgoState` | **B2-1** ✅ 1계단 검증 완료 |
> | **`ss2_control_bridge`** (**§9**) | 정규 `VehicleControl` → Autoware 제어 토픽 | **B2-3** — **루프를 닫는 조각** |
>
> 배경: [simulator-loops](../../docs/sils/sils-simulator-loops.md) ·
> 브링업: [ss2-bringup](../../docs/sils/sils-ss2-bringup.md) ·
> 실측 근거: [interop probe](../../docs/sils/evidence/ss2-interop-probe-2026-08-06.md)
>
> 상태 (2026-08-07): ✅ **검증 완료 — 1계단 종료.** SS_v2 → 어댑터 → 버스 → **vECU** 관통.
> V2(Fast DDS 2.6↔3.x interop) ✅ · V1(twist 프레임) ✅ **VEHICLE** · V3(클록) ✅ **SIM·50 ms 고정 스텝**.
> 판정 근거: [`ss2-stage1-verdict-2026-08-07.md`](../../docs/sils/evidence/ss2-stage1-verdict-2026-08-07.md).

```
  SS_v2 /entity/status ──▶ [ss2_ego_bridge] ──▶ sils/ego/state ──▶ vECU ──▶ trace ──▶ gate
       (ROS 2, rclcpp)                          (DDS, 정규 EgoState)
                                                                      제어 출력은 버려짐
```

---

## 1. 왜 어댑터가 필요한가 — 직접 구독이 안 되는 이유

같은 도메인·같은 RMW 여도 우리 vECU 는 SS_v2 토픽을 **못 받는다.** ROS 2 가 이름을 망글링하기
때문이다 ([실측](../../docs/sils/evidence/ss2-interop-probe-2026-08-06.md)):

```
  ROS 토픽  /entity/status      →  DDS 토픽  rt/entity/status
  ROS 타입  traffic_simulator_msgs/msg/EntityStatusWithTrajectoryArray
            →  DDS 타입  traffic_simulator_msgs::msg::dds_::EntityStatusWithTrajectoryArray_
```

우리 vECU 는 **순수 DDS 참가자**라 `sils/ego/state` + 타입 `EgoState` 를 본다. 접점이 없다.
그래서 [BR-C-2](../../docs/sils/sils-real-bridge-requirements.md) 가 브리지를 **rclcpp 노드**로
못박았다 — ROS 2 측을 rclcpp 로 받고 **네이티브 Fast DDS API 로 재발행**한다.

**이 어댑터는 `service/sils/` 밖에 있다.** 시뮬레이터가 바뀔 때 바뀌는 유일한 것이고, SiLS 는
그대로여야 하기 때문이다.

---

## 2. 매핑표

원천 (tier4/scenario_simulator_v2 master 소스 확인, 2026-08-07):

```
EntityStatusWithTrajectoryArray { EntityStatusWithTrajectory[] data }
  └ EntityStatusWithTrajectory  { float64 time, string name, EntityStatus status, ... }
      └ EntityStatus { EntityType type, float64 time, string name,
                       ActionStatus action_status, geometry_msgs/Pose pose, ... }
          └ ActionStatus { string current_action, Twist twist, Accel accel, float64 linear_jerk }
```

| 정규 `EgoState` | SS_v2 소스 | 변환 |
|---|---|---|
| `run_id` | — | 주입 (`SILS_RUN_ID`) |
| `seq` | — | 어댑터가 증가 |
| `stamp_ns` | `status.time` (float64 **초**) | ×1e9, **반올림** |
| `position` | `status.pose.position` | **1:1** (m, map) |
| `orientation_rpy` | `status.pose.orientation` (**쿼터니언**) | **quat → RPY** (intrinsic Z-Y-X) |
| `velocity` | `status.action_status.twist.linear` | **1:1** (m/s) ⚠ 프레임 §5 |
| `acceleration` | `status.action_status.accel.linear` | **1:1** (m/s²) |
| `yaw_rate` | `status.action_status.twist.angular.z` | **1:1** (rad/s) |
| `steering_angle` | **없음** ❌ | **합성** (§3) |
| `gear` | **없음** ❌ | **합성** (§3) |

### ★ 단위 변환이 없다는 것이 오히려 위험하다

SS_v2 는 이미 SI + 라디안(`geometry_msgs`)이라 변환이 **없다**. MORAI 어댑터가 deg→rad,
km/h→m/s, deg/s→rad/s 세 개를 바꿔야 하는 것과 대조된다.

그래서 더 위험하다 — **리뷰할 때 무서워 보이는 게 없다.** 쿼터니언 부호가 뒤집혀 있거나 ego 를
잘못 고르면 아무도 눈치채지 못한다. 그 부분을 `include/ss2_ego_mapping.hpp` 로 떼어내
**의존성 없이 단위 시험**하는 이유다(§6).

---

## 3. ⚠ SS_v2 가 주지 않는 두 필드 — `steering_angle` · `gear`

`EntityStatus` 에 **조향각도 기어도 없다.** Autoware/concealer 가 있으면
`/vehicle/status/*` 로 나오지만, 1계단은 Autoware 없이 돌므로 원천이 없다
(실측에서도 해당 토픽이 없었다).

정규 `EgoState` 는 두 필드를 요구한다. 어댑터는 **합성하고, 합성했다고 밝힌다.**

### 왜 NaN 이 아닌가 — NaN 은 이미 예약돼 있다

[`vehicle_input_fault.hpp`](../../service/sils/vecu/src/vehicle_input_fault.hpp) 가
`state.steering_angle = nan` 을 **결함 주입**으로 쓴다. 어댑터가 NaN 을 내면 주입된 결함과
구별되지 않고, `analyze_fault` 의 완전성 검사(선언 수 == 관측 수)가 **아무것도 주입하지 않은
런에서 실패**한다. 정직한 "값 없음"이 엉뚱한 판정으로 바뀌는 것이다.

### 대신 하는 것 세 가지

1. **문서화된 상수**로 채운다 — `steering_angle = 0.0`, `gear = 3`(DRIVE, vECU 자체 기본값과 동일)
2. 기동 시 **경고 배너**로 합성 목록을 찍는다 (`EgoStateProvenance::summary()`)
3. **plant id 로 격리한다** — 이 어댑터로 만든 런은 `--plant ss2-wire` 를 쓴다.
   [`compare_golden`](../../tools/sils/conformance/compare_golden.py) 의 `gate_plant()` 가
   다른 plant 의 golden 과의 비교를 **exit 2 로 거부**하므로, 합성값이 진짜 값처럼 golden 에
   섞이는 경로가 기계적으로 막힌다.

> **1계단 산출물로 fidelity 를 주장하지 말 것.** 열린 루프라 L0 이고, 두 필드는 합성이다.

---

## 4. 실행

```bash
# 1) 이미지 (SS_v2 이미지가 먼저 있어야 한다)
docker/sils/build-ss2.sh
docker/sils/build-ss2-adapter.sh

# 2) 시뮬레이터 (Autoware 없이)
SILS_DOMAIN=42 docker compose -f docker/compose/docker-compose-sils-ss2.yaml up ss2-mock

# 3) 어댑터
SILS_DOMAIN=42 docker compose -f docker/compose/docker-compose-sils-ss2.yaml up ss2-adapter

# 4) vECU 를 같은 도메인에 (SILS_CHANNELMAP=config/sils/channelmap/ego-min.yaml)
```

> **전용 도메인을 쓸 것.** 도메인 0 은 우리 스택의 다른 참가자와 공유되고 있었다
> ([실측 §도메인](../../docs/sils/evidence/ss2-interop-probe-2026-08-06.md)).

### 설정

| 변수 / 파라미터 | 기본 | 뜻 |
|---|---|---|
| `VISS_DDS_DOMAIN_ID` | `0` | 발행측 DDS 도메인 — **vECU 와 같아야** |
| `ROS_DOMAIN_ID` | `0` | 구독측 ROS 도메인 — **시뮬레이터와 같아야** |
| `SILS_RUN_ID` | `ss2-wire` | 트레이스 상관용 run id. ⚠ **호스트 환경이 새어 들어온다** — 아래 |
| `SILS_DURABILITY` | `transient-local` | **vECU 와 같아야** — 다르면 조용히 미매칭 |
| `SS2_EGO_NAME` | (빈값) | 비우면 첫 EGO 타입 엔티티. **1계단에서는 지정이 사실상 필수** — 아래 |
| `ss2_topic` (param) | `/entity/status` | |
| `sils_topic` (param) | `sils/ego/state` | |

> ⚠ **compose 는 호스트 환경변수를 그대로 받는다.** `${SILS_RUN_ID:-ss2-wire}` 는 호스트에 그
> 변수가 남아 있으면 **기본값을 쓰지 않는다.** 실제로 이전 `run_phase1.sh` 세션에서 남은
> `SILS_RUN_ID` 가 새어 들어와 run id 가 엉뚱하게 찍히는 것을 겪었다. `run_id` 는 트레이스
> 상관 키이므로, **golden 을 캡처하는 런에서는 명시적으로 넘길 것**:
> `SILS_RUN_ID=ss2-wire SILS_DOMAIN=42 docker compose ... up ss2-adapter`

### ★ Autoware 가 없으면 ego 는 EGO 타입이 아니다 (2026-08-07 실측)

`cpp_mock_scenarios` 를 붙였을 때 버스에서 관측된 엔티티:

```
'ego'(type=1), 'tom'(type=2), 'bob'(type=2),
'npc1'(type=1), 'npc2'(type=1), 'npc3'(type=1), 'obstacle'(type=3)
      [EntityType: 0=EGO 1=VEHICLE 2=PEDESTRIAN 3=MISC_OBJECT]
```

**이름이 `ego` 인 엔티티의 타입이 EGO(0)가 아니라 VEHICLE(1)이다.** SS_v2 에서 EGO 타입은
concealer(Autoware 어댑터)가 붙은 엔티티에만 부여되므로, Autoware 없이 도는 구성에서는 ego
역할 차량도 일반 차량으로 나온다.

| 구성 | ego 엔티티 타입 | 선택 방법 |
|---|---|---|
| **1계단 · B-det** (Autoware 없음) | **VEHICLE(1)** | **이름으로** — `SS2_EGO_NAME` 필수 |
| B-full (Autoware 포함) | EGO(0) | 타입으로 (기본값) |

```bash
SILS_SS2_EGO_NAME=ego SILS_DOMAIN=42 \
  docker compose -f ../../docker/compose/docker-compose-sils-ss2.yaml up ss2-adapter
```

> 이름 지정 시 **fallback 은 없다**(`select_ego`): 지정한 이름이 없으면 다른 엔티티를 대신
> 고르지 않고 건너뛴다. 다른 차의 상태를 ego 이름으로 발행하는 것보다 아무것도 안 내는 편이 낫다.

### 동작 확인

```
[ss2_ego_bridge] received=150 published=150 skipped_no_ego=0 write_failed=0
```

- `received=0` → 시나리오 미실행이거나 `ROS_DOMAIN_ID` 불일치
- `received>0, published=0` → ego 선택 실패. `SS2_EGO_NAME` 지정 필요
- `write_failed>0` → DDS 발행 실패 (도메인·타입 등록 확인)
- vECU 쪽 트레이스에 `state_received` 가 찍히면 **1계단 관통 성공**

---

## 5. 검증 항목 — ✅ 전부 판정됨 (2026-08-07)

`tools/sils/ss2/run_vecu_wire.sh` 한 번으로 셋 다 나온다.
원측정값·해석: [`ss2-stage1-verdict-2026-08-07.md`](../../docs/sils/evidence/ss2-stage1-verdict-2026-08-07.md).

| # | 항목 | 결과 | 근거 |
|---|---|---|---|
| **V1** | `action_status.twist` 가 차량 프레임인가 map 프레임인가 | ✅ **VEHICLE** | `lanechange_left` 에서 헤딩 0.166 rad 편차 지점의 `\|vy\|/\|v\| = 0.0000`. map 이면 `sin(0.166)=0.165` 였어야 함. **어댑터에 회전 변환 불필요** |
| **V2** | Fast DDS **2.6 ↔ 3.x** interop | ✅ **PASS** | distinct **513 샘플** 수신, `src_seq` 역행 0 |
| **V3** | `status.time` 이 시뮬 시각인가 | ✅ **SIM · 50 ms 고정 스텝** | stamp 가 34.05 s 에서 시작(epoch 아님) + 전 stamp 가 50 ms 배수 |
| **V4** | ~~`fastddsgen` 버전 정합~~ | ✅ 해결 | 아래 |

### V1 이 VEHICLE 로 나온 것을 왜 그대로 믿어도 되나

`vy` 가 항상 0 인 것은 **"어댑터가 y 를 안 채워서"** 일 수도 있었다. 전 구간을 확인했다:

| 단계 | 코드 |
|---|---|
| bridge | `e.twist_linear.y = d.status.action_status.twist.linear.y` |
| mapper | `s.velocity = ego.twist_linear` (1:1) |
| writer | `msg.velocity(MakeVec3(s.velocity))` |
| vECU | `e.vy = s.velocity().y()` |

그리고 **같은 벡터의 `vx = 10.0` 이 실려 왔다** — 채널은 살아 있고 `y` 만 0 이다.

> ⚠ **직선 시나리오로는 V1 을 판정할 수 없다.** 두 가설 모두 `vy ≈ 0` 을 예측한다.
> `traffic_simulation_demo` 는 직선이라 INCONCLUSIVE 였고, `lanechange_left` 가 답을 줬다.
> `analyze_wire.py` 는 이 경우를 통과로도 실패로도 집계하지 않는다 — 아직 열린 질문을
> 닫아버리는 것이 가장 나쁜 결과이기 때문이다.

### Fast DDS 버전 — 실측 (2026-08-07)

`sils-ss2-dev:humble` 안에서 확인한 값:

| 패키지 | 버전 | 함의 |
|---|---|---|
| `ros-humble-fastrtps` | **2.6.11** | Fast DDS **2.x** → CMake 타깃은 `fastrtps` |
| `ros-humble-fastcdr` | **1.0.29** | Fast CDR **1.0.x** — `include/fastcdr/cdr/` **없음** |

**생성기를 고정하는 것은 Fast DDS 가 아니라 Fast CDR 이다.** 생성 헤더가 Fast CDR 을 직접
include 하기 때문이다:

| Fast-DDS-Gen | 대상 |
|---|---|
| v4.x (`master`) | Fast DDS 3.x |
| v3.x | Fast DDS 2.10+ / **Fast CDR 2.x** |
| **v2.x** (`v2.5.3`) | **Fast DDS 2.6 / Fast CDR 1.0.x** ← 이 이미지 |

v3.x 생성물은 `#include <fastcdr/cdr/fixed_size_string.hpp>` 로 시작하는데 이 이미지에는
그 디렉터리가 없다. 증상이 colcon 로그 깊숙한 곳의 "no such file" 로 나와 **버전 문제로 안
보이고 의존성 누락처럼 보인다.** 그래서 [Dockerfile](../../docker/sils/Dockerfile.ss2-adapter)
에 **preflight** 를 넣어 자체 단계에서 잡고, 고칠 방법(`FASTDDSGEN_REF`)을 메시지에 적는다.

소스는 두 계열 모두에서 컴파일된다 — 헤더는 `__has_include`(`.h`/`.hpp`), `write()` 반환형은
오버로드 쌍(2.x `bool` / 3.x `ReturnCode_t`)으로 처리한다.

---

## 5.5 반복 빌드 — `docker build` 로 고치지 말 것

C++ 를 고칠 때 `docker/sils/build-ss2-adapter.sh` 를 반복하면 **왕복 한 번에 컴파일 에러가
하나씩만** 보인다. 실패한 레이어는 버려지므로 수정마다 전체 재빌드이고, 로그는 길어서 잘린다.

```bash
docker/sils/dev-ss2-adapter.sh     # 소스를 bind-mount 한 셸
#   build     colcon build, 컴파일러 출력 전체 (console_direct+)
#   run       브리지 실행
```

호스트에서 편집하고 셸에서 `build` 를 반복한다. **에러 목록이 한 번에 다 나온다.**
깨끗하게 컴파일되면 그때 이미지를 굽는다:

```bash
docker/sils/build-ss2-adapter.sh
```

> Dockerfile 은 `toolchain`(베이스+JDK+fastddsgen+IDL)과 `adapter`(소스+colcon) 두 스테이지로
> 갈라져 있고, dev 셸은 앞쪽만 쓴다. 어댑터 소스를 고쳐도 toolchain 은 다시 안 만든다.

### 첫 빌드에서 걸렸던 것 5가지 (2026-08-07)

같은 함정을 두 번 밟지 않도록 기록한다. 전부 **남의 빌드 레이아웃에 대한 가정**이 틀린 경우였고,
`docker build` 왕복 한 번에 하나씩만 드러나서 다섯 번 돌았다.

| # | 증상 | 진짜 원인 |
|---|---|---|
| 1 | gradle 이전에 실패 | `--depth 1` + submodule — clone 은 "성공"하고 submodule 만 빈다 |
| 2 | `Unable to access jarfile` | `scripts/fastddsgen` 이 jar 를 **자기 경로 기준**으로 찾는데 심링크를 해석하지 않는다 → 래퍼로 |
| 3 | `find_package(traffic_simulator_msgs)` 실패 | 상류 **runtime 스테이지가 cmake·헤더를 지운다.** 실행은 되지만 빌드는 불가 → dev 스테이지로 |
| 4 | `fastcdr/cdr/... no such file` | 생성기를 고정하는 것은 Fast DDS 가 아니라 **Fast CDR** (§Fast DDS 버전) |
| 5 | 컴파일 OK, **링크에서** undefined reference | `file(GLOB)` 이 configure 시점에 도는데 생성 디렉터리가 비어 있어 `.cxx` 가 빌드에 안 들어감 → `execute_process` 로 configure 시점 생성 |

**5번이 가장 위험한 종류다.** 전부 깨끗하게 컴파일된 뒤 링크에서만 죽어서 "소스가 누락됐다"가
아니라 "라이브러리가 없다"처럼 보인다.

**교훈**: C++ 를 `docker build` 로 고치지 말 것(§5.5), 그리고 버전 가정은 컨테이너 안에서
`dpkg -l` 한 줄로 먼저 확인할 것 — 그 한 줄이 세 번의 추측보다 빨랐다.

## 6. 단위 시험 — 어디서나 돈다

```bash
adapters/ss2/run_tests.sh      # C++20 컴파일러만 있으면 됨. ROS·DDS·시뮬레이터 불필요
```

`include/ss2_ego_mapping.hpp` 는 **ROS 2 의존도 DDS 의존도 없다.** 조용히 틀릴 수 있는 부분
(프레임·단위·쿼터니언 규약·ego 선택)만 순수 함수로 떼어냈기 때문에, 시뮬레이터 없는 노트북에서도
검증된다. 시험이 덮는 것:

- 쿼터니언 **왕복** + **부호 규약**(자기 역함수와의 왕복만으로는 전역 부호 뒤집힘을 못 잡는다)
- 정규화 안 된 쿼터니언 · 전부 0 · 짐벌락 → **NaN 을 내지 않는다**(§3 참조)
- ego 선택: 타입 기준 / 이름 기준 / **이름 지정했는데 없으면 fallback 금지**
- 초→나노초 **반올림**(절삭 아님)
- 통과 필드가 **스케일되지 않음**(누가 "친절하게" 변환을 넣으면 시험이 깨진다)

현재 **61 checks, 0 failures** (`-Wall -Wextra -Werror`).

---

## 7. 범위 밖 — 다음 단계

| | 무엇 | 선행 |
|---|---|---|
| ~~제어 되먹임~~ | ✅ **B2-3 구현 완료** → §9 | ~~B1-2~~ |
| **결정론** | barrier 참가자화 | **W3/B2-2** (SS_v2 frame 구동부 patch) — ★ V3 실측으로 **범위 축소**: 스텝을 새로 만드는 게 아니라 **이미 있는 50 ms 스텝의 트리거를 외부로** 돌리는 작업 |
| `/clock` 종속 | tick 브리지 | **B2-5/B2-6** ⚠ `/clock` 은 BEST_EFFORT — 유실 탐지 필수 |

**이 파일(`ss2_ego_bridge.cpp`)을 제어 경로로 확장하지 말 것.** 되먹임은 별도 실행 파일
`ss2_control_bridge` 다 (§9) — 둘은 독립적으로 고장 나고 진단 방법이 다르다.

---

## 8. 파일

| | |
|---|---|
| `include/ss2_ego_mapping.hpp` | 순수 매핑 (ROS·DDS 의존 없음) |
| `src/ss2_ego_bridge.cpp` | rclcpp 노드 + Fast DDS writer |
| `test/test_mapping.cpp` · `run_tests.sh` | 단위 시험 |
| `CMakeLists.txt` · `package.xml` | ament 패키지 |
| [`docker/sils/Dockerfile.ss2-adapter`](../../docker/sils/Dockerfile.ss2-adapter) | 컨테이너 |
| [`docker/sils/build-ss2-adapter.sh`](../../docker/sils/build-ss2-adapter.sh) | 빌드 |


---

## 9. `ss2_control_bridge` — 되먹임 (B2-3) ★ 루프를 닫는 조각

> 상태 (2026-08-10): ✅ **폐루프 검증 완료.** 실제 `EgoEntity` + 진짜 concealer 로
> ego 가 **10.00 m/s 로 주행**했고, 그 값은 vECU 의 `SILS_CTRL_SETPOINT` 과 일치한다 (§9.7).
> 순수 매핑 단위시험 22 checks.

```
  [vECU] ──VehicleControl──▶ [ss2_control_bridge] ──/control/command/control_cmd──▶ concealer::
           (DDS, 정규)                             ──/control/command/gear_cmd────▶ AutowareUniverse
              ▲                                                                          │
              └────────── EgoState ◀── ss2_ego_bridge ◀── /entity/status ◀────────── 차량모델
```

**이 노드가 돌기 전까지 ego 는 vECU 가 무엇을 계산하든 서 있는다.** 다음 프레임이 우리 ECU 의
판단의 결과가 되는 지점이 여기다.

토픽 두 개는 선택이 아니다 — `concealer::AutowareUniverse` 가 정확히 그 이름을 구독하고
([`autoware_universe.cpp`](../../docs/sils/evidence/ss2-concealer-survey-2026-08-07.md), 핀 `3370c817`),
거기서 **숫자 세 개만** 읽는다: `lateral.steering_tire_angle` · `longitudinal.velocity` ·
`longitudinal.acceleration`. **Autoware 는 필요 없다** — 이 데이터 평면은 handshake 없는
토픽→구조체 어댑터라서, 생명주기 쪽이 `autoware_stub`(2계단)이든 진짜 Autoware(3계단)든
같은 배선이 그대로 동작한다.

### 9.1 ★ 가속도 부호는 그대로 통과한다 — 뒤집으면 안 된다

가장 틀리기 쉬운 곳이고, **틀려도 DRIVE 시험은 전부 통과한다.**

시뮬레이터는 이렇게 계산한다 (`ego_entity_simulation.cpp:321`):

```cpp
input(0) = gear_sign * acceleration + acceleration_by_slope;   // gear_sign: REVERSE 면 -1
```

`input(0)` 은 **차체 프레임** 종방향 가속도(전방 +)이고, 우리 정규값은 **진행 방향 기준**
부호(+ 가속 / − 제동)다. REVERSE 를 대입해 보면:

| 정규 명령 | `input(0)` | 물리적 결과 |
|---|---|---|
| `+a`, REVERSE | `−a` | 차체 후방 가속 → **후진 가속** ✅ |
| `−a`, REVERSE | `+a` | 차체 전방 가속 → **후진 중 감속** ✅ |

두 규약이 이미 일치한다. `abs()` 나 부호 반전을 넣으면 **REVERSE 만 깨지고 DRIVE 는 멀쩡하다** —
그래서 `test_control_mapping.cpp` 가 시뮬레이터의 산술을 그대로 옮겨 놓고 **차가 어떻게
움직이는지**로 단언한다. 구조체 필드끼리 비교하는 시험은 규약 오류를 절대 못 잡는다.

> concealer 자신의 TODO 주석이 이 값을 "unsigned" 라고 부른다. 그 문장만 읽고 고치면 깨진다.

### 9.2 알 수 없는 기어는 **주행이 아니라 타행(coast)** 이다

`sign_of(NONE) == 0` 이라 가속도가 0 이 된다. 범위 밖 기어 바이트를 DRIVE 로 기본값 처리하면
**파싱 못 한 입력으로 차를 굴리게** 되고, 그렇게 목표에 도달한 시나리오는 SUT 가 얻지 않은
PASS 를 만든다. 그래서 `kNone` 으로 보내고 세어서 로그에 남긴다.

### 9.3 vECU 가 조용해지면 명령을 **거둔다** (watchdog)

concealer 는 마지막 명령을 **영원히 보관하고 매 프레임 적용한다.** 그래서 vECU 가 중간에
죽으면 ego 가 멈추는 게 아니라 **혼자 계속 달린다.** 그러다 목표에 도달하면 아무도 얻지 않은
PASS 가 기록된다.

`stale_command_timeout_s`(기본 1.0초, 0=끄기) 뒤에 **가속도만 0 으로** 보낸다. 조향과 기어는
**유지한다** — 핸들을 0 으로 꺾는 것도 하나의 명령이고, 지금 하려는 건 명령을 그만두는 것이다.

### 9.4 QoS — 독자는 가장 **약한** 쪽으로 요청한다

stub 이 발행 측에서 지키는 규칙의 **거울상**이다. 제공(offer)은 그보다 약한 모든 요청을
만족시키므로 쓰는 쪽은 **가장 강하게** 제공하고, 요청(request)은 같거나 강한 제공에만 물리므로
읽는 쪽은 **가장 약하게** 요청한다. 여기서 TRANSIENT_LOCAL 을 요청하면
`SILS_DURABILITY=volatile` 로 뜬 vECU 와 **조용히 미매칭**된다. 어차피 10초 전 제어 명령은
쓸 값이 아니다.

### 9.5 ⚠ `longitudinal.velocity = 0.0` — 함정

정규 `VehicleControl` 에는 속도 필드가 없다(IDL 이 종방향 모드를 가속도로 못박았다).
`*_STEER_ACC*` 차량모델은 이 값을 **무시**하고 ego 는 기본적으로 그쪽이다. 하지만
`*_STEER_VEL` 모델은 이걸 **지령 속도로 읽고 0 = 정지**다. 명령이 흐르는데 차가 안 움직이면
`vehicle_model_type` 을 먼저 보라. 기동 시 경고로도 찍는다.

없는 속도를 지어내지 않는 이유는 하나다 — **vECU 가 내리지 않은 명령을 만들어내는 것**이기 때문.

#### ★ 2026-08-10: 이 경고가 실제로 터졌다

첫 폐루프 실행에서 **모든 카운터가 완벽한데 `vx` 가 정확히 0** 이었다.

```
ss2-adapter  received=602 published=601 skipped_no_ego=1 write_failed=0
ss2-control  forwarded=755 published=755 stale_holds=0 bad_gear=0 matched=1
ego          vx max +0.000 m/s
```

원인은 시뮬레이터 쪽 기본값이었다:

```cpp
// ego_entity_simulation.cpp
const auto vehicle_model_type =
    common::getParameter<std::string>("vehicle_model_type", "IDEAL_STEER_VEL");
...
case VehicleModelType::IDEAL_STEER_VEL:
    input(0) = speed;            // = longitudinal.velocity → 우리는 0.0
```

**`vehicle_model_type` 을 직접 설정하는 곳은 없다.** 런처가
`<vehicle_model>_description/config/simulator_model.param.yaml` 에서 가져오는데,
`vehicle_model` 이 비어 있으면 그 단계를 통째로 건너뛴다 — 그게 상류 기본값이고,
`launch_autoware:=false` 에서는 아무도 채워 주지 않는다.

→ **`vehicle_model:=sample_vehicle`** (상류 patch 0).
`run_closed_loop.sh` 가 시작 전에 실제로 어떤 모델로 해석되는지 읽어서,
`*_VEL` 이면 **경고하고 진행**한다 — 그 조합에서 나온 "정지" 결과는 제어 경로에
대한 판정이 아니기 때문이다.

### 9.6 실행

```bash
SILS_DOMAIN=42 docker compose -f docker/compose/docker-compose-sils-ss2.yaml up -d ss2-control
```

| 파라미터 / 변수 | 기본 | 뜻 |
|---|---|---|
| `VISS_DDS_DOMAIN_ID` | `0` | **vECU 와 같아야** — 이 노드는 vECU 의 `out` 채널을 읽는다 |
| `ROS_DOMAIN_ID` | `0` | **시뮬레이터와 같아야** |
| `sils_topic` | `sils/ego/cmd` | 채널맵 `Vehicle.Control` 과 일치해야. 트리의 모든 채널맵이 이 이름을 쓴다 — 예외는 `steer-min.yaml`(`sils/ego/steer_cmd`, 다른 액추에이터) |
| `control_topic` | `/control/command/control_cmd` | concealer 구독 이름 |
| `gear_topic` | `/control/command/gear_cmd` | |
| `stale_command_timeout_s` | `1.0` | §9.3. `0` 이면 watchdog 끔 |

```
[ss2_control_bridge] forwarded=500 published=500 stale_holds=0 bad_gear=0 matched=1
```

- `matched=0` → vECU 가 우리 토픽에 안 쓰고 있다. 도메인·채널맵 확인
- `matched=1, forwarded=0` → vECU 는 떴는데 tick 을 안 돌거나 명령을 안 낸다
- `stale_holds>0` → vECU 가 중간에 멈췄다. **그 런의 결과를 믿지 말 것**


---

## 9.7 ✅ 폐루프 검증 (2026-08-10)

```bash
tools/sils/ss2/run_closed_loop.sh --bin-dir <probe 빌드 경로>
```

```
/simulation/entity/status        401 message(s)
entities on the bus: 'ego'(type=0)
vx first +10.000   vx last +10.000   vx max +10.000 m/s

ss2-adapter  received=707 published=706 skipped_no_ego=1 write_failed=0
ss2-control  forwarded=854 published=854 stale_holds=0 bad_gear=0 matched=1
```

### ★ 10.000 이라는 숫자가 "움직였다" 보다 강한 이유

vECU 의 `SILS_CTRL_SETPOINT` 기본값이 **정확히 10.0** 이다. P 제어기가 목표에 수렴해서
**유지하고 있다**.

상태 경로가 끊겨 있었다면 vECU 는 `vx=0` 을 영원히 읽고 `Kp*(10-0)=8.0 m/s²` 를 계속
명령해서 **속도가 발산**했을 것이다. 정확히 10 에서 멈춰 있다는 것은 오차가 0 으로
수렴했다는 뜻이고, 그건 제어기가 **속도를 볼 수 있어야만** 가능하다.

> 즉 이 한 숫자가 **양방향이 모두 살아 있다**는 증거다. 프로브가 이 논증을 판정문에
> 직접 출력한다 — "MOVING" 만으로는 되먹임 경로 하나만 증명된다.

### 여기까지 오는 데 깨진 것 넷 (전부 조용한 실패)

| # | 증상 | 진짜 원인 |
|---|---|---|
| 1 | `no VehicleControl` | 이미지가 B2-3 이전 — 실행 파일 자체가 없음 |
| 2 | `NOTHING RECEIVED` | **토픽 이름.** 런처마다 네임스페이스가 다르다 (§9.8) |
| 3 | `vx = 0.000`, 카운터는 완벽 | **`vehicle_model_type` 기본값이 속도 모델** (§9.5) |
| 4 | vECU 미기동 | 빌드 사본 3개 중 선택 필요 + `LD_LIBRARY_PATH` |

**넷 다 에러를 내지 않는다.** 카운터는 전부 건강했고, 로그에도 아무것도 안 남았다.
그래서 판정을 **시뮬레이터가 보고하는 ego 속도** 하나로 좁힌 것이다 — 파이프가 위조할 수
없는 유일한 지점이다.

## 9.8 ⚠ `/entity/status` 는 런처마다 이름이 다르다

`traffic_simulator` 는 `entity/status` 를 **상대 이름**으로 발행한다
(`entity_manager.hpp:70`). 그래서 발행 노드의 네임스페이스에 붙는다:

| 런처 | 노드 | 토픽 |
|---|---|---|
| `mock_test.launch.py` (1계단) | `cpp_scenario_node` — 네임스페이스 없음 | `/entity/status` |
| `scenario_test_runner.launch.py` (2계단) | `openscenario_interpreter_node` — `namespace="simulation"` | **`/simulation/entity/status`** |

**틀린 쪽을 구독하면 조용하다.** 구독이 그냥 매칭 안 되고 양쪽 다 아무 로그도 안 남긴다.

`SILS_SS2_STATE_TOPIC` 으로 넘긴다. compose 기본값은 **1계단 이름 그대로** — 그 판정은
공표된 산출물이라 계속 재현되어야 한다. `closed_loop_probe` 는 **두 이름을 모두 구독**해서
이 차이를 영구히 자가 진단으로 바꾼다.

> 제어 토픽은 `/control/command/control_cmd` 로 **절대 이름**이라 네임스페이스가 안 붙는다
> (`autoware_universe.cpp:28`). 되먹임 쪽은 처음부터 맞았다.
