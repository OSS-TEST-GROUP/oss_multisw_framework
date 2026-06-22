#!/bin/bash
#
WORK_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

OUTPUT_DIR=${WORK_DIR}/output/x86_64/Debug
RESULT_DIR=${WORK_DIR}/output/unittest-results

export LD_LIBRARY_PATH=${WORK_DIR}/output/x86_64/lib
export MMF_CONFIG_PATH=${OUTPUT_DIR}/core/bin/config
export DIAG_CONFIG_PATH=${OUTPUT_DIR}/core/bin/unittest/diagnostics/config
export DIAG_IDL_PATH=${OUTPUT_DIR}/core/bin/unittest/diagnostics/idl
export VEHICLE_CONFIG_PATH=${OUTPUT_DIR}/core/bin/config
export SAVED_REGISTRY_BACKUP=${OUTPUT_DIR}/extension/ros2/bin/unittest/adsw_manager/config

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
	ctest \
		--test-dir ${testdir} \
		--label-add ${label} \
		--timeout ${CTEST_TIMEOUT:-60} \
		--output-junit ${RESULT_DIR}/${label}/junit_results.xml \
		--output-on-failure \
		--no-compress-output || true
done

