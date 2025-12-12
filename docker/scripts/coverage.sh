#!/bin/bash - 

WORK_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD_DIR=${WORK_DIR}/.build/Debug/ros2_x86_64
COVERAGE_DIR=${WORK_DIR}/output/coverage

mkdir -p ${COVERAGE_DIR}

echo "Capturing coverage data from ${BUILD_DIR}..."

# IMPORTANT: Remove .gcno files that don't have corresponding .gcda files
# This prevents LCOV from picking up 0-coverage .gcno files from main binaries
# that were never executed (only unittest binaries have .gcda files)
echo "Removing .gcno files without corresponding .gcda (unexecuted code)..."
find ${BUILD_DIR} -name "*.gcno" | while read gcno; do
    gcda="${gcno%.gcno}.gcda"
    if [ ! -f "$gcda" ]; then
        echo "Removing unexecuted: $gcno"
        rm -f "$gcno"
    fi
done

# Capture actual coverage data (execution data from .gcda files)
lcov \
	--capture \
	--directory ${BUILD_DIR} \
	--output-file ${COVERAGE_DIR}/test.info \
	--rc lcov_branch_coverage=1 \
	--ignore-errors mismatch,gcov 2>/dev/null || true

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
	'*Test.cpp' \
	'*/test/*' \
	'*/logger/*' \
	--output-file ${COVERAGE_DIR}/filtered.info \
	--rc lcov_branch_coverage=1 \
	--ignore-errors mismatch

genhtml \
	${COVERAGE_DIR}/filtered.info \
	--output-directory ${COVERAGE_DIR}/report \
	--branch-coverage \
	--legend \
	--rc lcov_branch_coverage=1

lcov_cobertura \
	${COVERAGE_DIR}/filtered.info \
	--output ${COVERAGE_DIR}/coverage.xml
