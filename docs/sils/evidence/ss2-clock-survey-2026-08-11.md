# 증거 — SS_v2 의 시계, 그리고 스탬프 정책 결정

> 측정 2026-08-11 · `sils-ss2:humble`, ROS 도메인 42 · Fast DDS
> 1차: `ss2-mock`(cpp_mock_scenarios) · 2차(§2 드리프트, §4 해결): `ss2-scenario` +
> `RoutingAction.AcquirePositionAction`, `realtime_factor=1.0`, 20 Hz
> 배경: [`vehicle-seat-no-simulator-2026-08-11.md`](vehicle-seat-no-simulator-2026-08-11.md) §5.5 발견 4

Autoware 를 SS_v2 데이터로 주행시키려면 먼저 시계를 맞춰야 한다. 어느 시계를 쓸지
정하려고 SS_v2 가 실제로 무엇을 내는지 쟀다.

**결론 (이 문서를 다 읽지 않을 사람을 위해)**

| | |
|---|---|
| §4 블로커 | ✅ 해결. 원인 둘 다 우리 쪽 — XML 주석의 `--`, 그리고 `:ro` 마운트에 제자리 재작성 |
| §2 드리프트 | −6.8e−07 s/s (30분에 1.2 ms). 지터 바닥 2.7 ms 보다 작다 |
| §3 tick 브리지 | **보류.** 지금은 불필요. W3/B2-2 때 정확도가 아니라 **시간 주인** 때문에 다시 온다 |
| §5 채택 | 두 Autoware 브리지의 `stamp_policy=epoch-anchored` + 런타임 앵커 감시. 구현·시험 완료 |
| §5b 파생 | 실배선에서 결함 둘 — **NaN 을 스탬프로 변환**, 그리고 **앵커가 TRANSIENT_LOCAL 백로그에 걸림**. 둘 다 감시가 잡았다 |

---

## 1. 핵심 — SS_v2 는 시간을 **두 가지 방식**으로 표현한다

> ⚠ **§2 와 §5b 가 이 절을 정정한다.**
> - `/clock` = epoch 는 맞지만 이유가 다르다 — `use_sim_time=false` 일 때 `/clock` 은
>   **시뮬레이션 시계가 아니라 그냥 벽시계**(`now()`)다. (§2)
> - `EntityStatus.time` 은 "0 에서 시작하는 sim 초" 가 **항상**은 아니다. 시나리오 클럭이
>   시작되기 전에는 **NaN** 이고, EgoEntity 시나리오에서 그 시작은 **Autoware 가 ego 를
>   engage 할 때**다. 아래 `56.449...` 는 NPC 로직이 이미 시작된 `ss2-mock` 관측이다. (§5b)
>
> 문제의 진단(§1 나머지)은 그대로 유효하고, 해법의 방향만 바뀐다.

| 출처 | 값 (같은 런) | 기준 |
|---|---|---|
| `/clock` (`rosgraph_msgs/Clock`) | `sec: 1786416515` | **epoch** |
| `EntityStatus.time` (`traffic_simulator_msgs`) | `56.44999999999893` | **0 에서 시작하는 sim 초** |

같은 시뮬레이터, 같은 순간, 완전히 다른 기준이다.

### 이게 왜 문제인가

[`ss2_ego_mapping.hpp:246`](../../../adapters/ss2/include/ss2_ego_mapping.hpp) 이 쓰는 것은
**후자**다:

```cpp
s.stamp_ns = sim_seconds_to_ns(ego.time_sec);   // EntityStatus.time
```

그래서 `EgoState.stamp_ns` 가 0 에서 시작하고, 두 브리지가 그 값을 그대로 Autoware 에
전달한다(의도적이다 — 도착 시각으로 덮으면 플랜트 지연이 감춰진다). Autoware 는 자기
벽시계와 비교해 **56년 묵은 메시지**로 판정하고 `topic_state_monitor` 가 전부 ERROR 를
내며 자율주행 모드를 막는다. **픽스처만의 문제가 아니었다 — SS_v2 도 똑같이 걸린다.**

`EntityStatus.time` 을 고른 것 자체는 옳다. 그건 **샘플의 시각**이고, SiLS 트레이스·결정론
판정이 기대하는 값이다. 잘못된 것은 그 값을 **벽시계 기준 소비자에게 변환 없이 넘긴** 것이다.

---

## 2. `/clock` 실측

```
Reliability: BEST_EFFORT        ← README 가 미리 경고한 그대로
History (Depth): UNKNOWN
Durability: VOLATILE
Publisher count: 1
Subscription count: 0           ← 이 런에는 소비자가 없었다
```

