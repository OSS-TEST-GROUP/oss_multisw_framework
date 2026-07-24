# adaptive-ecu — Case 2: AUTOSAR Adaptive (ara::com) ECU

개발자 코드 플러그인 **3 케이스** 중 두 번째. → [Case 1 plainc-ecu](../plainc-ecu/) · [Case 3 classic-vecu](../classic-vecu/)

## 왜 Adaptive는 자연스러운가

AUTOSAR **Adaptive**(AP) 앱은 이미 **POSIX 프로세스 + 서비스 지향 통신(`ara::com`)** 입니다. Classic과 달리 **가상화할 RTE/BSW가 없습니다** — 이음새는 "ara::com 이벤트를 정규 계약에 바인딩"뿐이고, `ara::com` 자체가 DDS 바인딩을 가질 수 있어 표준 위의 얇은 접점이 됩니다.

```
MORAI/AWSIM ─EgoState(DDS)─▶ [adapter=런타임 역할] ─Deliver()─▶ ara::com ProxyEvent ─▶ SWC
                                                                                        │ 제어
SWC ─Send()─▶ ara::com SkeletonEvent ─TakeLast()─▶ [adapter: AP→정규 매핑] ─VehicleControl(DDS)─▶ MORAI
```

| 파일 | 역할 | 프로덕션에서 |
|---|---|---|
| `src/adaptive_swc.hpp` | **개발자 SWC** — `ara::com`에만 의존, SDK/DDS 무지 | 그대로 유지 |
| `src/ara_com_shim.hpp` | **교육용 shim** — ara::com 모양 최소 stub | **삭제**, 벤더 AP 런타임 링크 |
| `src/adaptive_behavior.hpp` | 어댑터 = 런타임 역할 + AP↔정규 브리지(gear enum 등) | 벤더 DDS 바인딩이 대체 |

> ⚠️ **정직한 경계**: `ara_com_shim.hpp`는 **실제 AP 스택이 아닙니다.** SWC 소스가 SiLS에서도 도는 걸 보이기 위한 최소 stand-in이며, 실 배포에서는 shim을 지우고 벤더 `ara::com`(Vector/ETAS/EB/Apex.AI 등) 런타임을 링크합니다. 그 런타임의 SOME/IP 또는 DDS 바인딩이 어댑터가 하는 일(구독 이벤트 delivery, 오퍼 이벤트 dispatch)을 대신합니다.

## anti-corruption 매핑이 여기 있는 이유

SWC의 `ControlCommand.gear`는 **AP enum**(Autoware류: `AP_DRIVE=2`, `AP_PARK=22`…)입니다. 정규 계약은 `0=P 1=R 2=N 3=D`. 어댑터의 `MapGear()`가 이 변환을 맡습니다 — SWC는 자기 어휘로, 버스는 정규 어휘로. 단위(rad·m/s²)는 SWC가 SI로 작성돼 변환 불필요(변환이 필요하면 정확히 이 경계에 둠).

## 실행 (fixture 왕복 스모크)

```bash
export VISS_DDS_DOMAIN_ID=0 SILS_RUN_ID=adaptive
SILS_CHANNELMAP=service/sils/examples/adaptive-ecu/channelmap/adaptive-min.yaml \
SILS_CTRL_SETPOINT=10 SILS_CTRL_KP=0.8  bin/adaptive_ecu &
bin/sils_ego_source --count 40 --rate-ms 100
```

- `state_received`가 SWC의 `OnEgoStatus` 핸들러를 태우고 → `control_published`가 SWC의 `Send()` 명령을 정규로 변환해 발행.
- 결정론/staging/lockstep은 SDK 공통(케이스 무관).

## env 계약
| env | 의미 | 기본 |
|---|---|---|
| `SILS_CHANNELMAP` | 채널맵 경로 | — (필수) |
| `SILS_CTRL_SETPOINT` | SWC 목표속도, m/s | 10 |
| `SILS_CTRL_KP` | SWC 종방향 P 게인 | 0.8 |
| `SILS_STAGE_INPUT` / `SILS_CLOCK_MODE` / `VISS_DDS_DOMAIN_ID` | SDK 공통 | 0 / wall / 0 |
