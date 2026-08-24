# B1-2 조사 — concealer 삽입 지점 (소스 근거)

> **소스 조사 (2026-08-07).** 대상: `tier4/scenario_simulator_v2` @
> `3370c817efc5458fbe69cc3c02a9bc8956cb8a81` ([pin](../../../docker/sils/ss2-upstream.pin)).
> 이 파일은 **소스에서 읽은 사실**과 **그로부터의 추론**을 구분해 적는다.
> 결정과 배선도는 [simulator-loops §4](../sils-simulator-loops.md).

---

## 0. 결론

**② 출력 경로(= `VehicleControl` 이 어떻게 ego 를 움직이나)의 답이 나왔다.**

> 우리 어댑터가 **`/control/command/control_cmd` 와 `/control/command/gear_cmd` 를 발행**하면 된다.
> 상류 patch 0, remap 도 2계단에서는 불필요.

그 근거는 **concealer 가 하나가 아니라 둘**이라는 것이다.

---

## 1. 가장 중요한 사실 — concealer 는 노드가 아니다

`external/concealer/CMakeLists.txt` 는 `ament_auto_add_library(concealer SHARED)` **하나뿐**이다.
실행파일도 `rclcpp_component` 도 없다.

따라서 **"concealer 가 Autoware 없이 뜨는가" 는 성립하지 않는 질문이었다.**
concealer 는 뜨는 것이 아니라 **다른 노드에 링크되는 라이브러리**다.

`package.xml` 의존성은 전부 **메시지 패키지**다 — `autoware_control_msgs`,
`autoware_vehicle_msgs`, `autoware_adapi_v1_msgs`, `tier4_*_msgs` …
**Autoware 런타임 패키지는 하나도 없다.** ([ss2-bringup §1](../sils-ss2-bringup.md) 의 주장 확인)

---

## 2. concealer 의 두 얼굴 ★

같은 라이브러리가 **서로 다른 두 노드에, 서로 다른 역할로** 링크된다.

| | 클래스 | 링크되는 곳 | 역할 | 우리에게 |
|---|---|---|---|---|
| **데이터 평면** | `AutowareUniverse` | `simple_sensor_simulator` | `control_cmd` → 차량모델, `/vehicle/status/*` 발행 | ✅ **필요** |
| **제어 평면** | `FieldOperatorApplication` | `traffic_simulator` (`EgoEntity` 의 **기반 클래스**) | Autoware 생명주기 · engage · routing · 상태 대기 | ⚠ **피하고 싶다** |

이 분리가 B1-2 의 답이다. **우리는 데이터 평면만 필요하고, 무거운 것은 전부 제어 평면에 있다.**

### 2.1 `AutowareUniverse` — 순수 어댑터 (경량)

`external/concealer/src/autoware_universe.cpp` 생성자:

```cpp
getCommand("/control/command/control_cmd", rclcpp::QoS(1), *this),
getGearCommand("/control/command/gear_cmd", rclcpp::QoS(1), *this)
```

`getVehicleCommand()` 는 그 두 구독의 최신값을 튜플로 낸다:

```cpp
std::make_tuple(control_command.longitudinal.velocity,
                control_command.longitudinal.acceleration,
                control_command.lateral.steering_tire_angle,
                sign_of(gear_command.command), gear_command.command)
```

| | |
|---|---|
| 구독 | `/control/command/control_cmd` · `/control/command/gear_cmd` · `…/path_with_lane_id` |
| 발행 | `/vehicle/status/*` (steering·gear·control mode·velocity·turn indicators) · `/localization/kinematic_state` · `/localization/acceleration` |
| **생명주기 handshake** | **없음 — 순수 topic→struct 어댑터** |

그리고 `simple_sensor_simulator` 의 `EgoEntitySimulation::update()` 가 이것을 그대로 쓴다:

```cpp
const auto [speed, acceleration, tire_angle, gear_sign, gear_command]
    = autoware->getVehicleCommand();
...
vehicle_model_ptr_->setInput(input);
vehicle_model_ptr_->update(step_time);
```

> **→ ROS 토픽 두 개를 발행하면 ego 차량모델이 움직인다.** 이것이 ② 의 답이다.

### 2.2 `FieldOperatorApplication` — Autoware 생명주기 (무겁다)

