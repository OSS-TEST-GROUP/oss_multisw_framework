# Autoware 파티션 이미지 — 벤더 요청

> 대상: `ghcr.io/oss-test-group/autoware-partition:adsw-{control,decision,perception}`
> 작성 2026-08-11 · 근거는 전부 **이미지 안에서 직접 읽은 것**이다 (재현 명령은 각 항 말미)
> 배경: [`evidence/autoware-return-path-2026-08-11.md`](evidence/autoware-return-path-2026-08-11.md) §7~§10

요청은 **한 건**이고, 그 한 건이 막고 있는 것은 **SS_v2 시나리오 기반 폐루프 하나**다.
그 밖의 것은 우리가 이미 해결했거나 스스로 해결할 수 있다 — 아래 §3 에 명시한다.
요청을 최소로 유지하기 위해, "무엇이 필요한가" 가 아니라 **"이미지의 어느 파일 몇 줄인가"**
로 적는다.

---

## 1. 요청 — `/localization/initialize` 서비스에 응답하는 것이 없습니다

### 증상

SS_v2(scenario_simulator_v2)의 `concealer` 가 자차를 spawn 할 때 실패합니다.

```
AutowareError: Requested the service "/api/localization/initialize" 30 times,
               but was not successful.
```

30회는 재시도 횟수이고, 매회 **서비스는 응답합니다.** 응답 내용이 `status.success = false`
입니다. 즉 "서비스가 없다" 가 아니라 **"서비스는 광고되는데 뒤에 구현이 없다"** 입니다.

### 원인 (이미지 안에서 확인)

`/api/localization/initialize` 는 `libautoware_default_adapi.so` 가 광고하고, 내부적으로
**`/localization/initialize`** 서비스로 넘깁니다. 그 내부 서비스의 서버는 원래
`autoware_pose_initializer` 인데, **이 이미지에는 설치돼 있지 않습니다.**

```
/opt/autoware/share/autoware_pose_initializer   → 없음
/opt/autoware/share/autoware_ekf_localizer      → 없음
```

없는 것이 정상입니다 — 이 파티션은 `planning_simulation` 전용이고
`obigo_launch/launch/planning_simulator.launch.xml:75` 가 `launch_localization` 을
**`value=`(하드코딩)** 으로 `false` 에 묶어두었습니다. 런치 인자로 되돌릴 수 없습니다.

그래서 귀사는 그 자리를 shim 으로 메우셨고, 저희는 그것을 사용하고 있습니다:

```python
# /opt/autoware/lib/obigo_launch/planning_sim_localization_state.py
#   subscribes /initialpose3d  →  state = INITIALIZED
#   publishes  /localization/initialization_state (TRANSIENT_LOCAL, 1 Hz)
```

**이 shim 은 토픽 경로만 덮고, 서비스 경로는 덮지 않습니다.** 사람이 RViz 나
`ros2 topic pub` 으로 초기위치를 찍는 경우에는 충분하지만, **concealer 는 토픽이 아니라
서비스로 초기화**하므로 그대로 실패합니다.

### 요청 (둘 중 하나, 위쪽이 훨씬 쌉니다)

**A. `planning_sim_localization_state.py` 에 서비스 서버를 추가** — 같은 파일, 약 15 줄

```python
from autoware_adapi_v1_msgs.srv import InitializeLocalization

self.create_service(InitializeLocalization, "/localization/initialize", self._on_initialize)

def _on_initialize(self, request, response):
    self._state = LocalizationInitializationState.INITIALIZED
    self._publish()
    response.status.success = True
    return response
```

`request.pose` 는 **무시해도 됩니다.** 이 구성에서 자차 위치는 저희 브리지
(`aw_localization_bridge`)가 `/localization/kinematic_state` + `map→base_link` TF 로
직접 공급하며, 이미 동작이 확인돼 있습니다(§3). shim 이 이미 `/initialpose3d` 의 내용을
버리고 있는 것과 같은 이유입니다.

**B. `autoware_pose_initializer` 패키지를 이미지에 포함** — 원본 그대로

동작은 A 와 같아지지만 `launch_localization=false` 인 구성에서 그 노드를 어떻게 띄울지가
따라옵니다. 저희 쪽 필요는 A 로 100% 충족됩니다.

