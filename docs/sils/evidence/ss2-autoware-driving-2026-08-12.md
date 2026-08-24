# 증거 — Autoware 가 SS_v2 차량을 실제로 몰았다 (그리고 벤더를 기다리지 않았다)

> 측정 2026-08-12 · **도메인 42 단일** · kashiwanoha · headless
> 선행: [`autoware-return-path-2026-08-11.md`](autoware-return-path-2026-08-11.md) §7~§11 ·
> [`multi-vecu-bus-2026-08-12.md`](multi-vecu-bus-2026-08-12.md)
> 벤더 요청은 **그대로 유효하다** — [`sils-autoware-partition-vendor-requests.md`](../sils-autoware-partition-vendor-requests.md)

어제까지의 결론은 "SS_v2 의 EgoEntity 를 쓰려면 concealer 가
`/api/localization/initialize` 를 통과해야 하는데, 이 파티션에는 그 구현이 없다.
**벤더 대기**" 였다(§7·§9). 오늘 그 진단이 **절반만 맞았다**는 것을 확인했다.

---

## 1. 결과

| 단계 | 관측 |
|---|---|
| concealer 초기화 | `/api/localization/initialize` → **success=True** (전에는 30회 실패) |
| localization 상태 | `initialization_state` **3 (INITIALIZED)** |
| 경로 | `/api/routing/state` **2 (SET)** — concealer 가 세웠다 |
| 자율주행 | `/api/operation_mode/state` **mode 2** |
| `/vehicle/status/*` 발행자 | **1 = concealer** (우리 브리지는 꺼져 있다) |
| 시나리오 판정 | **Passed** · `result.junit.xml` `failures="0" errors="0"` |
| **차가 움직였나** | ✅ **아래 속도 곡선** |

```
t=1  vx 0.000  x 3722.98   mode 2
t=2  vx 0.154  x 3723.32
t=3  vx 1.524  x 3726.84
t=4  vx 2.586  x 3733.49
t=5  vx 2.702  x 3740.81
t=6  vx 2.705  x 3748.11
t=7  vx 1.955  x 3753.47      ← 감속
t=8  vx 0.557  x 3755.61
t=9  vx 0.000  x 3755.62      ← 정지
t=10 vx 0.651  x 3756.96      ← 재출발
t=13 vx 1.244  x 3772.89
```

**상수가 아니라 곡선**이라는 것이 요점이다 — 가속·순항·감속·정지·재출발이 나타난다.
`RoutingAction.AcquirePositionAction` 은 목표 도달이 통과 조건이고, 그것이 `Passed` 다.

---

## 2. 무엇이 막고 있었나 — 진단이 절반만 맞았다

§9 는 이렇게 닫았다:

> `pose_initializer` 부재 → `/api/localization/initialize` 가 `success=false` → **벤더 요청**

맞다. 그런데 **그 서비스 이름이 비어 있다는 것**을 확인하지 않았다.

```
$ ros2 service type /localization/initialize
tier4_localization_msgs/srv/InitializeLocalization
$ ros2 service call /localization/initialize ... "{}"
waiting for service to become available...        ← 서버가 없다
```

ADAPI(`/api/localization/initialize`)는 **클라이언트**로서 내부 서비스
`/localization/initialize` 를 부르고, 그 서버가 원래 `pose_initializer` 다. 그 이름을
아무도 광고하지 않으므로 — **우리가 서버를 세울 수 있다.** 이름 충돌이 없다.

> **막혀 있던 것은 "벤더만 할 수 있는 일" 이 아니라 "아무도 안 한 일" 이었다.**

---

## 3. 우리가 채운 것 — 서비스→토픽 다리 하나

[`tools/sils/ss2/localization_initialize_shim.py`](../../../tools/sils/ss2/localization_initialize_shim.py)

```
concealer ──서비스── /api/localization/initialize ──ADAPI──▶ /localization/initialize
                                                                    │  ← 우리 shim
                                                                    ▼ 요청의 포즈를 그대로
                                                            /initialpose3d 발행
                                                                    ▼
                              벤더 shim(planning_sim_localization_state) → INITIALIZED
```

**상태를 지어내지 않고 벤더 자신의 메커니즘을 태운다.** 우리 노드는 `initialization_state`
를 직접 발행하지 않는다 — 그건 벤더 shim 의 몫이고, 둘이 같은 토픽에 쓰면 발행자가 둘이
된다(이 저장소가 반복해 밟은 함정).

요청에 담긴 **포즈를 흘린다**(버리지 않는다). 이 구성에서는 concealer 가 위치를 정하므로
쓰이지 않지만, sps 가 켜진 구성이나 앞으로 `pose_initializer` 가 들어오는 구성에서는
그 값이 정답이 된다. 버리면 그때 조용히 틀린 초기위치가 된다.

실측 로그:
```
initialize #2: method=0 pose=(3722.981, 73775.929, -1.302) -> /initialpose3d
```

### ⚠ 이건 임시방편이다

벤더 요청은 취소되지 않는다. 이유:

- 이 shim 은 **측위를 하지 않는다.** `method=AUTO` 를 흉내 내지 않고 그대로 통과시킨다.
  이 파티션에 정렬할 대상(NDT/GNSS)이 없으므로 흉내 내는 쪽이 거짓말이 된다