`BEST_EFFORT` 라 **유실 탐지가 필수**다. 조용히 빠지면 Autoware 의 시간이 멈춘다.

### 드리프트 — 쟀다 (2026-08-11, §4 해결 후)

`ss2-scenario` 가 살아난 뒤 [`measure_clock_drift.py`](../../../tools/sils/ss2/measure_clock_drift.py)
로 연속 측정했다. 원자료: [`ss2-clock-drift-2026-08-11.json`](ss2-clock-drift-2026-08-11.json)

| 항목 | 값 |
|---|---|
| 표본 | 2022개 / 101.1초 (20 Hz, 중앙값 간격 50.0 ms) |
| 유실 (`> 3× 중앙값` 간격) | **0** |
| 시작 오프셋 | **−0.47 ms** |
| **드리프트 기울기** | **−6.8e−07 s/s** |
| 잔차 최대 (지터 바닥) | 2.70 ms |

30분을 돌려도 1.2 ms 다. **지터(2.7 ms)가 드리프트보다 두 자릿수 크다** — 즉 한 번 잡은
오프셋은 연속 추적과 구별되지 않는다. §3 의 "보류" 안이 이겼다.

### 그런데 왜 드리프트가 0 인가 — `/clock` 은 sim 시계가 아니었다

이 값이 0 인 것은 우연이 아니라 **`/clock` 이 그냥 벽시계이기 때문**이다:

```cpp
// simulation_clock.cpp:41
auto SimulationClock::getCurrentRosTime() -> rclcpp::Time
{
  if (not use_sim_time) {
    return now();                                  // ← 벽시계. 시뮬레이션과 무관
  } else {
    return time_at_the_start_of_the_simulator_     // = 0 (use_sim_time 일 때)
           + rclcpp::Duration::from_seconds(getCurrentSimulationTime());
  }
}
```

우리 런은 `use_sim_time := False` 다(런처 인자 덤프로 확인). 그래서 위 표는 사실
**"벽시계 대 벽시계"** 를 잰 것이고, 0.47 ms 오프셋과 2.7 ms 지터는 DDS 전송·스케줄링
지연이다.

**§3 의 전제가 틀렸었다.** "`/clock`(epoch) 과 `EntityStatus.time`(0 기준) 두 시계" 라고
썼지만, 정확히는:

| | `use_sim_time=false` (지금) | `use_sim_time=true` |
|---|---|---|
| `/clock` | **벽시계** (`now()`) | `seconds_since_the_simulator_started_` — **0 기준** |
| `EntityStatus.time` | `getCurrentScenarioTime()+step` — **0 기준** | 같음 |

즉 `use_sim_time:=true` 로 넘겨도 **`/clock` 마저 0 기준이 되어 문제가 커진다** — Autoware
쪽도 전부 `use_sim_time` 을 켜서 같은 0 기준 시계를 공유할 때만 성립한다(그게 §3 의 벤더
인자 경로다). 지금 구성에서 `/clock` 을 브리지해봐야 벽시계를 벽시계로 옮기는 것뿐이다.

**따라서 앵커의 출처는 `/clock` 일 필요가 없다.** 소비자 자신의 벽시계면 충분하고, 그게
도메인을 넘는 배선을 없앤다.

> ⚠ **성립 조건**: 위 결론은 `realtime_factor = 1.0` 이라서 sim 시간이 벽시계 속도로
> 전진한다는 데 의존한다. 이 조건이 깨지면 오프셋이 조용히 낡는다 — 스탬프는 그럴듯한
> 채로 Autoware 의 신선도 창에서 벗어난다. 그래서 **가정으로 두지 않고 런타임에 감시**한다
> (§5).

---

## 3. 그래서 tick 브리지는 어떤 모양이어야 하나

> ⚠ **성립 조건 (2026-09-07 TIER IV 문서 검토로 확인)**: 아래 판정은 **`realtime_factor = 1.0`**
> 에서만 성립한다. TIER IV 자신의 기준 구성은 **AWSIM `TimeScale 0.4`**, 즉 0.4배속이다.
> 그 구성에서는 sim 시간이 초당 600 ms 씩 벌어져 20 ms 허용치를 **33 ms 만에** 넘고,
> 고정 오프셋은 무의미하다 — **그때는 tick 브리지 또는 `use_sim_time` 전면 도입이 필요하다.**
> §5 의 앵커 감시가 정확히 그 경계를 지킨다(그 구성에서 STALE 은 오탐이 아니라 정탐이다).
> → [`sils-tieriv-evaluator-review.md` §3](../sils-tieriv-evaluator-review.md)

