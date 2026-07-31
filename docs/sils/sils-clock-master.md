# SiLS Clock Master — lockstep 시간 권위

> lockstep 결정론을 구동하는 **독립 프로세스**. TickGrant/TickComplete barrier로 참가자들의 tick을 동기 전진시킨다.
>
> 근거 코드: [`service/sils/vecu/src/sils_clock_master.cc`](../../service/sils/vecu/src/sils_clock_master.cc) · 참가자 측 [`clock_source.hpp`](../../service/sils/vecu/src/clock_source.hpp) · 계약 [`idl/dds/sils/TickControl.idl`](../../idl/dds/sils/TickControl.idl)
>
> 📊 시각 요약: [sils-clock-master.html](sils-clock-master.html)

---

## 1. 무엇이고, 왜 별도 프로세스인가

Clock Master는 run의 **시간 권위(time authority)** 다. 매 tick마다 `TickGrant`를 발행하고, **모든 참가자**의 `TickComplete`를 받은 뒤에야 다음 tick을 grant한다(=barrier). 이 barrier가 참가자들의 실행 순서·타이밍을 고정해 **동일 입력 = 동일 trace**(bit 재현)를 만든다.

**왜 vECU 안이 아니라 별도 프로세스인가** (아키텍처 v3 §8.2):
- **권위 분리** — 시간 배분 권한이 시험 대상(vECU) 바깥에 있어야 "vECU가 자기 시간을 스스로 정한다"는 오염을 막는다.
- **run당 1개 · 일회성** — run마다 하나가 떠서 `--ticks`를 다 돌면 종료. 병렬 run은 서로 다른 도메인에 각자의 Clock Master.
- **참가자와 동일 도메인** — 제어 평면(grant/complete)이 run의 DDS 도메인을 공유해야 만난다.

---

## 2. 역할과 배선

Clock Master는 **TickGrant 발행자 + TickComplete 구독자**다. 참가자(vECU 프로브 + 입력 소스)는 그 반대.

| 방향 | 토픽 | 타입 | 발행자 → 구독자 |
|---|---|---|---|
| grant | `sils/tick/grant` | `TickGrant` | Clock Master → 참가자 N |
| complete | `sils/tick/complete` | `TickComplete` | 참가자 N → Clock Master |

**계약** ([TickControl.idl](../../idl/dds/sils/TickControl.idl)):

```idl
struct TickGrant    { string run_id; unsigned long long tick_seq; long long sim_time_ns; long long dt_ns; };
struct TickComplete { string run_id; unsigned long long tick_seq; string participant_id; long status; }; // 0=ok
```

- **QoS**: 제어 평면은 `RELIABLE` + `KEEP_LAST(256)` — grant가 조용히 유실되지 않도록.
- **도메인**: `VISS_DDS_DOMAIN_ID`(참가자와 동일). `run_id` = `SILS_RUN_ID`.

---

## 3. 한 tick의 흐름 (barrier)

```
tick i:
  Clock Master ── TickGrant(seq=i, sim_time=i·dt, dt) ──▶ 모든 참가자
                                                          │  tick 실행 + drain(큐 소진)
  Clock Master ◀── TickComplete(seq=i, participant_id) ── 각 참가자
  N개 distinct participant 모이면 → barrier 닫힘 → 다음 grant(i+1)
```

- **grant = broadcast, complete = barrier** (순차 아님): 한 tick의 grant는 **`grant_writer->write()` 한 번**으로 발행되고 DDS가 **모든 참가자에게 동시 전달**한다(참가자별로 순차 배분하지 않음). 여러 vECU는 같은 `TickGrant(seq)`를 받아 **병렬로** tick을 실행하고, 각자 `participant_id`로 TickComplete를 보낸다. Clock Master는 `wait_for(seq, N)`로 **N개 distinct participant_id**가 모일 때까지 기다린 뒤 다음 grant → **틱 사이에만 순차(strict), 틱 안의 참가자 실행 순서는 서로 무관**. vECU 간 데이터는 tick 경계에서 staged(1-tick lag)라 실행 순서가 결정론에 영향을 주지 않는다.
- **핵심 불변식**: **모든 참가자의 complete를 받기 전엔 다음 tick을 grant하지 않는다.** → 순서·타이밍 고정.
- `sim_time_ns = tick_seq · dt_ns` — 즉 tick은 **1부터** 시작하고 첫 grant의 sim_time은 `dt`(0 아님). 참가자는 출력을 `sim_time`의 함수로 만들어 재현성을 얻는다.
- **timeout 분기**: `--tick-timeout-ms` 안에 N개가 안 모이면 `tick_timeout` 방출 후 종료(비결정론 = 게이트에서 실패).

---

## 4. 실행 방법

빌드하면 `bin/sils_clock_master` 실행 파일이 생긴다. 참가자들과 **같은 도메인**에서 백그라운드로:

```bash
export VISS_DDS_DOMAIN_ID=0 SILS_RUN_ID=vi SILS_CLOCK_MODE=lockstep

bin/sils_clock_master --participants 3 --ticks 40 --dt-ms 100 --tick-interval-ms 100 &   # ① 시간 권위
bin/vehicle_interface &            # ② 참가자 = vECU  (SILS_CLOCK_MODE=lockstep 이면 자동 참가)
bin/sils_ego_source     --count 40 &
bin/sils_control_source --count 40 &
wait
```

### 인자