`EgoEntity` 의 **기반 클래스**다 (`simulation/traffic_simulator/src/entity/ego_entity.cpp`):

```cpp
EgoEntity::EgoEntity(...)
: VehicleEntity(name, entity_status, parameters),
  FieldOperatorApplication([&]() {
      return common::getParameter<bool>(node_parameters, "launch_autoware", true)
           ? concealer::ros2_launch(...)   // ← Autoware 프로세스를 실제로 띄운다
           : 0;                            // ← launch_autoware:=false 면 pid 0
  }())
```

- **`EgoEntity` 를 spawn 하면 기본적으로 Autoware 가 실행된다** (`launch_autoware` 기본 `true`).
- 소멸자는 그 pid 에 `SIGINT` 를 보낸다.
- `waitForAutowareStateToBe()` 는 **1초 간격으로 폴링하며 timeout 시 예외를 던진다.**
- 구독: `/autoware/state` · `/api/operation_mode/state` · `/api/routing/state` ·
  `/api/localization/initialization_state` · `/api/fail_safe/mrm_state` 등 ADAPI 전면.
- 서비스 클라이언트: engage · routing · localization 초기화 · operation mode.

> **→ 이쪽을 살리려면 Autoware 의 ADAPI 표면 전체를 흉내내야 한다.** 피해야 할 대상이다.

---

## 3. 08-07 실측이 여기서 설명된다

우리가 관측한 것: `traffic_simulation_demo` 의 `'ego'` 가 **type=1 (VEHICLE)** 이지 EGO 가 아니었다.

이제 이유가 명확하다 — **`EgoEntity` 로 spawn 하면 Autoware 가 뜨기 때문에** mock 시나리오가
`VehicleEntity` 로 회피한 것이다. "Autoware 가 없으면 ego 타입이 EGO 가 아니다" 는 증상이었고,
원인은 **spawn 함수 선택**이었다.

---

## 4. 배선 — 2계단과 3계단이 같은 이유

```
  3계단:  Autoware ──/control/command/control_cmd──▶ [AutowareUniverse]
                        (remap 으로 사슬을 끊고 우리가 중간에 들어감)        │
                                                                            ▼
  2계단:  fixture ─ControlTarget─▶ [vECU] ─VehicleControl─▶ [어댑터] ──▶ vehicle_model
                                                              │
                                              /control/command/control_cmd
                                              /control/command/gear_cmd
```

| | 2계단 | 3계단 |
|---|---|---|
| `ControlTarget` 출처 | fixture | Autoware (`control_cmd` → 어댑터) |
| **어댑터 → 시뮬레이터** | `control_cmd` 발행 | **동일** |
| remap 필요 | ❌ 경쟁자 없음 | ✅ Autoware 출력을 우리 쪽으로 |
| 상류 patch | 0 | 0 |

**하류 절반이 완전히 동일하다.** [§4 선정 기준](../sils-simulator-loops.md)을 만족한다.

### `/vehicle/status/*` 는 우리가 만들지 않아도 된다

`AutowareUniverse` 가 **발행 측**이다. 3계단에서 Autoware 가 기대하는 피드백은
시뮬레이터가 알아서 낸다 — 확인 항목 3 해결.

---

## 5. 남은 리스크 하나 — 런타임 시험으로만 판정 가능

`EgoEntitySimulation`(차량모델 + `AutowareUniverse`)은 **ego 를 spawn 해야** 생긴다.
그런데 `traffic_simulator` 쪽에서 ego 를 spawn 하면 `EgoEntity` → `FieldOperatorApplication`
도 **함께** 만들어진다. 둘은 "ego spawn" 으로 묶여 있다.

| | |
|---|---|
| **질문** | `launch_autoware:=false` 로 `EgoEntity` 를 spawn 했을 때 **블로킹 없이 동작하는가** |
| 낙관 근거 | `waitForAutowareStateToBe()` 는 `engage()`/`plan()`/`initialize()` 에서 불린다. 시나리오가 그걸 **호출하지 않으면** 블로킹 지점에 도달하지 않을 수 있다 |
| 비관 근거 | 생성자·`updateFieldOperatorApplication()` 경로에 상태 의존이 숨어 있을 수 있다 |
| 판정 방법 | `launch_autoware:=false` 로 ego 를 spawn 하는 mock 을 돌려 **행(hang) 하는지** 본다 |

