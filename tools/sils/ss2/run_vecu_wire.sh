#!/usr/bin/env bash
#
# run_vecu_wire.sh -- attach the vECU to the SS_v2 adapter and settle stage 1 (loop B).
#
# The adapter (adapters/ss2) already proves SS_v2 -> canonical DDS bus. This script closes
# the other half: a vECU reading that bus, which is the only thing that can prove Fast DDS
# 2.6 (adapter, pinned to the ROS 2 Humble stack) actually reaches Fast DDS 3.x (ours).
# See docs/sils/sils-ss2-bringup.md §6 and docs/sils/sils-simulator-loops.md §7.3.
#
#   V2  interop  <- the stage-1 GATE.       exit 0 / 1
#   V1  frame    <- measurement, needs a CURVED scenario to be conclusive
#   V3  clock    <- measurement, feeds the stage-2 time-sync design (W1/W2)
#
# The verdict logic lives in analyze_wire.py (unit-tested); this script only arranges for
# a correct run to happen. That split matters -- the failures this script exists to
# prevent are environmental, not analytical.
#
# WHAT THIS SCRIPT IS ACTUALLY FOR: every stage-1 bring-up failure so far was a wrong
# environment, not wrong code.
#
#   * a stale SILS_RUN_ID from an earlier run_phase1.sh session leaked in and the run was
#     tagged `rt` -- so the trace could not be correlated with the adapter's
#   * the vECU on one domain, the adapter on another -- reads as "no data", looks like an
#     interop failure, is not one
#   * SILS_DURABILITY=volatile against the adapter's TRANSIENT_LOCAL writer -- DDS declines
#     to match and says NOTHING; identical symptom, unrelated cause
#
# So this script WIPES every SILS_*/VISS_* variable out of the inherited environment and
# sets only what it means (--keep-env opts out). Non-SiLS variables are preserved, because
# FASTDDS_DEFAULT_PROFILES_FILE and friends legitimately come from the shell.
#
# Usage:
#   tools/sils/ss2/run_vecu_wire.sh --bin-dir build/bin [--domain 42] [--duration 30]
#   tools/sils/ss2/run_vecu_wire.sh --bin-dir build/bin --dry-run     # print, launch nothing
#
# Exit: 0 V2 passed, 1 V2 failed, 2 usage/setup error.

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"

EXIT_OK=0
EXIT_GATE_FAIL=1
EXIT_USAGE=2

# Defaults. The domain is 42 to match the adapter's own default (adapters/ss2 --
# SS2_DDS_DOMAIN) and the repo-wide default set in commit e486d4c; if you move one, the
# preflight below will tell you rather than letting it read as "no data".
BIN_DIR=""
DOMAIN=42
RUN_ID="ss2-wire"
INSTANCE_ID="vecu-wire-1"
DURATION=30
PERIOD_MS=50
MIN_SAMPLES=20
CHANNELMAP="config/sils/channelmap/ego-min.yaml"
OUT_DIR=""
DRY_RUN=0
KEEP_ENV=0

die() { echo "run_vecu_wire: $*" >&2; exit "${EXIT_USAGE}"; }

usage() {
    sed -n '2,40p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
    exit "${EXIT_USAGE}"
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --bin-dir)      BIN_DIR="${2:-}"; shift 2 ;;
        --domain)       DOMAIN="${2:-}"; shift 2 ;;
        --run-id)       RUN_ID="${2:-}"; shift 2 ;;
        --instance-id)  INSTANCE_ID="${2:-}"; shift 2 ;;
        --duration)     DURATION="${2:-}"; shift 2 ;;
        --period-ms)    PERIOD_MS="${2:-}"; shift 2 ;;
        --min-samples)  MIN_SAMPLES="${2:-}"; shift 2 ;;
        --channelmap)   CHANNELMAP="${2:-}"; shift 2 ;;
        --out)          OUT_DIR="${2:-}"; shift 2 ;;
        --dry-run)      DRY_RUN=1; shift ;;
        --keep-env)     KEEP_ENV=1; shift ;;
        -h|--help)      usage ;;
        *)              die "unknown argument: $1 (try --help)" ;;
    esac
done

[[ "${DOMAIN}" =~ ^[0-9]+$ ]]      || die "--domain must be a number, got '${DOMAIN}'"
[[ "${DURATION}" =~ ^[0-9]+$ ]]    || die "--duration must be a number, got '${DURATION}'"
[[ "${PERIOD_MS}" =~ ^[0-9]+$ ]]   || die "--period-ms must be a number, got '${PERIOD_MS}'"
[[ "${MIN_SAMPLES}" =~ ^[0-9]+$ ]] || die "--min-samples must be a number"