| 인자 | 기본 | 뜻 |
|---|---|---|
| `--participants N` | 1 | barrier가 기다릴 참가자 수. **lockstep 참가자 수와 정확히 일치**해야 함(위 예: vECU+소스2 = 3) |
| `--ticks T` | 10 | 총 tick 수. 다 돌면 종료 |
| `--dt-ms D` | 10 | tick당 sim_time 증가량(ms) |
| `--tick-interval-ms I` | 0 | grant 사이 **최소 wall 시간**. 0=free-run(barrier 닫히는 즉시 다음). wall-clock 입력 소스가 tick에 걸치게 하려면 dt와 맞춰 설정 |
| `--tick-timeout-ms` | 5000 | 한 tick의 complete 수집 제한. 초과 시 `tick_timeout`+종료 |
| `--startup-timeout-ms` | 10000 | 참가자들이 grant를 구독할 때까지 초기 대기 |

### env

| env | 뜻 |
|---|---|
| `VISS_DDS_DOMAIN_ID` | run 도메인. 마스터·참가자 **동일**해야 grant/complete가 만남 |
| `SILS_RUN_ID` | trace 상관용 run 식별자 |
| `SILS_CLOCK_MODE=lockstep` | **참가자 쪽** env — 이게 있어야 `LockstepClockSource`가 선택돼 barrier에 참가(없으면 wall-clock으로 돌아 마스터 무시) |

### ⚠️ 대칭 규칙

**`clock_master --ticks` == 소스 `--count`** (예: 둘 다 40). 마스터가 tick을 다 돌고 종료하면 grant writer 연결이 끊기고, 그걸 감지한 **모든 참가자가 자동 종료**한다. 프로브는 종료 직전 남은 입력을 drain-freeze → staging 꼬리 0. **수동 kill 불필요.**

---

## 5. 수명과 자동 종료

```
clock_start
  → (참가자 N개 grant 구독까지 대기; 초과 시 clock_start_timeout → exit 1)
  → tick 1..T: tick_grant → [barrier] → tick_barrier   (혹은 tick_timeout → break)
  → clock_stop(exit_code)
  → participant/writer 정리
```

- 마스터가 `--ticks` 완주 → **grant writer 연결 해제** → 참가자의 `GrantListener`가 `on_subscription_matched(count 0)` 감지(+ever_matched 가드) → `master_gone` → `WaitForNextTick`가 false 반환 → **참가자 자동 종료**.
- 이 자동 종료 덕에 hang/수동 kill 없이 drain이 신뢰성 있게 실행된다(과거 hang 버그의 근본 수정).

---

## 6. 결정론 계약

- **sim_time_ns가 시간 권위** — 실행 속도(wall)와 무관. 참가자는 `doStep`/출력을 `sim_time`의 함수로.
- **wall 지터에 둔감** — barrier는 논리적 순서지 실시간이 아니다. 그래서 Clock Master를 어디에 두든(바쁜 컨테이너 포함) 결정론은 안 흔들린다.
- **1-tick I/O 지연** — 단일 barrier라 sim↔vECU 간 결정론적 1-tick lag(T-1 소비). zero-lag은 phased barrier(향후, 실수요 시).
- 완전 재현의 전제: 참가자 모두가 lockstep 참가(`SILS_CLOCK_MODE=lockstep`) + 입력 소스가 sim_time 함수.

---

## 7. Trace 이벤트 & conformance

| 이벤트 | producer | 언제 |
|---|---|---|
| `clock_start` | clock_master | 시작(mode·participants·ticks·dt_ns) |
| `clock_start_timeout` | clock_master | 참가자 구독 미달 |
| `tick_grant` | clock_master | 매 tick grant(tick_seq·sim_time_ns·dt_ns) |
| `tick_barrier` | clock_master | tick의 N개 complete 수집 완료 |
| `tick_timeout` | clock_master | complete 미달 → 종료 |
| `clock_stop` | clock_master | 종료(exit_code) |
| `tick_complete` | 참가자(vecu/소스) | 자기 tick 작업 완료 응답 |

`evaluate.py`의 **`analyze_lockstep`**: `tick_grant` 존재 = lockstep, `tick_seq` 1..N 연속(+1씩), `sim_time` 상수 dt, barrier 닫힘, timeout 없음 → 결정론 판정. (스키마 v1에 이벤트 등재됨.)

---

## 8. 자주 겪는 함정

- **stale env** — 이전 세션의 `SILS_CLOCK_MODE=lockstep`가 셸에 남으면, 마스터 없이 vECU만 띄웠다가 grant를 기다리며 멈춘다. lockstep 안 쓸 땐 `unset SILS_CLOCK_MODE SILS_RUN_ID SILS_STAGE_INPUT`.
- **`--participants` 불일치** — 실제 lockstep 참가자 수와 다르면 barrier가 안 닫혀 `tick_timeout` → 비결정론(L0).
- **`--ticks` ≠ 소스 `--count`** — 어긋나면 grants>barriers로 timeout이 나거나 꼬리가 남는다. 대칭 규칙을 지키면 산수 없이 자동 종료+꼬리0.
- **`--tick-interval-ms 0` + wall 입력 소스** — free-run이 ms 안에 전 tick을 질주해 wall-clock 입력이 run 이후 도착=미staging. 이 경우 dt에 맞춰 interval을 준다.

---
*근거: sils_clock_master.cc(225L) · clock_source.hpp · TickControl.idl · evaluate.py analyze_lockstep. 관련: [sils-understanding](sils-understanding.md) 6장, [sils-fmu-plant](sils-fmu-plant.md), [sils-virtual-ecu-architecture-v3](sils-virtual-ecu-architecture-v3.md) §8.*
