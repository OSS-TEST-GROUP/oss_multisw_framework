#!/bin/bash
# =============================================================================
# coverage-modules.sh — lifecycle_mgr / ros2 adsw_manager 커버리지 (모듈 스코프)
#
# 왜 필요한가:
#   docker/scripts/coverage.sh 는 전체 트리를 하나의 리포트로 합친다. 모듈별
#   수치가 안 나오고, 어떤 파일이 0% 인지도 한눈에 안 보인다. Phase 2(테스트 갭
#   메우기)는 "어느 파일의 어느 브랜치가 비었나"를 알아야 시작할 수 있다.
#   이 스크립트는 같은 lcov 캡처에서 --extract 로 모듈만 남기고,
#   파일별 라인/브랜치 커버리지를 낮은 순으로 정렬해 낸다.
#
# 전제: ./build.sh --build-unittest 로 빌드하고 테스트를 한 번 돌린 상태
#       (.gcda 가 생성돼 있어야 한다). 순서:
#         ./build.sh --build-unittest
#         ./docker/scripts/run_unittest.sh
#         ./scripts/coverage-modules.sh
#
# 실행 위치: 빌더 컨테이너 안 (lcov / genhtml / lcov_cobertura 필요)
#
# 사용법:
#   ./scripts/coverage-modules.sh                 # 캡처 + 모듈별 리포트
#   ./scripts/coverage-modules.sh --reuse-capture # 기존 test.info 재사용 (빠름)
#   ./scripts/coverage-modules.sh --with-logger   # logger/ 포함 (CI 는 제외함)
#   ./scripts/coverage-modules.sh --with-simulator# lifecycle simulator/ 포함
#   BUILD_DIR=... ./scripts/coverage-modules.sh   # 빌드 트리 위치 직접 지정
#
# 산출물 (output/coverage/modules/):
#   <mod>/module.info        모듈만 남긴 lcov tracefile
#   <mod>/report/index.html  genhtml 리포트
#   <mod>/by-file.txt        파일별 라인/브랜치 커버리지 (오름차순)
#   <mod>/zero-coverage.txt  라인 커버리지 0% 파일 목록 (Phase 2 최우선 대상)
#   <mod>/coverage.xml       cobertura (CI 연동용)
#   SUMMARY.md               모듈별 총계 — 이 파일을 베이스라인으로 보고
#
# 기본 제외 항목과 그 이유:
#   */logger/*     — CI coverage.sh 가 제외한다. 두 모듈 모두 logger/Logger.cpp
#                    를 갖고 있어 수치에서 빠진다. 의도인지 확인 필요 →
#                    --with-logger 로 포함시켜 차이를 볼 수 있다.
#   */simulator/*  — lifecycle_mgr/simulator 는 별도 main() 을 가진 개발용
#                    퍼블리셔/서브스크라이버 하네스다. 프로덕션 로직이 아니므로
#                    커버리지 분모에서 빼는 것이 맞다. CI coverage.sh 는 빼지
#                    않으므로 CI 수치와 다를 수 있다. --with-simulator 로 포함.
# =============================================================================
set -u
set -o pipefail

WORKDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )/.." && pwd )"

# CI 와 동일한 빌드 트리. docker/scripts/build.sh 가
#   .build/${BUILD_MODE}/${TARGET_MF_NAME}_${TARGET_ARCH}
# 로 만들며, core / ext-ros2 / ext-agl 이 모두 이 한 디렉토리를 재사용한다.
BUILD_DIR="${BUILD_DIR:-${WORKDIR}/.build/Debug/ros2_x86_64}"
COVERAGE_DIR="${WORKDIR}/output/coverage"
MODULE_DIR="${COVERAGE_DIR}/modules"

REUSE_CAPTURE=FALSE
WITH_LOGGER=FALSE
WITH_SIMULATOR=FALSE

while [[ $# -gt 0 ]]; do
    case "$1" in
        --reuse-capture)  REUSE_CAPTURE=TRUE; shift ;;
        --with-logger)    WITH_LOGGER=TRUE; shift ;;
        --with-simulator) WITH_SIMULATOR=TRUE; shift ;;
        -h|--help)        sed -n '2,45p' "${BASH_SOURCE[0]}"; exit 0 ;;
        *) echo "unknown option: $1" >&2; exit 2 ;;
    esac
