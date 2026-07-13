#!/usr/bin/env bash
#
# SiLS Run Coordinator -- Phase 1 minimal (architecture v3 Final §5, §17 Scope Gate).
#
# Implements exactly the Phase 1 flow, nothing more:
#   Start -> process/container running -> readiness wait -> sample verification
#   -> Stop -> idempotent Cleanup
#
# Two backends, selected with --backend:
#   local  (default) -- runs sils_vecu_viss_probe as a plain background process.
#   docker            -- runs it as `docker run --network host <image>`
#                        (docker/Dockerfile.rel target runtime-mmf-sils-vecu),
#                        satisfying the Scope Gate's "Docker backend 하나" bullet.
#
# Both backends still use the SAME independent verify step (the host-side
# sils_vecu_viss_verify binary) -- for the docker backend this relies on
# `--network host` putting the container on the same network namespace as
# the host, so a locally-run verify binary reaches the same DDS domain.
# adsw_manager/lifecycle_mgr registration (StartContainerRequest instead of a
# direct `docker run`) is Topology Compiler / Fleet Runtime territory (§11) --
# not this script; see service/sils/vecu/examples/saved-service-list.viss_probe.yaml
# for how that registration would look.
#
# Not implemented here (explicit Phase 1 exclusions, §17):
#   - RunControl/VecuReport control-plane IDL (§5.4) -- this script uses process/
#     container liveness + trace grep as a readiness *surrogate*, not the real protocol.
#   - Relay Gateway, lockstep, replay, fault injection.
#
# Usage:
#   run_phase1.sh start --backend local  --bin-dir <dir> [--domain <id>] [--run-id <id>] [--instance-id <id>]
#   run_phase1.sh start --backend docker --image <image> --verify-bin <path> [--domain <id>] [--run-id <id>] [--instance-id <id>]
#   run_phase1.sh cleanup --run-id <id>
#
# --run-id identifies one Coordinator run (Start->verify->Stop lifecycle); it is
# NOT the vECU's own name. --instance-id (default "viss-probe-1", matching
# descriptors/viss_probe.yaml) is the vECU's identity -- distinct so a future
# multi-instance fleet (§11) can tell "which run" apart from "which vECU"
# in trace/participant names.

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

# --- cleanup (idempotent, backend-aware) ------------------------------------

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

    if [[ "${BACKEND:-local}" == "docker" ]]; then
        if [[ -n "${CONTAINER_NAME:-}" ]] && docker inspect "${CONTAINER_NAME}" >/dev/null 2>&1; then
            docker stop -t 3 "${CONTAINER_NAME}" >/dev/null 2>&1
            docker rm -f "${CONTAINER_NAME}" >/dev/null 2>&1
        fi
    else
        if [[ -n "${PROBE_PID:-}" ]] && kill -0 "${PROBE_PID}" 2>/dev/null; then
            kill -TERM "${PROBE_PID}" 2>/dev/null
            for _ in $(seq 1 20); do
                kill -0 "${PROBE_PID}" 2>/dev/null || break
                sleep 0.1
            done
            kill -0 "${PROBE_PID}" 2>/dev/null && kill -KILL "${PROBE_PID}" 2>/dev/null
        fi
    fi
    rm -f "${state}"
    trace "${run_id}" "cleanup_done" | tee -a "$(trace_file "${run_id}")"
}

# --- start: local backend ----------------------------------------------------

start_local() {
    local run_id="$1" domain="$2" bin_dir="$3" tfile="$4" instance_id="$5"

    local probe_bin="${bin_dir}/sils_vecu_viss_probe"
    local verify_bin="${bin_dir}/sils_vecu_viss_verify"
    if [[ ! -x "${probe_bin}" || ! -x "${verify_bin}" ]]; then
        echo "start: expected executables not found under ${bin_dir}" >&2
        echo "  (build first: ./build.sh --build-sils)" >&2
        exit 2
    fi

    SILS_RUN_ID="${run_id}" VISS_DDS_DOMAIN_ID="${domain}" SILS_VECU_INSTANCE_ID="${instance_id}" \
        "${probe_bin}" >> "${tfile}" 2>&1 &
    local probe_pid=$!
    {
        echo "BACKEND=local"
        echo "PROBE_PID=${probe_pid}"
        echo "DOMAIN=${domain}"
        echo "INSTANCE_ID=${instance_id}"
    } > "$(state_file "${run_id}")"

    sleep 0.2
    if ! kill -0 "${probe_pid}" 2>/dev/null; then
        trace "${run_id}" "start_failed" "\"reason\":\"probe process exited immediately\"" | tee -a "${tfile}"
        exit 1
    fi
    trace "${run_id}" "process_running" "\"pid\":${probe_pid}" | tee -a "${tfile}"
    echo "${verify_bin}"
}

# --- start: docker backend ---------------------------------------------------

