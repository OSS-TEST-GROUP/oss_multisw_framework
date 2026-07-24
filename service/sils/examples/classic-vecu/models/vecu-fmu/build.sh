#!/bin/bash
# Build vecu.fmu from source (FMI 2.0 Co-Simulation) -- the stand-in V-ECU controller FMU.
#
# Produces a self-contained .fmu (a zip) with the layout fmi4c / the FmuBehavior loader
# expects:
#     vecu.fmu
#     ├── modelDescription.xml
#     └── binaries/<platform>/vecu_ctrl.<ext>          (modelIdentifier from the XML)
#
# No binary is committed (git-ignored, see ../.gitignore). Run from this directory:
#     ./build.sh
# then point the example ECU at it:
#     SILS_FMU_FILE=$PWD/vecu.fmu SILS_FMU_VR_VX=0 SILS_FMU_VR_ACCEL_OUT=1 \
#         SILS_CHANNELMAP=.../classic-min.yaml  .../bin/classic_vecu
#
# > Build the FMU on the SAME platform the vECU runs on (the Linux container) -- fmi4c loads
# > the binaries/<platform>/ folder matching the host; a mismatch can't be found and crashes.
set -euo pipefail
cd "$(dirname "$0")"

MODEL_ID="vecu_ctrl"              # must match <CoSimulation modelIdentifier> in the XML
FMI_VERSION_TAG="v2.0.4"          # Modelica FMI standard headers (FMI 2.0)

case "$(uname -s)" in
    Linux)  case "$(uname -m)" in
                x86_64)          PLATFORM=linux64;       ARCHFLAGS="" ;;
                aarch64|arm64)   PLATFORM=aarch64-linux; ARCHFLAGS="" ;;
                *) echo "unsupported arch $(uname -m)"; exit 1 ;;
            esac
            EXT=so; SOFLAGS="-shared -fPIC" ;;
    Darwin) PLATFORM=darwin64; EXT=dylib; SOFLAGS="-dynamiclib -fPIC"; ARCHFLAGS="" ;;
    *) echo "unsupported OS $(uname -s)"; exit 1 ;;
esac

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

# 1. FMI 2.0 headers (fetched, not vendored).
HDR="$WORK/headers"
mkdir -p "$HDR"
BASE="https://raw.githubusercontent.com/modelica/fmi-standard/${FMI_VERSION_TAG}/headers"
for h in fmi2TypesPlatform.h fmi2FunctionTypes.h fmi2Functions.h; do
    if command -v curl >/dev/null; then curl -fsSL "$BASE/$h" -o "$HDR/$h"
    else wget -qO "$HDR/$h" "$BASE/$h"; fi
done

# 2. Compile the model to the platform shared library.
mkdir -p "$WORK/binaries/$PLATFORM"
cc $SOFLAGS $ARCHFLAGS -O2 -I"$HDR" vecu_ctrl.c -lm \
   -o "$WORK/binaries/$PLATFORM/${MODEL_ID}.${EXT}"

# 3. Package: modelDescription.xml at the root + binaries/ tree, zipped as vecu.fmu.
cp modelDescription.xml "$WORK/modelDescription.xml"
rm -f vecu.fmu
( cd "$WORK" && zip -r -q "$OLDPWD/vecu.fmu" modelDescription.xml binaries )

echo "wrote $(pwd)/vecu.fmu  (platform: $PLATFORM)"
