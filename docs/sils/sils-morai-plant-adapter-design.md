# SiLS — MORAI / AWSIM Plant 어댑터 설계안 (3안 비교)

> 상태: **설계 검토 단계 (구현 전)**
> 대상: `service/sils/vecu` (libsils_vecu SDK)
> 관련: [sils-morai-autoware-topology.html](sils-morai-autoware-topology.html) ①/④, [sils-morai-simulator-integration-review.md](sils-morai-simulator-integration-review.md), 아키텍처 v3 §11 (L3 plant coupling)
> 작성: 2026-07-22

---

## 0. 이 문서의 목적

MORAI(또는 TIER IV **AWSIM**)를 SiLS vECU의 plant로 붙일 때, **어느 포트로 어떻게 결합할지**를 확정하기 위한 설계 비교. 코드부터 짜지 않고 **프레이밍 결정**을 먼저 내리는 게 목적 — 이 결정이 이후 구현 전체를 가른다. earned-abstraction 원칙상, "MORAI라는 두 번째 실소비자"가 생겼으므로 지금이 추상화를 확정할 시점이다.

---

## 1. 갭 — 두 축으로 정확히

현재 SDK의 plant 결합(`IPlant`, `BehaviorContext`, `BehaviorOutput`)은 **스칼라(SISO) + in-process**를 전제한다. MORAI/AWSIM은 두 축 모두에서 다르다.

| 축 | 현재 계약 | MORAI / AWSIM |
|---|---|---|
| **채널 수** | **SISO** — `IPlant::SetInputs(double)`, `Step(dt)→double`, `output()→double`; `BehaviorContext.plant_feedback` 1개; `BehaviorOutput.value` 1개 | **MIMO** — 입력: 조향·가감속·브레이크·기어…; 출력: pose·velocity·wheel_speed·gps·imu·camera·lidar… |
| **결합 위치** | **in-process** — vECU tick 루프가 `plant->Step(dt)`로 직접 적분 (동기) | **out-of-process** — 별도 시뮬레이터, DDS 버스(또는 자체 프로토콜→어댑터) 너머 |
| **소유권** | 그 vECU 전용 (1:1) | **공유 세계, authority=morai** — N개 vECU + Autoware가 동시에 STATE 소비 |
| **시간 전진** | tick 루프가 dt 부여 | MORAI 자체 물리엔진이 전진 (외부 클럭) |

### 현재 코드의 결합점 (변경 영향 범위)

- [plant.hpp](../../service/sils/vecu/src/plant.hpp): `IPlant` 포트 + `AnalyticPlant`/`ReplayPlant`/`FmuPlant`.
- [behavior.hpp](../../service/sils/vecu/src/behavior.hpp): `BehaviorContext.plant_feedback`(double), `BehaviorOutput.value`(double).
- [run_loop.hpp](../../service/sils/vecu/src/run_loop.hpp): tick 루프가 폐루프를 닫는 곳 —
  `plant->SetInputs(out.value); double y = plant->Step(tick.dt_ns); ctx.plant_feedback = plant->output();`
  그리고 `plant_step` trace `{feedback_in, command, output}` 방출.
- **conformance**: `evaluate.py analyze_plant`이 trace에서 `feedback_in[N] == output[N-1]`(폐루프 연속성), 상수 dt, 유한값을 판정. `plant_step`은 golden 비교 이벤트.

> 핵심: 세 곳(포트·컨텍스트·루프)이 모두 스칼라를 가정한다. MIMO는 이 셋을 동시에 건드린다.

---

## 2. 세 가지 안

### Option 1 — `IPlant`를 MIMO in-process 코시뮬로 확장

`IPlant`를 벡터화(`SetInputs(span<double>)`, `Step(dt)→span<double>`)하고, MORAI 어댑터(`MoraiPlant : IPlant`)가 매 tick DDS 요청/응답으로 MORAI를 한 스텝 구동. 폐루프는 여전히 tick 루프의 `plant->Step()`에서 닫힌다.