start_docker() {
    local run_id="$1" domain="$2" image="$3" verify_bin="$4" tfile="$5" instance_id="$6"

    if ! command -v docker >/dev/null 2>&1; then
        echo "start: docker CLI not found" >&2
        exit 2
    fi
    if [[ -z "${image}" ]]; then
        echo "start: --image is required for --backend docker" >&2
        exit 2
    fi
    if [[ -z "${verify_bin}" || ! -x "${verify_bin}" ]]; then
        echo "start: --verify-bin (host-side sils_vecu_viss_verify) is required and must be executable for --backend docker" >&2
        exit 2
    fi

    local container_name="sils-vecu-${run_id}"
    if ! docker run -d --name "${container_name}" \
        --network host \
        -e "VISS_DDS_DOMAIN_ID=${domain}" \
        -e "SILS_RUN_ID=${run_id}" \
        -e "SILS_VECU_INSTANCE_ID=${instance_id}" \
        "${image}" >/dev/null 2>>"${tfile}"; then
        trace "${run_id}" "start_failed" "\"reason\":\"docker run failed\"" | tee -a "${tfile}"
        exit 1
    fi
    {
        echo "BACKEND=docker"
        echo "CONTAINER_NAME=${container_name}"
        echo "DOMAIN=${domain}"
        echo "INSTANCE_ID=${instance_id}"
    } > "$(state_file "${run_id}")"

    sleep 0.5
    local running
    running="$(docker inspect -f '{{.State.Running}}' "${container_name}" 2>/dev/null)"
    if [[ "${running}" != "true" ]]; then
        docker logs "${container_name}" >> "${tfile}" 2>&1
        trace "${run_id}" "start_failed" "\"reason\":\"container not running\"" | tee -a "${tfile}"
        docker rm -f "${container_name}" >/dev/null 2>&1
        exit 1
    fi
    trace "${run_id}" "process_running" "\"container\":\"${container_name}\"" | tee -a "${tfile}"
    echo "${verify_bin}"
}

# --- start: common flow (dispatches to a backend, then readiness+verify) ----

cmd_start() {
    local backend="local" bin_dir="" image="" verify_bin_arg="" domain="90" run_id=""
    local instance_id="viss-probe-1"
    local ready_timeout_sec=5
    while [[ $# -gt 0 ]]; do
        case "$1" in
            --backend) backend="$2"; shift 2 ;;
            --bin-dir) bin_dir="$2"; shift 2 ;;
            --image) image="$2"; shift 2 ;;
            --verify-bin) verify_bin_arg="$2"; shift 2 ;;
            --domain) domain="$2"; shift 2 ;;
            --run-id) run_id="$2"; shift 2 ;;
            --instance-id) instance_id="$2"; shift 2 ;;
            *) echo "start: unknown arg $1" >&2; exit 2 ;;
        esac
    done
    if [[ -z "${run_id}" ]]; then
        run_id="sils-$(date +%s)-$$"
    fi

    local tfile
    tfile="$(trace_file "${run_id}")"
    : > "${tfile}"
    trace "${run_id}" "run_start" \
        "\"domain_id\":${domain},\"backend\":\"${backend}\",\"instance_id\":\"${instance_id}\"" \
        | tee -a "${tfile}"

    local verify_bin
    case "${backend}" in
        local)
            if [[ -z "${bin_dir}" ]]; then
                echo "start: --bin-dir is required for --backend local" >&2
                exit 2
            fi
            verify_bin="$(start_local "${run_id}" "${domain}" "${bin_dir}" "${tfile}" "${instance_id}")"
            ;;
        docker)
            verify_bin="$(start_docker "${run_id}" "${domain}" "${image}" "${verify_bin_arg}" "${tfile}" "${instance_id}")"
            ;;
        *)
            echo "start: unknown --backend ${backend} (expected local|docker)" >&2
            exit 2
            ;;
    esac

    # --- "VecuReady" surrogate: wait for the probe's own "ready" trace line.
    # local backend writes straight to $tfile; docker backend's stdout lives in
    # the container's log (`docker logs` dumps the full log every call, so just
    # re-fetch and grep it each poll -- no incremental tracking needed here).
    local container_name="sils-vecu-${run_id}"
    local waited=0 ready=false
    while (( waited < ready_timeout_sec * 10 )); do
        if [[ "${backend}" == "docker" ]]; then
            if docker logs "${container_name}" 2>&1 | grep -q '"event":"ready"'; then
                ready=true
                break
            fi
        elif grep -q '"event":"ready"' "${tfile}" 2>/dev/null; then
            ready=true
            break
        fi
        sleep 0.1
        waited=$((waited + 1))
    done
    if [[ "${backend}" == "docker" ]]; then
        docker logs "${container_name}" >> "${tfile}" 2>&1 || true
    fi
    if [[ "${ready}" != true ]]; then
        trace "${run_id}" "ready_timeout" | tee -a "${tfile}"
        cmd_cleanup --run-id "${run_id}" >> "${tfile}"
        exit 1
    fi
    trace "${run_id}" "vecu_ready" | tee -a "${tfile}"

    # --- sample verification: independent one-shot subscriber ---
    trace "${run_id}" "verify_invoke" | tee -a "${tfile}"
    SILS_RUN_ID="${run_id}" VISS_DDS_DOMAIN_ID="${domain}" SILS_VECU_INSTANCE_ID="${instance_id}" \
        "${verify_bin}" >> "${tfile}" 2>&1
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
