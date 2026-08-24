#!/bin/bash
# =============================================================================
# analyze-modules.sh — lifecycle_mgr / ros2 adsw_manager 정적분석 (모듈 스코프)
#
# 왜 필요한가:
#   Jenkinsfile 의 Clang-tidy / Cppcheck 스테이지는 service/core 와
#   service/extension 을 통째로 분석하고 리포트를 target(core/ros2/agl) 단위로만
#   나눈다. 그래서 "lifecycle_mgr 경고가 몇 건인가"에 답할 수 없다.
#   이 스크립트는 같은 분석 로직을 쓰되 대상 파일 목록을 두 모듈로 좁히고,
#   체크별·파일별 집계까지 뽑아 베이스라인 수치를 만든다.
#
# 실행 위치: 반드시 빌더 컨테이너 안. (clang-tidy / cppcheck / fastddsgen 필요)
#   docker run --rm -it -u 1000:1000 \
#       -w /home/ossuser/workspace \
#       -v ${PWD}:/home/ossuser/workspace \
#       -v ${HOME}/obigo-sdk:/opt \
#       registry.obigo.com/oss/oss-mmf-builder:latest \
#       ./scripts/analyze-modules.sh
#
# 사용법:
#   ./scripts/analyze-modules.sh                    # 두 모듈 모두
#   ./scripts/analyze-modules.sh --only lifecycle    # lifecycle_mgr 만
#   ./scripts/analyze-modules.sh --only adsw         # ros2/adsw_manager 만
#   ./scripts/analyze-modules.sh --skip-cppcheck     # clang-tidy 만
#   ./scripts/analyze-modules.sh --skip-clang-tidy   # cppcheck 만
#   ./scripts/analyze-modules.sh --reconfigure       # cmake 캐시 버리고 재configure
#
# 산출물 (output/reports/module-analysis/):
#   clang-tidy-<mod>.txt            clang-tidy 원본 출력
#   clang-tidy-<mod>.by-check.txt   체크별 건수 (수정 우선순위 판단 근거)
#   clang-tidy-<mod>.by-file.txt    파일별 건수
#   cppcheck-<mod>.xml              cppcheck 원본 (Jenkins recordIssues 호환)
#   cppcheck-<mod>.summary.txt      severity / id 별 건수
#   SUMMARY.md                      전체 요약 — 이 파일 내용을 베이스라인으로 보고
#
# Jenkins 와 의도적으로 다른 점 (베이스라인 해석 시 유의):
#   Jenkins 는 run-clang-tidy 에 *.hpp/*.h 까지 위치인자로 넘긴다. 헤더는
#   compilation database 에 컴파일 커맨드가 없어 사실상 분석되지 않는다.
#   여기서는 TU(*.cpp/*.cc)만 넘기고 -header-filter 로 모듈 헤더를 잡는다.
#   → 같은 코드인데도 Jenkins 리포트보다 건수가 많이 나올 수 있다. 정상이다.
#
# 주의: cmake configure 는 CMAKE_INSTALL_PREFIX(= output/<arch>/Debug/...) 아래
#   generated/idl 을 갱신한다. Jenkins Clang-tidy 스테이지와 동일한 동작이며
#   타임스탬프 기반이라 멱등하지만, 진행 중인 실제 빌드 산출물과 같은 트리를
#   건드린다는 점은 알고 있어야 한다.
# =============================================================================
set -u
set -o pipefail

WORKDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )/.." && pwd )"
REPORT_DIR="${WORKDIR}/output/reports/module-analysis"
BUILD_ROOT="${WORKDIR}/.build/analyze"

ONLY=""
DO_TIDY=TRUE
DO_CPPCHECK=TRUE
RECONFIGURE=FALSE

