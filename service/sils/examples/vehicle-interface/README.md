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
| `SILS_STAGE_INPUT` | `1`이면 tick 경계 입력 staging(§7.3, SDK 공통 S2) — 두 in-채널(target·state) 각각 채널별로 freeze. off=free-run 폴링 | 0 |
| `SILS_CLOCK_MODE` | `wall`(자유주행) 또는 `lockstep`(S3 — ClockMaster barrier, 공유 sim 클럭 결정론) | wall |
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

### lockstep 결정론 (S3)
ClockMaster가 barrier를 돌리고, 세 참가자(VCU + 두 소스)가 sim 클럭에 맞춰 동기 전진 → 두 실행이 동일 trace.
```bash
export VISS_DDS_DOMAIN_ID=0 SILS_RUN_ID=vi SILS_CLOCK_MODE=lockstep
# ClockMaster: 40 tick, dt 100ms, wall-pace 100ms(소스/프로브가 tick에 붙을 시간)
bin/sils_clock_master --participants 3 --ticks 40 --dt-ms 100 --tick-interval-ms 100 &
# 세 lockstep 참가자 (--count == --ticks)
SILS_CHANNELMAP=service/sils/examples/vehicle-interface/channelmap/vehicle-interface.yaml \
SILS_STAGE_INPUT=1  bin/vehicle_interface | tee /tmp/vi-lockstep.jsonl &
bin/sils_ego_source     --count 40 &
bin/sils_control_source --count 40 &
```
- `tick_grant`(master) → 각 참가자 `tick_complete` → `tick_barrier`. `control_published`는 sim_time 함수라 재현 가능.
- 두 번 돌려 `grep control_published`가 동일하면 결정론 회복 확인. `evaluate.py`로 `analyze_lockstep` PASS 판정.

## 성숙도
S1(매핑 pass-through + 무-target safe-stop) + S2 입력 staging + S3 lockstep. 예정: 리포트(Steering/VelocityReport → Autoware),
S3 시간기반 타임아웃·클램프·아비트, S4 lockstep/conformance. Autoware↔DDS 브리지·MORAI 어댑터는 외부.
