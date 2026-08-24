# `autoware_stub` — Autoware 대역 (SiLS 루프 B **2계단**)

> 상태 (2026-08-10): ✅ **진짜 concealer 로 검증 완료.**
> [`run_ego_scenario.sh`](../../tools/sils/ss2/run_ego_scenario.sh) 가 실제 `EgoEntity` 를
> 띄웠고, concealer 가 이 stub 을 `initializing → waiting_for_route → waiting_for_engage →
> driving` 으로 걸었다 (§4.3). 상태 기계 단위시험 12건 · 스모크 12/12.
>
> 거기 오기까지 스모크가 못 잡은 두 가지가 깨졌다 — 서비스가 7개가 아니라 **10개 전부**
> 필요했고(§1.5), tier4 응답의 `status.code` 를 0 으로 박아둔 것이 잠복 버그였다(§3).
> 명세 근거: [`ss2-concealer-survey-2026-08-07.md`](../../docs/sils/evidence/ss2-concealer-survey-2026-08-07.md) §5.3
> (`scenario_simulator_v2` @ [pin](../../docker/sils/ss2-upstream.pin) `3370c817`).

```
  fixture ─ControlTarget─▶ [vECU] ─VehicleControl─▶ [ss2 어댑터]
                                                        │ /control/command/control_cmd
                                                        ▼
                              [autoware_stub] ····▶ AutowareUniverse ──▶ 차량모델
                               (이 패키지)              (SS_v2 안)
                                    ╰─ 생명주기만 응답. 제어 명령은 내지 않는다.
```

---

## 1. 왜 필요한가 — 2계단의 딜레마

| | |
|---|---|
| 2계단(B-det)의 목적 | **결정론 L3** — golden 회귀 기준선 |
| 그런데 | 비동기 제어기(Autoware)가 루프에 있으면 **결정론이 성립하지 않는다** ([BR-S-3](../../docs/sils/sils-real-bridge-requirements.md)) |
| 그런데 또 | ego 를 움직이려면 **`EgoEntity`** 가 있어야 한다 — 그것만이 차량모델과 `AutowareUniverse` 를 시뮬레이터에 만든다 |
| 그런데 또 | SS_v2 에서 **`EgoEntity` = Autoware** 다. `Controller::isAutoware()` 가 `isEgo` 를 읽는다 |

**"Autoware 없는 ego"가 필요한데 상류에는 그런 게 없다.** 이 패키지가 그 간극이다.

> **Autoware 시뮬레이터가 아니다.** 계획도 인지도 하지 않고 **제어 명령을 내지 않는다.**
> concealer 의 생명주기 질문에만 답해서 `waitForAutowareStateToBe()` 가 리턴하게 한다.
> 실제 주행 명령은 그 뒤에 **우리 vECU** 가 낸다.

3계단(B-full)에서는 이것을 **진짜 Autoware 로 교체**한다. 하류 배선은 그대로다.

---

## 1.5 ⚠ 상태를 안 바꾸는 서비스도 **필수**다 (2026-08-10, 실런으로 확인)

처음엔 서비스 7개만 제공했다. 나머지 3개(`velocity_limit`·`rtc_commands`·`rtc_auto_mode`)는
상태를 안 건드리니 생명주기 대역엔 필요 없다고 판단했다. **틀렸다.**

```
AutowareError: Service "/api/autoware/set/velocity_limit" is not ready after 180 seconds
```

`RoutingAction.AcquirePositionAction` — ego 하나, NPC 없음, RTC 없음 — 에서 죽었다.
concealer 는 열 개 **전부**에 대해 클라이언트를 만들고, 서버가 없을 때의 실패 방식은
**180초 침묵 후 throw** 다. 그래서 "이 기능은 우리가 안 쓰니까" 는 호출부에서 내릴 수 있는
판단이 아니다.

**모델하지 않는 동작이라도 응답은 해야 한다.** 답이 없을 때 concealer 가 하는 일은
거절이 아니라 행(hang) 이다.

---

## 2. 무엇을 흉내내는가 — 상태 기계

concealer 는 `initialize() → plan() → engage()` 를 부르며 **각 단계마다 상태 전이를 기다린다.**
그리고 최신 세대(`architecture_type >= awf/universe/20230906`, 핀 기본값 `20240605`)에서
상태는 토픽 하나가 아니라 **세 값에서 파생**된다.

| 내부 | 기동 시 | `localization/initialize` 후 | `routing/set_route*` 후 | `set/engage` 후 |
|---|---|---|---|---|
| localization | UNKNOWN | **INITIALIZED** | INITIALIZED | INITIALIZED |
| route | UNKNOWN | UNSET | **SET** | SET |
| operation_mode | UNKNOWN | STOP / off | STOP / off | **AUTONOMOUS / on** |
| **→ concealer 가 읽는 상태** | `initializing` | `waiting_for_route` | `waiting_for_engage` | **`driving`** |