done

die()  { echo "[coverage] ERROR: $*" >&2; exit 1; }
info() { echo "[coverage] $*"; }

for t in lcov genhtml python3; do
    command -v "${t}" >/dev/null 2>&1 || die "'${t}' 없음. 빌더 컨테이너 안에서 실행해야 한다."
done

[[ -d ${BUILD_DIR} ]] || die "빌드 트리 없음: ${BUILD_DIR}
  → ./build.sh --build-unittest 를 먼저 실행했는지 확인.
  → 경로가 다르면 BUILD_DIR=... 로 지정."

if ! find "${BUILD_DIR}" -name '*.gcda' -print -quit 2>/dev/null | grep -q .; then
    die ".gcda 가 없다 (${BUILD_DIR}).
  → 테스트를 한 번도 실행하지 않았다는 뜻이다. ./docker/scripts/run_unittest.sh 먼저."
fi

mkdir -p "${COVERAGE_DIR}" "${MODULE_DIR}"

# lcov 1.x / 2.x 의 branch coverage 옵션 이름이 다르다. 1.16(Ubuntu 22.04)은
# lcov_branch_coverage, 2.x 는 branch_coverage. 버전을 보고 고른다.
LCOV_VER="$(lcov --version 2>/dev/null | sed -nE 's/.*version ([0-9]+).*/\1/p' | head -1)"
if [[ ${LCOV_VER:-1} -ge 2 ]]; then
    BR_RC=(--rc branch_coverage=1)
    LCOV_EXTRA=(--ignore-errors inconsistent,unused,empty)
else
    BR_RC=(--rc lcov_branch_coverage=1)
    LCOV_EXTRA=()
fi
info "lcov major=${LCOV_VER:-?}  branch opt=${BR_RC[*]}"

# ── 1. 캡처 ─────────────────────────────────────────────────────────────────
RAW="${COVERAGE_DIR}/test.info"
if [[ ${REUSE_CAPTURE} == TRUE && -s ${RAW} ]]; then
    info "기존 캡처 재사용: ${RAW}"
else
    info "lcov --capture (${BUILD_DIR}) — 수 분 소요"
    lcov --capture \
        --directory "${BUILD_DIR}" \
        --output-file "${RAW}" \
        "${BR_RC[@]}" "${LCOV_EXTRA[@]}" \
        > "${COVERAGE_DIR}/capture.log" 2>&1 \
        || die "lcov --capture 실패 → ${COVERAGE_DIR}/capture.log"
fi
[[ -s ${RAW} ]] || die "캡처 결과가 비었다: ${RAW}"

# ── 2. 공통 제외 (CI coverage.sh 와 동일 목록 + 옵션) ───────────────────────
REMOVE=(
    '/usr/*' '/opt/*'
    '*/rapidjson/*' '*/nlohmann/*' '*/valijson/*'
    '*/googletest-src/*' '*/paho-mqtt-cpp-src/*'
    '*/generated/*' '*/idl/*'
    '*/.build/*' '*/unittest/*'
)
[[ ${WITH_LOGGER}    == FALSE ]] && REMOVE+=( '*/logger/*' )
[[ ${WITH_SIMULATOR} == FALSE ]] && REMOVE+=( '*/simulator/*' )

FILTERED="${COVERAGE_DIR}/filtered-modules.info"
info "lcov --remove (제외 ${#REMOVE[@]}종)"
lcov --remove "${RAW}" "${REMOVE[@]}" \
    --output-file "${FILTERED}" \
    "${BR_RC[@]}" "${LCOV_EXTRA[@]}" \
    > "${COVERAGE_DIR}/remove.log" 2>&1 \
    || die "lcov --remove 실패 → ${COVERAGE_DIR}/remove.log"

# ── 3. 모듈별 추출 ──────────────────────────────────────────────────────────
MODULES=(
    "lifecycle|service/core/lifecycle_mgr"
    "ota|service/core/ota_mgr"
    "otaclient|service/extension/agl/ota_client"
    "adsw|service/extension/ros2/adsw_manager"
    "general|service/extension/general/adsw_manager"
)