# --- resolve the binary -------------------------------------------------------
#
# sils_vecu_vehicle_probe, NOT sils_vecu_viss_probe: the vehicle probe is the channel-map
# driven plant-coupling probe (Option 2), which is what reads sils/ego/state. The VISS
# probe speaks the VISS wire and would sit there matching nothing.
BIN_NAME="sils_vecu_vehicle_probe"
if [[ -n "${BIN_DIR}" ]]; then
    VECU_BIN="${BIN_DIR%/}/${BIN_NAME}"
    [[ -x "${VECU_BIN}" ]] || die "no executable ${BIN_NAME} in --bin-dir '${BIN_DIR}'"
else
    # Search the usual build layouts rather than making the caller remember which one this
    # checkout used.
    #
    # A hardcoded candidate list does not work here. The top-level CMakeLists sets
    # CMAKE_RUNTIME_OUTPUT_DIRECTORY to the RELATIVE path "out/bin", so the binary lands
    # under whatever build directory was configured (<build>/out/bin), and the install
    # prefix is a computed OUTPUT_DIR/core. Both move per checkout and per build flags.
    # Searching is the only thing that stays correct.
    #
    # -perm -u+x, not -executable: BSD find (macOS dev boxes) has no -executable.
    # A read loop rather than mapfile: macOS still ships bash 3.2 as /bin/bash, and this
    # script is meant to be runnable on the dev mac even though the real run is on Linux.
    FOUND=()
    while IFS= read -r f; do
        [[ -n "${f}" ]] && FOUND+=("${f}")
    done < <(
        find "${REPO_ROOT}" \
            \( -name .git -o -name node_modules -o -name .cache \) -prune -o \
            -type f -name "${BIN_NAME}" -perm -u+x -print 2>/dev/null | sort
    )
    if [[ ${#FOUND[@]} -eq 0 ]]; then
        die "cannot find ${BIN_NAME} anywhere under ${REPO_ROOT}.
  Build the vECU first (it is an install target of service/sils/vecu/CMakeLists.txt),
  or pass --bin-dir <dir> if it lives outside the repo.
  NOTE: this run needs a vECU built AFTER the vy/stamp_ns trace fields were added --
  an older binary still runs, but V1/V3 come back INCONCLUSIVE."
    fi
    if [[ ${#FOUND[@]} -gt 1 ]]; then
        # Do not guess. Picking the wrong one silently is how you spend an hour measuring
        # a stale binary -- exactly the failure mode that would hide the new trace fields.
        echo "run_vecu_wire: ${#FOUND[@]} copies of ${BIN_NAME} found; pass --bin-dir to choose:" >&2
        for f in "${FOUND[@]}"; do
            echo "    --bin-dir $(dirname "${f}")   ($(date -r "${f}" '+%Y-%m-%d %H:%M' 2>/dev/null || echo '?'))" >&2
        done
        exit "${EXIT_USAGE}"
    fi
    VECU_BIN="${FOUND[0]}"
fi

# --- is the binary new enough to answer V1/V3? --------------------------------
#
# vy and stamp_ns are recent additions to the state_received trace line, and they are the
# ONLY observables that decide V1 and V3. A binary built before them still runs and still
# passes V2 -- so the run looks fine, and V1/V3 come back INCONCLUSIVE for a reason that
# has nothing to do with the simulator. That is a wasted measurement session, and the
# report it produces ("frame undetermined") is indistinguishable from a genuine one.
#
# mtime against the header that emits those fields is a good enough proxy: it cannot be
# fooled in the direction that matters (a rebuild always bumps it).
VEHICLE_RUN_HPP="${REPO_ROOT}/service/sils/vecu/src/vehicle_run.hpp"
STALE_BIN=0
if [[ -f "${VEHICLE_RUN_HPP}" && "${VEHICLE_RUN_HPP}" -nt "${VECU_BIN}" ]]; then
    STALE_BIN=1
fi

CMAP_PATH="${CHANNELMAP}"
[[ -f "${CMAP_PATH}" ]] || CMAP_PATH="${REPO_ROOT}/${CHANNELMAP}"
[[ -f "${CMAP_PATH}" ]] || die "channel map not found: ${CHANNELMAP}"

ANALYZER="${SCRIPT_DIR}/analyze_wire.py"
[[ -f "${ANALYZER}" ]] || die "analyze_wire.py missing next to this script"

[[ -z "${OUT_DIR}" ]] && OUT_DIR="${REPO_ROOT}/runs/ss2-wire/${RUN_ID}"

# --- environment hygiene ------------------------------------------------------
#
# Everything the vECU reads is SILS_* or VISS_* (service/sils/vecu/src/config.hpp), so
# clearing those two namespaces wholesale is both sufficient and safe -- and it fails
# CLOSED: a variable added to config.hpp later is wiped by default rather than silently
# inherited from whatever shell happened to run this.
#
# Enumerated with `compgen -v` (variable NAMES) rather than by parsing `env` output: a
# value containing a newline would split into bogus "names" there, and the wipe would miss
# the variable it was aimed at.
LEAKED=()
if [[ "${KEEP_ENV}" -eq 0 ]]; then
    while read -r name; do
        case "${name}" in
            SILS_*|VISS_*) LEAKED+=("${name}"); unset "${name}" ;;
        esac
    done < <(compgen -v)
fi

export SILS_RUN_ID="${RUN_ID}"
export SILS_VECU_INSTANCE_ID="${INSTANCE_ID}"
export VISS_DDS_DOMAIN_ID="${DOMAIN}"
export SILS_CHANNELMAP="${CMAP_PATH}"
# TRANSIENT_LOCAL: the adapter's writer offers it, and a VOLATILE reader against a
# TRANSIENT_LOCAL writer is one of the combinations DDS declines to match in silence.
export SILS_DURABILITY="transient-local"
# Stage 1 is free-run on purpose: no clock master, no barrier. A lockstep clock here would
# block on TickGrants that nothing publishes, and the run would hang looking like a wire
# failure. Stage 2 is where the barrier arrives.
export SILS_CLOCK_MODE="wall"
export SILS_VISS_PROBE_PERIOD_MS="${PERIOD_MS}"
# Plant OFF. The state comes from the bus; an internal plant would ALSO drive the state and
# the trace would be a mix of simulated and received values, with nothing marking which.
export SILS_PLANT="none"
export SILS_SCENARIO="nominal"
# No injected faults. vehicle_input_fault.hpp writes NaN into steering_angle as a fault, and
# a NaN reaching V1's ratio arithmetic would produce a verdict out of thin air.
export SILS_SCENARIO_FAULTS=""
export SILS_STAGE_INPUT="0"
export SILS_ATTEST_GATEWAY="sim"
export SILS_ATTEST_PROTOCOL="sim"

TRACE="${OUT_DIR}/vecu.jsonl"
RESULT_JSON="${OUT_DIR}/wire-verdict.json"
ENV_DUMP="${OUT_DIR}/env.txt"

BIN_MTIME="$(date -r "${VECU_BIN}" '+%Y-%m-%d %H:%M' 2>/dev/null || echo '?')"

# --- which simulator was this? ------------------------------------------------
#
# A measurement of scenario_simulator_v2 that does not say WHICH scenario_simulator_v2 is
# not a measurement: SS2_REF defaults to `master`, a moving branch, so two runs a week
# apart can disagree for reasons that have nothing to do with our code.
# docker/sils/build-ss2.sh writes the resolved SHA to this file on every successful build.
#
# Stamping it into the run output here, rather than trusting someone to copy it into the
# report by hand -- that has now been missed twice (2026-08-06 and -07 evidence files both
# carry a "SHA unrecorded" warning).
SS2_PIN_FILE="${REPO_ROOT}/docker/sils/ss2-upstream.pin"
SS2_COMMIT="unrecorded"
if [[ -f "${SS2_PIN_FILE}" ]]; then
    SS2_COMMIT="$(sed -n 's/^commit=//p' "${SS2_PIN_FILE}" | head -1)"
    [[ -n "${SS2_COMMIT}" ]] || SS2_COMMIT="unparseable"
fi

echo "run_vecu_wire: stage-1 wiring check (loop B)"
echo "  binary      ${VECU_BIN}"
echo "              built ${BIN_MTIME}"
echo "  domain      ${DOMAIN}     (must equal the adapter's SS2_DDS_DOMAIN)"
echo "  run_id      ${RUN_ID}     (must equal the adapter's SILS_RUN_ID)"
echo "  channelmap  ${CMAP_PATH}"
echo "  duration    ${DURATION}s @ ${PERIOD_MS}ms tick"
echo "  SS_v2       ${SS2_COMMIT}"
echo "  out         ${OUT_DIR}"
if [[ ${#LEAKED[@]} -gt 0 ]]; then
    echo "  wiped ${#LEAKED[@]} inherited SILS_*/VISS_* var(s): ${LEAKED[*]}"
fi

if [[ "${SS2_COMMIT}" == "unrecorded" ]]; then
    echo
    echo "  WARNING: docker/sils/ss2-upstream.pin is missing, so this run cannot say WHICH"
    echo "           scenario_simulator_v2 it measured. SS2_REF defaults to the moving"
    echo "           'master' branch -- without the SHA the result is not reproducible."
    echo "           Run docker/sils/build-ss2.sh (it writes the pin), and COMMIT the file."
fi

if [[ "${STALE_BIN}" -eq 1 ]]; then
    echo
    echo "  WARNING: this binary is OLDER than service/sils/vecu/src/vehicle_run.hpp."
    echo "           V2 will still be judged correctly -- state_received predates the"
    echo "           change. But vy/stamp_ns may be missing, and then V1 and V3 report"
    echo "           INCONCLUSIVE because of the BUILD, not because of the simulator."
    echo "           Rebuild the vECU to get all three from one run."
fi

# Soft preflight. Cannot see the adapter's DDS endpoints from here, but the container name
# is a cheap proxy and this is the single most common reason for an empty trace.
if command -v docker >/dev/null 2>&1; then
    if ! docker ps --format '{{.Names}}' 2>/dev/null | grep -q '^sils-ss2-adapter$'; then
        echo "  WARNING: no running container named sils-ss2-adapter. If the adapter is not"
        echo "           up, V2 will report FAIL for a reason that has nothing to do with"
        echo "           interop. Start it with:"
        echo "             docker compose -f docker/compose/docker-compose-sils-ss2.yaml up -d ss2-mock ss2-adapter"
    fi
fi

if [[ "${DRY_RUN}" -eq 1 ]]; then
    echo
    echo "--- dry run: resolved environment ---"
    env | grep -E '^(SILS_|VISS_)' | sort
    echo "--- would run: ${VECU_BIN} > ${TRACE} for ${DURATION}s ---"
    exit "${EXIT_OK}"
fi

# mkdir failure here is almost always ONE thing, and the bare errno does not say it:
# docker created a bind-mount target as root. docker-compose-sils-ss2.yaml mounts
# ../../runs/ss2-probe and ../../runs/ss2-scenario, and when a mount source does not exist
# docker creates it -- as root, owning runs/ itself. Every later user-space write under
# runs/ then fails with a permission error that reads like a script bug.
if ! mkdir -p "${OUT_DIR}" 2>/dev/null; then
    # Walk up to the first existing ancestor and report who owns it; that is the directory
    # the fix has to target, and it is usually not the one named in the error.
    blocker="${OUT_DIR}"
    while [[ ! -d "${blocker}" && "${blocker}" != "/" ]]; do
        blocker="$(dirname "${blocker}")"
    done
    owner="$(ls -ld "${blocker}" 2>/dev/null | awk '{print $3":"$4}')"
    echo "run_vecu_wire: cannot create ${OUT_DIR}" >&2
    echo "  blocked at: ${blocker}  (owner ${owner:-unknown}, you are $(id -un))" >&2
    if [[ "${owner%%:*}" == "root" ]]; then
        echo "  This is docker, not the script. Compose mounts runs/* into the SS_v2" >&2
        echo "  containers, and docker creates a missing mount source AS ROOT." >&2
        echo "  Fix:  sudo chown -R \"\$USER\":\"\$USER\" $(dirname "${blocker}")" >&2
    else
        echo "  Fix:  chmod u+w ${blocker}   (or pass --out <writable dir>)" >&2
    fi
    exit "${EXIT_USAGE}"
fi
{
    echo "# run_vecu_wire.sh provenance"
    echo "ss2_upstream_commit=${SS2_COMMIT}"
    echo "vecu_binary=${VECU_BIN}"
    echo "vecu_binary_mtime=${BIN_MTIME}"
    echo "#"
    env | grep -E '^(SILS_|VISS_)' | sort
} > "${ENV_DUMP}"
if [[ -f "${SS2_PIN_FILE}" ]]; then
    cp "${SS2_PIN_FILE}" "${OUT_DIR}/ss2-upstream.pin"
fi

# --- nobody else is already being the vECU -------------------------------------
#
# Two vECUs on one domain is not a crash, it is a QUIET corruption. Both read the same
# state and both write sils/ego/cmd, so the adapter downstream receives commands from
# two independent controllers interleaved -- with independent seq counters -- and the run
# is nondeterministic in a way no counter reports. Both also write traces.
#
# Refuse rather than kill: this is a host process, possibly someone else's, and the cost of
# stopping the wrong one is higher than the cost of asking. (Docker leftovers ARE removed
# automatically elsewhere in this suite -- those are ours by name; a bare pid is not.)
if command -v pgrep >/dev/null 2>&1; then
    OTHERS="$(pgrep -f "${BIN_NAME}" 2>/dev/null | grep -v "^$$\$" || true)"
    if [[ -n "${OTHERS}" ]]; then
        echo "run_vecu_wire: ${BIN_NAME} is ALREADY RUNNING:" >&2
        # shellcheck disable=SC2086
        ps -o pid=,lstart=,args= -p ${OTHERS} 2>/dev/null | cut -c1-160 | sed 's/^/    /' >&2
        die "two vECUs on domain ${DOMAIN} would both write sils/ego/cmd and the run
  would be quietly nondeterministic -- no counter anywhere reports it. Stop the other one
  first:  kill ${OTHERS//$'\n'/ }"
    fi
fi

# --- run ----------------------------------------------------------------------
#
# SIGTERM, not SIGKILL: vehicle_run.hpp installs a handler that leaves the tick loop
# cleanly and emits run_stopping/run_stopped. Killing hard truncates the trace mid-line,
# which analyze_wire.py correctly rejects as corrupt -- a self-inflicted usage error.
echo
echo "  running..."
"${VECU_BIN}" > "${TRACE}" 2>"${OUT_DIR}/vecu.stderr" &
VECU_PID=$!

cleanup() {
    if kill -0 "${VECU_PID}" 2>/dev/null; then
        kill -TERM "${VECU_PID}" 2>/dev/null
        for _ in $(seq 1 50); do
            kill -0 "${VECU_PID}" 2>/dev/null || break
            sleep 0.1
        done
        kill -KILL "${VECU_PID}" 2>/dev/null
    fi
    wait "${VECU_PID}" 2>/dev/null
}
trap 'cleanup; exit '"${EXIT_USAGE}" INT TERM

SLEPT=0
while [[ "${SLEPT}" -lt "${DURATION}" ]]; do
    if ! kill -0 "${VECU_PID}" 2>/dev/null; then
        echo "  the vECU exited after ${SLEPT}s -- see ${OUT_DIR}/vecu.stderr"
        break
    fi
    sleep 1
    SLEPT=$((SLEPT + 1))
done
cleanup
trap - INT TERM

if [[ ! -s "${TRACE}" ]]; then
    echo "run_vecu_wire: the vECU produced an EMPTY trace -- it failed before its first" >&2
    echo "  trace line, so this is a startup problem, not a wire problem. stderr:" >&2
    sed 's/^/    /' "${OUT_DIR}/vecu.stderr" >&2
    exit "${EXIT_USAGE}"
fi

# --- verdict ------------------------------------------------------------------
echo
python3 "${ANALYZER}" "${TRACE}" --min-samples "${MIN_SAMPLES}" --json "${RESULT_JSON}" \
    --ss2-commit "${SS2_COMMIT}"
RC=$?

echo
echo "  trace   ${TRACE}"
echo "  verdict ${RESULT_JSON}"
if [[ "${STALE_BIN}" -eq 1 ]]; then
    echo
    echo "  REMINDER: the binary (${BIN_MTIME}) predates vehicle_run.hpp. Any INCONCLUSIVE"
    echo "  on V1/V3 above is most likely THIS, not the simulator -- rebuild and re-run"
    echo "  before recording either as a finding."
fi
if [[ "${RC}" -eq "${EXIT_GATE_FAIL}" ]]; then
    echo
    echo "  V2 did not pass. Before suspecting Fast DDS 2.6 <-> 3.x interop, rule out the"
    echo "  three environmental causes with identical symptoms (see this script's header):"
    echo "    domain mismatch · durability mismatch · adapter not actually publishing"
    echo "  The adapter logs its own received=/published= counters -- check those first."
fi
exit "${RC}"
