# vehicle-interface — VCU (Vehicle Interface) 예제 (Option 2 / Profile B)

`libsils_vecu` SDK로 만든 **Vehicle Interface(VCU)** — Autoware와 plant(MORAI) 사이에서
**Autoware 제어 target을 받아 아비트·안전·통합**해 plant가 받는 단일 명령을 낸다. steer/brake-ecu가
"상태→단일 축 제어"라면, VCU는 "**상위 명령 + 상태 → 통합 명령**"(입력 2개 = 프로파일 B).

- 설계: [../../../../docs/sils-vehicle-interface-autoware.md](../../../../docs/sils-vehicle-interface-autoware.md)
- Autoware 인터페이스 근거: `/Volumes/DATAPart/AUTOWARE` (control_cmd/gear_cmd, /vehicle/status/*)

## 구성

| 파일 | 역할 |
|---|---|
| `src/vehicle_interface_behavior.hpp` | VCU 로직 = `IVehicleBehavior` (target→통합 명령 매핑 + 무-target 시 safe stop) |
| `src/main.cc` | `ConfigFromEnv` → `VehicleInterfaceBehavior` → `RunVehicleProbe` |
| `channelmap/vehicle-interface.yaml` | 입력 2개(ControlTarget + EgoState) + 출력 1개(VehicleControl) |

## 데이터 흐름
```
Autoware ──Control/Gear──▶ [브리지(외부): →canonical ControlTarget] ──▶ autoware/control_target ┐
MORAI ────EgoState──────────────────────────────────────────────────▶ sils/ego/state          ┤→ VCU
VCU ──통합 VehicleControl──▶ sils/ego/cmd ──▶ [MORAI 어댑터(외부)] ──▶ MORAI                      ┘
```

## env 계약
| env | 의미 | 기본 |
|---|---|---|
| `SILS_CHANNELMAP` | 채널맵 경로 | — (필수) |
| `VI_SAFE_DECEL` | 무-target 시 safe-stop 감속도, m/s² (고유 knob) | 2.0 |
| `VISS_DDS_DOMAIN_ID` | DDS 도메인 | 0 |

## 실행 (Autoware/MORAI 없이 스모크)
```bash
export VISS_DDS_DOMAIN_ID=0 SILS_RUN_ID=vi
# VCU
SILS_CHANNELMAP=service/sils/examples/vehicle-interface/channelmap/vehicle-interface.yaml \
VI_SAFE_DECEL=2.0  bin/vehicle_interface
# 입력 fixture 2개
bin/sils_ego_source     --count 40 --rate-ms 100                 # EgoState
bin/sils_control_source --count 40 --rate-ms 100                 # ControlTarget (Autoware 대역)
```
- `target_received`·`state_received` 들어오면 → `control_published`가 target을 통과시킴.
- control_source를 멈추면(무-target) → `control_published`의 acceleration이 `-VI_SAFE_DECEL`로 전환(safe stop).

## 성숙도
S1(매핑 pass-through + 무-target safe-stop). 예정: S2 리포트(Steering/VelocityReport → Autoware),
S3 시간기반 타임아웃·클램프·아비트, S4 lockstep/conformance. Autoware↔DDS 브리지·MORAI 어댑터는 외부.
