#!/bin/bash
# Build brake.fmu from source (FMI 2.0 Co-Simulation).
#
# Produces a self-contained .fmu (a zip) with this layout, which fmi4c / the FmuPlant
# loader expects:
#     brake.fmu
#     ├── modelDescription.xml
#     └── binaries/<platform>/brake_vehicle.<ext>      (modelIdentifier from the XML)
#
# No binary is committed to the repo -- you build it here, reproducibly. The output
# brake.fmu is git-ignored (see ../.gitignore). Run from this directory:
#     ./build.sh
# then point the ECU at it:
#     SILS_FMU_FILE=$PWD/brake.fmu SILS_FMU_INPUT_VR=1 SILS_FMU_OUTPUT_VR=0 \
#         .../bin/brake_ecu_fmu
set -euo pipefail
cd "$(dirname "$0")"

MODEL_ID="brake_vehicle"          # must match <CoSimulation modelIdentifier> in the XML
FMI_VERSION_TAG="v2.0.4"          # Modelica FMI standard headers (FMI 2.0)

# Platform folder + shared-lib extension per the FMI 2.0 standard naming.
case "$(uname -s)" in
    Linux)  case "$(uname -m)" in
                x86_64)          PLATFORM=linux64;   ARCHFLAGS="" ;;
                aarch64|arm64)   PLATFORM=aarch64-linux; ARCHFLAGS="" ;;
                *) echo "unsupported arch $(uname -m)"; exit 1 ;;
            esac
            EXT=so; SOFLAGS="-shared -fPIC" ;;
    Darwin) PLATFORM=darwin64; EXT=dylib; SOFLAGS="-dynamiclib -fPIC"; ARCHFLAGS="" ;;
    *) echo "unsupported OS $(uname -s)"; exit 1 ;;
esac

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

# 1. FMI 2.0 headers (fetched, not vendored -- keeps the example dependency-free in git).
HDR="$WORK/headers"
mkdir -p "$HDR"
BASE="https://raw.githubusercontent.com/modelica/fmi-standard/${FMI_VERSION_TAG}/headers"
for h in fmi2TypesPlatform.h fmi2FunctionTypes.h fmi2Functions.h; do
    if command -v curl >/dev/null; then curl -fsSL "$BASE/$h" -o "$HDR/$h"
    else wget -qO "$HDR/$h" "$BASE/$h"; fi
done

# 2. Compile the model to the platform shared library.
mkdir -p "$WORK/binaries/$PLATFORM"
cc $SOFLAGS $ARCHFLAGS -O2 -I"$HDR" brake_vehicle.c -lm \
   -o "$WORK/binaries/$PLATFORM/${MODEL_ID}.${EXT}"

# 3. Package: modelDescription.xml at the root + binaries/ tree, zipped as brake.fmu.
cp modelDescription.xml "$WORK/modelDescription.xml"
rm -f brake.fmu
( cd "$WORK" && zip -r -q "$OLDPWD/brake.fmu" modelDescription.xml binaries )

echo "wrote $(pwd)/brake.fmu  (platform: $PLATFORM)"