```
vECU tick 루프
  ├ behavior->OnTick()  → 명령 벡터
  ├ plant->SetInputs(cmd_vec)      ─DDS→ MORAI (step 요청)
  ├ plant->Step(dt)     ←DDS─ MORAI 출력 벡터
  └ ctx.plant_feedback = plant->output_vec()
```

**장점**
- "plant는 IPlant 뒤에 있다"는 기존 정신 모델·conformance(`analyze_plant`) 그대로 재사용.
- Analytic/FMU와 대칭 — factory `MakePlant("morai", …)` 한 줄로 스위치.
- 폐루프 연속성 판정(`feedback_in[N]==output[N-1]`)이 그대로 성립.

**단점 (치명적)**
- **공유 세계를 한 vECU가 구동**하게 된다 → N개 vECU + Autoware가 MORAI를 공유하는 실제 토폴로지(①/④)와 **모순**. 두 번째 vECU를 붙이는 순간 "누가 MORAI를 step 하나?" 충돌.
- MIMO를 `BehaviorContext`/`BehaviorOutput`에 강제 → behavior 포트까지 스칼라 전제 붕괴.
- MORAI가 vECU tick당 정확히 1 step 하도록 요청/응답을 동기화해야 함(사실상 lockstep을 IPlant 안에 숨김) — 실패/타임아웃 처리가 포트 뒤로 감춰져 §14 fail-fast가 흐려짐.

**구현 규모**: 중~대. `IPlant`/`behavior.hpp`/`run_loop.hpp` 세 곳 시그니처 변경(파급 큼) + MORAI DDS 어댑터 + conformance 벡터화.

---

### Option 2 — MORAI는 IPlant가 아니라 "버스 결합 외부 세계" 모드 (권장)

MORAI 모드에서 vECU는 **로컬 plant를 돌리지 않는다.** MORAI가 STATE를 DDS 버스에 pub → vECU는 그것을 **staged 입력**(기존 input-staging / `IWireCodec::Take` 경로)으로 소비하고, ACTUATION을 **pub**(기존 wire write 경로). **폐루프는 버스에서 닫힌다** — 토폴로지 ① 그대로.

```
MORAI ──STATE(N토픽)──► DDS 버스 ──► vECU (staged 입력, MIMO 읽기)
                                         │ behavior
  MORAI ◄──ACTUATION──── DDS 버스 ◄────── vECU (pub, MIMO 쓰기)
  (폐루프는 MORAI가 버스 너머에서 닫음)
```

"plant 결합"을 **기존 wire + input-staging 포트의 MIMO 확장(채널맵)** + MORAI와의 **lockstep 시간계약**으로 실현한다. in-process `IPlant`(Analytic/FMU/Replay)는 **외부 시뮬 없는 standalone L3용으로 그대로 유지** — 삭제·변경 없음.

새로 필요한 것:
- **채널맵(ChannelMap)**: 이름있는 신호 ↔ (DDS 토픽, VSS path, 타입) 매핑. 입력(MORAI→vECU STATE) N개, 출력(vECU→MORAI ACTUATION) M개. 선언적 설정(yaml/env)으로.
- **MIMO staged 입력**: 여러 타입 토픽을 tick 경계에서 얼려 behavior에 전달 → **다중 엔드포인트 작업과 동일 문제**.
- **lockstep 시간계약**: MORAI가 vECU tick당 dt 한 번 전진하도록 tick-control 동기(기존 `LockstepClock`/`TickComplete` 프로토콜 확장).

**장점**
- 실제 토폴로지(공유 세계, authority=morai, N consumer)와 **정합** — 두 번째 vECU를 붙여도 각자 버스에서 구독만 하면 됨. 구동 충돌 없음.
- **"MORAI 어댑터"와 "다중 엔드포인트"라는 두 티어2 항목이 하나로 합쳐진다** — 둘 다 MIMO-over-DDS. MIMO의 두 번째 실소비자가 곧 MORAI라 earned-abstraction에 정직.
- in-process `IPlant`·`analyze_plant` 손대지 않음 → 기존 L3 회귀 무손상.
- fail-fast가 포트 뒤에 숨지 않고 wire/clock 경계에 그대로 노출.

