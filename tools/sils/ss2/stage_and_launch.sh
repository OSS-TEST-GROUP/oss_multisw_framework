#!/usr/bin/env bash
#
# stage_and_launch.sh -- copy the scenario somewhere WRITABLE, then run scenario_test_runner.
#
# WHY THIS EXISTS
#
# openscenario_preprocessor does not read the scenario and leave it alone. Look at
# Preprocessor::preprocessScenario (openscenario_preprocessor.cpp):
#
#     auto raw_scenario_directory = scenario.path.parent_path() / "raw";
#     std::filesystem::create_directories(raw_scenario_directory);
#     std::filesystem::rename(scenario.path, raw_scenario_directory / ...);
#     script->script.save_file(preprocessed_scenario_path.c_str());
#
# It creates a sibling `raw/` directory, MOVES the original into it, and writes the
# parameter-substituted scenario back at the original path. All three need write
# permission on the directory the scenario lives in.
#
# `ss2-scenario` mounts scenarios/ss2 as :ro, on purpose -- those are source assets and
# the simulator has no business rewriting them. So every one of those calls throws EROFS.
#
# AND THE FAILURE IS INVISIBLE. The throw is caught by the Load service handler, which
# turns it into `response->has_succeeded = false` with a message -- but
# scenario_test_runner.py:256 only checks that a response ARRIVED:
#
#     if future.result() is not None:
#         return True          # <- has_succeeded and message are never read
#
# So Load "succeeds", the derivative queue is empty, `while derivative_remained(...)` is
# false on the first call, run_preprocessed_scenarios([]) prints nothing, and the runner
# shuts down ~6 s in with no error anywhere. See
# docs/sils/evidence/ss2-clock-survey-2026-08-11.md §4.
#
# WHAT THIS DOES
#
# Copies the scenario into ${SILS_SS2_STAGING} (default /out/staging, which is the
# bind-mounted runs/ss2-scenario/staging on the host) and points the launch at the copy.
# The mutation then lands on a throwaway copy, the :ro mount stays :ro, and both the raw
# and the preprocessed scenario survive on the host as run evidence.
#
# /out/staging is deliberately NOT under the runner's output_directory (/out/
# scenario_test_runner): ScenarioTestRunner.__init__ wipes that tree on every start.
#
# Usage: exactly scenario_test_runner.launch.py's arguments, passed straight through.
#   stage_and_launch.sh scenario:=/scenarios/x.xosc launch_rviz:=false ...
#
# Exit: whatever ros2 launch exits with; 2 on a staging error.

set -uo pipefail

EXIT_USAGE=2
STAGING="${SILS_SS2_STAGING:-/out/staging}"

die() { echo "stage_and_launch: $*" >&2; exit "${EXIT_USAGE}"; }

args=()
staged=0

for arg in "$@"; do
    case "${arg}" in
        scenario:=*)
            src="${arg#scenario:=}"

            # An empty or /dev/null scenario is the runner's own "nothing to do" path.
            # Pass it through rather than failing on a file that was never meant to exist.
            if [[ -z "${src}" || "${src}" == "/dev/null" ]]; then
                args+=("${arg}")
                continue
            fi

            [[ -f "${src}" ]] || die "scenario not found in this container: ${src}
  scenarios/ss2 is mounted at /scenarios; an absolute in-image path also works."

            mkdir -p "${STAGING}" || die "cannot create staging dir ${STAGING}.
  Is runs/ss2-scenario bind-mounted read-write? Override with SILS_SS2_STAGING."

            # A stale raw/ from a previous run makes the preprocessor's rename land on top
            # of an older copy of the same name. Harmless today, but it means "the file in
            # raw/" stops being "the file this run was given" -- so clear it.
            rm -rf "${STAGING:?}/raw"

            dst="${STAGING}/$(basename "${src}")"
            cp -f "${src}" "${dst}" || die "cannot copy ${src} -> ${dst}"

            echo "stage_and_launch: ${src}"
            echo "stage_and_launch:   -> ${dst}  (writable; preprocessor rewrites in place)"

            args+=("scenario:=${dst}")
            staged=1
            ;;
        *)
            args+=("${arg}")
            ;;
    esac
done

if [[ "${staged}" -eq 0 ]]; then
    echo "stage_and_launch: WARNING no scenario:= argument seen; launching unchanged." >&2
fi

exec ros2 launch scenario_test_runner scenario_test_runner.launch.py "${args[@]}"
