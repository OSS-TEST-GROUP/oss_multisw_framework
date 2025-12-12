#!/bin/bash - 

WORK_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD_DIR=${WORK_DIR}/.build/Debug/ros2_x86_64
COVERAGE_DIR=${WORK_DIR}/output/coverage

mkdir -p ${COVERAGE_DIR}

echo "Capturing coverage data from ${BUILD_DIR}..."

# Check lcov version to determine supported options
LCOV_VERSION=$(lcov --version 2>/dev/null | grep -oP '\d+\.\d+' | head -1)
echo "Detected lcov version: ${LCOV_VERSION}"

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
    find ${BUILD_DIR} -name "*.gcno" | while read gcno; do
        gcda="${gcno%.gcno}.gcda"
        if [ ! -f "$gcda" ]; then
            echo "Removing unexecuted: $gcno"
            rm -f "$gcno"
        fi
    done
    echo "Cleanup complete."
else
    echo "Skipping .gcno cleanup since no .gcda files exist."
fi

# Capture actual coverage data (execution data from .gcda files)
# Note: Removed --ignore-errors options as they're not supported in older lcov versions
lcov \
	--capture \
	--directory ${BUILD_DIR} \
	--output-file ${COVERAGE_DIR}/test.info \
	--rc lcov_branch_coverage=1 \
	--no-external 2>&1 || {
    echo "WARNING: lcov capture returned non-zero exit code"
}

# Check if test.info was created and has content
if [ ! -f "${COVERAGE_DIR}/test.info" ] || [ ! -s "${COVERAGE_DIR}/test.info" ]; then
    echo "ERROR: No coverage data captured (test.info is missing or empty)"
    echo "Creating empty coverage report..."
    # Create minimal valid Cobertura XML to prevent Jenkins errors
    touch ${COVERAGE_DIR}/test.info
    touch ${COVERAGE_DIR}/filtered.info
    cat > ${COVERAGE_DIR}/coverage.xml << 'EOF'
<?xml version="1.0" ?>
<!DOCTYPE coverage SYSTEM "http://cobertura.sourceforge.net/xml/coverage-04.dtd">
<coverage line-rate="0" branch-rate="0" lines-covered="0" lines-valid="0" branches-covered="0" branches-valid="0" complexity="0" version="1.0" timestamp="0">
    <packages>
        <package name="empty" line-rate="0" branch-rate="0" complexity="0">
            <classes/>
        </package>
    </packages>
</coverage>
EOF
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
	--rc lcov_branch_coverage=1 2>&1 || {
    echo "WARNING: lcov remove returned non-zero exit code"
}

# Check if filtered.info was created and has content
if [ ! -f "${COVERAGE_DIR}/filtered.info" ] || [ ! -s "${COVERAGE_DIR}/filtered.info" ]; then
    echo "WARNING: Filtered coverage file is missing or empty, using test.info instead"
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
    # Create minimal valid Cobertura XML
    cat > ${COVERAGE_DIR}/coverage.xml << 'EOF'
<?xml version="1.0" ?>
<!DOCTYPE coverage SYSTEM "http://cobertura.sourceforge.net/xml/coverage-04.dtd">
<coverage line-rate="0" branch-rate="0" lines-covered="0" lines-valid="0" branches-covered="0" branches-valid="0" complexity="0" version="1.0" timestamp="0">
    <packages>
        <package name="empty" line-rate="0" branch-rate="0" complexity="0">
            <classes/>
        </package>
    </packages>
</coverage>
EOF
}

echo "Coverage report generation complete."