### 5.1 ⚠ `cpp_mock_scenarios` 로는 이 시험을 할 수 없다 (정정)

처음에 `follow_polyline_trajectory_with_do_nothing_plugin` 을 "Autoware 없는 ego 경로" 의
정황으로 들었으나 **틀렸다.** 그 시나리오는 이렇게 spawn 한다:

```cpp
api_.spawn("ego", spawn_pose, getVehicleParameters(),
           traffic_simulator::entity::VehicleEntity::BuiltinBehavior::doNothing());
```

**이름만 `"ego"` 인 `VehicleEntity`** 다. `EgoEntity` 가 아니다.

더 결정적인 것은 `mock/cpp_mock_scenarios/CMakeLists.txt` 의 주석이다:

```cmake
# The scenarios recorded in this directory are commented out as they
# require EgoEntity and cannot be used for CI.
# add_subdirectory(src/respawn_ego)
```

**상류가 유일한 `EgoEntity` 목 시나리오를 빌드에서 뺐다** — "EgoEntity 를 요구해서 CI 에
쓸 수 없다" 는 이유로. 그래서 빌드되는 목 시나리오는 **전부 `VehicleEntity`** 이고,
우리 프로브가 `'ego'(type=1)` 을 본 것이 바로 이 때문이다(§3 의 설명을 이 사실이 확정한다).

> 상류의 이 주석은 **비관 쪽 근거**다. EgoEntity 가 Autoware 없이 쉽게 서는 것이라면
> CI 에서 못 쓸 이유가 없다. 다만 주석은 `launch_autoware:=false` 를 시도한 결과가
> 아닐 수도 있으므로, 확정은 아니다.

### 5.0 ★ 결론 — EgoEntity 는 곧 Autoware 다 (소스 확정)

`openscenario_interpreter/src/syntax/controller.cpp`:

```cpp
auto Controller::isAutoware() const & -> bool
{
  return properties.get<Boolean>("isEgo") or name == traffic_simulator::VehicleBehavior::autoware();
}
```

**메서드 이름이 `isAutoware()` 다.** 상류는 "ego 인가" 와 "Autoware 인가" 를 **같은 질문으로
취급한다.** `isEgo=true` 는 곧 "이 엔티티는 Autoware 가 몬다" 는 선언이다.

이것이 `EgoEntity : FieldOperatorApplication` 의 이유이기도 하다 —
*field operator* = 자율주행 스택. **EgoEntity 는 정의상 Autoware 가 모는 엔티티다.**

#### 정황이 아니라 일관된 증거 사슬이다

| 근거 | 내용 |
|---|---|
| `cpp_mock_scenarios/CMakeLists.txt` | 유일한 EgoEntity 시나리오를 빌드 제외 — *"require EgoEntity and cannot be used for CI"* |
| `scenario_test_runner/scenario/` 79개 `.yaml` | 확인한 `LateralAction.LaneChangeAction.yaml` 의 `"ego"` 에 **`isEgo` 없음** → VehicleEntity |
| `Controller::isAutoware()` | ego ≡ Autoware |

**상류의 CI 가능 시나리오 전체가 EgoEntity 를 회피한다.** 상류 자신도 Autoware 없이
EgoEntity 를 돌리지 않는다.

> **→ "Autoware 없이 EgoEntity 가 서는가" 의 답은 사실상 아니오다.**
> `launch_autoware:=false` 는 "Autoware 를 **따로** 띄웠다" 는 뜻이지
> "Autoware 없이 돈다" 는 뜻이 아니다.

#### 그래서 2계단은 최소 ADAPI stub 이 필요하다

다만 **§2 의 좋은 소식은 그대로 유효하다** — 무거운 것은 전부 제어 평면에 있고,
**데이터 평면(`AutowareUniverse`)은 handshake 가 없다.**

| | 필요한가 | 비용 |
|---|---|---|
| `AutowareUniverse` (control_cmd → 차량모델) | ✅ | **0** — 토픽 2개 발행 |
| `FieldOperatorApplication` 만족시키기 | ✅ | **최소 stub** — 상태 토픽 몇 개 |