**단점**
- 폐루프 연속성 판정(`feedback_in[N]==output[N-1]`)이 **더 이상 성립하지 않음** — 루프가 버스 너머라 새 conformance 관점(버스 왕복 지연·lockstep 정합) 필요.
- `BehaviorContext`/`BehaviorOutput`에 **다중 채널 접근자**가 필요(스칼라 유지 + 채널맵 확장). 다만 이건 plant 전용이 아니라 MIMO 공통 수요.
- 채널맵/VSS 매핑이라는 **새 설정 표면**이 생김.

**구현 규모**: 대(범위 넓음)이나 **기존 포트 재사용** — 시그니처 파괴적 변경 최소. 새 축은 "채널맵 + MIMO staged 읽기 + lockstep 확장".

---

### Option 3 — 하이브리드: `IExternalPlant` 얇은 포트로 Option 2를 "plant 모드"로 노출

Option 2의 채널맵+lockstep을 **새 포트 `IExternalPlant`** 뒤에 두고, 개념적으로는 여전히 "plant"로 부른다. `MakePlant("morai", …)`가 in-process 대신 external을 반환. RunProbe는 external plant면 `Step()`을 in-process로 호출하지 않고 "버스가 닫는다"를 안다.

**장점**
- 개발자 API 표면에서 "plant를 고른다"는 일관성 유지(Analytic ↔ FMU ↔ MORAI 한 축).
- Option 2의 토폴로지 정합성 + Option 1의 "plant 뒤에 있다" 멘탈모델을 절충.

**단점**
- 포트가 하나 더 늘어남(`IPlant` + `IExternalPlant`) — earned-abstraction 위반 소지(실소비자 1개뿐인데 새 포트). Option 2가 기존 포트로 충분하면 과잉.
- in-process vs external 분기가 RunProbe에 노출 → "plant 뒤에 숨긴다"는 이점이 반쯤 무너짐.

**구현 규모**: 중. Option 2 + 포트 래핑 1겹.

---

## 3. 부차 결정 — 시간 동기 (모든 안에 공통)

| 방식 | 내용 | SiLS 적합성 |
|---|---|---|
| **lockstep** | MORAI가 vECU tick당 dt 한 번 전진. 기존 `LockstepClock`/`TickComplete` 확장 | **결정론 보장 → 목표.** 단 MORAI/AWSIM이 외부 스텝 제어(step API/동기 모드)를 지원해야 함 |
| **free-run** | MORAI 실시간 자유주행, vECU는 최신 샘플 채택 | 비결정론(재현성↓). S1 초기 브링업/데모용 |

> AWSIM은 ROS2 네이티브라 lockstep 제어 훅(`/simulation/*`)이 상대적으로 접근 용이. MORAI는 시뮬레이터 스텝 제어 API 지원 여부 확인 필요(→ integration-review 후속).

---

## 4. 권장안과 근거

**권장: Option 2 (버스 결합 외부 세계).** 필요시 API 일관성이 중요하다고 판단되면 Option 3로 얇게 감싼다.

근거:
1. **토폴로지 정합** — 방금 확정한 ①/④ 그림이 이미 MORAI를 "버스 위 공유 세계, authority=morai"로 규정. Option 1은 이와 정면 모순.
2. **티어2 통합** — MORAI 어댑터 = 다중 엔드포인트 = MIMO-over-DDS. 한 번에 해결하면 중복 설계 없음.
3. **회귀 무손상** — in-process `IPlant`/`analyze_plant`를 안 건드림.
4. **earned-abstraction** — MIMO의 실소비자(MORAI)가 실제로 등장했으므로 지금 MIMO 채널맵을 확정하는 게 정당. 반면 새 포트(Option 3)는 실수요가 더 쌓이기 전엔 보류 가능.

---

## 5. 단계화 (권장안 기준, S-레벨)