> **판정 (§2 실측 후)**: **tick 브리지는 지금 필요 없다.** 드리프트가 지터의 1/100 이라
> 한 번 잡은 오프셋으로 충분하다. 아래 설계는 **W3/B2-2 이후**(시간 주인이 SiLS 로 넘어갈
> 때) 유효한 그림으로 남긴다 — 그때 필요한 이유는 정확도가 아니라 **시간의 주인**이다.
> 지금 채택한 것은 이 절 끝의 "보류" 안이며, 구현은 §5.

### 클럭 마스터(`sils_clock_master`)를 쓸 수 있나 — 조건부로 예

`sils_clock_master` 가 내는 `TickGrant.sim_time_ns` 는 `rosgraph_msgs/Clock` 이 필요로 하는
바로 그 값이다. 브리지 하나면 `/clock` 이 된다.

**그러나 배리어는 못 넘긴다.** 클럭 마스터는 N개의 `TickComplete` 를 모아야 다음 tick 을
준다. Autoware 는 SiLS 참가자가 아니라 `TickComplete` 를 보낼 수 없다.

- 참가자 수에 **안 넣으면**: 배리어가 Autoware 를 안 기다리고 전진 → **lockstep 이 아니라
  "sim time 공급"**. 3계단은 `B-full = 결정론 포기` 이므로 이걸로 충분하다. 다만
  "lockstep 을 붙였다" 고 말하면 안 된다.
- **넣으면**: 누군가 Autoware 대신 완료를 찍어야 하는데, 끝나지도 않은 것을 끝났다고 하는
  거짓말이다.

### 더 중요한 제약 — **시간 주인은 하나여야 한다**

지금 구성에서 시간 주인은 **SS_v2** 다. 자기 `SimulationClock`(50 ms 스텝)을 스스로
전진시킨다. 클럭 마스터도 전진시키면 주인이 둘이 되어 서로 어긋난다.

클럭 마스터가 주인이 되려면 SS_v2 가 **배리어 참가자**가 되어야 하는데, 그건
[`adapters/ss2/README.md`](../../../adapters/ss2/README.md) §7 의 미완 항목이다
(**W3/B2-2**, SS_v2 frame 구동부 patch).

### 결론 — 한 브리지, 두 소스

```
  tick 브리지 (신규)  ──▶  /clock   (Autoware 도메인, Cyclone)
     ▲
     ├── 소스 A: SS_v2 의 /clock         ← 지금. 시간 주인 = SS_v2
     └── 소스 B: TickGrant.sim_time_ns   ← W3/B2-2 이후. 시간 주인 = SiLS
```

같은 노드, 같은 출력. 시간 주인이 바뀌면 입력만 갈아끼운다. 이렇게 만들어야
배리어 참가자화가 끝났을 때 브리지를 다시 안 짜도 된다.

Autoware 쪽은 커스텀 런치가 필요 없다 — 벤더 인자 하나로 된다:

```
scenario_simulation:=true
  → launch_dummy_vehicle=false        (시뮬레이터 off)
  → launch_scenario_simulator_v2_adapter=true
  → use_sim_time=true
```

### 그리고 EgoState 쪽도 같은 시계에 앵커해야 한다

`/clock` 만 넘겨도 `EgoState.stamp_ns` 가 여전히 0 기준이면 아무것도 안 바뀐다.
`ss2_ego_bridge` 가 `EntityStatus.time` 을 **SS_v2 의 `/clock` 앵커에 얹어** 내보내야
한다(앵커 = 어느 순간의 `/clock − EntityStatus.time`).

> ~~**대안으로 검토했다가 보류한 것**: 두 Autoware 브리지에 `stamp_policy=epoch_offset` 을
> 넣어 첫 샘플에서 오프셋을 잡는 방법. 더 싸지만 **드리프트가 있으면 무너진다**~~
>
> ✅ **이 안을 채택했다.** §2 가 드리프트를 −6.8e−07 s/s 로 실측했다 — 30분에 1.2 ms,
> 지터 바닥(2.7 ms)보다 작다. "드리프트가 있으면 무너진다" 는 우려는 실측으로 해소됐고,
> 남은 위험(realtime_factor ≠ 1)은 가정이 아니라 **런타임 감시**로 처리한다. §5.

### 다만 이 절의 "앵커는 SS_v2 의 `/clock` 에서" 는 틀렸다

§2 가 보였듯 `use_sim_time=false` 인 지금 `/clock` 은 벽시계다. 앵커를 그것에서 가져오면
**도메인을 넘는 배선을 추가하고 소비자의 벽시계와 같은 값을 얻는다.** 그래서 앵커는 각
Autoware 브리지가 **자기 `now()`** 에서 잡는다 — 노드 추가 없음, 토픽 추가 없음,
정준 `EgoState` 계약 불변(vECU·트레이스 게이트가 보는 값은 그대로 sim 시간).