### 여기서 틀리기 쉬운 것 (2026-08-07 상류 소스로 재확인 — ★ 앞판이 틀렸다)

**★ "상태를 건너뛰면 안 된다"는 틀렸다.** `waitForAutowareStateToBe` 는 등호가 아니라
**부등식**으로 판정한다:

```cpp
// concealer/field_operator_application.hpp:138
auto not_to_be = [&](auto current) {
  return from_state.value <= current.value and current.value < to_state.value;
};
while (not finalized.load() and not_to_be(getLegacyAutowareState())) { ... }
```

enum 이 `undefined < initializing < waiting_for_route < planning < waiting_for_engage <
driving` 순서라, **지나쳐 버린 상태는 그냥 조건을 벗어나 루프가 즉시 끝난다.**
그래서 우리 stub 이 `planning` 을 전혀 내지 않는데도 `engage()` 의
`waitForAutowareStateToBe(waiting_for_route, planning)` 는 막히지 않는다.

진짜로 지켜야 하는 것은 셋이다:

1. **`initialize()` 호출 시점의 상태가 `waiting_for_route` 이하여야 한다.** 그 switch 는
   `undefined`·`initializing`·`waiting_for_route` 만 받고 **나머지는 `AutowareError` 를 던진다.**
   UNKNOWN 시작이 안전한 진짜 이유가 이것이다(앞판이 적었던 "구독 전에 지나간다" 가 아니다).
2. **셋 다 구체적이어야 한다.** 하나라도 UNKNOWN 이면 파생 규칙이 `initializing` 에서
   단락되어 영원히 못 벗어난다.
3. ⚠ **`is_autonomous_mode_available` 이 하중을 받는다.** route SET + mode STOP 일 때
   `return is_autonomous_mode_available ? waiting_for_engage : planning;` 이다. false 로
   내면 `planning` 에 고정되고 `engage()` 가 거기서 기다리다 죽는다 — **bool 하나 때문에**.
   (진짜 Autoware 는 이걸로 "경로 계산 중"을 표현한다.)

막히면 concealer 는 `initialize_duration`(기본 30초)이 다 될 때까지 기다리다
`AutowareError` 를 던진다 — 원인을 가리키지 않는 30초짜리 행(hang)이다.

---

## 3. 설계 결정

### 상수를 하드코딩하지 않는다

```python
self._loc_const = {Localization.INITIALIZED: _const(LocalizationInitializationState, "INITIALIZED"), ...}
```

정수 값은 Autoware 세대마다 다를 수 있다. 틀린 정수를 박으면 **형식은 멀쩡한데 의미가 다른**
메시지를 내게 되고, concealer 는 우리가 이미 보고했다고 믿는 상태를 영원히 기다린다.
이름으로 읽어서 **없으면 기동 시점에 실패**시킨다 — 진단 비용이 비교가 안 된다.

### QoS 는 가장 강한 쪽으로 제공한다

`RELIABLE` + `TRANSIENT_LOCAL` + `KEEP_LAST(1)`.

RxO 규칙상 제공 RELIABLE 은 RELIABLE·BEST_EFFORT 독자를 모두 만족시키고, 제공
TRANSIENT_LOCAL 은 TRANSIENT_LOCAL·VOLATILE 독자를 모두 만족시킨다. concealer 가 무엇을
요청하든 매칭된다. **RxO 불일치는 조용하다** — 독자가 그냥 매칭 안 되고 양쪽 다 아무 로그도
남기지 않는다. 1계단에서 이미 당했다.

TRANSIENT_LOCAL 은 덤으로, 늦게 붙은 구독자가 다음 발행 주기를 기다리지 않고 **현재 상태를
즉시** 받게 한다.

### 응답은 모양에 관대하게 채운다

두 서비스 계열의 응답 형태가 다르다 — `autoware_adapi_v1_msgs` 는 `status.success`,
`tier4_external_api_msgs` 는 `status.code`. **있는 필드를 채운다.** 검증하지 않은 모양을
단정하지 않기 위해서다.

⚠ **`code` 에 0 을 박으면 안 된다** (2026-08-10 정정). concealer 는 tier4 계열을

```cpp
return response->status.code == tier4_external_api_msgs::msg::ResponseStatus::SUCCESS;
```

