#!/bin/bash - 

WORK_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD_DIR=${WORK_DIR}/.build/Debug/ros2_x86_64
COVERAGE_DIR=${WORK_DIR}/output/coverage

mkdir -p ${COVERAGE_DIR}

lcov \
	--capture \
	--directory ${BUILD_DIR} \
	--output-file ${COVERAGE_DIR}/test.info \
	--rc lcov_branch_coverage=1

lcov \
	--remove ${COVERAGE_DIR}/test.info \
	'/usr/*' \
	'/opt/*' \
	'*/rapidjson/*' \
	'*/nlohmann/*' \
	'*/valijson/*' \
	'*/googletest-src/*' \
	'*/generated/*' \
	'*/paho-mqtt-cpp-src/*' \
	'*/idl/*' \
	'*/.build/*' \
	'*/unittest/*' \
	'*/logger/*' \
	--output-file ${COVERAGE_DIR}/filtered.info \
	--rc lcov_branch_coverage=1

genhtml \
	${COVERAGE_DIR}/filtered.info \
	--output-directory ${COVERAGE_DIR}/report \
	--branch-coverage \
	--legend \
	--rc lcov_branch_coverage=1 \
	--rc genhtml_branch_hi_limit=50 \
  	--rc genhtml_branch_med_limit=30

lcov_cobertura \
	${COVERAGE_DIR}/filtered.info \
	--output ${COVERAGE_DIR}/coverage.xml