---

## 4. ~~블로커~~ 해결 — `ss2-scenario` 즉시 종료의 원인은 **둘**이었다

전처리기가 "파생 0" 을 응답하는 지점까지 좁혔던 것(아래 원본 분석)은 맞았다. 그 지점에서
실제로 무슨 일이 있었는지 잡았다. **독립적인 고장이 두 개 겹쳐 있었고, 둘 다 우리 쪽이다.**

### 원인 A — 우리가 만든 `.xosc` 가 XML 로서 깨져 있었다

```
$ ros2 run openscenario_utility validation.py /scenarios/ego-lane-change.xosc
[NG] /scenarios/ego-lane-change.xosc
Error: not well-formed (invalid token): line 2, column 56
```

2행 56열은 **우리가 붙인 생성 배너**다:

```xml
<!-- GENERATED by tools/sils/ss2/make_ego_scenario.sh -- do not edit by hand.
                                                     ↑ 56열
```

XML 주석 안에는 `--` 를 쓸 수 없다(XML 1.0 §2.5). 이 저장소의 주석 관용구 —
`name.sh -- 설명` — 가 그대로 `.xosc` 배너로 들어가면서 파일을 비적합으로 만들었다.
`Preprocessor::preprocessScenario` 는 **다른 무엇보다 먼저** `validateXOSC` 를 부르므로,
시나리오는 파싱조차 되기 전에 죽었다.

고침: [`make_ego_scenario.sh`](../../../tools/sils/ss2/make_ego_scenario.sh) 의 배너에서
`--` 제거 + 배너 자체 검사 + 쓰기 직전 `ET.fromstring()` 파싱 검증. 같은 부류가 다시
새어나갈 수 없다.

### 원인 B — 전처리기는 시나리오를 **제자리에서 다시 쓴다**. 마운트는 `:ro` 였다

A 를 고치자 검증은 통과했지만 여전히 파생 0 이었다. 소스를 보면:

```cpp
// openscenario_preprocessor.cpp — preprocessScenario()
auto raw_scenario_directory = scenario.path.parent_path() / "raw";
std::filesystem::create_directories(raw_scenario_directory);              // ← 쓰기
std::filesystem::rename(scenario.path, raw_scenario_directory / ...);     // ← 쓰기
script->script.save_file(preprocessed_scenario_path.c_str());             // ← 쓰기
```

읽고 마는 게 아니다. **형제 `raw/` 를 만들고, 원본을 거기로 옮기고, 파라미터 치환본을
원래 경로에 쓴다.** `ss2-scenario` 는 `scenarios/ss2` 를 `:ro` 로 마운트한다(의도적이다 —
소스 자산이다). 세 호출 전부 EROFS.

```
$ mkdir -p /scenarios/raw
mkdir: cannot create directory '/scenarios/raw': Read-only file system
```

### 왜 아무 메시지도 없었나 — 상류가 `has_succeeded` 를 안 읽는다

A 든 B 든 예외는 `Load` 서비스 핸들러가 잡아 `has_succeeded=false` + 사유 문자열로
돌려준다. **그런데 호출자가 그걸 안 본다:**

```python
# scenario_test_runner.py:256 — post_scenario_to_preprocessor
if future.result() is not None:
    return True          # has_succeeded 도 message 도 읽지 않는다
```

응답이 **오기만 하면** 성공이다. 그래서 Load 는 "성공", 큐는 비어 있고,
`derivative_remained()` 는 첫 호출부터 거짓, `run_preprocessed_scenarios([])` 는 아무것도
찍지 않고, 6초 뒤 조용히 종료. **원본 분석이 여기까지 정확히 좁혔던 그 지점이다** —
빠져 있던 건 서비스 응답 본문뿐이었고, 그건 상류가 버린다.

### 고침

| | 무엇 | 어디 |
|---|---|---|
| A | 배너에서 `--` 제거, 생성물 XML 파싱 검증 | `make_ego_scenario.sh` |
| B | 시나리오를 쓰기 가능한 `/out/staging` 으로 복사 후 실행 | [`stage_and_launch.sh`](../../../tools/sils/ss2/stage_and_launch.sh) (신규) |
| B | `ss2-scenario` 가 `ros2 launch` 대신 래퍼 호출 | `docker-compose-sils-ss2.yaml`, `Dockerfile.ss2` |

`:ro` 는 그대로 둔다. 시뮬레이터가 소스 자산을 고쳐 쓸 이유가 없고, 스테이징 사본에
남는 `raw/` + 전처리본은 그대로 런 증거가 된다(`runs/ss2-scenario/staging/`).

### 결과 — 산다

