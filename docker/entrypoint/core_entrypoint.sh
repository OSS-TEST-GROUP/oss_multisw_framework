#!/bin/bash
# Container entrypoint for runtime-mmf-core (F011).
#
# Sequentially launches the core middleware processes that used to be
# fork()'d from lifecycle_mgr's MWServiceManager via config_middleware.yaml.
# Moving them out of the parent process means each one gets its own clean
# entrypoint, gets supervised by Docker/k8s, and lifecycle_mgr stays a
# single-purpose binary.
#
# Launch order (matches the user-stated F011 contract):
#   1. lifecycle_mgr               — must come up first so subsequent
#                                    services find its DDS endpoints
#                                    (UpdateServiceListResponse, etc.).
#   2. diagnostics                 — log/metrics service.
#   3. vehicle_info_manager_app    — vehicle info publisher.
#   4. OtaManager                  — OTA handler; takes a delay_time arg
#                                    (matches config_middleware.yaml default).
#
# Container lifetime tracks lifecycle_mgr (PID waited on at the end). If
# lifecycle_mgr exits, the script exits and the container's restart policy
# takes over. Other services failing are logged but do NOT bring the
# container down — matching ros2_entrypoint.sh's contract for ext-ros2.
#
# Intentionally no `set -e`: a failed background spawn must not abort
# subsequent launches.

set -u

# These ENV vars are normally provided by the Dockerfile (ENV directives in
# runtime-mmf-core); re-export them as a safety net for direct shell invocation
# during local debugging.
export LD_LIBRARY_PATH="${LD_LIBRARY_PATH:-/oss/lib}"
export MMF_CONFIG_PATH="${MMF_CONFIG_PATH:-/oss/bin/config}"
export DIAG_CONFIG_PATH="${DIAG_CONFIG_PATH:-/oss/bin/config}"
export VEHICLE_CONFIG_PATH="${VEHICLE_CONFIG_PATH:-/oss/bin/config}"
export OTAM_CONFIG_PATH="${OTAM_CONFIG_PATH:-/oss/bin/config}"
export DIAG_IDL_PATH="${DIAG_IDL_PATH:-/oss/bin/idl}"

LOG_TAG="[core_entrypoint]"

# Launch a background service with stderr/stdout passthrough and a small
# wrapper that logs the exit code (so a silent crash is visible). Skips
# (with a warning) if the binary isn't present or executable.
start_bg() {
    local bin="$1"; shift
    local name
    name="$(basename "$bin")"
    if [ ! -x "$bin" ]; then
        echo "$LOG_TAG WARN: $bin not executable, skipping" >&2
        return 0
    fi
    echo "$LOG_TAG launching $name $*"
    ( "$bin" "$@"; rc=$?
      echo "$LOG_TAG $name exited rc=$rc (container stays up unless lifecycle_mgr also exits)"
    ) &
}

# Step 1: lifecycle_mgr — capture its PID separately so we can wait on it
# at the end. The container's lifetime is bound to this single process.
if [ ! -x /oss/bin/lifecycle_mgr ]; then
    echo "$LOG_TAG FATAL: /oss/bin/lifecycle_mgr missing — aborting" >&2
    exit 1
fi
echo "$LOG_TAG launching lifecycle_mgr"
/oss/bin/lifecycle_mgr &
LIFECYCLE_PID=$!
sleep 1

# Step 2-4: supporting services. Brief sleeps give each one time to publish
# its DDS endpoints before the next comes up (avoids on_publication_matched
# races during initial discovery).
start_bg /oss/bin/diagnostics
sleep 1

start_bg /oss/bin/vehicle_info_manager_app
sleep 1

# OtaManager's first positional arg is delay_time (seconds); the previous
# config_middleware.yaml default was 3, preserve it here so the OTA boot
# timing for downstream clients doesn't shift.
start_bg /oss/bin/OtaManager 3

# Block on lifecycle_mgr. When it exits, propagate its return code so the
# orchestrator's restart policy can act on it.
wait "$LIFECYCLE_PID"
rc=$?
echo "$LOG_TAG lifecycle_mgr exited rc=$rc — shutting down container"
exit "$rc"