# .info 를 직접 파싱해 파일별 LF/LH/BRF/BRH 를 표로 만든다.
# (lcov --list 출력은 컬럼 폭이 버전마다 달라져 파싱이 취약하다.)
emit_by_file() {   # $1=info 파일 $2=출력 디렉토리
    python3 - "$1" "$2" <<'PY'
import sys, os
info_path, outdir = sys.argv[1], sys.argv[2]

# LOG_* 매크로가 만드는 브랜치를 따로 센다.
#
# 왜: LOG_DEBUG(a << b << c) 한 줄이 std::stringstream 과 operator<< 를 호출부에
# 인라인 확장하면서 브랜치를 수십 개 만든다. 그것들은 우리 로직이 아니라
# libstdc++ 내부(locale/width 등)라 어떤 테스트로도 대부분 실행되지 않는다.
# 실측상 두 모듈 브랜치 분모의 54~64% 를 차지하고 커버율은 30% 언저리에 고정이라,
# 분모에 남겨두면 브랜치 커버리지가 영구히 35% 근처에 캡된다.
# 진단은 매크로 첫 줄에 귀속되므로 그 줄에 "LOG_" 가 있는지로 판정한다 — 근사값이다.
_src_cache = {}
def is_log_line(path, lineno):
    lines = _src_cache.get(path)
    if lines is None:
        try:
            lines = open(path, errors="replace").read().splitlines()
        except OSError:
            lines = []
        _src_cache[path] = lines
    return "LOG_" in lines[lineno - 1] if 0 < lineno <= len(lines) else False

files, cur = [], None
for line in open(info_path, errors="replace"):
    line = line.strip()
    if line.startswith("SF:"):
        cur = {"sf": line[3:], "LF": 0, "LH": 0, "BRF": 0, "BRH": 0,
               "BRF_NOLOG": 0, "BRH_NOLOG": 0}
    elif cur is None:
        continue
    elif line.startswith("BRDA:"):
        # BRDA:<line>,<block>,<branch>,<taken|->
        parts = line[5:].split(",")
        if len(parts) >= 4:
            try:
                ln = int(parts[0])
            except ValueError:
                continue
            if not is_log_line(cur["sf"], ln):
                cur["BRF_NOLOG"] += 1
                if parts[3] not in ("-", "0"):
                    cur["BRH_NOLOG"] += 1
    elif line.startswith(("LF:", "LH:", "BRF:", "BRH:")):
        k, v = line.split(":", 1)
        try:
            cur[k] = int(v)
        except ValueError:
            pass
    elif line == "end_of_record":
        files.append(cur); cur = None

def pct(h, f):
    return (100.0 * h / f) if f else None

def fmt(p):
    return "   n/a" if p is None else f"{p:6.2f}"

files.sort(key=lambda d: (pct(d["LH"], d["LF"]) if d["LF"] else -1.0, d["sf"]))

root = os.path.commonpath([f["sf"] for f in files]) if files else ""
def short(p):
    for marker in ("/service/",):
        i = p.find(marker)
        if i != -1:
            return p[i + 1:]
    return os.path.relpath(p, root) if root else p

with open(os.path.join(outdir, "by-file.txt"), "w") as fh:
    fh.write(f"{'line%':>7} {'lines':>12}  {'br%':>7} {'branches':>14}  file\n")
    fh.write("-" * 100 + "\n")
    for d in files:
        fh.write(
            f"{fmt(pct(d['LH'], d['LF']))} {d['LH']:5d}/{d['LF']:<6d}  "
            f"{fmt(pct(d['BRH'], d['BRF']))} {d['BRH']:6d}/{d['BRF']:<7d}  "
            f"{short(d['sf'])}\n"
        )

zero = [d for d in files if d["LF"] > 0 and d["LH"] == 0]
with open(os.path.join(outdir, "zero-coverage.txt"), "w") as fh:
    for d in zero:
        fh.write(f"{d['LF']:6d} lines  {short(d['sf'])}\n")

tot_lf = sum(d["LF"] for d in files); tot_lh = sum(d["LH"] for d in files)
tot_bf = sum(d["BRF"] for d in files); tot_bh = sum(d["BRH"] for d in files)
nl_bf = sum(d["BRF_NOLOG"] for d in files); nl_bh = sum(d["BRH_NOLOG"] for d in files)

# LOG_ 유래 브랜치만 따로 (전체 - 비로그). BRDA 총합과 BRF 가 어긋날 수 있어
# BRDA 기준으로 계산한 비로그 값과 BRF 총합을 같이 남긴다.
with open(os.path.join(outdir, "branches-nolog.txt"), "w") as fh:
    fh.write("LOG_* 매크로 라인의 브랜치를 분모에서 제외한 값.\n")
    fh.write("근사: 진단은 매크로 첫 줄에 귀속되므로 그 줄의 'LOG_' 유무로 판정한다.\n\n")
    fh.write(f"{'br%(전체)':>10} {'br%(LOG_제외)':>14}  file\n")
    fh.write("-" * 100 + "\n")
    for d in sorted(files, key=lambda x: (pct(x["BRH_NOLOG"], x["BRF_NOLOG"])
                                          if x["BRF_NOLOG"] else -1.0, x["sf"])):
        fh.write(f"{fmt(pct(d['BRH'], d['BRF'])):>10} "
                 f"{fmt(pct(d['BRH_NOLOG'], d['BRF_NOLOG'])):>14}  "
                 f"{d['BRH_NOLOG']:5d}/{d['BRF_NOLOG']:<6d}  {short(d['sf'])}\n")

with open(os.path.join(outdir, "totals.txt"), "w") as fh:
    fh.write(f"files={len(files)}\n")
    fh.write(f"zero_coverage_files={len(zero)}\n")
    fh.write(f"lines_hit={tot_lh}\nlines_found={tot_lf}\n")
    fh.write(f"line_pct={'' if not tot_lf else round(100.0*tot_lh/tot_lf, 2)}\n")
    fh.write(f"branches_hit={tot_bh}\nbranches_found={tot_bf}\n")
    fh.write(f"branch_pct={'' if not tot_bf else round(100.0*tot_bh/tot_bf, 2)}\n")
    fh.write(f"branches_nolog_hit={nl_bh}\nbranches_nolog_found={nl_bf}\n")
    fh.write(f"branch_nolog_pct={'' if not nl_bf else round(100.0*nl_bh/nl_bf, 2)}\n")
print(f"[coverage] files={len(files)} zero={len(zero)} "
      f"lines={tot_lh}/{tot_lf} branches={tot_bh}/{tot_bf} "
      f"branches(LOG_제외)={nl_bh}/{nl_bf}")
PY
}