```
[scenario_test_runner.py-1] Run RoutingAction.AcquirePositionAction.xosc (1 of 1)
...
<error type="AutowareError" message="Service &quot;/api/operation_mode/change_to_stop&quot;
       is not ready after 180 seconds" />
```

6초 → **180초 완주**, 그리고 **실제 오류 메시지**가 나온다. `/clock` 도 그 내내 발행됐고,
§2 는 그 위에서 측정했다.

마지막 오류는 다음 단계 그 자체다: `EgoEntity` 의 concealer 가 Autoware 의 AD API 를
기다린다. `launch_autoware:=false` 라 아무도 응답하지 않는다. **폐루프에서는 이미 떠 있는
실제 Autoware 가 그 자리를 채운다** — 그게 §6 의 남은 항목이다.

### 곁가지 — `ego-lane-change.xosc` 는 지도가 안 맞는다 (별개 문제)

A·B 를 고치자 이번엔 인터프리터가 제대로 된 오류를 냈다:

```
InternalError: Failed to lookup element with id 178
```

상류 `openscenario_interpreter/example/lane_change.xosc` 가 쓰는 레인 178/180/879 는
**이미지에 든 어느 지도에도 없다**:

| 지도 | 레인 relation id |
|---|---|
| `kashiwanoha_map` | 34378 … 34693 |
| `simple_cross_map` | 7, 14, 18, 22, 133, 140, 147, 154 |

`make_ego_scenario.sh` 는 `<LogicFile>` 만 kashiwanoha 로 바꿨을 뿐 레인 id 는 상류 그대로
두었다 — 상류 예제가 **동봉되지 않은 지도**를 겨냥한 것이다. 이 자산은 지금 못 쓴다.

상류에 `LateralAction.LaneChangeAction.yaml`(kashiwanoha, 레인 34513) 이 있지만
`Property: []` 라 **`isEgo` 가 없다** → EgoEntity 가 아니다. 반대로
`RoutingAction.AcquirePositionAction.yaml` 은 `isEgo` 도 있고 지도도 맞다 — 그래서 위
성공 런은 이걸로 했다. 지도가 맞는 *ego* 레인체인지 자산이 필요하면 후자에 레인체인지를
넣거나 전자에 `isEgo` 를 넣어야 하고, 어느 쪽이든 "상류 그대로" 는 깨진다.

<details>
<summary>원본 분석 (좁히기 과정, 결론은 맞았다)</summary>

`launch_autoware:=false`, `vehicle_model:=sample_vehicle`, `scenario:=/scenarios/ego-lane-change.xosc`
로 띄우면 인터프리터가 **약 6초 만에 "Shutdown interpreter"** 를 내고 전체가 내려간다.
오류 메시지도, `/out` 의 새 결과도 없다.

### 배제한 것 (전부 실측)

| 가설 | 시험 | 결과 |
|---|---|---|
| 우리 브리지가 도메인 42 를 방해 | `sils-aw-*` 내리고 재시도 | ❌ 그대로 죽음 |
| Autoware 파티션 125노드의 부하·간섭 | 파티션 3종 내리고 재시도 | ❌ 그대로 죽음 |
| 시나리오 파일이 컨테이너에 안 보임 | `/scenarios` 확인 | ❌ 있음 (12978 B) |
| `/out` 쓰기 불가 | `touch /out/_wtest` | ❌ 쓰기 됨 |
| `vehicle_model` 미지정 | `sample_vehicle` 지정 | ❌ 그대로 죽음 (다른 증상은 고쳤다) |

> `/out` 은 쓸 수 있었다 — 그래서 배제됐다. **전처리기가 쓰려던 곳은 `/out` 이 아니라
> 시나리오 옆(`/scenarios/raw`)** 이었다. 08-10 성공 런이 `.yaml` 이라 변환본이 쓰기
> 가능한 `/out` 에 놓였던 것이 두 런의 차이다.

### 좁혀진 지점 — 전처리기가 "파생 시나리오 없음" 을 응답한다

`scenario_test_runner.py` 의 흐름을 읽고 로그와 대조하면 실패 지점이 한 곳으로 좁혀진다:

```python
def run_scenario(self, scenario):
    xosc_scenarios = convert_scenario_to_xosc(...)   # .xosc 는 그대로 통과 → 1개
    for xosc_scenario in xosc_scenarios:
        if self.post_scenario_to_preprocessor(xosc_scenario):   # 실패면 exit(1) — 안 났다
            preprocessed_scenarios = []
            while self.derivative_remained_on_preprocessor():   # ← 여기가 처음부터 False
                ...
            self.run_preprocessed_scenarios(preprocessed_scenarios)   # 빈 리스트
    self.shutdown()
```

