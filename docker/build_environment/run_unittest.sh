#!/bin/bash

WORKDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

OUTPUTDIR=${WORKDIR}/output/x86_64/Debug
RESULTSDIR=${WORKDIR}/output/unittest-results

export LD_LIBRARY_PATH=${WORKDIR}/output/x86_64/lib
export MMF_CONFIG_PATH=${OUTPUTDIR}/core/bin/config
export DIAG_CONFIG_PATH=${OUTPUTDIR}/core/bin/unittest/diagnostics/config
export VEHICLE_CONFIG_PATH=${OUTPUTDIR}/core/bin/config

echo $LD_LIBRARY_PATH

# Core
find "${OUTPUTDIR}/core" -type f -executable -name "*Test" -o -name "*_test" | while read test; do
	test_name=$(basename "$test")
	echo "Running: ${test_name}"
	${test} --gtest_output=xml:"${RESULTSDIR}/core/${test_name}.xml" || true
done

# Extensio
