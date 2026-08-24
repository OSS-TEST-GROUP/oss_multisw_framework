#!/bin/bash
# =============================================================================
# ci-gate.sh — 품질 게이트 판정 (lifecycle_mgr / ros2 adsw / general adsw)
#
# 왜 필요한가:
#   Jenkinsfile 의 clang-tidy qualityGates 는 threshold 1000 (사실상 게이트 없음),
#   Jenkinsfile.unittest 의 성공률 기준은 50%, 커버리지는 recordCoverage 로
#   기록만 하고 하한이 없다. 그래서 P1 을 0 으로 만들어 놔도 다음 커밋에 조용히
#   되돌아간다. 이 스크립트가 그 판정을 한다.
#
#   판정 로직을 Groovy 가 아니라 셸에 두는 이유: 로컬에서 그대로 돌려볼 수 있다.
#   개발자가 커밋 전에 `./scripts/ci-gate.sh` 로 자기 변경이 게이트를 통과하는지
#   확인할 수 있고, Jenkins 는 같은 스크립트를 호출만 한다.
#
# 전제: 아래 세 가지가 이미 생성돼 있어야 한다.
#         ./scripts/analyze-modules.sh     → output/reports/module-analysis/
#         (run_unittest.sh)                → output/unittest-results/
#         ./scripts/coverage-modules.sh    → output/coverage/modules/
#       없으면 무엇을 먼저 돌려야 하는지 알려주고 종료한다.
#
# 실행 위치: 아무 데서나. python3 만 있으면 되고 컨테이너가 필요 없다
#           (측정 산출물을 읽기만 한다).
#
# 사용법:
#   ./scripts/ci-gate.sh                 # 전체 판정
#   ./scripts/ci-gate.sh --warn-only     # 미달이어도 exit 0 (도입 초기용)
#   ./scripts/ci-gate.sh --skip-build    # 빌드 무결성 검사 생략 (.build 없을 때)
#
# 기준값은 아래 상수 또는 동명의 환경변수로 조정한다. 예:
#   GATE_LINE_MIN_general=70 ./scripts/ci-gate.sh
#
# 종료코드: 0 = 전부 통과, 1 = 하나 이상 미달, 2 = 입력 산출물 없음
# =============================================================================
set -u
set -o pipefail

WORKDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )/.." && pwd )"

ANALYSIS_DIR="${WORKDIR}/output/reports/module-analysis"
COVERAGE_DIR="${WORKDIR}/output/coverage/modules"
JUNIT_DIR="${WORKDIR}/output/unittest-results"
BUILD_DIR="${BUILD_DIR:-${WORKDIR}/.build/Debug/ros2_x86_64}"

# ── 기준값 ───────────────────────────────────────────────────────────────────
#
# P1 은 0 이다. 지금 세 모듈 모두 0 건이므로 하나만 새로 생겨도 잡힌다.
# 이게 이 게이트에서 가장 값어치 있는 항목이다 — 나머지는 서서히 나빠지지만
# P1 은 "실제 결함 가능성" 이라 한 건도 들이지 않는 편이 낫다.
GATE_P1_MAX="${GATE_P1_MAX:-0}"

# 라인 커버리지 하한. 실측(84.04 / 83.39 / 78.70)에서 3~4%p 여유를 뒀다.
# 여유를 두는 이유: 리팩터링으로 분모가 줄면 %가 요동칠 수 있어서 그때마다
# 빌드가 붉어지면 게이트를 꺼버리게 된다.
GATE_LINE_MIN_lifecycle="${GATE_LINE_MIN_lifecycle:-80}"
GATE_LINE_MIN_adsw="${GATE_LINE_MIN_adsw:-80}"
GATE_LINE_MIN_general="${GATE_LINE_MIN_general:-75}"

# 테스트 성공률(skip 제외한 실행분 기준). Jenkins 가 쓰는 것과 같은 정의다.
GATE_SUCCESS_RATE_MIN="${GATE_SUCCESS_RATE_MIN:-95}"