stub 은 **Autoware 흉내가 아니라 블로킹 해제**가 목적이다. `waitForAutowareStateToBe()` 가
보는 `/autoware/state` 와 ADAPI 상태 몇 개만 그럴듯하게 내면 된다. 어댑터 밖 유지.

⚠ **정확한 최소 집합은 아직 모른다.** `FieldOperatorApplication` 의 어느 호출이 실제로
불리는지는 시나리오가 무엇을 시키느냐에 달렸고, 이는 **다음 조사 항목**이다.

### 5.2 시험대 — `openscenario_interpreter` (실행 결과 포함)

`.xosc` 경로(`scenario_test_runner`)는 **진짜 `EgoEntity` 를 spawn** 하고,
`scenario_test_runner.launch.py` 는 `launch_autoware`(기본 `true`)를 인자로 받는다.

`docker-compose-sils-ss2.yaml` 의 `ss2-scenario` 를 이 시험대로 정비했다 —
`launch_autoware` 기본값을 **`false`** 로 두고(우리에겐 Autoware 이미지가 없다),
`SILS_SS2_SCENARIO` 에 **절대 경로**를 줄 수 있게 했다.

#### 1차 시도 (2026-08-07) — 자산 선택 실패

`openscenario_interpreter/example/lane_change.xosc` 를 골라
(`tools/sils/ss2/make_ego_scenario.sh` 로 맵 경로만 치환) 돌렸다. 결과:

```
[scenario_test_runner.py-1] Shutdown interpreter.
sils-ss2-scenario exited with code 0
```

**5개 노드가 다 뜬 뒤 즉시 정상 종료.** 에러 없음. 판정 산출물은 `/out`(= `runs/ss2-scenario/`).

원인 후보 — **자산 선택이 틀렸다:**

| | |
|---|---|
| 그 파일은 유지보수되는 시나리오가 아니다 | 엔티티 이름에 개발자 실명(`"Tatsuya Yamasaki"`)이 들어 있고, `isEgo=true` 가 `ego` 와 `npc` **둘 다**에 붙어 있다 |
| 맵 경로를 손으로 고친 것 자체가 우회였다 | 정식 시나리오는 `$(find-pkg-share kashiwanoha_map)/map` **치환**을 쓴다. 상류가 제대로 하는 것을 우리가 어설프게 흉내낸 셈 |
| 정식 자산은 `.xosc` 가 아니다 | `test_runner/scenario_test_runner/scenario/` 에 **`.yaml` 79개, `.xosc` 0개** |

> **교훈**: 상류 트리에서 확장자만 보고 자산을 고르지 말 것. `example/`·`test/` 아래
> `.xosc` 는 파서 시험용이고, 실행되는 시나리오는 `.yaml` 로 따로 있다.
> ([B1-4](../sils-simulator-loops.md) 의 Scenario Format v2.0 경로가 바로 이것이다.)

다만 **§5.0 의 결론은 이 실패와 무관하다** — 그것은 소스에서 나왔고, 자산을 바꿔 다시
돌려도 EgoEntity 는 여전히 Autoware 를 요구한다.

### 막혔을 때의 대안

| | |
|---|---|
| **B** | `simple_sensor_simulator` 에 ego 만 spawn (`simulation_interface` 직접) — `traffic_simulator` 의 `EgoEntity` 우회 |
| **C** | `FieldOperatorApplication` 최소 stub (`/autoware/state` 등 몇 개만 발행) — 어댑터 밖 유지 |
| **D** | 상류 patch (W3 와 합류) — 최후 |

C 가 현실적인 fallback 이다. **전면 ADAPI 가 아니라 블로킹을 푸는 최소 집합**만 흉내내면 된다.

---

## 5.3 ★ stub 명세 (B1-2b 조사 결과, 2026-08-07)

"최소 집합이 무엇인지 모른다" 던 것이 소스에서 나왔다. **stub 은 상수 발행이 아니라
작은 상태 기계**다.

### 상태 8개

`undefined` · `initializing` · `waiting_for_route` · `planning` ·
`waiting_for_engage` · `driving` · `arrived_goal` · `finalizing`
(`concealer/legacy_autoware_state.hpp`)

### 상태를 읽는 길이 **두 개**다 — 하나만 되면 된다

