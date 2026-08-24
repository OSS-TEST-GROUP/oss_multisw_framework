# TIER IV 자신의 구성은 어떻게 되어 있나 — Evaluator 문서 검토

> 검토 2026-08-11 · 원본: `reference/2025-09-07_AWSIM_on_evaluator_ overview.pdf`
> (TIER IV Confluence, "AWSIM on Evaluator : Overview", 3쪽)
> 관련: [`sils-integration-topology.md` §1.1](sils-integration-topology.md) ·
> [`evidence/ss2-clock-survey-2026-08-11.md`](evidence/ss2-clock-survey-2026-08-11.md)

**왜 이 문서를 남기나**: 우리는 SS_v2 를 **플랜트**로 써왔는데, TIER IV 자신의 구성에서는
아니다. 그 차이가 2026-08-11 하루를 쓴 벽들의 근원이고, 우리 시계 결정의 성립 조건도
여기서 하나 드러난다. 원본 PDF 는 사내 Confluence 인쇄물이라 저장소에 없다 — 사실만 옮긴다.

---

## 1. TIER IV 의 E2E 구성

> In Evaluator, E2E simulations can be executed by **combining AWSIM with scenario_simulator_v2**.

| 역할 | TIER IV |
|---|---|
| **플랜트** (차량 물리 + 센서) | **AWSIM** — Unity 바이너리, zip 으로 Evaluator 에 등록. Evaluator 가 빌드는 안 해준다 |
| **시나리오 러너** | `scenario_simulator_v2` (소스 빌드) |
| **Autoware** | **SS_v2 가 띄운다** — "Autoware is then launched by scenario_simulator_v2" |
| 지도 | Vector Map Builder 로 등록 |
| 시나리오 | Scenario Editor 로 등록, OpenSCENARIO |
| 설정 | `.webauto-ci.yaml` |
| 산출물 | 결과 리포트 · stdout 로그 · **rosbag** |

실행 구조는 Kubernetes 다:

```
Simulation Node (Pod)
 ├─ wasim (container)                       지도·시나리오·AWSIM 바이너리·이미지 내려받고 오케스트레이션
 └─ Autoware + scenario_simulator_v2 + AWSIM   ← 셋이 한 컨테이너
        docker run
          xvfb-run AWSIM.x86_64
          ros2 launch scenario_test_runner ...
```

---

## 2. 우리와 다른 점 셋, 그리고 각각의 대가

### ① SS_v2 는 플랜트가 아니다 — AWSIM 이다

우리가 쓰는 `simple_sensor_simulator` 의 차량 모델은 **AWSIM 이 없을 때의 축약 물리**다.
그래서 2026-08-11 에 만난 두 가지가 설명된다
([evidence §8](evidence/autoware-return-path-2026-08-11.md)):

- 차량 모델(`EgoEntitySimulation`)이 **EgoEntity 전용**인 이유 — 본래 플랜트는 AWSIM 이다
- concealer 가 AD API 로 route·engage·**localization 초기화**까지 하는 이유 — 본래 SS_v2 가
  Autoware 를 띄우므로 그게 정상 흐름이다

### ② Autoware 를 우리는 따로 띄운다

TIER IV: SS_v2 → Autoware 기동. 우리: KATECH 파티션을 먼저 띄우고 `launch_autoware:=false`.

**이 분기점이 concealer 불일치의 근원이다.** 우리 파티션 이미지에는 localization 이 설치조차
없어서 concealer 의 초기화가 완료될 수 없다([evidence §7-§9](evidence/autoware-return-path-2026-08-11.md)).
파티션이 주어진 전제이므로 우리가 잘못한 것은 아니지만, **상류가 전제하는 흐름 밖에 있다**는
사실은 적어둬야 한다.

### ③ 전부 한 컨테이너 = 도메인·RMW 문제가 구조적으로 없다

TIER IV 구성에는 도메인 분리도, Cyclone↔Fast DDS 혼재도, TypeObject 교환도 **발생할 자리가
없다.** 셋이 한 프로세스 공간에 있기 때문이다.

2026-08-11 에 우리가 하루를 쓴 문제들 — 우리 `EgoState` writer 가 Autoware 파티션을
segfault 시킨 것, 도메인 통일 여부, `fastdds.type_propagation` —
**은 Autoware 를 별도 파티션으로 쪼갠 대가다**
([stage3 문서](sils-stage3-autoware-partition.md) 도메인 42 절).

---

## 3. ★ `TimeScale 0.4` — 우리 시계 결정의 성립 조건을 드러낸다

> AWSIM is currently configured to run with a **TimeScale of 0.4**. It has not been validated
> at a TimeScale higher than this value.