# 브랜치 커버리지는 게이트로 쓰지 않는다.
# 분모의 절반 이상이 LOG_* 매크로에서 나오고(가이드 4-A 참조) 그 대부분은
# libstdc++ 내부라 어떤 테스트로도 실행되지 않는다. 굳이 걸려면
# SUMMARY.md 의 "LOG_ 제외" 열을 써야 하는데, 아직 근사값이라 기준으로 삼기 이르다.

WARN_ONLY=FALSE
SKIP_BUILD=FALSE
while [[ $# -gt 0 ]]; do
    case "$1" in
        --warn-only) WARN_ONLY=TRUE; shift ;;
        --skip-build) SKIP_BUILD=TRUE; shift ;;
        -h|--help)   sed -n '2,38p' "${BASH_SOURCE[0]}"; exit 0 ;;
        *) echo "unknown option: $1" >&2; exit 2 ;;
    esac
done

command -v python3 >/dev/null 2>&1 || { echo "[gate] ERROR: python3 없음" >&2; exit 2; }

FAILED=0
pass() { printf '  \033[32mPASS\033[0m  %s\n' "$1"; }
fail() { printf '  \033[31mFAIL\033[0m  %s\n' "$1"; FAILED=$((FAILED+1)); }
note() { printf '        %s\n' "$1"; }
head2() { printf '\n\033[1m%s\033[0m\n' "$1"; }

missing() {
    echo "[gate] ERROR: $1 이(가) 없다." >&2
    echo "  → $2 를 먼저 실행할 것." >&2
    exit 2
}
[[ -d ${ANALYSIS_DIR} ]] || missing "${ANALYSIS_DIR#"${WORKDIR}"/}" "./scripts/analyze-modules.sh"
[[ -d ${COVERAGE_DIR} ]] || missing "${COVERAGE_DIR#"${WORKDIR}"/}" "./scripts/coverage-modules.sh"
[[ -d ${JUNIT_DIR}    ]] || missing "${JUNIT_DIR#"${WORKDIR}"/}"    "run_unittest.sh (가이드 1-2 참조)"

# 모듈 정의: <분석/커버리지 이름>|<소스 경로>|<junit 레이블>
MODULES=(
    "lifecycle|service/core/lifecycle_mgr|lifecycle_mgr"
    "adsw|service/extension/ros2/adsw_manager|ros2_adsw_manager"
    "general|service/extension/general/adsw_manager|general_adsw_manager"
)

# ── 1. 측정 유효성 ───────────────────────────────────────────────────────────
#
# fatal/include-error 가 0 이 아니면 clang-tidy 가 파싱하지 못한 파일이 있다는
# 뜻이고, 그 상태의 건수는 애초에 기준으로 쓸 수 없다. 다른 판정보다 먼저 본다.
head2 "측정 유효성"
for entry in "${MODULES[@]}"; do
    IFS='|' read -r name _ _ <<< "${entry}"
    stats="${ANALYSIS_DIR}/clang-tidy-${name}.stats"
    if [[ ! -f ${stats} ]]; then
        fail "${name}: clang-tidy 결과 없음 (${stats#"${WORKDIR}"/})"
        continue
    fi
    fatal="$(sed -nE 's/^fatal_or_missing_include=([0-9]+)$/\1/p' "${stats}")"
    if [[ ${fatal:-1} -eq 0 ]]; then
        pass "${name}: fatal/include-error 0"
    else
        fail "${name}: fatal/include-error ${fatal} — 분석이 정상 파싱되지 않았다. 이 상태의 건수는 기준으로 쓸 수 없다"
    fi
done

# ── 2. P1 정적분석 ───────────────────────────────────────────────────────────
head2 "P1 정적분석 (bugprone-* / clang-analyzer-* / cppcheck error)  허용 ${GATE_P1_MAX}건"
for entry in "${MODULES[@]}"; do
    IFS='|' read -r name _ _ <<< "${entry}"
    by_check="${ANALYSIS_DIR}/clang-tidy-${name}.by-check.txt"
    xml="${ANALYSIS_DIR}/cppcheck-${name}.xml"

    tidy_p1=0
    if [[ -f ${by_check} ]]; then
        tidy_p1="$(awk '$2 ~ /^(bugprone-|clang-analyzer-)/ { s += $1 } END { print s+0 }' "${by_check}")"
    fi

    cppcheck_err=0
    if [[ -f ${xml} ]]; then
        cppcheck_err="$(python3 -c "
