#!/usr/bin/env bash
#
# SiLS CI pipeline runner -- architecture v3 Final §9/§11/§12/§16.
#
# One entry point that chains the SiLS tool layer into the pipeline a CI job runs:
#
#   compile        topology -> resolved-run.yaml + saved-service-list.yaml
#     (tools/sils/topology_compiler/compile.py)
#   run            per resolved container: Coordinator start -> trace.jsonl
#     (service/sils/run_coordinator/run_phase1.sh)
#   evaluate       traces -> conformance verdict.json  (§9/§11/§12)
#     (tools/sils/conformance/evaluate.py)
#   junit          verdict.json -> junit.xml           (CI ingest, §16)
#     (tools/sils/conformance/to_junit.py)
#   compare        each trace vs a golden trace        (§12.4 regression, optional)
#     (tools/sils/conformance/compare_golden.py)
#
# Every stage is skippable so the pipeline is usable at any maturity:
#   --resolved R     skip compile, use an existing resolved-run.yaml
#   --traces-dir D   skip run, evaluate pre-captured traces (the deterministic
#                    CI path once goldens exist; also the sandbox-testable path
#                    with no DDS toolchain)
#   --dry-run        compile for real, run the Coordinator in --dry-run (prints
#                    each container's plan, launches nothing), then stop before
#                    evaluate (nothing was produced to evaluate)
#   --golden-dir G   enable the regression compare stage
#
# Artifacts land under --out-dir: resolved-run.yaml, saved-service-list.yaml,
# traces/, verdict.json, junit.xml, compare/.
#
# Exit: 0 = pipeline OK (and fleet PASS / no regression), 1 = a stage failed or a
# verdict was FAIL / REGRESSION, 2 = usage error.

set -u

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"

COMPILE_PY="${REPO_ROOT}/tools/sils/topology_compiler/compile.py"
RESOLVED_TO_SHELL_PY="${REPO_ROOT}/tools/sils/topology_compiler/resolved_to_shell.py"
RUN_PHASE1_SH="${REPO_ROOT}/service/sils/run_coordinator/run_phase1.sh"
EVALUATE_PY="${REPO_ROOT}/tools/sils/conformance/evaluate.py"
TO_JUNIT_PY="${REPO_ROOT}/tools/sils/conformance/to_junit.py"
COMPARE_PY="${REPO_ROOT}/tools/sils/conformance/compare_golden.py"

log() { echo "[sils-ci] $*" >&2; }
die() { echo "[sils-ci] error: $*" >&2; exit 2; }

usage() {
    sed -n '2,40p' "$0" | sed 's/^# \{0,1\}//'
    exit "${1:-2}"
}

# --- args --------------------------------------------------------------------
topology="" out_dir="" contracts="" qos="" image=""
resolved_in="" traces_dir_in="" golden_dir="" rules=""
backend="local" bin_dir="" verify_bin="" run_id="" suite_name="sils-conformance"
dry_run=false keep_going=false

while [[ $# -gt 0 ]]; do
    case "$1" in
        --topology)    topology="$2"; shift 2 ;;
        --out-dir)     out_dir="$2"; shift 2 ;;
        --contracts)   contracts="$2"; shift 2 ;;
        --qos)         qos="$2"; shift 2 ;;
        --image)       image="$2"; shift 2 ;;
        --resolved)    resolved_in="$2"; shift 2 ;;
        --traces-dir)  traces_dir_in="$2"; shift 2 ;;
        --golden-dir)  golden_dir="$2"; shift 2 ;;
        --rules)       rules="$2"; shift 2 ;;
        --backend)     backend="$2"; shift 2 ;;
        --bin-dir)     bin_dir="$2"; shift 2 ;;
        --verify-bin)  verify_bin="$2"; shift 2 ;;
        --run-id)      run_id="$2"; shift 2 ;;
        --suite-name)  suite_name="$2"; shift 2 ;;
        --dry-run)     dry_run=true; shift ;;
        --keep-going)  keep_going=true; shift ;;
        -h|--help)     usage 0 ;;
        *) die "unknown arg $1 (see --help)" ;;
    esac
done

[[ -z "${out_dir}" ]] && die "--out-dir is required"
if [[ -z "${resolved_in}" && -z "${topology}" ]]; then
    die "provide --topology (to compile) or --resolved (to reuse a resolved-run.yaml)"
fi
[[ -z "${run_id}" ]] && run_id="ci-$(date +%s)-$$"

mkdir -p "${out_dir}"
traces_dir="${out_dir}/traces"
mkdir -p "${traces_dir}"
resolved="${out_dir}/resolved-run.yaml"

# --- stage 1: compile --------------------------------------------------------
if [[ -n "${resolved_in}" ]]; then
    log "compile: skipped (using ${resolved_in})"
    cp "${resolved_in}" "${resolved}" || die "cannot copy --resolved into out-dir"
else
    log "compile: ${topology}"
    compile_args=("${topology}" --repo-root "${REPO_ROOT}" --out-dir "${out_dir}")
    [[ -n "${contracts}" ]] && compile_args+=(--contracts "${contracts}")
    [[ -n "${qos}" ]] && compile_args+=(--qos "${qos}")
    [[ -n "${image}" ]] && compile_args+=(--image "${image}")
    if ! python3 "${COMPILE_PY}" "${compile_args[@]}"; then
        die "compile stage failed (validation error) -- see messages above"
    fi
