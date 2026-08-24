# vECU 아비트레이션 — 여럿이 낸 명령을 하나로 만드는 규칙

> 2026-08-12 · 설계 + **구현·실측 완료** (§8)
> 선행 실측: [`evidence/multi-vecu-bus-2026-08-12.md`](evidence/multi-vecu-bus-2026-08-12.md)
> 관련: [`sils-vehicle-interface-autoware.md`](sils-vehicle-interface-autoware.md)(VCU 설계) ·
> [`sils-integration-topology.md` §3](sils-integration-topology.md)(채널 규칙)

## 0. 먼저, 스펙이 없다는 사실부터

이 문서가 다루는 규칙에 **권위 있는 출처가 없다.** 지금 VCU 로직은

```cpp
if (ctx.have_target) { /* Autoware target 을 그대로 통과 */ }
else                 { /* VI_SAFE_DECEL(2.0) 로 감속 */ }
```

이고, "왜 통과인가", "왜 2.0 인가" 의 근거가 코드에도 문서에도 없다. **자리 표시자다.**

실제 아비트레이션 규칙은 차량·법규·OEM 요구에서 오고, **고객/공급사가 가져오는 것**이다.
이 프레임워크의 역할은 규칙을 정하는 것이 아니라 **규칙이 주어졌을 때 시험 가능하게
만드는 것**이다. 그래서 이 문서는 두 가지를 분리해서 다룬다:

| | |
|---|---|
| **§1~§3 기본값** | 스펙이 없을 때 고를 수 있는, **틀려도 위험한 방향으로 틀리지 않는** 규칙 |
| **§4 검증** | 스펙이 없어도 "명백히 틀림" 을 잡는 그물 |
| **§5 선언** | 스펙이 왔을 때 **코드 변경 없이** 받을 자리 |

---

## 1. 왜 필요한가 — 판정은 ECU 단위, 액추에이터는 하나

두 요구가 충돌한다.

**판정은 ECU 단위여야 한다.** 출처가 다른 ECU 들(Adaptive AUTOSAR · FMU 안의 공급사 제어
math · 레거시 C)을 같은 자극에 걸고 각각 판정하는 것이 이 프레임워크가 파는 것이다.
통합된 결과 하나만 남기면 누가 틀렸는지, 어느 ECU 가 회귀했는지 못 가른다.

**그런데 차에 들어가는 명령은 하나다.** 통합 없이 여럿을 플랜트에 던지면 승자는 발행
순서가 정한다 — 그리고 아무도 오류를 안 낸다(실측: evidence §3 ①).

그래서 **각자 자기 채널로 내고(판정), 한 곳에서 합친다(통합)**. 합치는 곳이 VCU 다.

> ★ **두 층은 서로 환원되지 않는다.** 각 ECU 가 개별로 다 통과해도 통합이 위험할 수
> 있다 — A 가 정당하게 낸 감속과 B 가 정당하게 낸 조향이 동시에 적용되면 위험한 조합이
> 되는 식이다. **통합 규칙 자체가 시험 대상**이어야 하는 이유다.

**언제 필요 없나**: vECU 가 하나뿐인 구성. VCU 가 Autoware target 을 받아 넘기면 끝이고,
[autoware-return-path §1](evidence/autoware-return-path-2026-08-11.md) 구성이 이미 그것이다.
이 문서는 **둘 이상일 때만** 의미가 있다.

---

## 2. 규칙 — 축 소유권 + 단조 안전 덮어쓰기

### 2.1 축마다 소유자는 하나

액추에이션 채널마다 기본 소유자를 정한다. 기본 소유자는 상위 명령(Autoware target)이다.

| 축 | 채널 | 기본 소유자 |
|---|---|---|
| 종축 (accel/gear) | `sils/ego/cmd` | Autoware `ControlTarget` |
| 횡축 (steering) | `sils/ego/steer_cmd` | Autoware `ControlTarget` |

`steer-ecu` 가 원래부터 별도 채널을 갖는 것("액추에이터가 다르면 채널도 다르다",
topology §3)의 연장이다. **"VCU 가 하나" 가 아니라 "채널마다 소유자 하나"** 가 정확한 표현이고,
축이 늘면 소유자도 는다.

### 2.2 다른 ECU 는 **안전 방향으로만** 바꾼다

```
out.acceleration = min(target.acceleration, req_1.acceleration, …)   // 더 강한 제동이 이긴다
out.steering     = clamp(target.steering, -limit, +limit)             // 제한만, 새 값 주입 없음
out.gear         = target.gear                                        // 기어는 소유자만
```

**핵심 성질: 도착 순서와 무관하게 결과가 같다.** `min`·`clamp` 는 교환·결합법칙이
성립하므로 어느 ECU 가 먼저 도착하든 같은 명령이 나온다. S3 lockstep 골든 재현이
안 깨지고, "누가 먼저 왔느냐" 로 결과가 뒤집히는 사고가 원천적으로 없다.