import sys, xml.etree.ElementTree as ET
try:
    r = ET.parse(sys.argv[1]).getroot()
except Exception:
    print(-1); sys.exit()
print(sum(1 for e in r.iter('error') if e.get('severity') == 'error'))
" "${xml}")"
    fi
    if [[ ${cppcheck_err} -lt 0 ]]; then
        fail "${name}: cppcheck XML 파싱 실패"
        continue
    fi

    total=$((tidy_p1 + cppcheck_err))
    detail="clang-tidy ${tidy_p1} + cppcheck error ${cppcheck_err}"
    if [[ ${total} -le ${GATE_P1_MAX} ]]; then
        pass "${name}: P1 ${total}건  (${detail})"
    else
        fail "${name}: P1 ${total}건 > ${GATE_P1_MAX}  (${detail})"
        note "무엇이 늘었는지: ${by_check#"${WORKDIR}"/}"
    fi
done

# ── 3. 라인 커버리지 ─────────────────────────────────────────────────────────
head2 "라인 커버리지"
for entry in "${MODULES[@]}"; do
    IFS='|' read -r name relpath _ <<< "${entry}"
    totals="${COVERAGE_DIR}/${name}/totals.txt"
    if [[ ! -f ${totals} ]]; then
        fail "${name}: 커버리지 결과 없음 (${totals#"${WORKDIR}"/})"
        continue
    fi
    var="GATE_LINE_MIN_${name}"
    floor="${!var}"
    pct="$(sed -nE 's/^line_pct=([0-9.]+)$/\1/p' "${totals}")"
    zero="$(sed -nE 's/^zero_coverage_files=([0-9]+)$/\1/p' "${totals}")"

    if [[ -z ${pct} ]]; then
        fail "${name}: line_pct 를 읽을 수 없다"
        continue
    fi
    if python3 -c "import sys; sys.exit(0 if float('${pct}') >= float('${floor}') else 1)"; then
        pass "${name}: 라인 ${pct}% ≥ ${floor}%"
    else
        fail "${name}: 라인 ${pct}% < ${floor}%"
        note "낮은 파일부터: ${COVERAGE_DIR#"${WORKDIR}"/}/${name}/by-file.txt"
    fi

    # 0% 파일은 별도 항목이다. 전체 %가 기준을 넘어도 통째로 안 도는 파일이
    # 새로 생겼다면 그건 테스트가 링크에서 빠졌다는 신호일 때가 많다
    # (ADSWServiceRegistryTest 가 정확히 그랬다).
    if [[ ${zero:-0} -eq 0 ]]; then
        pass "${name}: 라인 0% 파일 없음"
    else
        fail "${name}: 라인 0% 파일 ${zero}개"
        note "목록: ${COVERAGE_DIR#"${WORKDIR}"/}/${name}/zero-coverage.txt"
    fi
done

# ── 4. 테스트 성공률 ─────────────────────────────────────────────────────────
#
# skip 을 분모에서 뺀 "실행분 기준" 이다. Jenkinsfile.unittest 의 successRateRun
# 과 같은 정의를 쓴다 — 두 곳이 다른 숫자를 보면 판정이 어긋난다.
head2 "테스트 (실행분 기준 성공률 ≥ ${GATE_SUCCESS_RATE_MIN}%)"
for entry in "${MODULES[@]}"; do
    IFS='|' read -r name _ label <<< "${entry}"
    junit="${JUNIT_DIR}/${label}/junit_results.xml"
    if [[ ! -f ${junit} ]]; then
        fail "${label}: junit 결과 없음 (${junit#"${WORKDIR}"/})"
        continue
    fi
    read -r total failed skipped <<< "$(python3 -c "
