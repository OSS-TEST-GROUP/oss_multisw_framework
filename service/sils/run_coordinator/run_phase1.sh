#!/usr/bin/env bash
#
# SiLS Run Coordinator -- Phase 1 minimal (architecture v3 Final §5, §17 Scope Gate).
#
# Implements exactly the Phase 1 flow, nothing more:
#   Start -> process running -> readiness wait -> sample verification -> Stop -> idempotent Cleanup
#
# Not implemented here (explicit Phase 1 exclusions, §17):
#   - RunControl/VecuReport control-plane IDL (§5.4) -- this script uses process
#     liveness + trace grep as a readiness *surrogate*, not the real protocol.
#   - Docker/lifecycle_mgr/adsw_manager integration (Topology Compiler, Fleet
#     Runtime territory, §11/§13) -- this runs the vecu binary as a plain local
#     process. Wiring it behind `docker run` + saved-service-list is the next
#     increment, not this script.
#   - Relay Gateway, lockstep, replay, fault injection.
#
# Usage:
#   run_phase1.sh start   --bin-dir <dir> [--domain <id>] [--run-id <id>]
#   run_phase1.sh cleanup --run-id <id>
#
# <dir> must contain sils_vecu_viss_probe and sils_vecu_viss_verify (built via
# ./build.sh --build-sils ... ; see docs/sils-virtual-ecu-architecture-v3.md §17).

set -u

STATE_DIR="${SILS_COORDINATOR_STATE_DIR:-/tmp/sils-runs}"
mkdir -p "${STATE_DIR}"

trace() {
    local run_id="$1" event="$2" extra="${3:-}"
    local ts
    ts=$(date +%s%3N 2>/dev/null || date +%s000)
    if [[ -n "${extra}" ]]; then
        echo "{\"run_id\":\"${run_id}\",\"event\":\"${event}\",\"wall_time_ms\":${ts},${extra}}"
    else
        echo "{\"run_id\":\"${run_id}\",\"event\":\"${event}\",\"wall_time_ms\":${ts}}"
    fi
}

state_file() { echo "${STATE_DIR}/${1}.state"; }
trace_file() { echo "${STATE_DIR}/${1}.trace.jsonl"; }

cmd_cleanup() {
    local run_id="" state
    while [[ $# -gt 0 ]]; do
        case "$1" in
            --run-id) run_id="$2"; shift 2 ;;
            *) echo "cleanup: unknown arg $1" >&2; exit 2 ;;
        esac
    done
    if [[ -z "${run_id}" ]]; then
        echo "cleanup: --run-id is required" >&2
        exit 2
    fi

    state="$(state_file "${run_id}")"
    if [[ ! -f "${state}" ]]; then
        # Idempotent: already cleaned (or never existed) is success, not an error.
        trace "${run_id}" "cleanup_noop" | tee -a "$(trace_file "${run_id}")"
        exit 0
    fi

    # shellcheck disable=SC1090
    source "${state}"
    if [[ -n "${PROBE_PID:-}" ]] && kill -0 "${PROBE_PID}" 2>/dev/null; then
        kill -TERM "${PROBE_PID}" 2>/dev/null
        for _ in $(seq 1 20); do
            kill -0 "${PROBE_PID}" 2>/dev/null || break
            sleep 0.1
        done
        kill -0 "${PROBE_PID}" 2>/dev/null && kill -KILL "${PROBE_PID}" 2>/dev/null
    fi
    rm -f "${state}"
    trace "${run_id}" "cleanup_done" | tee -a "$(trace_file "${run_id}")"
}

cmd_start() {
    local bin_dir="" domain="90" run_id="" ready_timeout_sec=5
    while [[ $# -gt 0 ]]; do
        case "$1" in
            --bin-dir) bin_dir="$2"; shift 2 ;;
            --domain) domain="$2"; shift 2 ;;
            --run-id) run_id="$2"; shift 2 ;;
            *) echo "start: unknown arg $1" >&2; exit 2 ;;
        esac
    done
    if [[ -z "${bin_dir}" ]]; then
        echo "start: --bin-dir is required (dir containing sils_vecu_viss_probe/verify)" >&2
        exit 2
    fi
    if [[ -z "${run_id}" ]]; then
        run_id="sils-$(date +%s)-$$"
    fi

    local probe_bin="${bin_dir}/sils_vecu_viss_probe"
    local verify_bin="${bin_dir}/sils_vecu_viss_verify"
    if [[ ! -x "${probe_bin}" || ! -x "${verify_bin}" ]]; then
        echo "start: expected executables not found under ${bin_dir}" >&2
        echo "  (build first: ./build.sh --build-sils ; binaries land under the" >&2
        echo "   project's CMAKE_RUNTIME_OUTPUT_DIRECTORY for the configured build dir)" >&2
        exit 2
    fi

    local tfile
    tfile="$(trace_file "${run_id}")"
    : > "${tfile}"
    trace "${run_id}" "run_start" "\"domain_id\":${domain}" | tee -a "${tfile}"

    # --- Start ---
    SILS_RUN_ID="${run_id}" VISS_DDS_DOMAIN_ID="${domain}" \
        "${probe_bin}" >> "${tfile}" 2>&1 &
    local probe_pid=$!
    echo "PROBE_PID=${probe_pid}" > "$(state_file "${run_id}")"
    echo "BIN_DIR=${bin_dir}" >> "$(state_file "${run_id}")"
    echo "DOMAIN=${domain}" >> "$(state_file "${run_id}")"

    # --- "container running" surrogate: process liveness ---
    sleep 0.2
    if ! kill -0 "${probe_pid}" 2>/dev/null; then
        trace "${run_id}" "start_failed" "\"reason\":\"probe process exited immediately\"" | tee -a "${tfile}"
        exit 1
    fi
    trace "${run_id}" "process_running" "\"pid\":${probe_pid}" | tee -a "${tfile}"

    # --- "VecuReady" surrogate: wait for the probe's own "ready" trace line ---
    local waited=0
    local ready=false
    while (( waited < ready_timeout_sec * 10 )); do
        if grep -q '"event":"ready"' "${tfile}" 2>/dev/null; then
            ready=true
            break
        fi
        sleep 0.1
        waited=$((waited + 1))
    done
    if [[ "${ready}" != true ]]; then
        trace "${run_id}" "ready_timeout" | tee -a "${tfile}"
        cmd_cleanup --run-id "${run_id}" >> "${tfile}"
        exit 1
    fi
    trace "${run_id}" "vecu_ready" | tee -a "${tfile}"

    # --- sample verification: independent one-shot subscriber ---
    trace "${run_id}" "verify_invoke" | tee -a "${tfile}"
    SILS_RUN_ID="${run_id}" VISS_DDS_DOMAIN_ID="${domain}" "${verify_bin}" >> "${tfile}" 2>&1
    local verify_rc=$?

    # --- Stop + idempotent Cleanup ---
    cmd_cleanup --run-id "${run_id}" >> "${tfile}"

    if [[ ${verify_rc} -eq 0 ]]; then
        trace "${run_id}" "run_pass" | tee -a "${tfile}"
    else
        trace "${run_id}" "run_fail" "\"verify_exit_code\":${verify_rc}" | tee -a "${tfile}"
    fi
    echo "trace: ${tfile}" >&2
    exit "${verify_rc}"
}

main() {
    local sub="${1:-}"
    shift || true
    case "${sub}" in
        start) cmd_start "$@" ;;
        cleanup) cmd_cleanup "$@" ;;
        *)
            echo "usage: $0 {start|cleanup} [args...]" >&2
            exit 2
            ;;
    esac
}

main "$@"
