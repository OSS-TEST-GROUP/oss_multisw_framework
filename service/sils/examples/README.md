# libsils_vecu examples — build your own ECU

Reference developer ECUs built on the `sils_vecu` SDK. Each links the SDK and
implements its own ports **without touching core** — the extension boundary described
in [docs/sils/sils-extension-model.md](../../../docs/sils/sils-extension-model.md).

| example | plant path | fidelity | shows |
|---|---|---|---|
| [brake-ecu](brake-ecu/) | code (C++ `IPlant`) | L3 | full closed-loop ECU, custom env knobs; code + FMU plant paths |
| [brake-ecu-native](brake-ecu-native/) | code (reused) | L3 | native custom DDS type via `IWireCodec` + own IDL (no core change) |
| [steer-ecu](steer-ecu/) | bus (Option 2) | L3 | plant-coupled ECU: `IVehicleBehavior` + `RunVehicleProbe` |
| [vehicle-interface](vehicle-interface/) | bus (Option 2) | L3 | the VCU — Autoware target + ego state → unified command (Profile B) |

## 개발자 코드를 어떻게 넣나 — 3 케이스

현업 ECU 개발자가 자기 제어 코드를 SDK에 넣는 세 경로. 모두 버스결합(Option 2,
`IVehicleBehavior` + `RunVehicleProbe`)이고, **코드가 들어오는 이음새(seam)만** 다르다.

| 케이스 | example | 개발자가 들고 오는 것 | 이음새 |
|---|---|---|---|
| 1. 순수 C / 모델기반 자동코드 | [plainc-ecu](plainc-ecu/) | Simulink Embedded Coder·TargetLink C 모델 | 어댑터가 `IVehicleBehavior`로 wrap |
| 2. AUTOSAR Adaptive | [adaptive-ecu](adaptive-ecu/) | `ara::com` 기반 AP SWC | ara::com facade ↔ 정규 계약 브리지 |
| 3. AUTOSAR Classic | [classic-vecu](classic-vecu/) | 툴체인이 뽑은 **V-ECU FMU** | SDK `FmuBehavior`가 FMU 로드(C++ 0줄) |

공통 원칙: **개발자 코드는 손대지 않고**, 얇은 어댑터(또는 FMI 소켓) 하나로 정규 DDS
계약(EgoState in / VehicleControl out)에 붙인다. 단위·enum 변환(anti-corruption)은 그
어댑터 경계에 둔다. Classic의 RTE/BSW 가상화는 **고객 툴체인 몫**(우리는 FMI 소켓 제공).

## Start your own ECU

An ECU is a small executable that hands its ports to `sils::RunProbe`. Copy `brake-ecu/`
and change four things:

1. **Behavior** — implement `sils::IBehavior` (`behavior.hpp`): `OnTick(ctx)` returns
   `{request_json, value}`. `ctx.plant_feedback` is the plant output from last tick.
2. **Plant** *(optional, for a closed loop)* — either implement `sils::IPlant`
   (`plant.hpp`) for the **code path**, or ship an exported `.fmu` and use the **FMU
   path** (`sils::MakeFmuPlant(cfg)`). Pass `nullptr` for open-loop.
3. **`main.cc`** — the whole program:
   ```cpp
   auto cfg = sils::ConfigFromEnv();
   return sils::RunProbe(cfg, std::make_unique<MyBehavior>(...),
                              std::make_unique<MyPlant>(...));
   ```
4. **`CMakeLists.txt`** — `target_link_libraries(my_ecu PRIVATE sils_vecu)`. In-tree,
   add your dir to [`CMakeLists.txt`](CMakeLists.txt). Out-of-tree, `find_package(sils_vecu)`.

Then write a `descriptors/*.yaml` (run definition), reuse the single-process
`entrypoint.sh`, and package with a `Dockerfile`. Nothing in core changes.

## What the SDK gives you vs what you write

| SDK (`sils_vecu`, you reuse) | you write (per ECU) |
|---|---|
| DDS participant/topic/writer + QoS | `IBehavior` (control logic) |
| clock→behavior→publish→plant tick loop | `IPlant` *(or an `.fmu`)* |
| JSONL trace contract, teardown | `main.cc` (~25 lines), descriptor, Dockerfile |
| `ConfigFromEnv()` common `SILS_*` | your ECU's own env knobs (via `GetEnvDoubleOr`) |

The `sils_vecu` target is defined in
[`service/sils/vecu/CMakeLists.txt`](../vecu/CMakeLists.txt).