### 2.3 무입력은 특수 케이스가 아니다

target 이 없으면 `-VI_SAFE_DECEL`. 이것도 §2.2 의 방향(안전 쪽)과 같으므로 별도 분기가
아니라 같은 규칙의 극단값이다. 지금 구현이 이미 그렇게 돼 있다.

### 2.4 대가 — 이 규칙이 **못 하는 것**

> **"이 ECU 가 상황을 더 잘 아니 더 세게 가속하자" 는 구조적으로 불가능하다.**

안전 논증의 대가다. 바꾸려면 축 소유권 자체를 넘기는 **모드 전환**(누가 종축 주인인지
바꾸는 상태기계)이 필요하고, 그건 훨씬 큰 별개 설계다. 지금 범위 밖으로 둔다.

### 2.5 왜 가중 평균/블렌딩이 아닌가

- **안전 논증이 안 된다.** 두 ECU 가 각각 안전한데 평균이 위험할 수 있다
- **튜닝 값이 시나리오마다 달라진다** → 재현성 논의가 무너진다

---

## 3. 결정을 남긴다

매 tick **어느 입력이 이겼는지** 트레이스에 남긴다. 안 남기면 왜 그 명령이 나왔는지
사후에 판단할 수 없다 — 카운터만으로는 "정상" 과 "한 ECU 가 계속 이기는 이상" 이 구별되지
않는다.

```jsonc
{"event":"arbitrated","tick_seq":41,
 "axis":"longitudinal","winner":"cmd_adaptive","target":-1.5,"chosen":-2.4,
 "inputs":{"target":-1.5,"cmd_adaptive":-2.4,"cmd_plainc":-1.0}}
```

---

## 4. 검증 — 스펙 없이도 잡을 수 있는 것

### 4.1 불변식 (반증 가능한 형태로만)

| # | 불변식 | 근거 |
|---|---|---|
| I1 | 출력 accel ≤ 모든 입력 accel 의 최소 | §2.2 의 직접 귀결. **어떤 vECU 도 차를 더 위험하게 못 만든다** |
| I2 | 무입력 T 초 이상 → 출력은 감속 | 통신 두절 시 안전 상태 |
| I3 | 출력이 차량 물리 한계 안 | `maxDeceleration` 등 차량 파라미터에서 온다 |
| I4 | gear 전이는 정지 상태에서만 | 주행 중 R 전환 금지 |

**이건 "VCU 가 옳다" 를 증명하지 않는다. 명백히 틀린 것을 걸러낼 뿐이다.** 스펙 부재
상황에서 정직하게 주장할 수 있는 전부이고, 그래서 전부 **반증 가능한** 형태로 썼다.

구현 자리: `sils_vecu_vehicle_verify` (이미 `sils/ego/cmd` 를 구독한다) + `requirements.yaml`
assert 엔진([functional-verification-interop](sils-functional-verification-interop.md)).

### 4.2 pass-through differential — 지금 rig 로 바로 된다

```
벤더 기본 경로:  Autoware ──control_cmd──▶ 플랜트
우리 경로:      Autoware ──▶ 브리지 ──▶ VCU ──▶ 플랜트
```

**VCU 가 통과 모드일 때 두 값이 같아야 한다.** 어긋나면 아비트레이션이 아니라 우리
매핑·단위·부호가 틀린 것이고, 그건 스펙 없이도 버그다.

2026-08-12 실측에서 Autoware 가 `-1.5` 를 냈고 VCU 출력도 `-1.5` 였다 — **눈으로만**
확인했다. 자동 판정으로 만드는 것이 다음 작업.

### 4.3 시나리오 결과 요구

`.xosc` 에서 뽑는 판정 기준(`tools/sils/conformance/import_openscenario.py`). VCU 로직의
스펙은 아니지만 **결과의 스펙**이다.

---

## 5. 스펙이 왔을 때 — 선언으로 받는다

규칙을 코드에 박지 않는다. 채널맵이 토픽 바인딩을 config 로 뺀 것과 같은 방식으로,
아비트레이션도 선언으로 둔다.

```yaml
# 예시 — 확정 아님
arbitration:
  longitudinal:
    channel: sils/ego/cmd
    owner: autoware/control_target
    rule: min                      # 안전 방향으로만
    inputs: [sils/ego/cmd_adaptive, sils/ego/cmd_classic, sils/ego/cmd_plainc]
  lateral:
    channel: sils/ego/steer_cmd
    owner: autoware/control_target
    rule: clamp
    limit_rad: 0.6
```

이렇게 두면 (1) 스펙이 와도 코드가 안 바뀌고, (2) **선언 자체를 요구사항과 대조**할 수
있다. 이 저장소는 이미 그 취향이다(`SILS_CHANNELMAP`, `requirements.yaml`).