for entry in "${MODULES[@]}"; do
    IFS='|' read -r name relpath <<< "${entry}"
    out="${MODULE_DIR}/${name}"
    mkdir -p "${out}"
    minfo="${out}/module.info"

    echo
    info "═══ ${name} (${relpath}) ═══"
    if ! lcov --extract "${FILTERED}" "*/${relpath}/*" \
            --output-file "${minfo}" \
            "${BR_RC[@]}" "${LCOV_EXTRA[@]}" \
            > "${out}/extract.log" 2>&1; then
        info "[${name}] lcov --extract 실패 → ${out}/extract.log"
        continue
    fi
    if [[ ! -s ${minfo} ]] || ! grep -q '^SF:' "${minfo}"; then
        info "[${name}] 추출 결과가 비었다. 이 모듈 코드가 계측/실행되지 않았다는 뜻이다."
        info "  → ${out}/extract.log 및 캡처 대상 BUILD_DIR 확인"
        continue
    fi

    emit_by_file "${minfo}" "${out}"

    # ── 방안 A: gcovr --exclude-throw-branches (실제 제어흐름 branch) ──
    # C++ -fexceptions 는 모든 함수호출·<<·문자열연산에 숨은 throw/normal 분기쌍을
    # 만든다. 이 예외 edge 는 유닛으로 트리거 불가라 lcov 의 branch% 를 왜곡한다.
    # gcovr --exclude-throw-branches 로 예외 분기를 뺀 '실제 branch coverage' 를
    # 병기한다(lcov 1.x 는 예외제외 미지원). 이게 branch 게이트 지표다.
    if command -v gcovr >/dev/null 2>&1; then
        gsum="$(gcovr --root "${WORKDIR}" --filter "${relpath}/" \
            --exclude '.*/generated/.*' --exclude '.*/idl/.*' \
            --exclude '.*/unittest/.*' --exclude '.*/test/.*' \
            --exclude '.*/main\.(cc|cpp)$' \
            --exclude-throw-branches --gcov-ignore-parse-errors --print-summary \
            "${BUILD_DIR}" 2>/dev/null | grep -E '^branches:' | head -1)"
        if [[ -n ${gsum} ]]; then
            bh="$(sed -nE 's/.*\(([0-9]+) out of ([0-9]+)\).*/\1/p' <<< "${gsum}")"
            bf="$(sed -nE 's/.*\(([0-9]+) out of ([0-9]+)\).*/\2/p' <<< "${gsum}")"
            bp="$(sed -nE 's/^branches: ([0-9.]+)%.*/\1/p' <<< "${gsum}")"
            { echo "branch_nothrow_hit=${bh}"
              echo "branch_nothrow_found=${bf}"
              echo "branch_nothrow_pct=${bp}"; } >> "${out}/totals.txt"
            info "[${name}] branch(throw 제외, gcovr): ${bp}% (${bh}/${bf})"
        fi
    fi

    genhtml "${minfo}" \
        --output-directory "${out}/report" \
        --branch-coverage --legend \
        "${BR_RC[@]}" \
        --rc genhtml_branch_hi_limit=50 \
        --rc genhtml_branch_med_limit=30 \
        > "${out}/genhtml.log" 2>&1 \
        || info "[${name}] genhtml 실패 → ${out}/genhtml.log"

    if command -v lcov_cobertura >/dev/null 2>&1; then
        lcov_cobertura "${minfo}" --output "${out}/coverage.xml" \
            > "${out}/cobertura.log" 2>&1 \
            || info "[${name}] lcov_cobertura 실패 → ${out}/cobertura.log"
    fi
