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
# `--network host` (shared net namespace, so a host-run verify reaches the same
# DDS domain and endpoints match) AND `--ipc=host` (shared /dev/shm, so Fast DDS's
# shared-memory transport actually delivers samples across the container boundary
# -- without it endpoints match but no data arrives; see start_docker).
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
#   run_phase1.sh start --from-resolved <resolved-run.yaml> [--container-index N] --bin-dir <dir> ...
#   run_phase1.sh start ... --dry-run       # print the resolved plan and exit, launch nothing
#   run_phase1.sh cleanup --run-id <id>
#   run_phase1.sh reap [--max-age-sec SEC] [--lease-dir DIR]   # recover orphaned runs (§14.4)
#
# --run-id identifies one Coordinator run (Start->verify->Stop lifecycle); it is
# NOT the vECU's own name. --instance-id (default "viss-probe-1", matching
# descriptors/viss_probe.yaml) is the vECU's identity -- distinct so a future
# multi-instance fleet (§11) can tell "which run" apart from "which vECU"
# in trace/participant names.
#
# --from-resolved reads backend/domain/instance-id from a Topology Compiler
# resolved-run.yaml (tools/sils/topology_compiler/compile.py), so the deployment
# plan isn't re-typed on the CLI. Explicit flags override the resolved values;
# --bin-dir/--image (where built artifacts live) stay CLI-only.
#
# --domain-pool LO-HI [--lease-ttl SEC] [--lease-dir DIR] leases a free DDS domain
# from the pool (tools/sils/run_coordinator/domain_lease.py) instead of using a
# fixed --domain, and releases it on cleanup (§12). Takes precedence over
# --domain/resolved. Exit code 3 = pool exhausted (queue & retry), distinct from
# a generic failure so a CI scheduler can act on it.

set -u

STATE_DIR="${SILS_COORDINATOR_STATE_DIR:-/tmp/sils-runs}"
mkdir -p "${STATE_DIR}"

trace() {
    local run_id="$1" event="$2" extra="${3:-}"
    local ts
    # GNU date supports %3N (ms); BSD/macOS date does not and emits a literal "N",
    # so detect that and fall back to whole-second precision (x1000). The target
    # runtime is Linux (ms precision), but keep the coordinator runnable on a dev
    # mac without corrupting the JSON.
    ts=$(date +%s%3N 2>/dev/null)
    if [[ -z "${ts}" || "${ts}" == *N* ]]; then
        ts="$(date +%s)000"
    fi
    if [[ -n "${extra}" ]]; then
        echo "{\"run_id\":\"${run_id}\",\"event\":\"${event}\",\"wall_time_ms\":${ts},${extra}}"
    else
        echo "{\"run_id\":\"${run_id}\",\"event\":\"${event}\",\"wall_time_ms\":${ts}}"
    fi
}

state_file() { echo "${STATE_DIR}/${1}.state"; }
trace_file() { echo "${STATE_DIR}/${1}.trace.jsonl"; }

# tools/sils/run_coordinator/domain_lease.py, relative to this script.
lease_helper() { echo "$(dirname "$0")/../../../tools/sils/run_coordinator/domain_lease.py"; }

# --- teardown (idempotent, backend-aware) -----------------------------------

# do_teardown does the actual cleanup and RETURNS (no exit), so it's safe to call
# from mid-run failure paths (start_*/ready_timeout) without terminating the
# script. cmd_cleanup is the standalone subcommand wrapper that adds arg parsing
# and exits. Keeping them separate avoids the trap where an internal cleanup call
# would `exit 0` out from under the caller's intended exit code.
do_teardown() {
    local run_id="$1" state
    state="$(state_file "${run_id}")"
    if [[ ! -f "${state}" ]]; then
        # Idempotent: already cleaned (or never existed) is success, not an error.
        trace "${run_id}" "cleanup_noop" | tee -a "$(trace_file "${run_id}")"
        return 0
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

    # Release the DDS domain lease (if this run held one) so the domain returns
    # to the pool immediately rather than waiting out its TTL (§12.2). release is
    # keyed by run_id and idempotent, so a double teardown is harmless. The lease
    # info is written to the state file at allocation time (before backend start),
    # so even a start-failure teardown releases it.
    if [[ "${DOMAIN_LEASED:-0}" == "1" ]]; then
        local ld_args=()
        [[ -n "${LEASE_DIR:-}" ]] && ld_args=(--lease-dir "${LEASE_DIR}")
        python3 "$(lease_helper)" "${ld_args[@]}" release --run-id "${run_id}" >/dev/null 2>&1 || true
        trace "${run_id}" "domain_released" | tee -a "$(trace_file "${run_id}")"
    fi

    rm -f "${state}"
    trace "${run_id}" "cleanup_done" | tee -a "$(trace_file "${run_id}")"
    return 0
}

