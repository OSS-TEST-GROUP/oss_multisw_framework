#!/usr/bin/env bash
#
# Smoke test for sils-ci.sh -- self-contained, generates its own fixtures.
# Exercises the pipeline in the modes runnable without a DDS toolchain/daemon
# (compile is real; traces are pre-captured fixtures) and asserts exit codes.
#
# Run: bash tools/sils/ci/test_sils_ci.sh
#
set -u
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"
CI="${SCRIPT_DIR}/sils-ci.sh"
TOPO="${REPO_ROOT}/config/sils/topologies/viss_probe_smoke.yaml"

WORK="$(mktemp -d "${TMPDIR:-/tmp}/sils-ci-smoke.XXXXXX")"
trap 'rm -rf "${WORK}"' EXIT

pass=0 fail=0
check() {  # check <expected_exit> <label> -- rest are sils-ci args
    local expected="$1" label="$2"; shift 2
    bash "${CI}" "$@" >/dev/null 2>&1
    local rc=$?
    if [[ "${rc}" -eq "${expected}" ]]; then
        echo "PASS ${label} (exit ${rc})"; pass=$((pass+1))
    else
        echo "FAIL ${label} (exit ${rc}, expected ${expected})"; fail=$((fail+1))
    fi
}

# --- fixtures: a captured PASSED trace + identical/drifted goldens -----------
mkdir -p "${WORK}/traces" "${WORK}/golden-ok" "${WORK}/golden-bad" "${WORK}/traces-bad"
python3 - "${WORK}" "${REPO_ROOT}" <<'PY'
import json, sys
work, root = sys.argv[1], sys.argv[2]
sys.path.insert(0, root + "/tools/sils/conformance")
from test_evaluate import passing_events
evs = passing_events(run_id="cap-c0", instance_id="viss-probe-1")
evs.insert(6, {"run_id":"cap-c0","instance_id":"viss-probe-1","event":"publish_intent",
               "seq":0,"wall_time_ms":1005,"speed":10.0})
def dump(events, path): open(path,"w").write("\n".join(json.dumps(e) for e in events)+"\n")
dump(evs, work+"/traces/cap-c0.trace.jsonl")
dump(evs, work+"/golden-ok/cap-c0.trace.jsonl")
bad=[dict(e) for e in evs]
for e in bad:
    if e["event"]=="publish_intent": e["speed"]=99.0
dump(bad, work+"/golden-bad/cap-c0.trace.jsonl")
# truncated (no terminal event) -> INCOMPLETE
dump(evs[:6], work+"/traces-bad/cap-c0.trace.jsonl")
PY
cat > "${WORK}/rules.yaml" <<'YAML'
input_mode: replay
latency_window_ms: 50
sample_policy: {missing: fail, extra: fail}
YAML

# --- scenarios ---------------------------------------------------------------
check 0 "dry-run (compile + Coordinator plan, no launch)" \
    --topology "${TOPO}" --out-dir "${WORK}/dry" --dry-run
check 0 "traces-dir + golden MATCH -> PASS" \
    --topology "${TOPO}" --out-dir "${WORK}/ok" --traces-dir "${WORK}/traces" \
    --golden-dir "${WORK}/golden-ok" --rules "${WORK}/rules.yaml"
check 1 "golden drift -> REGRESSION" \
    --topology "${TOPO}" --out-dir "${WORK}/bad" --traces-dir "${WORK}/traces" \
    --golden-dir "${WORK}/golden-bad" --rules "${WORK}/rules.yaml"
check 1 "incomplete trace -> fleet FAIL" \
    --topology "${TOPO}" --out-dir "${WORK}/inc" --traces-dir "${WORK}/traces-bad"
check 2 "usage: no --topology and no --resolved" --out-dir "${WORK}/u1"
check 2 "usage: no --out-dir" --topology "${TOPO}"

# lint gate: a broken catalog must fail BEFORE compile. --contracts is relative to
# the repo root (that's what sils-ci hands the linter), so drop a broken catalog at
# a repo-root-relative temp path and clean it up in the trap.
BADREL="tmp-sils-ci-badcat-$$.yaml"
cat > "${REPO_ROOT}/${BADREL}" <<'YAML'
contracts:
  - id: broken
    transport: dds
    domain_env: TYPO_DDS_DOMAIN_ID     # unknown env
    idl: idl/does/not/exist.idl         # missing file
    struct: X
    registered_type: X
    topic: /x
    qos_profile: ghost                  # dangling qos ref
    ownership: single
    roles: {sut: subscribe, vecu: publish}
YAML
trap 'rm -rf "${WORK}"; rm -f "${REPO_ROOT}/${BADREL}"' EXIT
check 1 "lint gate: broken catalog fails before compile" \
    --topology "${TOPO}" --out-dir "${WORK}/lintfail" --contracts "${BADREL}"
check 0 "lint gate: --no-lint bypasses lint (dry-run)" \
    --topology "${TOPO}" --out-dir "${WORK}/nolint" --no-lint --dry-run

# --- artifact presence check on the happy path -------------------------------
for f in resolved-run.yaml saved-service-list.yaml verdict.json junit.xml; do
    if [[ -f "${WORK}/ok/${f}" ]]; then
        echo "PASS artifact ${f}"; pass=$((pass+1))
    else
        echo "FAIL artifact ${f} missing"; fail=$((fail+1))
    fi
done

echo
echo "$((pass))/$((pass+fail)) passed"
[[ "${fail}" -eq 0 ]] || exit 1
