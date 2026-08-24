#!/bin/bash
# =============================================================================
# fix-modules.sh — clang-tidy 자동수정 (모듈 스코프)
#
# analyze-modules.sh 가 센 진단 중 clang-tidy 가 직접 고칠 수 있는 것들을
# 적용한다. 같은 모듈 목록·같은 compile_commands.json 을 쓰므로 분석과
# 수정 대상이 정확히 일치한다.
#
# 실행 위치: 반드시 빌더 컨테이너 안.
#   docker run --rm -u 1000:1000 \
#       -w /home/ossuser/workspace \
#       -v ${PWD}:/home/ossuser/workspace \
#       -v ${HOME}/obigo-sdk:/opt \
#       registry.obigo.com/oss/oss-mmf-builder:latest \
#       ./scripts/fix-modules.sh --batch safe
#
# 배치를 나눈 이유 ─────────────────────────────────────────────────────────
#   safe   : 시그니처를 바꾸지 않는 수정. override 추가, 중괄호, 불린식 단순화
#            같은 것들이라 호출부·파생클래스에 영향이 없다.
#
#   api    : 파라미터/반환 타입을 바꾸는 수정 (값전달 → const&).
#            이 코드베이스의 유닛테스트 mock 은 gmock 파생이 아니라 **링크 심**
#            이다. 즉 unittest/mock/*.hpp 가 실제 클래스의 메서드를 그대로
#            정의해서 링커가 그걸 집도록 만든다. 그런데 mock 헤더는 프로덕션
#            TU 의 include 그래프에 없어서 clang-tidy 가 같이 고쳐주지 못한다.
#            → api 배치를 돌리면 시그니처가 어긋나 **undefined reference 로
#              테스트 링크가 깨진다.** 반드시 mock 쪽을 손으로 맞춰야 한다.
#
#   두 배치를 한 번에 돌리지 않는 것은 그래서다. safe 를 먼저 넣고 빌드·테스트로
#   기준선을 확인한 뒤 api 를 따로 다뤄야 원인을 가릴 수 있다.
#
# 아래 검사들을 일부러 뺐다 ────────────────────────────────────────────────
#   readability-convert-member-functions-to-static  (32건)
#   readability-make-member-function-const          (15건)
#     → 메서드에 static/const 를 붙이면 파생 클래스의 override 와 위 링크 심의
#       정의가 어긋난다. api 배치보다도 파급이 커서 자동수정 대상에서 제외했다.
#
#   readability-function-cognitive-complexity       (58건)
#   cppcoreguidelines-macro-usage                   (18건)
#     → 자동수정이 없다. 함수를 쪼개거나 매크로를 없애는 설계 작업이다.
# =============================================================================
set -u
set -o pipefail

WORKDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )/.." && pwd )"
BUILD_ROOT="${WORKDIR}/.build/analyze"
REPORT_DIR="${WORKDIR}/output/reports/module-analysis"

BATCH="safe"
ONLY=""
DRY_RUN=FALSE

while [[ $# -gt 0 ]]; do
    case "$1" in
        --batch)   BATCH="$2"; shift 2 ;;
        --only)    ONLY="$2";  shift 2 ;;
        --dry-run) DRY_RUN=TRUE; shift ;;
        -h|--help) sed -n '2,50p' "${BASH_SOURCE[0]}"; exit 0 ;;
        *) echo "알 수 없는 인자: $1" >&2; exit 2 ;;
    esac
done

info() { echo "[fix] $*"; }
die()  { echo "[fix][오류] $*" >&2; exit 1; }

# 시그니처를 건드리지 않는 수정만.
CHECKS_SAFE="-*,\
modernize-use-override,\
cppcoreguidelines-explicit-virtual-functions,\
readability-braces-around-statements,\
readability-simplify-boolean-expr,\
readability-redundant-member-init,\
cppcoreguidelines-prefer-member-initializer,\
readability-container-contains,\
readability-container-size-empty,\
readability-else-after-return,\
readability-redundant-string-init,\
readability-qualified-auto,\
readability-redundant-control-flow,\
readability-duplicate-include,\
readability-static-definition-in-anonymous-namespace,\
modernize-use-nullptr,\
modernize-deprecated-headers"

