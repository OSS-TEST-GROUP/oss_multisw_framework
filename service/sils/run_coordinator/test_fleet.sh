#!/usr/bin/env bash
#
# Smoke test for run_phase1.sh `fleet` subcommand -- self-contained, no DDS/docker.
# Uses FAKE probe/verify binaries (shell scripts) so the real fleet orchestration
# (concurrent start of N members -> wait ALL ready -> one verify -> aggregate ->
# teardown ALL) runs end-to-end in the sandbox. The real multi-vECU DDS run is
# verified separately on an x86 box with the real binaries.
#
# Run: bash service/sils/run_coordinator/test_fleet.sh
#
set -u
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"
RUN="${SCRIPT_DIR}/run_phase1.sh"
EVAL="${REPO_ROOT}/tools/sils/conformance/evaluate.py"

WORK="$(mktemp -d "${TMPDIR:-/tmp}/sils-fleet.XXXXXX")"
export SILS_COORDINATOR_STATE_DIR="${WORK}/runs"
mkdir -p "${SILS_COORDINATOR_STATE_DIR}"
trap 'pkill -f "${WORK}/bin-ok/sils_vecu_viss_probe" 2>/dev/null; rm -rf "${WORK}"' EXIT

pass=0 fail=0
ck() { if [[ "$1" == "$2" ]]; then echo "PASS $3"; pass=$((pass+1)); else echo "FAIL $3 (got '$1' want '$2')"; fail=$((fail+1)); fi; }
ckc() { if echo "$1" | grep -q "$2"; then echo "PASS $3"; pass=$((pass+1)); else echo "FAIL $3 (missing '$2')"; fail=$((fail+1)); fi; }

# --- fake binaries -----------------------------------------------------------
# OK probe: announce ready (with its instance_id) then stay alive so teardown has
# something to kill.
mkdir -p "${WORK}/bin-ok"
cat > "${WORK}/bin-ok/sils_vecu_viss_probe" <<'EOF'
#!/usr/bin/env bash
echo "{\"run_id\":\"${SILS_RUN_ID}\",\"instance_id\":\"${SILS_VECU_INSTANCE_ID}\",\"event\":\"ready\",\"topic\":\"/VIN/Vehicle\"}"
exec sleep 30
EOF
cat > "${WORK}/bin-ok/sils_vecu_viss_verify" <<'EOF'
#!/usr/bin/env bash
echo "{\"run_id\":\"${SILS_RUN_ID}\",\"expected_instance_id\":\"${SILS_VECU_INSTANCE_ID}\",\"event\":\"endpoint_matched\",\"topic\":\"/VIN/Vehicle\",\"matched_count\":1}"
echo "{\"run_id\":\"${SILS_RUN_ID}\",\"expected_instance_id\":\"${SILS_VECU_INSTANCE_ID}\",\"event\":\"sample_received\",\"topic\":\"/VIN/Vehicle\",\"valid_envelope\":true,\"received_count\":1}"
echo "{\"run_id\":\"${SILS_RUN_ID}\",\"expected_instance_id\":\"${SILS_VECU_INSTANCE_ID}\",\"event\":\"verify_pass\",\"topic\":\"/VIN/Vehicle\",\"received_count\":1}"
exit 0
EOF
chmod +x "${WORK}/bin-ok/"*

# probe that never becomes ready (for ready_timeout)
mkdir -p "${WORK}/bin-noready"
cat > "${WORK}/bin-noready/sils_vecu_viss_probe" <<'EOF'
#!/usr/bin/env bash
exec sleep 30
EOF
cp "${WORK}/bin-ok/sils_vecu_viss_verify" "${WORK}/bin-noready/"
chmod +x "${WORK}/bin-noready/"*

# probe that exits immediately (for start failure)
mkdir -p "${WORK}/bin-crash"
cat > "${WORK}/bin-crash/sils_vecu_viss_probe" <<'EOF'
#!/usr/bin/env bash
exit 1
EOF
cp "${WORK}/bin-ok/sils_vecu_viss_verify" "${WORK}/bin-crash/"
chmod +x "${WORK}/bin-crash/"*