fi
[[ -f "${resolved}" ]] || die "resolved-run.yaml not present after compile stage"

# Container count drives the run stage.
count_line="$(python3 "${RESOLVED_TO_SHELL_PY}" "${resolved}" --index 0 2>/dev/null | grep '^RESOLVED_CONTAINER_COUNT=')"
eval "${count_line}"
container_count="${RESOLVED_CONTAINER_COUNT:-0}"
log "resolved: ${container_count} container(s)"

# --- stage 2: run (or skip) --------------------------------------------------
if [[ -n "${traces_dir_in}" ]]; then
    log "run: skipped (evaluating pre-captured traces in ${traces_dir_in})"
    traces_dir="${traces_dir_in}"
elif [[ "${dry_run}" == true ]]; then
    log "run: --dry-run (Coordinator plan only, launching nothing)"
    for (( i=0; i<container_count; i++ )); do
        SILS_COORDINATOR_STATE_DIR="${traces_dir}" \
            bash "${RUN_PHASE1_SH}" start --from-resolved "${resolved}" \
                --container-index "${i}" --run-id "${run_id}-c${i}" --dry-run \
            || { [[ "${keep_going}" == true ]] || die "dry-run plan failed for container ${i}"; }
    done
    log "dry-run complete: compile+plan wiring verified; no traces produced, stopping before evaluate"
    exit 0
else
    log "run: backend=${backend}, ${container_count} container(s) -> ${traces_dir}"
    run_rc=0
    for (( i=0; i<container_count; i++ )); do
        rp_args=(start --from-resolved "${resolved}" --container-index "${i}"
                 --run-id "${run_id}-c${i}" --backend "${backend}")
        [[ -n "${bin_dir}" ]]    && rp_args+=(--bin-dir "${bin_dir}")
        [[ -n "${image}" ]]      && rp_args+=(--image "${image}")
        [[ -n "${verify_bin}" ]] && rp_args+=(--verify-bin "${verify_bin}")
        if ! SILS_COORDINATOR_STATE_DIR="${traces_dir}" bash "${RUN_PHASE1_SH}" "${rp_args[@]}"; then
            run_rc=1
            log "run: container ${i} did not pass (continuing to collect its trace)"
            [[ "${keep_going}" == true ]] || true  # keep collecting; verdict decided at evaluate
        fi
    done
    [[ "${run_rc}" -ne 0 ]] && log "run: at least one container did not pass (verdict decided by evaluate)"
fi

# --- stage 3+4: evaluate -> junit -------------------------------------------
shopt -s nullglob
trace_files=("${traces_dir}"/*.trace.jsonl)
shopt -u nullglob
if [[ "${#trace_files[@]}" -eq 0 ]]; then
    die "no *.trace.jsonl found in ${traces_dir} -- nothing to evaluate"
fi
log "evaluate: ${#trace_files[@]} trace(s) in ${traces_dir}"

verdict="${out_dir}/verdict.json"
eval_rc=0
python3 "${EVALUATE_PY}" "${traces_dir}" --resolved "${resolved}" > "${verdict}" || eval_rc=$?
log "verdict: ${verdict} (evaluate exit ${eval_rc})"

junit="${out_dir}/junit.xml"
python3 "${TO_JUNIT_PY}" --from-verdict "${verdict}" -o "${junit}" --suite-name "${suite_name}" \
    || die "junit conversion failed"

# --- stage 5: regression compare (optional) ----------------------------------
compare_rc=0
if [[ -n "${golden_dir}" ]]; then
    log "compare: golden-dir=${golden_dir}"
    mkdir -p "${out_dir}/compare"
    cmp_args=()
    [[ -n "${rules}" ]] && cmp_args+=(--rules "${rules}")
    for tf in "${trace_files[@]}"; do
        base="$(basename "${tf}")"
        golden="${golden_dir}/${base}"
        if [[ ! -f "${golden}" ]]; then
            log "compare: no golden for ${base} -- skipped"
            continue
        fi
        report="${out_dir}/compare/${base%.jsonl}.compare.json"
        if ! python3 "${COMPARE_PY}" --golden "${golden}" --candidate "${tf}" \
                "${cmp_args[@]}" --quiet > "${report}"; then
            compare_rc=1
            log "compare: REGRESSION vs golden for ${base} (see ${report})"
        fi
    done
fi

# --- aggregate ---------------------------------------------------------------
fleet_verdict="$(python3 -c "import json,sys; print(json.load(open('${verdict}')).get('fleet_verdict','?'))" 2>/dev/null || echo '?')"
log "SUMMARY: fleet=${fleet_verdict} junit=${junit}$( [[ -n "${golden_dir}" ]] && echo " regression=$( [[ ${compare_rc} -eq 0 ]] && echo PASS || echo FAIL )" )"

if [[ "${eval_rc}" -ne 0 || "${compare_rc}" -ne 0 ]]; then
    exit 1
fi
exit 0
