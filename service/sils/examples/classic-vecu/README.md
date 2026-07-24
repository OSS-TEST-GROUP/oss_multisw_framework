# classic-vecu — Case 3: AUTOSAR Classic ECU (V-ECU FMU)

개발자 코드 플러그인 **3 케이스** 중 세 번째(가장 무겁지만 표준 경로 존재). → [Case 1 plainc-ecu](../plainc-ecu/) · [Case 2 adaptive-ecu](../adaptive-ecu/)

## 왜 Classic은 "코드 붙여넣기"가 안 되나

AUTOSAR **Classic** 앱 코드는 `Rte_Read/Write/Call`에 의존하고, 그건 ARXML에서 생성된 **RTE + BSW 스택**(COM, PDU router, CAN driver, OS)에 의존합니다. 그래서 `.c`만 떼어 우리 SDK에 컴파일할 수 없습니다. 업계 표준 경로는 **가상화(V-ECU)**:

```
Classic SWC + ARXML ─▶ [개발자 툴체인 + 가상화 툴] ─▶ V-ECU FMU ─▶ (여기서 로드)
   (VEOS / Silver / vVIRTUALtarget / ETAS ...)         (FMI 2.0/3.0)
```

V-ECU FMU는 결국 `입력 → doStep → 출력`이라, 우리가 이미 가진 **FMI 로더(fmi4c)**에 그대로 얹힙니다. SDK에 추가된 `FmuBehavior`(`vehicle_fmu_behavior.hpp`)가 FMU를 `IVehicleBehavior`로 감쌉니다 — **C++ 제어 코드는 한 줄도 안 씁니다.**

```
EgoState(state) ─▶ FMU 입력 VR ─▶ doStep() ─▶ FMU 출력 VR ─▶ VehicleControl
                          (개발자의 V-ECU, 그대로)
```

> ⚠️ **정직한 경계**: 우리는 **V-ECU를 만들지 않습니다.** RTE/BSW 가상화는 상용 툴 영역(수백만 불짜리)이고 고객 툴체인 몫입니다. libsils_vecu가 제공하는 건 **FMI 소켓 + 정규 채널 바인딩 + 결정론/conformance 판정**입니다.

## 파일

| 파일 | 역할 |
|---|---|
| `src/main.cc` | env로 FMU 파일 + VR 매핑 읽어 `MakeFmuBehavior` → `RunVehicleProbe` |
| `channelmap/classic-min.yaml` | EgoState in / VehicleControl out |
| `models/vecu-fmu/` | **스탠드인 V-ECU FMU**(실 V-ECU export 대역). 종방향 속도 제어기(`accel=Kp·(Vset−vx)`), FMI 2.0 co-sim |

`FmuBehavior`는 SDK 코어에 있습니다: [`vehicle_fmu_behavior.hpp`](../../vecu/src/vehicle_fmu_behavior.hpp) — SISO `FmuPlant`의 MIMO 짝(state 필드 → 입력 VR, 출력 VR → control 필드). `-DSILS_ENABLE_FMU`로 빌드할 때만 활성(그 외엔 `MakeFmuBehavior`가 명확한 에러 반환).

## 실행

```bash
# 1) 스탠드인 V-ECU FMU 빌드 (vECU가 도는 것과 같은 플랫폼 = Linux 컨테이너에서)
service/sils/examples/classic-vecu/models/vecu-fmu/build.sh   # -> vecu.fmu

# 2) 실행 (fixture 왕복 스모크)
export VISS_DDS_DOMAIN_ID=0 SILS_RUN_ID=classic
SILS_CHANNELMAP=service/sils/examples/classic-vecu/channelmap/classic-min.yaml \
SILS_FMU_FILE=service/sils/examples/classic-vecu/models/vecu-fmu/vecu.fmu \
SILS_FMU_VR_VX=0 SILS_FMU_VR_ACCEL_OUT=1  bin/classic_vecu &
bin/sils_ego_source --count 40 --rate-ms 100
```

- `state_received`의 `vx`가 FMU 입력 VR 0으로 들어가 `doStep` → 출력 VR 1(accel)이 `control_published`로. `Kp·(10−vx)` 반응(다른 케이스와 동일 폐루프, 제어 math만 V-ECU FMU에서 옴).
- FMU co-sim `doStep`는 입력의 순수함수라 lockstep에서 **골든 재현** 가능(tick 1개 = doStep 1개).

## env 계약
| env | 의미 | 기본 |
|---|---|---|
| `SILS_CHANNELMAP` | 채널맵 경로 | — (필수) |
| `SILS_FMU_FILE` | V-ECU FMU(.fmu) 경로 | — (필수) |
| `SILS_FMU_VR_VX` | vx 입력 value reference | 0 |
| `SILS_FMU_VR_ACCEL_OUT` | accel 출력 value reference | 1 |
| `SILS_FMU_VR_STEER_IN` / `_STEER_OUT` / `_GEAR_OUT` | (선택) 추가 신호 VR, `<0`=미연결 | -1 |
| `SILS_STAGE_INPUT` / `SILS_CLOCK_MODE` / `VISS_DDS_DOMAIN_ID` | SDK 공통 | 0 / wall / 0 |

## 자기 V-ECU로 교체
`models/vecu-fmu/vecu.fmu`를 툴체인이 뽑은 실 V-ECU FMU로 바꾸고, 그 FMU의 `modelDescription.xml` value reference에 맞춰 `SILS_FMU_VR_*`만 지정하면 끝. `main.cc`/채널맵/코드는 그대로.