- **S0 (설계 확정)** — 이 문서 리뷰 + 프레이밍 결정. 채널맵 스키마 초안. ✅ **완료**.
- **S1 (브링업, free-run)** — 채널맵 1~2채널로 MORAI/AWSIM STATE 1종 구독 → behavior → ACTUATION 1종 pub. 비결정론 허용. 버스 왕복 실증. ✅ **완료·develop 머지**(EgoState/VehicleControl 정규 IDL + `VehicleWireCodec` + `RunVehicleProbe` + fixture; steer-ecu·vehicle-interface 예제).
- **S2 (MIMO staged 읽기)** — 다중 타입 토픽 tick 경계 freeze + `BehaviorContext` 다중 채널 접근자. 다중 엔드포인트 작업 흡수. ✅ **완료** — 아래 §5.1.
- **S3 (lockstep)** — MORAI/AWSIM 스텝 제어 연동, `TickComplete` 확장. 결정론 회복. ✅ **완료** — 아래 §5.2.
- **S4 (conformance)** — 버스 결합 폐루프용 새 판정(왕복 지연·lockstep 정합·채널 완전성). `analyze_plant`와 별도 관점. ✅ **완료** — 아래 §5.3.
- **S5 (결함 주입 / negative path)** — 인바운드 STATE를 결정론 tick 스케줄로 교란(drop/nan/oor)해 컨트롤러 **안전 저하** 검증. `IScenario`(아웃바운드 VISS)의 버스 짝. ✅ **완료** — 아래 §5.4.

### 5.1 S2 구현 — tick 경계 입력 staging (§7.3 계약 재사용)

VISS 경로가 이미 확립한 §7.3 입력 staging 계약(`tick_inbox.hpp`: 도착마다 `recv_seq` 부여 → tick 경계 `FreezeTick()` → `input_staging_on`/`input_received`/`tick_input_frozen` trace, 불변식 = "tick순 recv_seq 이어붙이면 strictly increasing")을 **플랜트결합 경로에 타입 있는 형태로 미러링**한다.

- **왜 타입 버전인가**: VISS는 도착 **개수**만 필요(behavior가 count 소비)하지만, 플랜트결합 behavior는 **state/target 값**을 소비(피드백 제어)한다. 그래서 `VehicleTickInbox<T>`(`vehicle_staging.hpp`)는 `recv_seq`와 함께 **디코드된 타입 샘플**을 버퍼링한다 — `TickInbox`의 타입 카운터파트.
- **MIMO**: in-채널마다 인박스 1개(EgoState용 / ControlTarget용). 각 채널 스트림이 독립적으로 pin되어 채널별로 §7.3 불변식을 만족하고 attribution 가능(`tick_input_frozen`에 `channel`·`kind` 필드 가산).
- **배선**: `SILS_STAGE_INPUT=1`이면 in-채널 reader에 staging **리스너**를 붙여 **진짜 도착 시점**에 `recv_seq`를 부여하고, tick 경계에서 인박스를 freeze한다(폴링 take 아님). off면 S1 free-run 폴링(도착순 최신 채택)을 **정확히 그대로** 보존 — 이미 머지·런타임 검증된 경로 무변경.
- **behavior가 보는 값**: freeze된 집합의 **최신 샘플**(ZOH — 도착 없는 tick은 이전 값 유지). 즉 staging은 제어 의미를 바꾸지 않고 "어느 raw 도착이 이 tick에 속하는가"만 결정론적으로 고정 + 감사가능 trace를 낸다. `late_sample_policy`=`next_tick`(늦은 도착은 다음 freeze로).
- **다중 채널 접근자**: 현 behavior(longitudinal/steering/VCU)는 state 1 + target 1을 소비하므로 `VehicleBehaviorContext`는 `state`/`target`로 collapse. state 채널을 >1개 개별 노출하는 접근자는 **그런 behavior가 생길 때** 추가(earned-abstraction) — freeze 인프라는 이미 채널별이라 확장 저비용.

**결정론 성격**: staging은 *소비 순서*를 고정(local-staged)하나 *도착 시점*은 여전히 wall clock. 완전 재현성은 S3 lockstep(공유 sim 클럭)에서 회복된다 — S2는 그 전제(어느 샘플이 어느 tick인지 pin)를 깐다.