`run_preprocessed_scenarios` 는 첫 줄에서 `Run <name> (1 of N)` 을 찍는데 **로그에 그게
없다.** 리스트가 비었다는 뜻이고, 곧 `derivative_remained_on_preprocessor()` 가 처음부터
거짓을 돌려줬다는 뜻이다.

서비스 타임아웃이면 `print_debug` 로 예외가 찍히는데(이 함수는 verbose 조건 없이 항상
`info` 로 출력한다) **그것도 없다.** 즉 전처리기가 **정상 응답으로 "남은 파생 없음"** 을
말하고 있다 — 시나리오를 받아놓고 아무것도 내놓지 않는다.

</details>

### 별건 — `ss2-mock` 은 뜨지만 오래 못 산다

`timeout:=900` 을 줘도 **~90초에 종료**한다. 미해결이지만 **더는 블로커가 아니다** —
`/clock` 연속 측정은 살아난 `ss2-scenario` 로 했다.

---

## 5. 채택한 것 — `stamp_policy` (구현 완료)

[`aw_stamp_policy.hpp`](../../../adapters/ss2/include/aw_stamp_policy.hpp) — 헤더 전용,
ROS·DDS 비의존, [단위시험 30개](../../../adapters/ss2/test/test_stamp_policy.cpp).
`aw_vehicle_status_bridge` 와 `aw_localization_bridge` 양쪽에 배선했다.

```
stamp_policy = epoch-anchored   (기본값)     stamp_policy = source
  첫 샘플에서 anchor = now() − stamp_ns        stamp_ns 를 그대로 발행
  이후 stamp_ns + anchor                       (이미 벽시계로 찍는 플랜트용)
```

**정준 `EgoState` 는 안 건드린다.** `ss2_ego_bridge` 는 계속 sim 시간을 싣고, vECU 와
트레이스 게이트가 보는 값은 그대로다. 변환은 **sim 시간이 벽시계 소비자를 만나는 경계**,
즉 Autoware 쪽 브리지에서만 일어난다. §1 이 "`EntityStatus.time` 을 고른 것 자체는 옳다"
고 한 것과 일관된다.

설계에서 신경 쓴 것 세 가지:

1. **간격은 보존한다.** 앵커는 상수 시프트다 — 도착 시각으로 덮지 않으므로 플랜트 지연과
   지터가 그대로 살아 남는다. 단위시험이 이걸 못 박는다(수신측 시계에 지터를 넣어도 출력
   간격은 소스 간격과 정확히 같아야 한다).
2. **앵커는 다시 안 맞춘다.** 자가보정 앵커는 드리프트를 조용히 흡수해서 **유일한 증거를
   지운다.** 고정 + 감시가 의도적 선택이다.
3. **감시가 곧 탐지기다.** 매 샘플 `(now − stamp) − anchor` 를 재고 허용치(20 ms, Autoware
   신선도 창 기준·지터 바닥의 7배) 초과를 센다. `realtime_factor ≠ 1`, `use_sim_time` 반전,
   시뮬레이터 정지 — 셋 다 여기서 잡힌다. 설정 플래그를 추측해 막는 대신 **실제 고장을**
   본다.

```
[aw_vehicle_status_bridge] stamp anchor ok: max deviation 0.4 ms (tolerance 20)
[aw_vehicle_status_bridge] STAMP ANCHOR STALE: 37 samples past tolerance, max 412.0 ms.
    Simulation time is no longer advancing at wall rate ... check realtime_factor / use_sim_time
```

오타는 기본값으로 안 떨어지고 기동 실패시킨다 — 이 둘 중 잘못 고르는 것이 바로 이 헤더가
막으려는 무증상 버그라서다.

---

## 5b. 폐루프 배선을 실제로 걸어보니

§5 를 실제 `ss2-scenario` 상대로 돌렸다. ① 은 이미 알려진 함정을 내가 다시 밟은 것이고,
② 는 진짜 결함이다 — 그리고 §5 의 앵커 감시가 그걸 잡아냈다.

### ① 토픽 이름 — 새 발견이 아니라 **내가 문서를 안 읽고 손으로 띄운 것**

```
cpp_mock_scenarios (ss2-mock)        /entity/status
scenario_test_runner (ss2-scenario)  /simulation/entity/status
```

`traffic_simulator` 가 `"entity/status"` 를 **상대 이름**으로 발행해서 발행 노드의
네임스페이스에 붙는다. 인터프리터는 `namespace=simulation` 이다.