로 검증하는데 `SUCCESS == 0` 이라는 보장이 없다 — 이 계열의 코드는 열거형이라 0 은 "ok" 보다
"unknown" 일 가능성이 높다. 틀렸다면 `/api/external/set/engage` 가 30회 거부당한 뒤 죽는다.
그리고 **우리 스모크는 코드를 검증하지 않으므로 이 버그를 영원히 못 본다** — 우리가 concealer
역할을 대신할 때 concealer 보다 관대하면, 통과는 아무것도 증명하지 않는다.
`_const` 와 같은 규칙으로 **이름으로 읽는다**.

`CooperateCommands` 만 `status` 가 없다. concealer 는 `responses` 원소가 전부 성공인지
보는데, 우리가 돌려주는 빈 벡터에 대해 `std::all_of` 는 참이다.

### 서비스 재호출은 무해해야 한다

concealer 는 서비스 요청을 **최대 30회 재시도**한다. 모든 전이가 멱등이다.

---

## 4. 실행

### 4.1 단독 스모크 — 시뮬레이터 없이 (GPU 불필요)

```bash
tools/sils/ss2/smoke_autoware_stub.sh
```

시나리오도 Autoware도 없이 **stub 만** 띄우고, 스크립트가 concealer 역할을 대신해
`initialize → set_route → engage` 를 손으로 호출하며 매 단계 **와이어에 실제로 올라온 값**을
확인한다. 단위시험이 못 덮는 구간 — 이 이미지에 메시지 패키지가 있는지, `_const()` 가 이름으로
찾는 상수가 남아 있는지, QoS 가 물리는지, 서비스가 응답하는지 — 이 전부가 여기서 갈린다.

```
[1/4] 메시지 패키지·상수      ← _const() 가 기동 시 죽을지 여부. 가장 큰 미지수
[2/4] 도메인이 비어 있는지    ← 남의 /api/routing/state 를 우리 값으로 읽는 사고 방지
[3/4] stub 기동
[4/4] 순서 주행               ← 단계마다 raw 값 대조
```

전용 도메인 **91** 을 쓴다 (42 아님). 이 시험은 버스에서 읽은 값으로 판정하므로, 진짜 Autoware나
이전 stub이 같은 토픽을 발행하고 있으면 결과가 조용히 오염된다.

```bash
tools/sils/ss2/smoke_autoware_stub.sh --check-only   # 사전점검만, 아무것도 안 띄움
tools/sils/ss2/smoke_autoware_stub.sh --keep         # stub 남겨두고 직접 찔러보기
```

> 이건 **부품 시험**이다. concealer 가 이 stub 을 받아들이는지는 `EgoEntity` 가 있어야 하고,
> 그건 `isEgo=true` 시나리오 자산(B1-4)이 필요하다. 여기 PASS 는 "설계대로 동작한다"까지다.

#### 2026-08-07 실행 결과 — 9/9 PASS

`sils-ss2:humble`, 도메인 91, 컨테이너 2개(stub·프로브). 핵심 4단계 + 나머지 5단계 전부 ok.

```
boot                     UNKNOWN/UNKNOWN/UNKNOWN        → initializing
initialize_localization  INITIALIZED/UNSET/STOP         → waiting_for_route
set_route_points         INITIALIZED/SET/STOP           → waiting_for_engage
engage                   INITIALIZED/SET/AUTONOMOUS·on  → driving
── 이하: happy path 밖 ──────────────────────────────────────────────
change_to_stop           driving 에서 탈출              → waiting_for_engage
enable_autoware_control  concealer 의 대체 engage 경로   → driving
clear_route                                             → waiting_for_route
set_route (points 아닌 쪽)                              → waiting_for_engage
set_route 재호출          변화 없음                      → waiting_for_engage
```

**이미지에 대해 알게 된 것 두 가지** (stub 로직과 무관한, 이미지의 성질):

| | |
|---|---|
| 메시지 세대 일치 | `autoware_adapi_v1_msgs`·`tier4_external_api_msgs` 가 있고 `_const()` 가 이름으로 찾는 상수가 **전부** 남아 있다. 이게 이 작업의 최대 미지수였다 |
| legacy 경로도 가용 | 사전점검이 `note: legacy path unavailable` 을 **안 찍었다** → `autoware_system_msgs` 도 있다. `-p legacy_state_topic:=true` 가 이 이미지에서 된다 (권장하진 않는다, §5) |

**두 독립 관측이 일치했다.** stub 로그(자기가 발행했다고 믿는 것)와 프로브(실제로 도착한 것)가
9단계 모두 같다. 어긋났다면 상태 기계가 아니라 **와이어**가 문제라는 뜻이었을 것이다 —
그래서 래퍼가 둘을 나란히 찍는다.

멱등성은 stub 로그에 **줄이 없는 것**으로 확인됐다. `_on()` 은 파생 상태가 바뀔 때만 INFO 를
찍으므로, 마지막 `set_route` 재호출에 해당하는 줄이 없다는 것 자체가 "아무것도 안 바뀌었다" 는
독립 증거다. concealer 는 요청을 최대 30회 재시도하므로 이게 성립해야 한다.