### 5.2 S3 구현 — lockstep 폐루프 결정론 (기존 ClockMaster/TickControl 재사용)

기존 lockstep 인프라(`sils_clock_master` + `TickControl.idl` `TickGrant`/`TickComplete` + `LockstepClockSource`)는 **일반 barrier**다 — grant를 구독하고 TickComplete를 응답하는 참가자면 누구나 barrier에 포함(`--participants N`). `RunVehicleProbe`는 이미 `MakeClockSource`로 이 경로를 타므로 `SILS_CLOCK_MODE=lockstep`이면 플랜트결합 프로브가 그대로 lockstep 참가자가 된다. **갭은 시뮬레이터(state 생산자)가 barrier 밖 wall-free-run**이라는 것 — 그러면 소비 *순서*만 결정론(S2)이고 소비 *값*은 wall 타이밍 의존.

- **시뮬레이터를 barrier에 넣는다**: 소스 fixture(`sils_ego_source`/`sils_control_source`)에 lockstep 모드 추가. `SILS_CLOCK_MODE=lockstep`이면 프로브와 **동일한 `LockstepClockSource`를 재사용**해 barrier 참가자가 된다(빌드 변경 0 — `sils_vecu`가 이미 TickControl 제공). grant마다 state를 **sim_time_ns의 순수 함수**로 계산(ego: `vx=a·t`, `px=½a·t²`)·발행하고 `TickComplete("ego-source")` 응답. 실제 MORAI/AWSIM 어댑터가 구현할 계약과 동형(“grant 받으면 sim 한 스텝 전진→state 발행→complete”).
- **결정론 회복의 의미**: free-run(S2)은 소비 순서만 고정, 소비 값은 wall 지터 의존. lockstep-aware 소스는 버스 위 샘플 값이 sim_time 함수 → 두 실행이 **동일 trace**(값 수준 재현). `analyze_lockstep`(evaluate.py): tick_seq 1..N +1, sim_time 상수 dt, 모든 tick barrier 닫힘, timeout 없음.
- **인과/1-tick lag**: 단일 barrier에서 grant T는 전원 동시 수신 → 같은 tick 내 sim의 state(T)와 vECU freeze는 경쟁. barrier가 T-1을 닫은 뒤에야 T를 grant하므로 vECU는 tick T에서 **T-1까지의 state를 결정론적으로** 소비(reliable 전달 + barrier 왕복 시간창; grant 수신 직후 freeze하므로 sim의 state(T)가 그 사이 전달될 여지는 사실상 없음). 즉 결정론적 **1-tick I/O 지연**. zero-lag 동일-tick 결합은 phased barrier(서브틱)가 필요 — 향후.
- **IDL 무변경**: `TickComplete`는 `participant_id`만 있으면 충분(sim 참가자는 고유 id `ego-source`/`control-source`). "TickComplete 확장"은 타입이 아니라 **참가자 집합 확장**을 뜻함(earned-abstraction: 타입은 안 건드림).
- **종료·꼬리**: **대칭 규칙 = clock_master `--ticks` == 소스 `--count`**. ClockMaster가 tick을 다 돌고 종료하면 **모든 lockstep 참가자가 자동 종료** — `LockstepClockSource`가 grant writer 연결 해제(마스터 종료)를 감지(`on_subscription_matched` current_count 0, ever-matched 가드)해 `WaitForNextTick`이 false를 반환하므로, 참가자가 grant를 무한 대기하며 hang하지 않는다(수동 kill 불필요). 1-tick lag상 마지막 tick 도착 샘플은 후속 tick이 없어 freeze가 안 되는데, **프로브가 종료 직전 남은 입력을 drain-freeze**(`tick_input_frozen … "drain":true`, reception order라 §7.3 불변식 유지)해 **staging 꼬리 손실 0**. `--ticks`/`--count` 산수 없이 대칭만 지키면 됨.
- **트레이스 원자성**: 플랜트결합 런은 메인 루프 + 채널별 staging 리스너 스레드 + 클록 TickDone이 각자 stdout에 JSONL을 쓴다. `cout << … << endl`은 스레드 간 원자적이지 않아 lockstep의 촘촘한 타이밍에서 라인이 인터리브·손상 → 하류에서 "froze never-received" 팬텀 위반으로 나타난다. `TraceMutex`(clock_source.hpp)로 모든 오프-메인 트레이스 emit를 직렬화해 해결.

