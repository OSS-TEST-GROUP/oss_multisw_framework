# brake-fmu — the brake plant as an FMU (FMI 2.0 Co-Simulation)

The **FMU path** counterpart of the code-path [`brake_plant.hpp`](../../src/brake_plant.hpp):
the same first-order speed model, exported behind the FMI 2.0 co-sim C API so it loads
via `sils::MakeFmuPlant` with no `IPlant` subclass in the ECU.

No `.fmu` binary is committed — you build it from source here (reproducible, git-clean).

## Files

| file | role |
|---|---|
| `brake_vehicle.c` | the model: `dv/dt = −(GAIN·force) − DRAG·speed`, in `fmi2DoStep`. Every exported `fmi2*` function is defined; co-sim ones carry logic, the rest are safe no-ops |
| `modelDescription.xml` | declares the value references: `speed`=VR 0 (output), `force`=VR 1 (input); initial speed 30 m/s |
| `build.sh` | fetch FMI 2.0 headers → compile → zip into `brake.fmu` with the standard layout |
| `.gitignore` | keeps the built `brake.fmu` out of git |

## Build

```bash
./build.sh          # writes ./brake.fmu for the host platform
```

Needs a C compiler, `zip`, and `curl`/`wget` (headers are fetched from the Modelica
FMI standard, not vendored). The output layout:

```
brake.fmu
├── modelDescription.xml
└── binaries/<platform>/brake_vehicle.<so|dylib>
```

`<platform>` is the FMI 2.0 folder for the host `build.sh` ran on: `linux64` (x86_64
Linux), `aarch64-linux` (arm64 Linux), `darwin64` (macOS).

> ⚠️ **Build the FMU on the SAME platform the ECU runs on.** `build.sh` only packages a
> binary for its own host, and fmi4c loads the folder matching the ECU's platform — if
> they differ (e.g. an FMU built on macOS = `darwin64`, run by a Linux vECU that wants
> `linux64`), the loader can't find the binary and **crashes**. The vECU builds/runs in
> a Linux container, so build the FMU there too (run this `build.sh` inside that Linux
> environment), not on a macOS/Windows host. A portable FMU may bundle several
> `binaries/<platform>/` folders; this example ships one (the build host's).

## Run the ECU against it

Build the SDK with FMU support so `brake_ecu_fmu` exists, then point it at the file.
The value references map straight to env (see `modelDescription.xml`):

```bash
SILS_ENABLE_FMU=ON ./build.sh --build-sils        # produces bin/brake_ecu_fmu + fmi4c
# (SILS_ENABLE_FMU is an ENV var forwarded into the build container, NOT a -D arg to build.sh)

SILS_FMU_FILE=$PWD/brake.fmu \
SILS_FMU_INPUT_VR=1 \      # force  (input)
SILS_FMU_OUTPUT_VR=0 \     # speed  (output)
SILS_CTRL_KP=4.0 SILS_CTRL_SETPOINT=5.0 \
VISS_DDS_DOMAIN_ID=42 \
    .../bin/brake_ecu_fmu
```

`plant_step` traces are emitted exactly as on the code path — same contract, same
golden/evaluate treatment. The only functional difference from `brake_ecu` is the one
plant line in [`main_fmu.cc`](../../src/main_fmu.cc).

## Code path vs FMU path

| | code path (`brake_ecu`) | FMU path (`brake_ecu_fmu`) |
|---|---|---|
| model source | `brake_plant.hpp` (C++ `IPlant`) | `brake_vehicle.c` → `brake.fmu` |
| ECU plant line | `make_unique<BrakePlant>(gain, drag)` | `sils::MakeFmuPlant(cfg, err)` |
| GAIN/DRAG set by | env (`BRAKE_GAIN`/`SILS_PLANT_DRAG`) | baked into the FMU |
| initial speed | `SILS_PLANT_INIT` | modelDescription start value |
| build needs | nothing extra | `-DSILS_ENABLE_FMU` (fmi4c) + `./build.sh` |
| security gate | n/a | `fmu_gate` allowlist/hash (see extension-model §3.C) |

When to pick which: **code path** for models you write/own in C++ and want to
version with the ECU; **FMU path** for models exported from Simulink/Dymola/AMESim or
shared as a validated black box. Everything else — behavior, SDK, trace, container —
is identical either way.