**이건 이미 알려져 있고 이미 처리돼 있다** —
[`docker-compose-sils-ss2.yaml`](../../../docker/compose/docker-compose-sils-ss2.yaml) 의
`ss2-adapter` 주석과 [`run_closed_loop.sh`](../../../tools/sils/ss2/run_closed_loop.sh) 가
`SILS_SS2_STATE_TOPIC` 으로 네임스페이스 붙은 이름을 넘긴다. 코드 기본값이 stage-1 이름인
것도 의도적이다(그 판정이 공표된 산출물이라 재현돼야 한다).

내가 브리지를 compose 대신 손으로 띄우면서 그 인자를 안 줬을 뿐이다. **코드는 안 고쳤다.**
기록해두는 이유는 하나 — 증상이 "매칭 0" 이라 도메인/QoS 문제로 읽히고, 그래서 두 번째로
당하기 쉽다.

### ② ★ `EntityStatus.time` 이 **NaN** 이다 — 그리고 우리는 그걸 스탬프로 바꾸고 있었다

이름을 고치자 데이터가 흘렀다. 그리고 **`received=899 published=899 skipped=0
write_failed=0`** — 모든 계수기가 완벽했다. 그런데 §5 의 앵커 감시가 울렸다:

```
STAMP ANCHOR STALE: 798 samples past tolerance, max deviation 39883.7 ms
```

편차가 **정확히 벽시계 속도로**(5초에 5000 ms) 자란다 = **소스 스탬프가 안 움직인다.**
실측:

```
name=ego type=0 status.time=nan msg.time=nan
```

원인은 상류 설계다:

```cpp
// simulation_clock.hpp:34,58
auto getCurrentScenarioTime() const {
  return seconds_since_the_simulator_started_ - seconds_at_the_start_of_the_scenario_;
}
double seconds_at_the_start_of_the_scenario_ = std::numeric_limits<double>::quiet_NaN();
```

`SimulationClock::start()` 전까지 감수가 NaN 이다. 그리고 상류는 **NaN 을 오류가 아니라
"아직 시작 안 함" 의 정의로 쓴다**:

```cpp
// openscenario_interpreter.cpp:162
if (std::isnan(evaluateSimulationTime())) { ... activateNonUserDefinedControllers(); }
```

이 상태를 벗어나는 조건이 **모든 Autoware 제어 엔티티가 `isEngaged`** 다.
`launch_autoware:=false` 면 아무도 engage 하지 않으므로 **런 내내 NaN** 이다.

우리 쪽 결함: `sim_seconds_to_ns(NaN)` → `std::llround(NaN)` 은 미규정이고 실제로는 고정
센티넬 하나를 뱉는다. 그 값이 정준 `EgoState.stamp_ns` 로 나갔다 — **스탬프가 전혀
전진하지 않는데 파이프라인 전 구간의 계수기는 건강했다.** 하류에서 진짜 스탬프와 구별할
방법이 없다.

**고침**: `is_usable_sim_time()`(= `std::isfinite`) 을 `ss2_ego_mapping.hpp` 에 두고,
`ss2_ego_bridge` 가 비유한 시각의 샘플을 **발행하지 않고** `skipped_no_time` 으로 세며
사유를 한 번 설명한다.

```
[ego] received=999 but the scenario clock has not started (skipped_no_time=999).
      Nothing is being published. ... this is normal until Autoware engages the ego.
```

> **이것이 §5 가 자가보정 앵커를 거부한 이유의 실증이다.** 앵커가 스스로 따라갔다면
> 스탬프는 계속 신선해 보였을 것이고, 이 버그는 발견되지 않았다. 고정 + 감시가
> 탐지기 역할을 했다.

### ③ 앵커가 TRANSIENT_LOCAL 백로그에 걸렸다 — 감시가 또 잡았다

실제 파티션(도메인 42)에 브리지를 붙이자 감시가 다시 울렸는데, 이번엔 값이 **일정**했다:

```
STAMP ANCHOR STALE: 1235 samples past tolerance, max deviation -5110.5 ms
```

우연이 아니다: **KEEP_LAST(256) × 20 ms = 5120 ms.** 정준 `EgoState` 는 TRANSIENT_LOCAL
이라, 늦게 붙은 브리지는 **과거 256개를 오래된 것부터** 먼저 받는다. "첫 샘플에서 앵커를
잡는다" 는 규칙이 **가장 오래된 것**에 앵커를 걸었고, 이후 모든 스탬프가 영구히 5.1초
과거가 됐다 — §5 가 막으려던 바로 그 신선도 실패를, §5 가 만들어냈다.

**고침**: 앵커를 첫 샘플이 아니라 **짧은 창(500 ms) 동안의 `(now − stamp)` 최솟값**으로
잡는다. 오래된 샘플일수록 그 차가 커지므로 최솟값이 가장 덜 지연된 관측이다. 창은 개수가
아니라 **시간** 기준이다 — 백로그는 수 ms 안에 버스트로 쏟아지므로 개수 N 은 버스트 안에서
끝날 수 있지만, 버스트보다 넉넉히 긴 벽시계 창은 그럴 수 없다.

