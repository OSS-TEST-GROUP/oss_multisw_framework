# plainc-ecu — Case 1: 순수 C / 모델기반 자동코드 ECU

개발자가 자기 제어 코드를 `libsils_vecu`에 넣는 **세 가지 케이스** 중 첫 번째(가장 저마찰).

| 케이스 | 예제 | 개발자가 들고 오는 것 | 이음새(seam) |
|---|---|---|---|
| **1. 순수 C / 자동코드** | **plainc-ecu (여기)** | Simulink Embedded Coder·TargetLink가 뽑은 C 모델 | 어댑터가 `IVehicleBehavior`로 wrap |
| 2. AUTOSAR Adaptive | [adaptive-ecu](../adaptive-ecu/) | ara::com 기반 AP SWC | ara::com facade ↔ 정규 계약 브리지 |
| 3. AUTOSAR Classic | [classic-vecu](../classic-vecu/) | 툴체인이 뽑은 **V-ECU FMU** | SDK `FmuBehavior`가 FMU 로드 |

## 핵심: 생성된 C 코드는 손대지 않는다

모델기반 개발(MBD) 팀은 Simulink/TargetLink 모델을 **C로 자동 생성**합니다. 그 C 유닛(`src/speedctl.c`/`.h`)은 **libsils_vecu·DDS·FMI를 전혀 모릅니다** — 루트 Inport(`ExtU`)·Outport(`ExtY`)·cal 파라미터·`_step()`만 있는 순수 함수입니다. 통합은 어댑터 한 개가 전부입니다.

```
EgoStateSample ─▶ speedctl_ExtU ─▶ speedctl_step() ─▶ speedctl_ExtY ─▶ VehicleControlSample
                              (개발자의 생성 모델, 그대로)
```

| 파일 | 역할 |
|---|---|
| `src/speedctl.h` / `.c` | **개발자 자동코드**(교체 대상). 종/횡 P 제어, SI 단위, SDK 무지 |
| `src/plainc_behavior.hpp` | 어댑터 = `IVehicleBehavior`. `ExtU`↔state, `ExtY`↔control 매핑(약 20줄) |
| `src/main.cc` | `ConfigFromEnv` → `PlainCBehavior` → `RunVehicleProbe` |
| `channelmap/plainc-min.yaml` | EgoState in / VehicleControl out |

**자기 모델로 교체하려면**: `speedctl.*`를 생성 코드로 바꾸고, 어댑터에서 필드명(`u.vx`, `y.accel_cmd` 등)만 자기 모델의 Inport/Outport 이름에 맞추면 끝. CMake는 `.c`를 그냥 한 translation unit으로 컴파일합니다.

## 실행 (fixture로 왕복 스모크)

```bash
export VISS_DDS_DOMAIN_ID=0 SILS_RUN_ID=plainc
SILS_CHANNELMAP=service/sils/examples/plainc-ecu/channelmap/plainc-min.yaml \
SILS_CTRL_SETPOINT=10 SILS_CTRL_KP=0.8  bin/plainc_ecu &
bin/sils_ego_source --count 40 --rate-ms 100     # EgoState 대역(vx 램프)
```

- `state_received`의 `vx`가 오르면 `control_published`의 `acceleration`이 `Kp·(setpoint−vx)`로 반응(steer-ecu와 같은 폐루프, 제어 math만 생성 C 모델에서 옴).
- 결정론/staging/lockstep은 SDK 공통(`SILS_STAGE_INPUT`, `SILS_CLOCK_MODE`) — 케이스와 무관하게 동일하게 적용.

## env 계약
| env | 의미 | 기본 |
|---|---|---|
| `SILS_CHANNELMAP` | 채널맵 경로 | — (필수) |
| `SILS_CTRL_SETPOINT` | 모델 `v_setpoint` cal 덮어쓰기, m/s | 10 |
| `SILS_CTRL_KP` | 모델 `kp_speed` cal 덮어쓰기 | 0.8 |
| `SILS_STAGE_INPUT` / `SILS_CLOCK_MODE` / `VISS_DDS_DOMAIN_ID` | SDK 공통(staging/lockstep/도메인) | 0 / wall / 0 |