# 파라미터/반환 타입을 바꾼다. mock 링크 심을 손으로 맞춰야 한다.
#
# readability-avoid-const-params-in-decls 를 여기서 뺐다.
#
# 그 검사는 선언의 무의미한 const 를 떼고(`const std::string ns` → `std::string ns`),
# performance-unnecessary-value-param 은 같은 자리에 & 를 붙인다. 두 수정이 같은
# 토큰 범위를 건드리는데 clang-apply-replacements 는 이걸 충돌로 보지 않아서
# 둘 다 적용되고, 결과가 깨진다. 실제로 두 곳이 이렇게 망가졌다:
#
#   ROS2Participant::SetGuessService(std::string)
#     → (const const std::string&& service)      ← const 두 번 + rvalue 참조
#   ROSServiceDiscovery::guessService(const std::string ns)
#     → 선언은 (std::string& ns), 정의는 (const std::string& ns)
#         ← const 가 사라지고 선언·정의가 어긋남
#
# 컴파일이 깨져서 드러났지만, 조용히 const 만 빠지는 조합도 가능하다. 그래서
# 같이 돌리지 않는다. 이 검사가 필요하면 api 배치를 먼저 넣고 빌드·테스트로
# 확인한 뒤 별도로 돌릴 것.
CHECKS_API="-*,\
performance-unnecessary-value-param,\
performance-unnecessary-copy-initialization,\
readability-const-return-type"

# 수정한 자리를 clang-format 으로 정리하기 위한 스타일.
#
# -format 없이 돌리면 중괄호 삽입 결과가 이렇게 나온다:
#     if (state_manager_) { state_manager_->OnStart(msg);
#     }
# 닫는 괄호가 0열에 붙어서 원본보다 읽기 나쁘다. 그래서 -format 이 필요한데,
# 이 저장소에는 .clang-format 이 없다. 저장소 전체에 포맷 규칙을 새로 들이는 건
# 이 작업의 범위가 아니므로(넣는 순간 무관한 파일까지 재포맷 대상이 된다)
# 인라인 스타일을 준다. run-clang-tidy 의 -format 은 **clang-tidy 가 실제로
# 고친 범위만** 재포맷하므로 파급은 그 문장들로 한정된다.
#
# ColumnLimit: 0 은 "줄 길이를 이유로 줄바꿈하지 말라" 는 뜻이다. 이게 없으면
# 손대지 않아도 될 긴 줄까지 접혀서 diff 가 불어난다.
FMT_STYLE='{BasedOnStyle: LLVM, IndentWidth: 4, ColumnLimit: 0, PointerAlignment: Left}'

case "${BATCH}" in
    safe) CHECKS="${CHECKS_SAFE}" ;;
    api)  CHECKS="${CHECKS_API}"  ;;
    *)    die "--batch 는 safe 또는 api" ;;
esac

# analyze-modules.sh 와 동일한 모듈 표.
MODULES=(
    "lifecycle|FALSE|ROS2|service/core/lifecycle_mgr"
    "adsw|TRUE|ROS2|service/extension/ros2/adsw_manager"
    "general|TRUE|GENERAL|service/extension/general/adsw_manager"
)

command -v run-clang-tidy >/dev/null || die "run-clang-tidy 없음 — 빌더 컨테이너 안에서 실행해야 한다"
command -v clang-apply-replacements >/dev/null \
    || die "clang-apply-replacements 없음 — run-clang-tidy -fix 가 동작하지 않는다"

NPROC="$(nproc)"
total_touched=0