import sys, xml.etree.ElementTree as ET
r = ET.parse(sys.argv[1]).getroot()
tc = r.findall('.//testcase')
f = sum(1 for c in tc if c.find('failure') is not None)
s = sum(1 for c in tc if c.find('skipped') is not None)
print(len(tc), f, s)
" "${junit}")"
    run=$((total - skipped))
    if [[ ${run} -le 0 ]]; then
        fail "${label}: 실행된 테스트가 없다 (총 ${total}, skip ${skipped})"
        continue
    fi
    passed=$((run - failed))
    rate="$(python3 -c "print(f'{${passed}*100/${run}:.2f}')")"
    if python3 -c "import sys; sys.exit(0 if float('${rate}') >= float('${GATE_SUCCESS_RATE_MIN}') else 1)"; then
        pass "${label}: ${rate}%  (총 ${total} / 실행 ${run} / 실패 ${failed} / skip ${skipped})"
    else
        fail "${label}: ${rate}% < ${GATE_SUCCESS_RATE_MIN}%  (실패 ${failed}/${run})"
    fi
done

# ── 5. 빌드 무결성 ───────────────────────────────────────────────────────────
#
# build.sh 의 `make -k … || true` 가 종료코드를 지운다. 그래서 테스트 타겟 하나가
# 링크에 실패해도 빌드는 성공으로 끝나고, ctest 는 없는 바이너리 자리에
# <name>_NOT_BUILT 플레이스홀더를 넣는다. ADSWServiceRegistryTest 가 정확히 그
# 상태였고 434줄이 0% 로 방치됐다.
#
# 불변식은 "gtest_discover_tests 로 등록한 타겟마다 바이너리가 있어야 한다" 이다.
# 반대(바이너리는 있는데 등록은 안 된 것)는 정상이다 — gtest_discover_tests 를
# 일부러 주석 처리해 둔 타겟이 실제로 있다.
if [[ ${SKIP_BUILD} == TRUE ]]; then
    head2 "빌드 무결성 — 건너뜀 (--skip-build)"
elif [[ ! -d ${BUILD_DIR} ]]; then
    head2 "빌드 무결성"
    fail "빌드 트리 없음: ${BUILD_DIR#"${WORKDIR}"/}  (BUILD_DIR=... 로 지정하거나 --skip-build)"
else
    head2 "빌드 무결성 (등록된 테스트 타겟마다 바이너리가 있는가)"
    for entry in "${MODULES[@]}"; do
        IFS='|' read -r name relpath _ <<< "${entry}"
        cml="${WORKDIR}/${relpath}/unittest/CMakeLists.txt"
        bin="${BUILD_DIR}/${relpath}/unittest/out/bin"

        if [[ ! -f ${cml} ]]; then
            fail "${name}: unittest/CMakeLists.txt 없음"
            continue
        fi
        if [[ ! -d ${bin} ]]; then
            fail "${name}: 테스트 바이너리 디렉토리 없음 (${bin#"${WORKDIR}"/}) — 빌드하지 않았거나 통째로 실패했다"
            continue
        fi

        # 주석 줄을 먼저 걷어낸다. `#gtest_discover_tests ( X )` 로 일부러 꺼둔
        # 타겟을 세면 없는 바이너리를 찾게 된다.
        declared="$(grep -vE '^\s*#' "${cml}" \
                    | grep -oP 'gtest_discover_tests\s*\(\s*\K\w+' | sort -u)"
        [[ -n ${declared} ]] || { pass "${name}: 등록된 테스트 타겟 없음"; continue; }

        missing_list=""
        count=0
        for t in ${declared}; do
            count=$((count+1))
            [[ -x ${bin}/${t} ]] || missing_list+=" ${t}"
        done

        if [[ -z ${missing_list} ]]; then
            pass "${name}: 등록 ${count}개 전부 생성됨"
        else
            fail "${name}: 등록됐으나 바이너리가 없는 타겟 —${missing_list}"
            note "링크 실패가 build.sh 의 'make -k … || true' 에 삼켜졌을 가능성이 높다"
        fi
    done
fi

# ── 결과 ─────────────────────────────────────────────────────────────────────
echo
if [[ ${FAILED} -eq 0 ]]; then
    printf '\033[32m[gate] 전부 통과\033[0m\n'
    exit 0
fi
printf '\033[31m[gate] %d개 항목 미달\033[0m\n' "${FAILED}"
if [[ ${WARN_ONLY} == TRUE ]]; then
    echo "[gate] --warn-only 라 exit 0 으로 끝낸다."
    exit 0
fi
exit 1