창이 열려 있는 동안은 `latched()` 가 거짓이고 **두 브리지가 발행하지 않는다**
(`warmup_dropped` 로 센다). 백로그를 버리는 것은 그 자체로도 옳다 — 5초치 과거를 현재
차량 상태라고 Autoware 에 흘리면 ego 가 자기 과거를 버스트 속도로 순간이동한다.

`test_transient_local_backlog_does_not_poison_the_anchor` 가 이 런을 축소해 재현한다.

> §5 의 "자가보정 앵커를 거부한다" 는 그대로다. 앵커가 움직이는 곳은 **워밍업 창 안뿐**이고,
> 그 뒤로는 다시 안 맞춘다. 단위시험이 워밍업 후 더 작은 관측이 와도 앵커가 안 움직이는
> 것을 못 박는다.

### 그래서 폐루프의 순서가 정해진다

`EntityStatus.time` 이 유효해지려면 **Autoware 가 먼저 ego 를 engage 해야 한다.**
"SS_v2 를 띄우고 → 브리지로 Autoware 를 먹인다" 가 아니라, Autoware 가 붙어서 engage 될
때까지는 SS_v2 가 **의도적으로 시각 없는 상태**로 대기한다. §6 의 배선은 이 순서를
전제해야 한다.

## 6. 다음

| 할 일 | 상태 | 왜 |
|---|---|---|
| `ss2-scenario` 즉시 종료 원인 | ✅ §4 | 원인 A(XML `--`) + B(`:ro` 마운트). 180초 완주 |
| `/clock` 드리프트 실측 | ✅ §2 | −6.8e−07 s/s. 오프셋으로 충분 |
| tick 브리지 구현 (소스 A) | ⏸ 보류 | §3. 지금 필요 없다. W3/B2-2 때 **시간 주인** 문제로 다시 온다 |
| 스탬프 앵커 | ✅ §5 | `ss2_ego_bridge` 가 아니라 Autoware 쪽 두 브리지에. 정준 계약 불변 |
| SS_v2 실배선 확인 | ✅ §5b | NaN 시각을 스탬프로 흘리던 결함 수정 |
| **`stamp_policy` 를 실제 Autoware 상대로 확인** | ▶ **다음** | 단위시험 통과, SS_v2 상대 배선 확인. 하지만 **유효한 스탬프가 흐르는 것은 아직 못 봤다** — §5b 때문에 Autoware 가 engage 해야 시각이 생긴다. `topic_state_monitor` 판정은 그 다음 |
| **폐루프** | ▶ **다음** | §4 마지막 오류(`/api/operation_mode/change_to_stop` 부재)가 곧 이 항목이다 |

### 폐루프까지 남은 배선

§4 의 마지막 오류가 정확히 말해준다 — `EgoEntity` 의 concealer 가 Autoware AD API 를
기다리는데 `launch_autoware:=false` 라 아무도 없다. 이미 떠 있는 실제 Autoware 가 그
자리에 있어야 한다:

```
SS_v2 (EgoEntity)  ──/entity/status──▶  ss2_ego_bridge  ──▶ sils/ego/state (정준, sim 시간)
                                                                │
                        ┌───────────────────────────────────────┤
                        ▼                                       ▼
          aw_vehicle_status_bridge                  aw_localization_bridge
          (epoch-anchored)                          (epoch-anchored)
                        └──────────────▶ Autoware ◀─────────────┘
                                            │
                                    /control/command/control_cmd
                                            ▼
                              aw_control_target_bridge ──▶ vECU ──▶ ss2_control_bridge ──▶ SS_v2
```

부품은 전부 있다. 확인할 것은 (1) concealer 가 실제 Autoware 를 잡는지, (2) engage 후
`EntityStatus.time` 이 실제로 전진하는지(§5b), (3) 그제서야 `stamp_policy` 로
`topic_state_monitor` 가 초록이 되는지. **순서가 이 순서다** — §5b 가 보인 대로 engage 전에는
시각 자체가 없다.

⚠ **도메인**: concealer 가 Autoware 를 보려면 둘이 같은 도메인이어야 하는데, 파티션은
Cyclone 이고 SS_v2/우리 브리지는 Fast DDS 다. 같은 도메인에서 두 RMW 를 섞으면 어댑터가
segfault 한다([stage3 §2단계 "왜 분리해야 하나" ②](../sils-stage3-autoware-partition.md)).
이 충돌을 어떻게 풀지가 폐루프의 **첫 번째** 결정이지, 마지막이 아니다.