### 4.3 ✅ 진짜 concealer 와의 검증 (B1-4, 2026-08-10)

```bash
tools/sils/ss2/run_ego_scenario.sh
```

스모크와 달리 **우리가 concealer 역할을 대신하지 않는다.** 진짜 `EgoEntity` 를 띄우고
진짜 concealer 가 이 stub 을 부른다. 결과:

```
t+0.0   derived autoware state -> initializing
t+6.9   initialize_localization:  initializing      -> waiting_for_route
t+6.9   set_route_points:         waiting_for_route -> waiting_for_engage
t+6.9   engage:                   waiting_for_engage -> driving
```

세 전이가 0.1초 안에 끝난다. **`EgoEntity` 는 Autoware 없이 존재할 수 있다** — 상류 patch 0,
`launch_autoware:=false`, 우리 stub 만으로.

여기 오기까지 스모크가 통과시킨 것 둘이 깨졌다(§1.5, §3). 그게 이 시험의 존재 이유다:
**우리가 concealer 흉내를 낼 때 concealer 보다 관대하면, 스모크 통과는 아무것도 증명하지 않는다.**

> 이 시점 이후 시나리오는 여전히 **목표에 도달하지 못하고 타임아웃**한다. 정상이다 —
> `/control/command/control_cmd` 를 내는 쪽이 아직 없어서 ego 가 서 있는다. 그게 B2-3 다.

---

### 4.2 루프 안에서 (B1-4 · B2-3 이후)

```bash
# 시뮬레이터 (EgoEntity 를 쓰는 .xosc/.yaml, launch_autoware:=false)
docker compose -f docker/compose/docker-compose-sils-ss2.yaml up -d ss2-scenario

# 대역
ros2 run autoware_stub autoware_stub

# 구세대 경로를 쓰려면 (아래 §5)
ros2 run autoware_stub autoware_stub --ros-args -p legacy_state_topic:=true
```

로그로 진행을 본다:

```
derived autoware state -> initializing
initialize_localization: initializing -> waiting_for_route
set_route_points: waiting_for_route -> waiting_for_engage
engage: waiting_for_engage -> driving      ← 여기 도달하면 ego 가 몰릴 준비 완료
```

**`driving` 이 안 나오면** 어느 서비스에서 멈췄는지가 마지막 줄에 그대로 있다.

---

## 5. ⚠ 미검증 / 알려진 한계

| | |
|---|---|
| **concealer 수용 여부 미검증** | 스모크(§4.1)는 **우리가 concealer 흉내를 낸 것**이다. 진짜 concealer 는 `EgoEntity` 와 함께만 나타나고, 그건 B1-4 를 기다린다. 이게 남은 가장 큰 미지수다 |
| 부하·장시간 미검증 | 스모크는 몇 초짜리다. 시나리오 길이만큼 5Hz 로 계속 발행할 때의 거동은 안 봤다 |
| `legacy_state_topic` 경로 미실행 | 이미지에 `autoware_system_msgs` 는 있지만(§4.1) 그 경로로 띄워본 적은 없다 |
| 서비스 목록이 충분한지 | concealer 는 `feature` 등급 서비스(RTC·velocity limit 등)도 갖고 있다. 시나리오가 그걸 부르면 응답이 없다 — [`check_autoware_adapi.sh`](../../tools/sils/ss2/check_autoware_adapi.sh) 의 표가 전체 목록이다 |
| `legacy_state_topic` | `architecture_type < awf/universe/20230906` 일 때의 경로. 토픽 1개로 줄지만 **2계단과 3계단의 배선이 갈라진다** — 지름길로만 쓸 것 |
| 파생 규칙 동기화 | `state.py` 의 `derive()` 는 상류 `legacy_autoware_state.hpp` 를 **손으로 옮긴 것**이다. 상류가 바뀌면 조용히 낡는다. 그래서 와이어에 나가는 값은 이것에 의존하지 않고, 로그·시험에만 쓴다 |

---

## 6. 단위 시험

```bash
python3 -m pytest adapters/autoware-stub/test -q
```

ROS 없이 돈다 (`state.py` 에 ROS import 가 없다 — `ss2_ego_mapping.hpp` 와 같은 이유).
현재 **12 passed**.

---

## 7. 파일

| | |
|---|---|
| `autoware_stub/state.py` | 순수 상태 기계 (ROS 의존 없음) |
| `autoware_stub/stub_node.py` | rclpy 배선 |
| `test/test_state.py` | 단위 시험 |
| `package.xml` · `setup.py` | ament_python |
