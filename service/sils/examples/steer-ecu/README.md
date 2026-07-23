# steer-ecu — 조향 vECU 예제 (Option 2 플랜트 결합)

`libsils_vecu` SDK로 만든 **플랜트 결합(버스) 개발자 ECU**. brake-ecu가 VISS 요청 모델(`RunProbe`)의
예제라면, steer-ecu는 **버스 결합 모델(`RunVehicleProbe`)**의 예제다 — MORAI/AWSIM이 버스에 올리는
정규 `EgoState`를 구독하고 조향 `VehicleControl`을 발행한다.

- 설계: [../../../../docs/sils-morai-plant-adapter-design.md](../../../../docs/sils-morai-plant-adapter-design.md)
- 개발 가이드: [../../../../docs/sils-vecu-development-guide.md](../../../../docs/sils-vecu-development-guide.md) §7

## 구성

| 파일 | 역할 |
|---|---|
| `src/steering_behavior.hpp` | 개발자 제어 로직 = `IVehicleBehavior` 구현(P: `steer = Kp·(target − angle)`) |
| `src/main.cc` | `ConfigFromEnv` → `SteeringBehavior` 조립 → `RunVehicleProbe` (~15줄) |
| `channelmap/steer-min.yaml` | 이 ECU가 구독/발행할 채널(EgoState in, VehicleControl out) |

SDK는 포트(`IVehicleBehavior`)와 러너(`RunVehicleProbe`)·코덱·채널맵만 제공하고, **조향 제어 로직은
이 ECU가 소유**한다(core 무수정).

## env 계약

| env | 의미 | 기본 |
|---|---|---|
| `SILS_CHANNELMAP` | 채널맵 경로 (이 ECU의 것 지정) | — (필수) |
| `SILS_CTRL_KP` | 비례 게인 (SDK 공통 필드 재사용) | 0.8 |
| `STEER_SETPOINT` | 목표 조향각, rad (이 ECU 고유 knob, `GetEnvDoubleOr`로 직접) | 0.1 |
| `SILS_STAGE_INPUT` | `1`이면 tick 경계 입력 staging(§7.3 결정론, SDK 공통 S2). off=free-run 폴링 | 0 |
| `SILS_CLOCK_MODE` | `wall` 또는 `lockstep`(S3 — ClockMaster barrier, 소스도 lockstep이면 값 재현) | wall |
| `VISS_DDS_DOMAIN_ID` | DDS 도메인 (상대와 동일) | 0 |

## 실행 (MORAI 없이 왕복 확인)

```bash
export VISS_DDS_DOMAIN_ID=0 SILS_RUN_ID=steer
# A — 조향 ECU
SILS_CHANNELMAP=service/sils/examples/steer-ecu/channelmap/steer-min.yaml \
STEER_SETPOINT=0.1  bin/steer_ecu
# B — EgoState 소스 (SDK 동봉 fixture)
bin/sils_ego_source --count 20 --rate-ms 100
```
`state_received` → `control_published`(steering_angle 반응)가 흐르면 왕복 성립.
초기 `steering_angle=0`이면 첫 명령 `Kp·(0.1−0)=0.08`, 상태가 목표에 가까워질수록 0으로 수렴.

## 패키징

brake-ecu와 동일 — `entrypoint.sh`가 바이너리를 PID 1로 exec, Dockerfile은
`docker/Dockerfile.rel`의 sils-vecu 런타임 스테이지를 따른다(바이너리·entrypoint만 교체).
