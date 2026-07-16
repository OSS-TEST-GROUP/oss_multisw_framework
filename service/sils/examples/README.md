# libsils_vecu examples — build your own ECU

Reference developer ECUs built on the `sils_vecu` SDK. Each links the SDK and
implements its own ports **without touching core** — the extension boundary described
in [docs/sils-extension-model.md](../../../docs/sils-extension-model.md).

| example | plant path | fidelity | shows |
|---|---|---|---|
| [brake-ecu](brake-ecu/) | code (C++ `IPlant`) | L3 | full closed-loop ECU, custom env knobs; code + FMU plant paths |
| [brake-ecu-native](brake-ecu-native/) | code (reused) | L3 | native custom DDS type via `IWireCodec` + own IDL (no core change) |

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
