#!/bin/bash

WORK_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

OUTPUT_DIR=${WORK_DIR}/output/x86_64/Debug
RESULTS_DIR=${WORK_DIR}/output/unittest-results

export LD_LIBRARY_PATH=${WORK_DIR}/output/x86_64/lib
export MMF_CONFIG_PATH=${OUTPUT_DIR}/core/bin/config
export DIAG_CONFIG_PATH=${OUTPUT_DIR}/core/bin/unittest/diagnostics/config
export VEHICLE_CONFIG_PATH=${OUTPUT_DIR}/core/bin/config

find ${OUTPUT_DIR} -type f -executable \( -name "*Test" -o -name "*_test" -o -name "*_unittest" \) | \
while read -r test; do
	type=""
	if [[ "$test" == */core/* ]]; then
		type="core"
	elif [[ "$test" == */extension/* ]]; then
		type="extension"
	fi
	test_name=$(basename "$test")
	echo "Running: ${test_name}"
	${test} --gtest_output=xml:"${RESULTS_DIR}/${type}/${test_name}.xml" || true
done
