#!/bin/bash
#
# Integration test lane — counterpart to run_unittest.sh.
#
# Runs ONLY the runtime-dependent tests listed in integration-tests.regex
# (real gRPC server / ROS graph / multi-participant DDS / docker container).
# Intended for a self-hosted or stack CI job that provides those services,
# mirroring the HMI ci.yml integration / ros-e2e jobs. The plain unit lane
# (run_unittest.sh) excludes exactly this set.
#
WORK_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )/../.." && pwd )"

OUTPUT_DIR=${WORK_DIR}/output/x86_64/Debug
RESULT_DIR=${WORK_DIR}/output/integration-results

export LD_LIBRARY_PATH=${WORK_DIR}/output/x86_64/lib
export MMF_CONFIG_PATH=${OUTPUT_DIR}/core/bin/config
export DIAG_CONFIG_PATH=${OUTPUT_DIR}/core/bin/unittest/diagnostics/config
export DIAG_IDL_PATH=${OUTPUT_DIR}/core/bin/unittest/diagnostics/idl
export VEHICLE_CONFIG_PATH=${OUTPUT_DIR}/core/bin/config
export SAVED_REGISTRY_BACKUP=${OUTPUT_DIR}/extension/ros2/bin/unittest/adsw_manager/config

# Single source of truth shared with run_unittest.sh (strip comments/blanks).
INTEGRATION_RE=$(grep -vE '^\s*#|^\s*$' "$( dirname "${BASH_SOURCE[0]}" )/integration-tests.regex")
if [[ -z "${INTEGRATION_RE}" ]]; then
	echo "[run_integration] integration-tests.regex empty/missing — nothing to run" >&2
	exit 0
fi

mkdir -p ${RESULT_DIR}

for testdir in $(find .build/ -not -path "*/_deps/*" -name CTestTestfile.cmake -exec dirname {} \;)
do
	label=
	if [[ ${testdir} == */unittest ]]; then
		label=$(basename "$(dirname ${testdir})")
	else
		label=$(basename ${testdir})
	fi
	mkdir -p ${RESULT_DIR}/${label}
	# --tests-regex selects only the integration set; a dir with no match is a no-op.
	ctest \
		--test-dir ${testdir} \
		--label-add ${label} \
		--tests-regex "${INTEGRATION_RE}" \
		--timeout ${CTEST_TIMEOUT:-120} \
		--output-junit ${RESULT_DIR}/${label}/junit_results.xml \
		--output-on-failure \
		--no-compress-output || true
done