for entry in "${MODULES[@]}"; do
    IFS='|' read -r name _ext _plat moddir <<< "${entry}"
    [[ -n ${ONLY} && ${ONLY} != "${name}" ]] && continue

    bdir="${BUILD_ROOT}/${name}"
    filelist="${REPORT_DIR}/files-${name}.txt"

    # -header-filter 는 절대경로로 줘야 한다.
    #
    # clang-tidy 는 이 정규식을 진단이 난 파일의 **절대경로**에 맞춘다. 표의
    # 값(service/core/lifecycle_mgr)을 그대로 써서 `^service/...` 로 앵커하면
    # /home/ossuser/workspace/service/... 와 매칭되지 않아 모든 헤더 진단이
    # "non-user code" 로 억제된다. 억제된 진단에는 fix 가 적용되지 않으므로
    # **헤더는 한 줄도 고쳐지지 않는다** (실측: 첫 실행에서 .cpp 69개만 바뀌고
    # override 관련 182건이 그대로 남았다. 같은 조건 실험에서 헤더 진단이
    # 상대 앵커 2건 vs 절대 앵커 297건).
    # analyze-modules.sh 는 moddir 을 WORKDIR 기준 절대경로로 만들어 쓴다.
    moddir_abs="${WORKDIR}/${moddir}"
    [[ -d ${moddir_abs} ]] || die "[${name}] 모듈 경로 없음: ${moddir}"

    [[ -f ${bdir}/compile_commands.json ]] \
        || die "[${name}] compile_commands.json 없음. analyze-modules.sh 를 먼저 돌려야 한다"
    [[ -f ${filelist} ]] \
        || die "[${name}] files-${name}.txt 없음. analyze-modules.sh 를 먼저 돌려야 한다"

    # analyze 와 같게 TU(*.cpp/*.cc)만 넘긴다. 헤더는 -header-filter 로 잡힌다.
    mapfile -t tus < <(grep -E '\.(cpp|cc)$' "${filelist}")
    if [[ ${#tus[@]} -eq 0 ]]; then
        info "[${name}] 대상 TU 없음 — 건너뜀"; continue
    fi

    info "═══ ${name} (${moddir}) — batch=${BATCH}, TU ${#tus[@]}개 ═══"

    if [[ ${DRY_RUN} == TRUE ]]; then
        info "[${name}] --dry-run: 적용할 검사"
        echo "${CHECKS}" | tr ',' '\n' | sed 's/^/    /'
        continue
    fi

    before="$(cd "${WORKDIR}" && git diff --numstat | wc -l)"

    # -fix 는 내부적으로 -export-fixes 후 clang-apply-replacements 로 적용한다.
    # 여러 TU 가 같은 헤더를 고칠 때의 중복은 거기서 정리된다.
    run-clang-tidy \
        -p "${bdir}" \
        -j "${NPROC}" \
        -quiet \
        -fix \
        -format -style="${FMT_STYLE}" \
        -checks="${CHECKS}" \
        -header-filter="^${moddir_abs}/.*" \
        "${tus[@]}" > "${REPORT_DIR}/fix-${name}-${BATCH}.log" 2>&1
    rc=$?

    # 인자를 잘못 주면 run-clang-tidy 는 usage 만 찍고 곧바로 끝난다. 그러면
    # "변경 0건" 이 되는데, 이건 "고칠 게 없었다" 와 구분이 안 된다. 처음에
    # -format=false 로 이 함정에 빠졌으므로 명시적으로 걸러낸다.
    if head -1 "${REPORT_DIR}/fix-${name}-${BATCH}.log" | grep -q '^usage: run-clang-tidy'; then
        die "[${name}] run-clang-tidy 인자 오류 — 로그: output/reports/module-analysis/fix-${name}-${BATCH}.log"
    fi

    after="$(cd "${WORKDIR}" && git diff --numstat | wc -l)"
    touched=$(( after - before ))
    total_touched=$(( total_touched + touched ))

    # run-clang-tidy 는 진단이 남아 있으면 0 이 아닌 코드를 낸다. 고칠 수 없는
    # 진단(예: 매크로 안)이 있으면 정상이므로 실패로 보지 않는다. 대신 로그를
    # 남겨두고 파일 변경 수만 보고한다.
    info "[${name}] 완료 (rc=${rc}) — 변경된 파일 누계 ${after}개, 이번 배치 +${touched}"
    info "[${name}] 로그: output/reports/module-analysis/fix-${name}-${BATCH}.log"
done

info "───────────────────────────────────────────────"
info "batch=${BATCH} 종료. 변경 파일 +${total_touched}"
info "다음: 빌드와 유닛테스트로 회귀가 없는지 확인할 것."
[[ ${BATCH} == api ]] && info "주의: unittest/mock/*.hpp 의 링크 심 시그니처를 손으로 맞춰야 한다."
exit 0