---

## 6. 고쳐야 할 곳

| 파일 | 무엇 | |
|---|---|---|
| `service/sils/vecu/src/vehicle_behavior.hpp` | `ControlRequest` + `VehicleBehaviorContext.requests[]` | ✅ |
| `service/sils/vecu/src/vehicle_wire.hpp` | `TakeControl()` — `VehicleControl` **읽기** 경로(전에는 쓰기만) | ✅ |
| `service/sils/vecu/src/vehicle_run.hpp` | 세 번째 in 채널 종류 바인딩 + ZOH 유지 + `arbitrated` 트레이스 | ✅ |
| `service/sils/examples/vehicle-interface/src/vehicle_interface_behavior.hpp` | `min`/`clamp` + 결정 기록 | ✅ |
| `config/sils/channelmap/multi/vcu-arbitration.yaml` | in 채널 추가 (config 만) | ✅ |
| `service/sils/examples/vehicle-interface/test/test_arbitration.cpp` | 불변식 검사 30개, ROS·DDS 없이 | ✅ |

---

## 7. 이 문서가 주장하지 않는 것

- **`VI_SAFE_DECEL=2.0` 이 맞다** — 근거 없다. 지금은 고른 값이다
- **min/clamp 가 옳은 규칙이다** — 옳아서가 아니라 **틀려도 위험한 방향으로 틀리지
  않아서** 고른 것이다. ISO 26262 로 치면 기능 스펙이 아니라 fail-safe 기본값에 해당한다
- **여러 vECU 를 붙이면 더 좋다** — 하나면 이 층 전체가 불필요하다(§1)


---

## 8. 구현·실측 (2026-08-12)

### 8.1 단위 테스트 — 30 checks, ROS·DDS·시뮬레이터 없이

`service/sils/examples/vehicle-interface/run_tests.sh` (C++20 컴파일러만 필요).
`adapters/ss2/run_tests.sh` 와 같은 형태다 — 조용히 틀릴 수 있는 것을 버스에 올리기 전에 못박는다.

| 검사 | |
|---|---|
| I1 단조 안전 | 더 강한 제동이 이긴다 / **피어는 가속을 못 늘린다** / 여럿이면 최소 |
| **순서 무관** | 같은 집합을 정순·역순·회전으로 넣어 **명령도 승자도 동일** |
| I2 무입력 | safe stop, 그리고 피어는 그보다 **더** 제동할 수 있으나 **약화는 못 한다** |
| 횡축 | clamp 만. 피어가 조향을 **주입하지 못한다**. 한계 미설정이면 clamp 안 함 |
| 기어 | 피어가 주행 중 REVERSE 로 못 바꾼다 |
| NaN | 깨진 피어가 명령을 가져가지 못한다 (순진한 비교는 NaN 을 통과시킨다) |
| 회귀 | 피어가 없으면 **예전과 동일**한 통과 동작 |

### 8.2 rig 실측 — 양방향으로 확인

SS_v2 플랜트 + Autoware + vECU 5개(컨테이너) 위에서, VCU 를
`config/sils/channelmap/multi/vcu-arbitration.yaml` 로 올렸다. 바인딩 in 5 / out 1 확인.

**가속 요구는 무시된다:**
```json
{"event":"arbitrated","tick_seq":70,"owner_acceleration":-1.5,"chosen_acceleration":-1.5,
 "inputs":{"Request.adaptive":8,"Request.plainc":8}}
```

**제동 요구는 채택된다** (제동 피어 하나 추가: `SILS_CTRL_SETPOINT=-1.0` → −4 요구):
```json
{"event":"arbitrated","tick_seq":261,"owner_acceleration":-1.5,"chosen_acceleration":-4,
 "inputs":{"Request.adaptive":8,"Request.classic":-4,"Request.plainc":8}}
{"event":"control_published","tick_seq":261,"acceleration":-4,"gear":3}
```

owner 가 −1.5 인데 −4 가 나갔다. **피어가 owner 를 안전 방향으로만 덮어썼다**는 §2.2 가
실제로 성립한 관측이다.

### 8.3 아직 아닌 것

- **횡축 clamp 는 기본 OFF** (`VI_STEER_LIMIT_RAD=0`). 차량 한계를 모르는데 숫자를 지어내면
  모든 런의 출력에 날조된 요구가 섞인다. 값이 오면 켠다
- **불변식이 verify 도구에 없다.** 지금은 단위 테스트에만 있고, 런 전체를 훑는 판정
  (`sils_vecu_vehicle_verify` / `requirements.yaml`)에는 아직 안 들어갔다
- **pass-through differential 자동 판정** 미구현 (§4.2). 여전히 눈으로 본다
- **선언형(§5) 미구현.** 규칙이 아직 `vehicle_interface_behavior.hpp` 안에 있다