TIER IV 기준 구성은 **0.4배속**이다. 그런데 우리가 채택한
`stamp_policy=epoch-anchored`([evidence §5](evidence/ss2-clock-survey-2026-08-11.md))의
성립 조건은 **`realtime_factor = 1.0`** 이다.

| | 0.4배속에서 |
|---|---|
| sim 시간 대 벽시계 | 초당 **600 ms** 씩 벌어진다 |
| 20 ms 허용치 | **33 ms** 만에 초과 |
| 앵커 감시 | 즉시 `STAMP ANCHOR STALE` |

**그리고 그것은 오탐이 아니라 정탐이다.** 고정 오프셋이 실제로 무의미해지는 구성이다.

> 그래서 `ss2-clock-survey` §3 의 "tick 브리지는 지금 필요 없다" 는 **우리가 1.0 으로
> 돌리기 때문에** 성립한다. TIER IV 기준 구성(또는 realtime_factor ≠ 1 인 어떤 구성)에서는
> tick 브리지 또는 `use_sim_time` 전면 도입이 **필요하다.** 감시가 그 경계를 지킨다.

---

## 4. 우리 용어 정리를 확인해준다

지도는 **Vector Map Builder**, 시나리오는 **Scenario Editor** 로 등록하고, Evaluator/wasim
이 그것을 내려받아 실행한다. 즉 **Web.Auto 는 저작·등록 플랫폼이고 런타임 플랜트가 아니다** —
[§1.1](sils-integration-topology.md) 의 정리와 일치한다.

---

## 5. 그래서 우리 구조는 맞나 — 맞다, 다만 전제가 하나 드러난다

**TIER IV Evaluator 에는 vECU 가 없다.** Autoware 대 시나리오의 E2E 평가다. 우리는 **차량
플랫폼 ECU 를 검증**하려는 것이므로 `vehicle_interface` 자리를 우리가 차지해야 한다
([topology §3](sils-integration-topology.md)). 목적이 다르므로 상류 스택이 우리를 배려해줄
이유가 없다.

그리고 **기성 시뮬레이터는 전부 자기 vehicle interface 를 갖고 온다:**

| 플랜트 | 그 자리에 이미 있는 것 | 우리 vECU 가 들어가려면 |
|---|---|---|
| SS_v2 | concealer | 우회 필요. **이 파티션에서는 불가**(§2-②) |
| **AWSIM** | AWSIM 자체가 `/vehicle/status/*` 발행 · `control_cmd` 구독 | 같은 문제가 반복될 것으로 보인다 (미확인) |
| **MORAI** | **없다 — 어댑터를 우리가 쓴다** | **자리가 비어 있다** |

**이것이 MORAI 가 본선인 진짜 이유다.** 편의가 아니라 구조적 이유다 — 어댑터를 우리가 쓰는
유일한 플랜트라서 seat 를 우리가 소유한다.

> **한 줄**: 우리 구조는 맞고, 필요한 것은 **"vehicle interface 를 우리가 소유하는 플랜트"**
> 다. SS_v2·AWSIM 은 자기 것을 갖고 오고, MORAI 는 갖고 오지 않는다.

### ⬜ 확인 안 한 것

- **AWSIM 이 정말 `/vehicle/status/*` 를 직접 발행하는가.** 위 표의 AWSIM 행은 이 PDF 가
  아니라 AWSIM 의 일반적 구조에 대한 이해에서 나온 것이다. AWSIM 을 플랜트로 검토할 때
  **먼저 확인할 것** — 만약 명령 입력 토픽을 인자화할 수 있으면 SS_v2 보다 유리하다.
- AWSIM 을 우리 KATECH 파티션과 조합할 수 있는가(TIER IV 는 Autoware 도 자기가 띄운다).

---

## 6. 운영상 알아둘 것

| | 원문 | 우리에게 |
|---|---|---|
| rosbag 부하 | 토픽을 많이 기록하면 CPU·메모리·I/O 부하로 **시뮬레이션이 실패**할 수 있다 | 우리 compose 가 `record:=false` 를 기본으로 둔 판단과 일치 |
| NVIDIA | **Autoware 컨테이너에서 NVIDIA 드라이버 라이브러리를 제거**해야 한다(호스트 것을 쓰게). `cuda_install_drivers=false` | 지금은 GPU 없는 `run_partitions.sh` 경로라 무관. **compose 경로(`nvidia` 예약)로 갈 때 걸린다** |
| AWSIM 빌드 | Evaluator 는 AWSIM 을 빌드하지 않는다 (바이너리 등록) | AWSIM 도입 시 바이너리 확보 경로가 별건 |