- 지원되는 구성이 아니다. 이미지가 바뀌면 같이 깨진다
- 벤더가 자기 shim 에 15줄을 넣으면 이 파일은 필요 없어진다

---

## 4. 구성 (§11 과 다른 부분만)

| | |
|---|---|
| 시나리오 | 이미지 내장 `RoutingAction.AcquirePositionAction.yaml` — **isEgo 있음** |
| 우리 상태 브리지 2개 | **꺼야 한다.** concealer 가 `/vehicle/status/*` 와 `/localization/*` 를 발행한다 |
| tier4 external API | **필요.** `deprecated_api.launch.xml` 을 제어 파티션에 붙인다 (`/api/autoware/set/velocity_limit`) |
| localization shim | `tools/sils/ss2/run_localization_shim.sh --domain-id 42` |
| sps | 계속 OFF |

```bash
tools/sils/ss2/run_localization_shim.sh --domain-id 42
docker exec -d adsw-control-42 bash -lc 'source /opt/ros/humble/setup.bash &&
  source /opt/autoware/setup.bash &&
  exec ros2 launch tier4_autoware_api_launch deprecated_api.launch.xml > /workspace/log/deprecated_api.txt 2>&1'
docker rm -f sils-aw-status sils-aw-localization     # concealer 와 충돌한다
SILS_DOMAIN=42 SILS_RMW=rmw_cyclonedds_cpp \
SILS_SS2_SCENARIO=<이미지 내장 AcquirePositionAction.yaml> \
SILS_SS2_LAUNCH_AUTOWARE=false SILS_SS2_VEHICLE_MODEL=sample_vehicle \
  docker compose -f docker/compose/docker-compose-sils-ss2.yaml up -d ss2-scenario
```

---

## 5. vECU 는 주행 내내 살아 있었다

제어 경로에 있지는 않다(관측자). 그러나 **살아 있는 Autoware 판단을 받아 자기 로직을 돌고
아비트레이션까지 했다** — 주행 중 실측:

```jsonc
{"event":"target_received","tick_seq":3812,"acceleration":0.504629,"steering":0.000433648}
{"event":"arbitrated","tick_seq":3812,"owner_acceleration":0.504629,"chosen_acceleration":-8.96604,
 "inputs":{"Request.adaptive":6.94119,"Request.classic":-8.96604,"Request.plainc":6.94119}}
```

정지 상태의 상수(`-1.5`)가 아니라 **주행 중 변하는 값**이다. 런 전체 집계:

| ECU | state | target | 출력 accel 범위 | 고유값 |
|---|---|---|---|---|
| steer | 3948 | 0 | 0.000 (조향 전용) | 1 |
| adaptive | 3948 | 0 | 5.687 … 8.000 | 243 |
| plainc | 3948 | 0 | 5.687 … 8.000 | 242 |
| **VCU** | 3947 | **3947** | −15.563 … −1.500 | 243 |

VCU 가 받은 **Autoware 의 판단** (주행 749 tick):
`accel −3.400 … +0.574` 고유값 252 · `steer −0.421 … +0.123` 고유값 253.

### ★ 아비트레이션 불변식이 실측에서 성립했다

```
승자 분포 (3975 tick)
  Request.classic   3734 (93.9%)   ← SILS_CTRL_SETPOINT=-1.0 로 강제동시킨 피어
  owner(Autoware)    241 (6.1%)

덮어쓴 경우가 전부 "더 강한 제동" 인가 →  예 (최대 15.383 m/s² 더 강하게)
```

**단 한 번도 owner 를 위험 방향으로 뒤집지 않았다.** [`sils-vecu-arbitration.md`](../sils-vecu-arbitration.md)
§4.1 의 I1 이 단위 테스트가 아니라 **주행 데이터 3975 tick** 에서 확인됐다.

93.9% 라는 편중 자체도 트레이스가 있어서 원인이 읽힌다(그 피어를 우리가 그렇게 만들었다).
결정 로그가 없었으면 "VCU 출력이 이상하다" 로만 보였을 것이다 — §3 이 그 로그를 요구한 이유다.

`ss2_ego_bridge` `received=5804 published=5709`.

> ⚠ `skipped_no_time=92` 가 새로 보인다. EgoEntity 구성에서 `EntityStatus.time` 이
> engage 전까지 NaN 이라는 §6 의 관측과 일치한다. 그 구간의 샘플이 버려진 것이고,
> engage 후에는 흐른다. 별도 항목으로 남긴다.

---

## 6. 이 시험이 증명하지 않는 것

- **vECU 가 차를 몬 것이 아니다.** Autoware → concealer → 차량모델 직결이고, vECU 는
  병렬 관측자다. vECU 를 제어 경로에 넣으려면 여전히 remap 이 필요하다(§11 의 (b))
- **아비트레이션 결과가 차에 반영되지 않았다.** 위 `chosen -8.96` 은 `sils/ego/cmd` 로
  나갔을 뿐 플랜트에 닿지 않는다
- **충실도 주장이 아니다.** SS_v2 의 축약 차량모델이고, 시나리오 하나의 통과다
