# brake-ecu — worked example developer ECU (libsils_vecu)

A complete, compiling ECU built on the `sils_vecu` SDK. It is the reference for
[building your own ECU](../README.md): a closed-loop brake controller with a
code-path plant, touching **no core code**.

## What it is

A vehicle braking to a target speed:

- **`src/brake_behavior.hpp`** — `BrakeBehavior : sils::IBehavior`. A proportional
  controller that reads the plant's speed feedback each tick and commands a brake force
  `clamp(kp·(speed − target), 0, max_force)` (brake-only, saturated).
- **`src/brake_plant.hpp`** — `BrakePlant : sils::IPlant`. A first-order longitudinal
  speed model, `dv/dt = −(gain·force) − drag·v`, integrated per tick (code path — no
  `.fmu`).
- **`src/main.cc`** — parse env → build the two ports → `sils::RunProbe(...)`. ~25 lines.

Everything else — DDS setup, the clock→behavior→publish→plant loop, the JSONL trace
contract, teardown — is the SDK's `RunProbe` (`run_loop.hpp`). The closed loop:

```
   ┌──────────────┐  force   ┌────────────┐
   │ BrakeBehavior│ ───────▶ │ BrakePlant │
   │  (IBehavior) │          │  (IPlant)  │
   └──────────────┘ ◀─────── └────────────┘
          ▲          speed          │
          └───────── RunProbe ──────┘   (feedback fed back next tick)
```

## Build

Built automatically by any SiLS build (`SILS_BUILD_EXAMPLES=ON` by default):

```bash
./build.sh --build-sils            # produces .../bin/brake_ecu alongside the probes
# or target it directly:
cmake --build <build-dir> --target brake_ecu
```

`CMakeLists.txt` links the SDK in one line — `target_link_libraries(brake_ecu PRIVATE
sils_vecu)` — and (today) re-globs the generated IDL sources the wire type needs; see
the file's comment for why that last step goes away in P4.

## Run

`brake_ecu` is entirely env-driven. Reused SDK env + this ECU's own knobs:

| env | meaning | default |
|---|---|---|
| `SILS_CTRL_KP` | proportional gain | 0.8 |
| `SILS_CTRL_SETPOINT` | target speed (m/s) | 10.0 |
| `SILS_PLANT_DRAG` | drag coefficient | 0.5 |
| `SILS_PLANT_INIT` | initial speed (m/s) | 0.0 |
| `BRAKE_GAIN` | force→decel gain (this ECU) | 0.02 |
| `BRAKE_MAX_FORCE` | actuator saturation (this ECU) | 100.0 |
| `VISS_DDS_DOMAIN_ID` | run's DDS domain | 0 |

```bash
SILS_CTRL_KP=4.0 SILS_CTRL_SETPOINT=5.0 SILS_PLANT_INIT=30.0 \
BRAKE_GAIN=0.02 VISS_DDS_DOMAIN_ID=42 .../bin/brake_ecu
```

Each tick emits a `plant_step` trace (`feedback_in`→`command`→`output`); speed decays
from 30 m/s toward the 5 m/s setpoint. `descriptors/brake_probe.yaml` is the declarative
run definition; `Dockerfile` + `entrypoint.sh` show packaging (identical to the built-in
probe's, only the binary name differs).

## The two plant paths

This example ships **both**, proving "only the plant line changes":

- **Code path** — `src/main.cc` + `src/brake_plant.hpp`, built as `brake_ecu`.
  `auto plant = std::make_unique<BrakePlant>(gain, drag);`
- **FMU path** — `src/main_fmu.cc` + [`models/brake-fmu/`](models/brake-fmu/), built as
  `brake_ecu_fmu` (only under `-DSILS_ENABLE_FMU`).
  `auto plant = sils::MakeFmuPlant(cfg, err);`

`diff src/main.cc src/main_fmu.cc` — the plant construction is the only functional
difference. The `.fmu` is built from source (`models/brake-fmu/build.sh`, no binary in
git); its value references map to `SILS_FMU_INPUT_VR`/`SILS_FMU_OUTPUT_VR`. See
[models/brake-fmu/README.md](models/brake-fmu/README.md) and
[extension-model §3.C](../../../../docs/sils-extension-model.md).