while [[ $# -gt 0 ]]; do
    case "$1" in
        --only)             ONLY="$2"; shift 2 ;;
        --only=*)           ONLY="${1#*=}"; shift ;;
        --skip-cppcheck)    DO_CPPCHECK=FALSE; shift ;;
        --skip-clang-tidy)  DO_TIDY=FALSE; shift ;;
        --reconfigure)      RECONFIGURE=TRUE; shift ;;
        -h|--help)          sed -n '2,48p' "${BASH_SOURCE[0]}"; exit 0 ;;
        *) echo "unknown option: $1" >&2; exit 2 ;;
    esac
done

# ── 모듈 정의: <name>|<BUILD_EXTENSION>|<EXTENSION_PLATFORM>|<모듈 소스 경로> ──
# lifecycle : BUILD_EXTENSION=FALSE → service/core 가 configure 대상.
#             EXTENSION_PLATFORM 은 쓰이지 않지만 CMake 캐시 변수라 값은 필요.
# adsw      : BUILD_EXTENSION=TRUE + ROS2 → service/extension/ros2.
MODULES=(
    "lifecycle|FALSE|ROS2|service/core/lifecycle_mgr"
    "adsw|TRUE|ROS2|service/extension/ros2/adsw_manager"
    # general 판은 ros2 판의 복사본이 아니라 포크다 (58개 중 로거 2개만 동일하고
    # 나머지는 내용이 갈렸다). ros2 쪽 수정을 그대로 옮길 수 없으므로 별도 모듈로
    # 측정한다. EXTENSION_PLATFORM=GENERAL 로 configure 해야 소스가 잡힌다.
    "general|TRUE|GENERAL|service/extension/general/adsw_manager"
)

die() { echo "[analyze] ERROR: $*" >&2; exit 1; }
info() { echo "[analyze] $*"; }

# ── 사전 점검 ────────────────────────────────────────────────────────────────
need_tools=(cmake)
[[ ${DO_TIDY} == TRUE ]] && need_tools+=(run-clang-tidy clang-tidy)
[[ ${DO_CPPCHECK} == TRUE ]] && need_tools+=(cppcheck python3)
for t in "${need_tools[@]}"; do
    command -v "${t}" >/dev/null 2>&1 || die "'${t}' 없음. 빌더 컨테이너 안에서 실행해야 한다."
done

# FastDDS SDK 환경. Jenkins / build.sh 와 같은 경로를 쓴다.
#
# 주의: `command -v fastddsgen` 을 sourced 여부의 대용으로 쓰면 안 된다.
# 빌더 이미지는 fastddsgen 을 /usr/local/bin 에 별도로 심어 두므로 SDK 를
# source 하지 않아도 항상 PATH 에 잡히고, 그러면 CMAKE_PREFIX_PATH 가 빈 채로
# configure 에 들어가 find_package(fastcdr 2 REQUIRED) 가 실패한다.
# 판정 기준은 fastcdr 의 CMake config 가 CMAKE_PREFIX_PATH 에서 보이는지다.
if [[ ${CMAKE_PREFIX_PATH:-} != *Fast-DDS* ]]; then
    FASTDDS_SETUP="${FASTDDS_SETUP:-/opt/Fast-DDS/output/ros2_x86_64/setup.bash}"
    [[ -f ${FASTDDS_SETUP} ]] || die "FastDDS setup.bash 없음: ${FASTDDS_SETUP}
  → SDK_DIR 를 /opt 로 마운트했는지, Fast-DDS 를 한 번 빌드했는지 확인."
    # colcon 이 만든 setup.bash 는 COLCON_TRACE 등 미정의 변수를 참조한다.
    # set -u 상태로 source 하면 "unbound variable" 로 죽으므로 이 구간만 푼다.
    set +u
    # shellcheck disable=SC1090
    source "${FASTDDS_SETUP}"
    set -u
    info "sourced ${FASTDDS_SETUP}"
fi
command -v fastddsgen >/dev/null 2>&1 || die "fastddsgen 을 PATH 에서 찾을 수 없다."

export BUILD_MODE=Debug   # 상위 CMakeLists 가 CMAKE_BUILD_TYPE 로 승격시킨다
NPROC="$(nproc)"
mkdir -p "${REPORT_DIR}"