# --- 1) happy path: 3 members up together, verify, aggregate pass, teardown all
out="$(bash "${RUN}" fleet --replicas 3 --backend local --bin-dir "${WORK}/bin-ok" \
        --domain 90 --run-id fleet-ok 2>/dev/null)"; rc=$?
ck "${rc}" "0" "fleet(3) exit 0"
tr="${SILS_COORDINATOR_STATE_DIR}/fleet-ok.trace.jsonl"
uniq_ready="$(grep '"event":"ready"' "${tr}" | grep -o '"instance_id":"[^"]*"' | sort -u | wc -l | tr -d ' ')"
ck "${uniq_ready}" "3" "3 distinct members reached ready"
ckc "$(cat "${tr}")" '"event":"fleet_ready"' "fleet_ready emitted"
ckc "$(cat "${tr}")" '"event":"run_pass"' "run_pass emitted"
# teardown: no members/state file left, no fake probes lingering
[[ ! -f "${SILS_COORDINATOR_STATE_DIR}/fleet-ok.members" ]]; ck "$?" "0" "members file removed (teardown)"
[[ ! -f "${SILS_COORDINATOR_STATE_DIR}/fleet-ok.state" ]]; ck "$?" "0" "state file removed (teardown)"
sleep 0.3
leftover="$(pgrep -f "${WORK}/bin-ok/sils_vecu_viss_probe" | wc -l | tr -d ' ')"
ck "${leftover}" "0" "no member processes left running (teardown killed all)"

# --- 2) evaluate.py judges the fleet trace as one conformant PASSED run w/ 3 ids
verdict="$(python3 "${EVAL}" "${tr}" --quiet 2>/dev/null)"
ckc "${verdict}" '"fleet_verdict": "PASS"' "evaluate.py: fleet trace PASS"
ckc "${verdict}" '"verdict": "PASSED"' "evaluate.py: verdict PASSED"
python3 "${EVAL}" "${tr}" --quiet > "${WORK}/v.json" 2>/dev/null
ids="$(python3 -c "import json;print(len(json.load(open('${WORK}/v.json'))['runs'][0]['instance_ids']))")"
ck "${ids}" "3" "evaluate.py: 3 instance_ids in fleet run"

# --- 3) ready_timeout: members never ready -> run_fail exit 1, teardown all
out="$(bash "${RUN}" fleet --replicas 2 --backend local --bin-dir "${WORK}/bin-noready" \
        --domain 90 --run-id fleet-noready 2>/dev/null)"; rc=$?
ck "${rc}" "1" "fleet ready_timeout exit 1"
ckc "$(cat "${SILS_COORDINATOR_STATE_DIR}/fleet-noready.trace.jsonl")" '"event":"ready_timeout"' "ready_timeout emitted"
[[ ! -f "${SILS_COORDINATOR_STATE_DIR}/fleet-noready.members" ]]; ck "$?" "0" "teardown after ready_timeout"

# --- 4) start failure: a member exits immediately -> run_fail exit 1
out="$(bash "${RUN}" fleet --replicas 2 --backend local --bin-dir "${WORK}/bin-crash" \
        --domain 90 --run-id fleet-crash 2>/dev/null)"; rc=$?
ck "${rc}" "1" "fleet start-failure exit 1"

# --- 5) dry-run: enumerate members, launch nothing
out="$(bash "${RUN}" fleet --replicas 4 --instance-prefix wheel --domain 90 --dry-run 2>/dev/null)"; rc=$?
ck "${rc}" "0" "fleet dry-run exit 0"
ckc "${out}" '"members":4' "dry-run lists 4 members"
ckc "${out}" 'wheel-4' "dry-run uses instance-prefix"

echo
echo "$((pass))/$((pass+fail)) passed"
[[ "${fail}" -eq 0 ]] || exit 1