**검증**: 두 실행의 프로브 `control_published` 시퀀스가 bit-동일(값 재현) + `evaluate.py`가 combined trace를 `analyze_lockstep` PASS로 판정.

### 5.3 S4 구현 — 버스결합 폐루프 conformance 판정

S3까지 재현 가능한 trace를 *만들었으니*, S4는 conformance 툴이 그 trace를 **자동 합격/불합격 판정**하게 한다. 코드 무관·Python(evaluate.py/validate_trace.py/gate.py)+스키마 작업.

- **스키마 등재(`trace-schema/v1.json`)**: vehicle 이벤트(`channelmap_start`·`channel_bound`·`state_received`·`target_received`·`control_published`)를 producer `vecu`로, lockstep fixture(`ego_source_*`·`control_source_*`)를 producer `sim_fixture`로 추가. 다중 producer가 공유하는 이벤트(`ready`·`behavior_start`·`input_staging_on`·`run_stopping`/`stopped`)의 `required`는 실제 교집합으로 완화(vehicle 프로브는 VISS의 seq/topic envelope를 안 emit) — `run_start` 노트와 동일 철학. `input_received`/`tick_input_frozen`엔 MIMO용 `channel` 필드 명시.
- **`analyze_staging` MIMO 대응**: 기존엔 전 채널 recv_seq를 전역 concat해 sorted+unique 검사 → 채널별 카운터(각 1..N)에 오탐. **채널별 그룹화**로 §7.3 불변식을 채널마다 판정(위반 메시지에 채널 라벨). 단일 스트림(VISS)은 채널 필드 없음 → 그룹 1개, 기존과 동일(backward-compatible).
- **신규 `analyze_vehicle`**: 버스결합 폐루프 판정(`analyze_plant`의 버스 짝). `control_published`가 있으면 버스결합 런. 검사 = tick당 1 명령·tick_seq 순증, 명령 well-formed(유한 steering/accel·유효 gear), **폐루프**(state_received≥1 소비, open-loop 아님). 값-수준 재현성은 여기서 안 봄(lockstep+compare_golden 몫).
- **fidelity 배선**: 버스결합 working loop(state in·control out) → L0. **폐루프 + lockstep 결정론** → L3(in-process `analyze_plant` L3와 대등 — 단 버스 결정론은 lockstep 필수라 `clock.deterministic` 요구). PASS 증거 게이트도 버스결합이면 VISS(sample/endpoint/verify_pass) 대신 **버스 증거(state_received·control_published)** 요구.
- **판정 권위**: 실배포는 Run Coordinator가 `run_pass`를 낸다(§5.2). coordinator 없는 raw 버스 런을 위해 **전용 bus verifier `sils_vecu_vehicle_verify`**(S4 후속) 추가 — VCU의 OUT 채널(`sils/ego/cmd`)을 구독해 명령을 검증(유한 actuation + 유효 gear)하고 `verify_pass`/`verify_fail`을 emit. 이는 이미 verifier producer로 스키마에 있고 `evaluate`가 `VERIFIER_TERMINAL`로 처리하므로 **스키마 변경 0으로 standalone terminal** 성립. VISS verifier의 버스 짝이며, 판정 자체(결정론·폐루프)는 `analyze_vehicle`가 하고 verifier는 "수신 가능 + well-formed"(버스 L0) 증거 + terminal만 제공.