# ── 모듈별 TU / 분석 대상 파일 목록 ─────────────────────────────────────────
# Jenkins 와 동일한 제외 규칙: unittest 및 벤더링된 서드파티 헤더.
list_module_files() {   # $1=모듈 경로(절대), $2=extensions 모드(tu|all)
    local dir="$1" mode="$2"
    if [[ ${mode} == tu ]]; then
        find "${dir}" \
            -not -path "*/unittest/*" \
            -not -path "*/rapidjson/*" \
            -not -path "*/nlohmann/*" \
            -not -path "*/valijson/*" \
            -not -path "*/CMakeFiles/*" \
            \( -name "*.cpp" -o -name "*.cc" \) | sort
    else
        find "${dir}" \
            -not -path "*/unittest/*" \
            -not -path "*/rapidjson/*" \
            -not -path "*/nlohmann/*" \
            -not -path "*/valijson/*" \
            -not -path "*/CMakeFiles/*" \
            \( -name "*.cpp" -o -name "*.cc" -o -name "*.hpp" -o -name "*.h" \) | sort
    fi
}

# ── cmake configure → compile_commands.json ─────────────────────────────────
configure_module() {   # $1=name $2=BUILD_EXTENSION $3=EXTENSION_PLATFORM
    local name="$1" ext="$2" plat="$3"
    local bdir="${BUILD_ROOT}/${name}"

    if [[ ${RECONFIGURE} == TRUE ]]; then rm -rf "${bdir}"; fi
    mkdir -p "${bdir}"

    if [[ -f ${bdir}/compile_commands.json && ${RECONFIGURE} == FALSE ]]; then
        info "[${name}] compile_commands.json 재사용 (--reconfigure 로 강제 재생성)"
        return 0
    fi

    info "[${name}] cmake configure (BUILD_EXTENSION=${ext}, PLATFORM=${plat})"
    ( cd "${bdir}" && cmake "${WORKDIR}" \
        -DBUILD_UNITTEST=FALSE \
        -DBUILD_EXTENSION="${ext}" \
        -DEXTENSION_PLATFORM="${plat}" \
        -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ) \
        > "${REPORT_DIR}/cmake-configure-${name}.log" 2>&1

    if [[ ! -f ${bdir}/compile_commands.json ]]; then
        die "[${name}] compile_commands.json 생성 실패.
  → ${REPORT_DIR}/cmake-configure-${name}.log 확인"
    fi
}