cmd_cleanup() {
    local run_id=""
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
    do_teardown "${run_id}"
    exit 0
}

# --- reap: recover orphaned runs (§14.4) ------------------------------------

# An external supervisor / CI finalizer runs this to recover runs whose
# coordinator died mid-run (SIGKILL, runner eviction, ...) without tearing down:
# a run is orphaned when its coordinator process (COORD_PID) is gone, or the run
# is older than --max-age-sec (pid-reuse fallback). For each orphan it re-runs
# the same idempotent do_teardown (stop container/process, release domain lease,
# remove state) and also reaps expired domain leases whose state file is already
# gone. Safe to run repeatedly and concurrently with live runs (a live run's
# COORD_PID is alive, so it's skipped).
cmd_reap() {
    local max_age_sec=7200 lease_dir=""
    while [[ $# -gt 0 ]]; do
        case "$1" in
            --max-age-sec) max_age_sec="$2"; shift 2 ;;
            --lease-dir) lease_dir="$2"; shift 2 ;;
            *) echo "reap: unknown arg $1" >&2; exit 2 ;;
        esac
    done

    local now reaped=0
    now="$(date +%s)"

    shopt -s nullglob
    local state
    for state in "${STATE_DIR}"/*.state; do
        local run_id coord_pid created
        run_id="$(basename "${state}" .state)"
        # Read fields in a subshell so per-file vars don't leak between iterations.
        coord_pid="$(source "${state}" 2>/dev/null; echo "${COORD_PID:-}")"
        created="$(source "${state}" 2>/dev/null; echo "${CREATED:-0}")"

        local orphan=false reason=""
        if [[ -n "${coord_pid}" ]] && ! kill -0 "${coord_pid}" 2>/dev/null; then
            orphan=true; reason="coordinator pid ${coord_pid} gone"
        elif (( now - created > max_age_sec )); then
            orphan=true; reason="age $((now - created))s > max ${max_age_sec}s"
        fi

        if [[ "${orphan}" == true ]]; then
            trace "${run_id}" "orphan_detected" "\"reason\":\"${reason}\"" \
                | tee -a "$(trace_file "${run_id}")"
            do_teardown "${run_id}"
            trace "${run_id}" "aborted_orphaned" "\"reason\":\"${reason}\"" \
                | tee -a "$(trace_file "${run_id}")"
            reaped=$((reaped + 1))
        fi
    done
    shopt -u nullglob

    # Also drop expired domain leases whose state file is already gone (e.g. state
    # removed but a stale lease lingered) -- domain_lease reap is TTL-based and
    # idempotent.
    if [[ -n "${lease_dir}" ]]; then
        python3 "$(lease_helper)" --lease-dir "${lease_dir}" reap >/dev/null 2>&1 || true
    else
        python3 "$(lease_helper)" reap >/dev/null 2>&1 || true
    fi

    echo "# reaped ${reaped} orphaned run(s)" >&2
    echo "${reaped}"
}

# The backend start_* functions run in the CURRENT shell (NOT `$(...)` command
# substitution) and hand their result back via this global. An earlier version
# captured them with `verify_bin="$(start_local ...)"`, which was doubly broken:
# their `exit` only killed the subshell (so a failed start fell through to the
# ready-wait instead of aborting), and their trace stdout got mixed into the
# captured "return value". Running in-shell makes `exit` abort cmd_start as
# intended and keeps trace output on the terminal/trace file.
g_verify_bin=""

# --- start: local backend ----------------------------------------------------

start_local() {
    local run_id="$1" domain="$2" bin_dir="$3" tfile="$4" instance_id="$5"

    local probe_bin="${bin_dir}/sils_vecu_viss_probe"
    local verify_bin="${bin_dir}/sils_vecu_viss_verify"
    if [[ ! -x "${probe_bin}" || ! -x "${verify_bin}" ]]; then
        echo "start: expected executables not found under ${bin_dir}" >&2
        echo "  (build first: ./build.sh --build-sils)" >&2
        do_teardown "${run_id}"
        exit 2
    fi

    SILS_RUN_ID="${run_id}" VISS_DDS_DOMAIN_ID="${domain}" SILS_VECU_INSTANCE_ID="${instance_id}" \
        "${probe_bin}" >> "${tfile}" 2>&1 &
    local probe_pid=$!
    # Append (state file was created at allocation time in cmd_start so lease info
    # is already there); do NOT truncate or the DOMAIN_LEASED line is lost.
    {
        echo "BACKEND=local"
        echo "PROBE_PID=${probe_pid}"
        echo "DOMAIN=${domain}"
        echo "INSTANCE_ID=${instance_id}"
    } >> "$(state_file "${run_id}")"

    sleep 0.2
    if ! kill -0 "${probe_pid}" 2>/dev/null; then
        trace "${run_id}" "start_failed" "\"reason\":\"probe process exited immediately\"" | tee -a "${tfile}"
        do_teardown "${run_id}"
        exit 1
    fi
    trace "${run_id}" "process_running" "\"pid\":${probe_pid}" | tee -a "${tfile}"
    g_verify_bin="${verify_bin}"
}

# --- start: docker backend ---------------------------------------------------

start_docker() {
    local run_id="$1" domain="$2" image="$3" verify_bin="$4" tfile="$5" instance_id="$6"

    if ! command -v docker >/dev/null 2>&1; then
        echo "start: docker CLI not found" >&2
        do_teardown "${run_id}"
        exit 2
    fi
    if [[ -z "${image}" ]]; then
        echo "start: --image is required for --backend docker" >&2
        do_teardown "${run_id}"
        exit 2
    fi
    if [[ -z "${verify_bin}" || ! -x "${verify_bin}" ]]; then
        echo "start: --verify-bin (host-side sils_vecu_viss_verify) is required and must be executable for --backend docker" >&2
        do_teardown "${run_id}"
        exit 2
    fi

    local container_name="sils-vecu-${run_id}"
    # CONTAINER_NAME must land in the state file BEFORE `docker run` so a teardown
    # can find and remove the container even if we die between run and the append.
    echo "BACKEND=docker" >> "$(state_file "${run_id}")"
    echo "CONTAINER_NAME=${container_name}" >> "$(state_file "${run_id}")"
    echo "DOMAIN=${domain}" >> "$(state_file "${run_id}")"
    echo "INSTANCE_ID=${instance_id}" >> "$(state_file "${run_id}")"

    # --network host: shares the host net namespace so UDP discovery reaches the
    #   host-side verify (endpoints match) and both sit on the same DDS domain.
    # --ipc=host: REQUIRED for data, not just discovery. Fast DDS negotiates its
    #   shared-memory transport for peers it sees as same-host (which --network host
    #   makes them). Without a shared IPC namespace the container has its own
    #   /dev/shm, so the writer's SHM segment is invisible to the host reader:
    #   endpoints match but ZERO samples arrive (verified 2026-07 -- the classic
    #   "matched but no data" symptom). Sharing the host IPC namespace opens the SHM
    #   data path. (Alternative would be disabling the SHM transport to force UDP.)
    if ! docker run -d --name "${container_name}" \
        --network host \
        --ipc=host \
        -e "VISS_DDS_DOMAIN_ID=${domain}" \
        -e "SILS_RUN_ID=${run_id}" \
        -e "SILS_VECU_INSTANCE_ID=${instance_id}" \
        "${image}" >/dev/null 2>>"${tfile}"; then
        trace "${run_id}" "start_failed" "\"reason\":\"docker run failed\"" | tee -a "${tfile}"
        do_teardown "${run_id}"
        exit 1
    fi

    sleep 0.5
    local running
    running="$(docker inspect -f '{{.State.Running}}' "${container_name}" 2>/dev/null)"
    if [[ "${running}" != "true" ]]; then
        docker logs "${container_name}" >> "${tfile}" 2>&1
        trace "${run_id}" "start_failed" "\"reason\":\"container not running\"" | tee -a "${tfile}"
        do_teardown "${run_id}"
        exit 1
    fi
    trace "${run_id}" "process_running" "\"container\":\"${container_name}\"" | tee -a "${tfile}"
    g_verify_bin="${verify_bin}"
}

# --- start: common flow (dispatches to a backend, then readiness+verify) ----

cmd_start() {
    local backend="" bin_dir="" image="" verify_bin_arg="" domain="" run_id=""
    local instance_id="" from_resolved="" container_index=0 dry_run=false
    local domain_pool="" lease_ttl=3600 lease_dir=""
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
            --from-resolved) from_resolved="$2"; shift 2 ;;
            --container-index) container_index="$2"; shift 2 ;;
            --domain-pool) domain_pool="$2"; shift 2 ;;
            --lease-ttl) lease_ttl="$2"; shift 2 ;;
            --lease-dir) lease_dir="$2"; shift 2 ;;
            --dry-run) dry_run=true; shift ;;
            *) echo "start: unknown arg $1" >&2; exit 2 ;;
        esac
    done

    # Topology Compiler integration: a resolved-run.yaml (tools/sils/topology_compiler)
    # is the source of truth for WHAT to run (backend, domain, which vECU instance).
    # Explicit CLI flags still WIN over it, so an operator can override a single field
    # without editing the artifact. Paths that aren't in the plan -- where the built
    # binaries (--bin-dir) or image (--image) actually live -- stay CLI-only.
    if [[ -n "${from_resolved}" ]]; then
        local helper shell_vars
        helper="$(dirname "$0")/../../../tools/sils/topology_compiler/resolved_to_shell.py"
        if ! shell_vars="$(python3 "${helper}" "${from_resolved}" --index "${container_index}")"; then
            echo "start: failed to read resolved-run '${from_resolved}' (index ${container_index})" >&2
            echo "${shell_vars}" >&2
            exit 2
        fi
        eval "${shell_vars}"
        [[ -z "${backend}"     ]] && backend="${RESOLVED_BACKEND}"
        [[ -z "${domain}"      ]] && domain="${RESOLVED_DOMAIN}"
        [[ -z "${instance_id}" ]] && instance_id="${RESOLVED_INSTANCE_ID}"
    fi

    # Defaults for anything still unset after (optional) resolved-run + CLI.
    [[ -z "${backend}"     ]] && backend="local"
    [[ -z "${domain}"      ]] && domain="90"
    [[ -z "${instance_id}" ]] && instance_id="viss-probe-1"
    if [[ -z "${run_id}" ]]; then
        run_id="sils-$(date +%s)-$$"
    fi

    if [[ "${dry_run}" == true ]]; then
        # Print the resolved plan and exit WITHOUT launching anything -- lets an
        # operator (or CI, or this sandbox with no binaries/daemon) confirm what a
        # run would do. Deliberately does NOT allocate a domain lease (that would
        # leak a lease for a run that never happens); shows the pool it *would*
        # allocate from instead. Not a trace file event: nothing was started.
        local dry_domain="${domain}"
        [[ -n "${domain_pool}" ]] && dry_domain="auto(pool ${domain_pool})"
        echo "{\"event\":\"dry_run\",\"run_id\":\"${run_id}\",\"backend\":\"${backend}\",\"domain_id\":\"${dry_domain}\",\"instance_id\":\"${instance_id}\",\"from_resolved\":\"${from_resolved}\",\"container_index\":${container_index}}"
        exit 0
    fi

    local tfile
    tfile="$(trace_file "${run_id}")"
    : > "${tfile}"

    # DDS domain lease (§12): if a pool is given, allocate a domain from it and
    # bind the run to it. Takes precedence over --domain/resolved domain -- the
    # whole point of a pool is that the Coordinator, not the operator, picks the
    # free id. A specific-error exit (3) means "pool exhausted, queue & retry",
    # distinct from a generic failure so a CI scheduler can act on it.
    # State file is created HERE (truncate once), before allocation/dispatch, so
    # the lease line is present from the moment a domain is leased -- any later
    # teardown (even a start failure) can then release it. Backends append.
    # COORD_PID/CREATED let `reap` (§14.4) detect this run as orphaned if this
    # coordinator process dies mid-run without tearing down.
    {
        echo "COORD_PID=$$"
        echo "CREATED=$(date +%s)"
    } > "$(state_file "${run_id}")"

    if [[ -n "${domain_pool}" ]]; then
        local alloc_args=(allocate --pool "${domain_pool}" --ttl "${lease_ttl}" --run-id "${run_id}")
        [[ -n "${lease_dir}" ]] && alloc_args=(--lease-dir "${lease_dir}" "${alloc_args[@]}")
        local allocated rc
        allocated="$(python3 "$(lease_helper)" "${alloc_args[@]}")"
        rc=$?
        if [[ ${rc} -eq 3 ]]; then
            trace "${run_id}" "domain_pool_exhausted" "\"pool\":\"${domain_pool}\"" | tee -a "${tfile}"
            rm -f "$(state_file "${run_id}")"
            exit 3
        elif [[ ${rc} -ne 0 ]]; then
            trace "${run_id}" "domain_alloc_failed" "\"pool\":\"${domain_pool}\"" | tee -a "${tfile}"
            rm -f "$(state_file "${run_id}")"
            exit 2
        fi
        domain="${allocated}"
        # Record the lease immediately so do_teardown releases it on any exit path.
        echo "DOMAIN_LEASED=1" >> "$(state_file "${run_id}")"
        [[ -n "${lease_dir}" ]] && echo "LEASE_DIR=${lease_dir}" >> "$(state_file "${run_id}")"
        trace "${run_id}" "domain_leased" "\"domain_id\":${domain},\"pool\":\"${domain_pool}\"" | tee -a "${tfile}"
    fi

    trace "${run_id}" "run_start" \
        "\"domain_id\":${domain},\"backend\":\"${backend}\",\"instance_id\":\"${instance_id}\"" \
        | tee -a "${tfile}"

    # NOTE: start_* run in the CURRENT shell (not $(...)) and set g_verify_bin;
    # their `exit` therefore aborts this function on failure (see g_verify_bin decl).
    g_verify_bin=""
    case "${backend}" in
        local)
            if [[ -z "${bin_dir}" ]]; then
                echo "start: --bin-dir is required for --backend local" >&2
                do_teardown "${run_id}"
                exit 2
            fi
            start_local "${run_id}" "${domain}" "${bin_dir}" "${tfile}" "${instance_id}"
            ;;
        docker)
            start_docker "${run_id}" "${domain}" "${image}" "${verify_bin_arg}" "${tfile}" "${instance_id}"
            ;;
        *)
            echo "start: unknown --backend ${backend} (expected local|docker)" >&2
            do_teardown "${run_id}"
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
        do_teardown "${run_id}" >> "${tfile}"
        exit 1
    fi
    trace "${run_id}" "vecu_ready" | tee -a "${tfile}"

    # --- sample verification: independent one-shot subscriber ---
    trace "${run_id}" "verify_invoke" | tee -a "${tfile}"
    SILS_RUN_ID="${run_id}" VISS_DDS_DOMAIN_ID="${domain}" SILS_VECU_INSTANCE_ID="${instance_id}" \
        "${g_verify_bin}" >> "${tfile}" 2>&1
    local verify_rc=$?

    # --- Stop + idempotent Cleanup ---
    # do_teardown, NOT cmd_cleanup: cmd_cleanup exits 0, which would swallow the
    # run_pass/run_fail trace and the verify-based exit code below.
    do_teardown "${run_id}" >> "${tfile}"

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
        reap) cmd_reap "$@" ;;
        *)
            echo "usage: $0 {start|cleanup|reap} [args...]" >&2
            exit 2
            ;;
    esac
}

main "$@"