| | 출처 | 언제 |
|---|---|---|
| **legacy** | `/autoware/state` 를 **그대로** | `architecture_type < awf/universe/20230906` |
| **adapi** | localization + routing + operation_mode **세 개에서 파생** | `>=` (핀 기본값 `20240605` → **이쪽**) |

adapi 파생 규칙:

```
셋 중 하나라도 UNKNOWN                                        → initializing
localization INITIALIZED + route SET/CHANGING
  + operation_mode AUTONOMOUS|LOCAL|REMOTE
  + is_autoware_control_enabled                               → driving
그 외                                                          → waiting_for_engage | planning
```

### 요구되는 전이 순서

| 호출 | 대기하는 전이 |
|---|---|
| `initialize()` | `undefined → initializing`(초기 pose 전송 후) `→ waiting_for_route` |
| `plan()` | `waiting_for_route → planning`(경로 전송 후) `→ waiting_for_engage` |
| `engage()` | `waiting_for_engage → driving`(engage 요청 후) |

`initialize_duration`(launch 인자, 기본 30초) 안에 못 오면 `AutowareError`.
서비스 요청은 30회 재시도.

### 그래서 stub 은 이렇게 생겼다 (adapi 경로)

**토픽 3개 발행 + 서비스 4개 응답.** 내부 플래그를 서비스 호출로 뒤집고, 그 플래그의
함수로 세 토픽을 낸다:

| 내부 | 기동 시 | `…/localization/initialize` 후 | `…/routing/set_route_points` 후 | `…/set/engage` 후 |
|---|---|---|---|---|
| localization | UNINITIALIZED | **INITIALIZED** | INITIALIZED | INITIALIZED |
| route | UNSET | UNSET | **SET** | SET |
| operation_mode | STOP / control off | STOP / off | STOP / off | **AUTONOMOUS / on** |
| **→ 파생 상태** | `initializing` | `waiting_for_route` | `waiting_for_engage` | **`driving`** |

`driving` 에 도달하면 concealer 가 더 이상 기다리지 않고, **`AutowareUniverse` 가
`/control/command/control_cmd` 를 받아 차량모델을 돌린다** — 그 토픽은 **우리 vECU 가 낸다.**

### ⚠ 더 싼 길이 있을 수 있다 — legacy 경로 강제

`architecture_type` 은 `"awf/universe"` 를 **부분문자열로만** 검사하고, 세대 구분은
`architecture_type >= "awf/universe/20230906"` **사전순 비교**다. 따라서
`architecture_type:=awf/universe/20230101` 을 주면 legacy 경로가 선택되고,
stub 은 **토픽 3개 → 1개**(`/autoware/state` 에 enum 하나)로 줄어든다.

> ⚠ **미검증.** 그 값이 상태 읽기 말고 무엇을 더 바꾸는지 확인하지 않았다.
> 3계단에서 진짜 Autoware 를 붙일 때는 실제 세대를 써야 하므로, **2계단 전용 지름길**로만
> 고려할 것 — 두 계단의 배선이 갈라지는 대가를 치른다.

---

## 6. 확인 항목 결산

| # | 확인 | 결과 |
|---|---|---|
| 1 | concealer 가 Autoware 없이 뜨는가 | ✅ **질문이 틀렸다** — 라이브러리이며 노드가 아니다. Autoware 런타임 의존 0 |
| 2 | 입력 토픽·타입, remap 가능한가 | ✅ `/control/command/control_cmd` · `/control/command/gear_cmd`, 평범한 ROS 2 구독 → remap 자유 |
| 3 | `/vehicle/status/*` 는 누가 만드나 | ✅ **`AutowareUniverse` 가 발행** — 우리 몫 아님 |
| 4 | Autoware 상태기계 의존 | 🟡 **역할별로 갈린다** — `AutowareUniverse` 없음 / `FieldOperatorApplication` 있음. §5 |
| 5 | mock 의 ego 구동과 충돌하나 | ✅ 설명됨 — mock 은 `VehicleEntity` 로 회피 중. `EgoEntity` 를 쓰면 경로가 갈린다 |

---

*작성 2026-08-07. 소스 인용은 전부 pin 커밋 `3370c817` 기준.*