**검증(sandbox 자체 완료)**: 커밋된 golden 샘플 `samples/vehicle-lockstep.trace.jsonl`(lockstep·2 in-채널·staging·1-tick lag)가 `gate.py` **SCHEMA+FIDELITY PASS(fidelity L3, conformant)**. evaluate/gate 단위테스트 +11(analyze_vehicle 6 + MIMO staging 3 + 샘플 gate 1 + …), 전체 141 passed. **채널 인터리브 오탐 회귀 테스트** 포함(채널 A/B 각 1..N이 전역 비정렬이어도 위반 아님).

### 5.4 S5 구현 — 인바운드 결함 주입 (negative path)

"로직이 도는가"를 넘어 "**센서 피드가 망가져도 안전한가**"를 검증한다. 기존 `IScenario`(scenario.hpp)는 **아웃바운드 VISS 요청**을 교란(drop/corrupt/duplicate)하고 `RunProbe`에만 붙어 있어 버스결합 경로엔 결함 인프라가 없었다. 그래서 대칭 포트를 staging 계층에 신설.

- **새 포트 `IInputFault`**(`vehicle_input_fault.hpp`, DDS-free) — `IScenario`의 **결정론 계약**을 그대로 재사용: 결함은 wall/RNG 아닌 **tick_seq 고정 스케줄**(`SILS_INPUT_FAULTS="10-14:drop,20:nan,30:oor"`)에서 나오므로 결함 런 자체가 golden 재현 가능. 3종: `drop`(센서 드롭 → 직전 값 ZOH 유지), `nan`(손상 프레임), `oor`(범위 초과). `NominalInputFault`/`ScheduledInputFault` 2구현으로 earned.
- **주입 지점 = staging 계층 interceptor**(`RunVehicleProbe`): 입력 취득(staging freeze 또는 free-run 폴링) **직후, behavior 소비 직전**에 적용 → 두 경로 모두 커버. `drop`은 tick 시작 시점 스냅샷(`prev_latest`)으로 복원해 아웃티지 동안 마지막 good 값 유지. `nan`/`oor`는 소비할 `EgoStateSample` 필드를 그 자리에서 손상. tick마다 `input_fault`(mode) + 시작 시 `input_fault_on`(스케줄 크기) 트레이스. **일반적 위치라 실 브리지가 와도 재사용**(fixture든 실 MORAI든 정규 계약 위에서 교란).
- **판정 `analyze_fault`**(evaluate.py) — 두 축: (1) **완전성** — `input_fault_on` 선언 수 == 실제 `input_fault` 수(조용히 미주입되면 결함 런이 거짓 PASS). (2) **강건성**(핵심) — 각 결함 tick의 `control_published`가 **유한 + 유효 gear** 유지 = 컨트롤러가 손상/부재 상태를 액추에이션으로 **전파하지 않고 안전 저하**. 전파 시 "did not degrade safely"(§14). 결함 원인 tick과 제어 결과를 직접 연결(전역 finiteness 검사로는 귀속 불가). `report["fault"]`에 declared/injected/by_mode/robust 기록.

**검증(sandbox 자체 완료)**: 포트 단위테스트(스케줄 파싱·range·apply 3종) + `analyze_fault` 4테스트(none/robust-conformant/선언불일치/전파실패), 전체 **148 passed**. 스키마 v1에 `input_fault`/`input_fault_on`(producer vecu) 등재. 결함 런타임 왕복은 빌드PC.

---

## 6. 열린 질문 (리뷰에서 결정)

1. **프레이밍**: Option 2 단독 vs Option 3(포트 래핑)?
2. **채널맵 위치**: 새 yaml(예 `config/sils/channelmap/*.yaml`) vs 기존 speclist/env 확장?
3. **VSS 정합**: 채널 신호명을 VISS/VSS path에 직접 바인딩할지(vehicle_info_mgr 계약 재사용) vs 독립 이름공간?
4. **lockstep 대상**: AWSIM 우선(ROS2 훅 용이) vs MORAI 우선(원 요구)?
5. **BehaviorOutput MIMO**: 스칼라 `value` 유지 + 채널 사이드채널 vs 구조 교체?

---

*리뷰 후 프레이밍이 확정되면 S1 스캐폴딩(채널맵 스키마 + free-run 브링업)부터 착수.*