done

# ── 4. SUMMARY.md ───────────────────────────────────────────────────────────
{
    echo "# 커버리지 베이스라인 — lifecycle_mgr / ros2 adsw_manager"
    echo
    echo "생성: $(date -u '+%Y-%m-%dT%H:%M:%SZ') (UTC)"
    echo "커밋: $(git -C "${WORKDIR}" rev-parse --short HEAD 2>/dev/null || echo 'n/a')"
    echo "빌드 트리: \`${BUILD_DIR#"${WORKDIR}"/}\`"
    echo "logger 포함: ${WITH_LOGGER} / simulator 포함: ${WITH_SIMULATOR}"
    echo
    g() {
        local f="$1" k="$2" v=""
        [[ -f ${f} ]] && v="$(sed -nE "s/^${k}=(.*)$/\1/p" "${f}")"
        [[ -n ${v} ]] && echo "${v}" || echo "-"
    }
    echo "| 모듈 | 파일 | 0% 파일 | 라인 | 라인% | 브랜치 | 브랜치% | 브랜치%(LOG_ 제외) |"
    echo "|---|---|---|---|---|---|---|---|"
    for entry in "${MODULES[@]}"; do
        IFS='|' read -r name relpath <<< "${entry}"
        t="${MODULE_DIR}/${name}/totals.txt"
        if [[ ! -f ${t} ]]; then
            echo "| \`${relpath}\` | - | - | - | - | - | - | - | <!-- 측정 실패: extract.log 확인 -->"
            continue
        fi
        echo "| \`${relpath}\` | $(g "${t}" files) | $(g "${t}" zero_coverage_files) | $(g "${t}" lines_hit)/$(g "${t}" lines_found) | $(g "${t}" line_pct)% | $(g "${t}" branches_hit)/$(g "${t}" branches_found) | $(g "${t}" branch_pct)% | $(g "${t}" branch_nolog_pct)% ($(g "${t}" branches_nolog_hit)/$(g "${t}" branches_nolog_found)) |"
    done
    echo
    echo "> **브랜치% 를 라인% 와 같은 잣대로 보지 말 것.** 브랜치 분모의 절반 이상이"
    echo "> \`LOG_*\` 매크로에서 나온다. \`LOG_DEBUG(a << b << c)\` 가 stringstream 과"
    echo "> \`operator<<\` 를 호출부에 인라인 확장하며 브랜치를 수십 개 만들고, 그 대부분은"
    echo "> libstdc++ 내부라 어떤 테스트로도 실행되지 않는다. 게이트를 걸 거라면"
    echo "> **라인% 에 걸고 브랜치는 참고치로 둘 것.** 굳이 브랜치로 건다면 LOG_ 제외 열을 쓴다."
    echo "> 파일별 값은 \`<mod>/branches-nolog.txt\`."
    echo
    for entry in "${MODULES[@]}"; do
        IFS='|' read -r name relpath <<< "${entry}"
        z="${MODULE_DIR}/${name}/zero-coverage.txt"
        echo "## ${relpath}"
        echo
        if [[ -s ${z} ]]; then
            echo "### 라인 커버리지 0% 파일 (Phase 2 최우선)"
            echo '```'
            cat "${z}"
            echo '```'
        else
            echo "라인 커버리지 0% 파일 없음."
        fi
        f="${MODULE_DIR}/${name}/by-file.txt"
        if [[ -s ${f} ]]; then
            echo "### 커버리지 낮은 순 상위 20"
            echo '```'
            head -22 "${f}"
            echo '```'
        fi
        echo
    done
    echo "---"
    echo
    # 유닛 레인에서 제외된 테스트 목록은 하드코딩하지 않는다. 예전에 손으로
    # 적어둔 목록이 integration-tests.regex 개정 뒤 그대로 남아 틀린 값을
    # 보고했다 (ADSWManagerTest 5건은 이미 유닛 레인으로 돌아왔고
    # ROSDiscoveryInfoEndpointTest 는 6건이 아니라 3건이었다).
    # ctest 에 직접 물어서 매번 새로 뽑는다.
    echo "**주의**: 아래 테스트들은 \`docker/scripts/integration-tests.regex\` 에 의해"
    echo "유닛 레인에서 제외된다. 통합 레인(\`run_integration.sh\`)을 커버리지 캡처"
    echo "전에 돌리지 않았다면 위 수치에 한 줄도 기여하지 않는다."
    echo
    _re=$(grep -vE '^\s*#|^\s*$' "${WORKDIR}/docker/scripts/integration-tests.regex" 2>/dev/null)
    if [[ -n ${_re} ]]; then
        echo '```'
        for _m in "${MODULES[@]}"; do
            # 레이블은 run_unittest.sh 와 같은 규칙으로 만든다. basename 만
            # 쓰면 ros2/general/agl 세 확장이 전부 "adsw_manager" 가 된다.
            _rel="${_m#*|}"
            case ${_rel} in
                service/extension/*) _label=$(echo "${_rel#service/extension/}" | tr '/' '_') ;;
                *)                   _label=$(basename "${_rel}") ;;
            esac
            _d="${BUILD_DIR}/${_rel}/unittest"
            [[ -f ${_d}/CTestTestfile.cmake ]] || continue
            _all=$(ctest --test-dir "${_d}" -N 2>/dev/null \
                   | grep -oE 'Test +#[0-9]+: \S+' | sed 's/.*: //' | sort)
            _kept=$(ctest --test-dir "${_d}" -N --exclude-regex "${_re}" 2>/dev/null \
                    | grep -oE 'Test +#[0-9]+: \S+' | sed 's/.*: //' | sort)
            _dropped=$(comm -23 <(echo "${_all}") <(echo "${_kept}") \
                       | sed 's/\..*//' | sort | uniq -c | sed 's/^ */  /')
            if [[ -n ${_dropped} ]]; then
                printf '%s\n%s\n' "${_label}" "${_dropped}"
            else
                printf '%s\n  (제외 없음)\n' "${_label}"
            fi
        done
        echo '```'
    else
        echo "(integration-tests.regex 를 읽지 못해 목록을 뽑지 못했다)"
    fi
} > "${MODULE_DIR}/SUMMARY.md"

echo
info "완료. 요약: ${MODULE_DIR#"${WORKDIR}"/}/SUMMARY.md"