### 이 요청이 아닌 것 — 오해 방지

> **Autoware 가 localization 을 필요로 해서가 아닙니다.**

저희 구성은 localization 스택 없이 이미 **자율주행 전환과 제어 명령 생성까지 성공**했습니다
(2026-08-11 실측: routing SET → trajectory 10 Hz → AUTONOMOUS → `control_cmd` 33.3 Hz).
필요한 이유는 **단 하나** — SS_v2 의 `EgoEntity` 를 쓰려면 concealer 의 초기화가 통과해야
하고, `EgoEntity` 없이는 `simple_sensor_simulator` 의 **차량 모델이 생성되지 않아**
(`simple_sensor_simulator.cpp:145` assert) 제어 명령을 적용할 대상 자체가 없기 때문입니다.

### 재현

```bash
docker run --rm --entrypoint bash ghcr.io/oss-test-group/autoware-partition:adsw-control -lc '
  ls -d /opt/autoware/share/autoware_pose_initializer 2>/dev/null || echo "pose_initializer: 없음"
  strings /opt/autoware/lib/libautoware_default_adapi.so | grep "^/localization/initialize"
  sed -n "75p" /opt/autoware/share/obigo_launch/launch/planning_simulator.launch.xml'
```

---

## 2. 참고 질문 (요청 아님) — `simulate_localization`

`concealer::AutowareUniverse` 에는 `simulate_localization` 인자가 있고, `false` 면
`/localization/*` 대신 `/simulation/debug/localization/*` 로 발행합니다
(`autoware_universe.cpp:35-44`). §1-A 가 반영되면 저희 브리지와 concealer 가 같은 토픽에
동시에 쓰는 상황이 생기는데, 이 인자를 `false` 로 두면 **patch 없이 해소**됩니다.
귀사 구성에서 이 인자를 노출/고정하고 계신 값이 있으면 알려주시면 됩니다.

---

## 3. 저희가 스스로 해결한/할 것 — 요청하지 않습니다

| 한때 벤더 요청 후보였던 것 | 현재 |
|---|---|
| `/vehicle/status/*` 발행자가 없다 | ✅ `aw_vehicle_status_bridge` |
| `/localization/*` 발행자가 없다 | ✅ `aw_localization_bridge` |
| `map → base_link` TF 를 아무도 방송 안 한다 | ✅ 같은 노드가 같은 샘플에서 방송 |
| `/initialpose3d` 를 수동으로 찍어야 한다 | ✅ **2026-08-11 자동화.** 같은 노드가 1 Hz 로 발행 (§4) |
| tier4 external API 미기동 | ✅ 저희가 별도 기동 |
| `initialize_localization` 인자로 초기화를 건너뛰기 | ❌ 불가 — 그 인자는 **재시도 간격**이지 스위치가 아님 |

즉 **`vehicle_interface` 자리는 저희가 이미 맡고 있고, 남은 것은 concealer 의 서비스
경로 하나**입니다.

---

## 4. 부수 확인 — `/initialpose3d` 소비자는 둘이고, 둘 다 값을 안 봅니다

질문도 요청도 아니고, 저희 자동화의 근거를 남기는 것입니다. 실행 중인 rig 에서
(2026-08-11, 도메인 42) `/initialpose3d` 구독자는 넷이고 그중 게이트에 관여하는 건 둘입니다.

| 구독자 | 어느 이미지 | 하는 일 |
|---|---|---|
| `planning_sim_localization_state` | perception | §1 의 shim → `initialization_state` |
| `topic_state_monitor_initialpose3d` | **decision** | `component_state_monitor`, `warn_rate`/`error_rate`/`timeout` 전부 `0.0` = **"한 번이라도 오면 OK"** |
| `planning_sim_empty_occupancy_grid` | perception | 게이트와 무관 |
| `simple_planning_simulator` | control | 저희 구성에선 꺼져 있음 |

**둘 다 메시지 내용을 보지 않습니다.** 그래서 §1-A 에서 `request.pose` 를 무시해도 된다고
말씀드린 것이고, 저희 브리지도 값을 보내되 그 값이 쓰이지 않는다는 전제로 만들었습니다
(1 Hz · RELIABLE · VOLATILE — 위 두 소비자의 구독 QoS 와 맞습니다).