# ── clang-tidy ──────────────────────────────────────────────────────────────
run_tidy() {   # $1=name $2=모듈 경로(절대)
    local name="$1" moddir="$2"
    local bdir="${BUILD_ROOT}/${name}"
    local raw="${REPORT_DIR}/clang-tidy-${name}.txt"

    # mapfile 은 bash 4+ 전용이다. 빌더(Ubuntu 22.04)는 bash 5 지만, 손으로
    # macOS 등에서 돌려볼 때 조용히 깨지지 않게 while-read 로 채운다.
    local tus=()
    while IFS= read -r line; do
        [[ -n ${line} ]] && tus+=("${line}")
    done < <(list_module_files "${moddir}" tu)

    if [[ ${#tus[@]} -eq 0 ]]; then
        info "[${name}] clang-tidy 대상 TU 없음 — 건너뜀"; return 0
    fi
    info "[${name}] clang-tidy: TU ${#tus[@]}개, -j ${NPROC}"

    # -header-filter: 모듈 자기 헤더의 진단만 남긴다. (FastDDS/생성 IDL 헤더의
    #   경고까지 끌어오면 베이스라인이 우리 코드 상태를 반영하지 못한다.)
    # -quiet: "N warnings generated." 잡음 제거 → 집계가 깔끔해진다.
    run-clang-tidy \
        -p "${bdir}" \
        -j "${NPROC}" \
        -quiet \
        -header-filter="^${moddir}/.*" \
        "${tus[@]}" > "${raw}" 2>&1

    # run-clang-tidy 가 clang-tidy 에 --use-color 를 붙여 넘기는 경우가 있어
    # 원본에 ANSI 이스케이프가 섞인다. 그러면 아래 진단 추출 정규식이 한 건도
    # 매칭하지 못해 집계가 통째로 0 이 된다. 파싱 전에 제거한다.
    sed -i -E 's/\x1B\[[0-9;]*[A-Za-z]//g' "${raw}"

    # 경로를 저장소 상대경로로 정규화 (다른 머신 리포트와 비교 가능하게)
    sed -i "s#${WORKDIR}/##g" "${raw}"

    # include 경로에 따라 같은 헤더가 `a/b.hpp` 와 `a/./b.hpp` 두 표기로 나온다.
    # 정규화하지 않으면 dedupe 가 안 먹어 unique 가 부풀고 by-file 에 같은 파일이
    # 두 줄로 뜬다. `/./` 는 반복 치환해야 `a/././b` 같은 경우도 접힌다.
    while grep -q '/\./' "${raw}"; do sed -i 's#/\./#/#g' "${raw}"; done

    # 진단 라인만 추출. 같은 헤더 경고가 TU 마다 중복 보고되므로 dedupe 한 값도
    # 같이 낸다 — 실제 "고쳐야 할 지점" 수는 unique 쪽이다.
    local diags_all diags_uniq
    diags_all="$(sed -nE 's#^([^: ]+):([0-9]+):([0-9]+): (warning|error): (.*)$#\1:\2:\3: \4: \5#p' "${raw}")"
    diags_uniq="$(printf '%s\n' "${diags_all}" | sed '/^$/d' | sort -u)"

    printf '%s\n' "${diags_uniq}" | sed '/^$/d' \
        | sed -nE 's/^.*\[([A-Za-z0-9_.,-]+)\]$/\1/p' \
        | tr ',' '\n' | sed '/^$/d' \
        | grep -v '^-warnings-as-errors$' \
        | sort | uniq -c | sort -rn \
        > "${REPORT_DIR}/clang-tidy-${name}.by-check.txt"

    printf '%s\n' "${diags_uniq}" | sed '/^$/d' \
        | sed -nE 's#^([^:]+):[0-9]+:[0-9]+:.*#\1#p' \
        | sort | uniq -c | sort -rn \
        > "${REPORT_DIR}/clang-tidy-${name}.by-file.txt"

    # 컴파일 자체가 깨진 경우(헤더 못 찾음 등)는 베이스라인이 무의미하므로 표시
    local fatal
    fatal="$(grep -c 'error: .*file not found\|fatal error' "${raw}" 2>/dev/null || true)"

    {
        echo "raw_diagnostics=$(printf '%s\n' "${diags_all}" | sed '/^$/d' | wc -l | tr -d ' ')"
        echo "unique_diagnostics=$(printf '%s\n' "${diags_uniq}" | sed '/^$/d' | wc -l | tr -d ' ')"
        echo "fatal_or_missing_include=${fatal:-0}"
        echo "tu_count=${#tus[@]}"
    } > "${REPORT_DIR}/clang-tidy-${name}.stats"

    info "[${name}] clang-tidy 완료 → $(cat "${REPORT_DIR}/clang-tidy-${name}.stats" | tr '\n' ' ')"
}

# ── cppcheck ────────────────────────────────────────────────────────────────
run_cppcheck() {   # $1=name $2=모듈 경로(절대)
    local name="$1" moddir="$2"
    local bdir="${BUILD_ROOT}/${name}"
    local filelist="${REPORT_DIR}/files-${name}.txt"
    local filtered="${REPORT_DIR}/compile_commands-${name}.json"
    local xml="${REPORT_DIR}/cppcheck-${name}.xml"

    list_module_files "${moddir}" all > "${filelist}"

    # Jenkins 는 jq 로 compile db 를 필터링하지만 빌더 이미지에 jq 가 없다.
    # 동작이 동일한 python3 구현. (compile db 에는 TU 만 있으므로 결과적으로
    # 모듈의 *.cpp/*.cc 항목만 남는다.)
    python3 - "${bdir}/compile_commands.json" "${filelist}" "${filtered}" <<'PY'
import json, os, sys
db_path, list_path, out_path = sys.argv[1], sys.argv[2], sys.argv[3]
allowed = {os.path.realpath(l.strip()) for l in open(list_path) if l.strip()}
with open(db_path) as f:
    db = json.load(f)
kept = []
for e in db:
    f = e.get("file", "")
    if not os.path.isabs(f):
        f = os.path.join(e.get("directory", ""), f)
    if os.path.realpath(f) in allowed:
        kept.append(e)
with open(out_path, "w") as f:
    json.dump(kept, f, indent=1)
print(f"[analyze] compile db entries: {len(db)} -> {len(kept)}")
PY
    [[ -s ${filtered} ]] || die "[${name}] compile db 필터링 실패"

    local n_entries
    n_entries="$(python3 -c 'import json,sys;print(len(json.load(open(sys.argv[1]))))' "${filtered}")"
    if [[ ${n_entries} -eq 0 ]]; then
        info "[${name}] cppcheck 대상 항목 0개 — 건너뜀"; return 0
    fi
    info "[${name}] cppcheck: ${n_entries}개 TU (--enable=all, 수 분 소요)"

    # 억제 목록은 Jenkinsfile Cppcheck 스테이지와 동일하게 유지 —
    # 베이스라인이 CI 리포트와 비교 가능해야 한다.
    cppcheck \
        --enable=all \
        --xml --xml-version=2 \
        --output-file="${xml}" \
        --project="${filtered}" \
        --suppress=missingIncludeSystem \
        --suppress=unmatchedSuppression \
        --suppress=unusedFunction \
        --suppress=missingInclude \
        `# fastddsgen 이 만든 *PubSubTypes.hpp 는 상단에 FastDDS 버전 가드 #error 를
         # 둔다. cppcheck 가 그 버전 매크로를 못 풀어서 severity=error 로 잡는데,
         # 우리 코드가 아니고 고칠 대상도 아니다. 이것 때문에 "cppcheck error N건"
         # 이 실제보다 부풀어 P1 판단을 흐린다.` \
        --suppress=preprocessorErrorDirective:*/generated/idl/* \
        --std=c++20 \
        > "${REPORT_DIR}/cppcheck-${name}.stderr.log" 2>&1 || true

    [[ -f ${xml} ]] || { info "[${name}] cppcheck XML 미생성 — stderr 로그 확인"; return 0; }
    sed -i "s#${WORKDIR}/##g" "${xml}"

    {
        echo "── severity 별 ──"
        grep -oE 'severity="[a-z]+"' "${xml}" | sed 's/severity=//;s/"//g' \
            | sort | uniq -c | sort -rn
        echo
        echo "── id 별 ──"
        grep -oE '<error id="[^"]+"' "${xml}" | sed 's/<error id="//;s/"//' \
            | sort | uniq -c | sort -rn
    } > "${REPORT_DIR}/cppcheck-${name}.summary.txt"

    local total
    total="$(grep -c '<error id=' "${xml}" 2>/dev/null || true)"
    echo "cppcheck_findings=${total:-0}" > "${REPORT_DIR}/cppcheck-${name}.stats"
    info "[${name}] cppcheck 완료 → findings=${total:-0}"
}

# ── 실행 ────────────────────────────────────────────────────────────────────
info "workdir=${WORKDIR}"
info "reports=${REPORT_DIR}"

for entry in "${MODULES[@]}"; do
    IFS='|' read -r name ext plat relpath <<< "${entry}"
    if [[ -n ${ONLY} && ${ONLY} != "${name}" ]]; then continue; fi

    moddir="${WORKDIR}/${relpath}"
    [[ -d ${moddir} ]] || die "모듈 경로 없음: ${relpath}"

    echo
    info "═══ ${name} (${relpath}) ═══"
    configure_module "${name}" "${ext}" "${plat}"
    [[ ${DO_TIDY} == TRUE ]] && run_tidy "${name}" "${moddir}"
    [[ ${DO_CPPCHECK} == TRUE ]] && run_cppcheck "${name}" "${moddir}"
done

# ── SUMMARY.md ──────────────────────────────────────────────────────────────
{
    echo "# 정적분석 베이스라인 — lifecycle_mgr / ros2 adsw_manager"
    echo
    echo "생성: $(date -u '+%Y-%m-%dT%H:%M:%SZ') (UTC)"
    echo "커밋: $(git -C "${WORKDIR}" rev-parse --short HEAD 2>/dev/null || echo 'n/a')"
    echo "clang-tidy: $(clang-tidy --version 2>/dev/null | sed -n 's/.*version \([0-9.]*\).*/\1/p' | head -1)"
    echo "cppcheck:   $(cppcheck --version 2>/dev/null)"
    echo
    echo "## 요약"
    echo
    # stats 파일에서 key 를 뽑되, 파일이나 key 가 없으면 "-" 로 채운다.
    # (값이 빈 셀이면 "0 건"인지 "측정 안 됨"인지 구분이 안 된다.)
    get() {
        local f="$1" k="$2" v=""
        [[ -f ${f} ]] && v="$(sed -nE "s/^${k}=(.*)$/\1/p" "${f}")"
        [[ -n ${v} ]] && echo "${v}" || echo "-"
    }
    echo "| 모듈 | TU | clang-tidy (unique) | clang-tidy (raw) | fatal/include-error | cppcheck |"
    echo "|---|---|---|---|---|---|"
    for entry in "${MODULES[@]}"; do
        IFS='|' read -r name _ _ relpath <<< "${entry}"
        if [[ -n ${ONLY} && ${ONLY} != "${name}" ]]; then continue; fi
        ts="${REPORT_DIR}/clang-tidy-${name}.stats"
        cs="${REPORT_DIR}/cppcheck-${name}.stats"
        echo "| \`${relpath}\` | $(get "${ts}" tu_count) | $(get "${ts}" unique_diagnostics) | $(get "${ts}" raw_diagnostics) | $(get "${ts}" fatal_or_missing_include) | $(get "${cs}" cppcheck_findings) |"
    done
    echo
    echo "> \`fatal/include-error\` 가 0 이 아니면 분석이 정상 파싱되지 않은 파일이 있다는 뜻이다."
    echo "> 그 상태의 건수는 베이스라인으로 쓸 수 없으니 먼저 원인을 잡아야 한다."
    echo
    for entry in "${MODULES[@]}"; do
        IFS='|' read -r name _ _ relpath <<< "${entry}"
        if [[ -n ${ONLY} && ${ONLY} != "${name}" ]]; then continue; fi
        echo "## ${relpath}"
        echo
        f="${REPORT_DIR}/clang-tidy-${name}.by-check.txt"
        if [[ -s ${f} ]]; then
            echo "### clang-tidy — 체크별 상위 25"
            echo '```'
            head -25 "${f}"
            echo '```'
        fi
        f="${REPORT_DIR}/clang-tidy-${name}.by-file.txt"
        if [[ -s ${f} ]]; then
            echo "### clang-tidy — 파일별 상위 15"
            echo '```'
            head -15 "${f}"
            echo '```'
        fi
        f="${REPORT_DIR}/cppcheck-${name}.summary.txt"
        if [[ -s ${f} ]]; then
            echo "### cppcheck"
            echo '```'
            cat "${f}"
            echo '```'
        fi
        echo
    done
} > "${REPORT_DIR}/SUMMARY.md"

echo
info "완료. 요약: ${REPORT_DIR}/SUMMARY.md"
