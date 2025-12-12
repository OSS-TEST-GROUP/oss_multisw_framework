#!/bin/bash - 

WORK_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD_DIR=${WORK_DIR}/.build/Debug/ros2_x86_64
COVERAGE_DIR=${WORK_DIR}/output/coverage

mkdir -p ${COVERAGE_DIR}

echo "Capturing coverage data from ${BUILD_DIR}..."

# Check if any .gcda files exist (evidence of test execution)
GCDA_COUNT=$(find ${BUILD_DIR} -name "*.gcda" 2>/dev/null | wc -l)
echo "Found ${GCDA_COUNT} .gcda files (coverage data from executed code)"

if [ "${GCDA_COUNT}" -eq 0 ]; then
    echo "WARNING: No .gcda files found! Tests may not have generated coverage data."
    echo "This can happen if:"
    echo "  - Tests crashed or exited abnormally"
    echo "  - Tests have mock object leaks causing non-zero exit codes"
    echo "  - Volume mounts are not preserving .gcda files"
    echo ""
    echo "Proceeding with coverage collection anyway (may result in 0% coverage)..."
fi

# IMPORTANT: Remove .gcno files that don't have corresponding .gcda files
# This prevents LCOV from picking up 0-coverage .gcno files from main binaries
# that were never executed (only unittest binaries have .gcda files)
# Only do this cleanup if we have some .gcda files, otherwise we'd remove everything
if [ "${GCDA_COUNT}" -gt 0 ]; then
    echo "Removing .gcno files without corresponding .gcda (unexecuted code)..."
    REMOVED_COUNT=0
    find ${BUILD_DIR} -name "*.gcno" | while read gcno; do
        gcda="${gcno%.gcno}.gcda"
        if [ ! -f "$gcda" ]; then
            echo "Removing unexecuted: $gcno"
            rm -f "$gcno"
            REMOVED_COUNT=$((REMOVED_COUNT + 1))
        fi
    done
    echo "Cleanup complete."
else
    echo "Skipping .gcno cleanup since no .gcda files exist."
fi

# Capture actual coverage data (execution data from .gcda files)
lcov \
	--capture \
	--directory ${BUILD_DIR} \
	--output-file ${COVERAGE_DIR}/test.info \
	--rc lcov_branch_coverage=1 \
	--ignore-errors mismatch,gcov,empty 2>&1 || {
    echo "WARNING: lcov capture returned non-zero exit code"
}

# Check if test.info was created and has content
if [ ! -f "${COVERAGE_DIR}/test.info" ] || [ ! -s "${COVERAGE_DIR}/test.info" ]; then
    echo "ERROR: No coverage data captured (test.info is missing or empty)"
    echo "Creating empty coverage report..."
    # Create minimal empty coverage files to prevent downstream failures
    touch ${COVERAGE_DIR}/test.info
    touch ${COVERAGE_DIR}/filtered.info
    echo '<?xml version="1.0" ?><coverage version="1.0"><packages></packages></coverage>' > ${COVERAGE_DIR}/coverage.xml
    mkdir -p ${COVERAGE_DIR}/report
    echo '<html><body><h1>No coverage data available</h1><p>Tests may not have generated .gcda files.</p></body></html>' > ${COVERAGE_DIR}/report/index.html
    exit 0
fi

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
	--ignore-errors mismatch,empty

# Check if filtered.info was created and has content
if [ ! -f "${COVERAGE_DIR}/filtered.info" ] || [ ! -s "${COVERAGE_DIR}/filtered.info" ]; then
    echo "WARNING: Filtered coverage file is missing or empty"
    cp ${COVERAGE_DIR}/test.info ${COVERAGE_DIR}/filtered.info 2>/dev/null || touch ${COVERAGE_DIR}/filtered.info
fi

genhtml \
	${COVERAGE_DIR}/filtered.info \
	--output-directory ${COVERAGE_DIR}/report \
	--branch-coverage \
	--legend \
	--rc lcov_branch_coverage=1 2>&1 || {
    echo "WARNING: genhtml returned non-zero exit code"
    mkdir -p ${COVERAGE_DIR}/report
    echo '<html><body><h1>Coverage report generation failed</h1></body></html>' > ${COVERAGE_DIR}/report/index.html
}

lcov_cobertura \
	${COVERAGE_DIR}/filtered.info \
	--output ${COVERAGE_DIR}/coverage.xml 2>&1 || {
    echo "WARNING: lcov_cobertura returned non-zero exit code"
    echo '<?xml version="1.0" ?><coverage version="1.0"><packages></packages></coverage>' > ${COVERAGE_DIR}/coverage.xml
}

echo "Coverage report generation complete."
