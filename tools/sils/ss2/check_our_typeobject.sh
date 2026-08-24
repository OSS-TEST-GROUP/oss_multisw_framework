#!/usr/bin/env bash
#
# check_our_typeobject.sh -- **우리 정준 타입**의 TypeObject 가 같은 도메인의 Cyclone 을
#                            (1) 오염시키는지 (2) 죽이는지, 둘을 **따로** 판정한다.
#
# ★ 왜 이 스크립트가 필요했나 — 반증 불가능해진 안전 주장
#
# sils_participant_qos.hpp 와 sils-stage3-autoware-partition.md 는 2026-08-11 관측을 근거로
# 이렇게 적혀 있었다:
#
#     우리 정준 타입을 든 Fast DDS 참가자가 Cyclone 이 도는 도메인에 합류한 순간
#     파티션 3종이 전부 오염됐고 **decision 파티션이 segfault 했다**
#
# 그런데 그 remedy(`type_propagation=disabled`)가 **관측과 같은 커밋**(d11a5a5)에 들어갔다.
# 그래서 그날 이후로는 몇 번을 다시 돌려도 0 건이 나오고, 그것은 "패치가 먹었다" 만
# 증명할 뿐 원래 주장을 검증하지도 반증하지도 못한다. 재현 스위치가 없는 안전 주장은
# 시간이 지나면 그냥 전승되는 문장이 된다.
#
# 그래서 sils_participant_qos.hpp 에 SILS_TYPE_PROPAGATION 을 뒀고, 이 스크립트가 그것으로
# A/B 를 돌린다.
#
# ★ 판정을 둘로 쪼개는 이유 — 2026-08-12 에 이 둘을 섞어서 하루를 날렸다
#
#   오염(에러 로그)  ... 리더에서 난다. 규칙을 어긴 쪽은 멀쩡하고 남의 로그가 더러워진다
#   사망(segfault)   ... 별개의 명제다
#
# 미패치 MMF 로 파티션 3종에 1260건을 찍고도 **파티션은 160초 멀쩡했다**. 즉
# **오염은 사망을 함의하지 않는다.** 그리고 그때 관측된 사망의 진짜 원인은 DDS 가 아니라
# 중복된 run_partitions.sh 의 EXIT 트랩이었다.
#   → docs/sils/evidence/mmf-typeobject-2026-08-12.md
#
# 여기서는 그래서 세 가지를 각각 따로 보고한다:
#   A. 오염     파티션 로그 `invalid type object` **증가분** (기준선 대비)
#   B. 사망     파티션 컨테이너 생존 + **컨테이너 ID 동일성** + 로그의 `Segmentation fault`
#   C. 가해자   sils_ego_source 자신은 살아 있나 (리더에서 터진다면 이쪽은 멀쩡해야 한다)
#
# ⚠ B 를 볼 때 **컨테이너 ID** 까지 비교하는 이유: 파티션은 `--rm` 이라 죽으면 흔적 없이
#   사라지고, 누가 다시 띄우면 "살아 있다" 로 보인다. 이름만 봐선 구별이 안 된다.
#
# ⚠⚠ **순서가 결과를 바꾼다.** 에러는 리더-라이터 매칭 시점의 타입 교환에서 나므로
#   writer 가 먼저 떠 있으면 미패치라도 0 건이다(MMF 로 실측). 그래서 이 스크립트는
#   **파티션이 이미 떠 있는 상태**를 전제로 하고, writer 를 나중에 넣는다.
#   그것이 2026-08-11 의 "케이스 B" 이고, 정직한 시험이다.
#
# ⚠⚠⚠ SILS_TYPE_PROPAGATION=enabled 는 **격리된 도메인에서만** 쓸 것. 실패가 남의
#   프로세스에서 나는 종류라, 운영 도메인에 넣으면 남을 망가뜨린다.
#
# Usage:
#   tools/sils/ss2/check_our_typeobject.sh --unpatched --watch 180   # 재현 시도
#   tools/sils/ss2/check_our_typeobject.sh --patched   --watch 180   # 대조군 (기본)
# Exit: 0 정상 종료(판정은 출력 참조) · 2 SKIP(전제 미충족)

set -uo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
LOG_DIR="${ADSW_LOG_DIR:-$HOME/oss/oss_adsw/log}"
DOMAIN="${SILS_DOMAIN:-42}"
WATCH=180
MODE="disabled"          # 기본은 안전한 쪽. --unpatched 를 명시해야 위험한 쪽으로 간다.
RATE_MS=20
JOIN_READER=0
READER_NAME="sils-typeprop-reader"
READER_IMAGE="${SILS_SS2_ADAPTER_IMAGE:-sils-ss2-adapter:humble}"
READER_EXE="aw_vehicle_status_bridge"
READER_DELAY=20

while [ "${1:-}" != "" ]; do
    case "$1" in
        --unpatched) MODE="enabled" ;;
        --patched)   MODE="disabled" ;;
        --join-reader) JOIN_READER=1 ;;
        --watch)     WATCH="$2"; shift ;;
        --domain)    DOMAIN="$2"; shift ;;
        --log-dir)   LOG_DIR="$2"; shift ;;
        --help|-h)   sed -n '2,60p' "${BASH_SOURCE[0]}"; exit 0 ;;
        *) echo "check_our_typeobject: unknown option: $1" >&2; exit 2 ;;
    esac
    shift
done

parts=(perception decision control)

# ⚠ `grep -c` 는 0건일 때 "0" 을 출력하고 **exit 1** 이다. `|| echo 0` 을 붙이면 "0" 이
# 두 줄이 되어 산술이 깨진다(2026-08-12 에 실제로 밟아 거짓 FAIL 을 만들었다).
# 종료코드는 버리고 출력만 쓴다.
count_in() {  # <파일> <패턴>
    local c
    c="$(grep -c "$2" "$1" 2>/dev/null)"
    case "$c" in ''|*[!0-9]*) c=0 ;; esac
    printf '%s' "$c"
}

cid_of() {  # <컨테이너명>  -- 없으면 빈 문자열
    docker inspect -f '{{.Id}}' "$1" 2>/dev/null | cut -c1-12
}

# ---------------------------------------------------------------- 전제 확인

# 상류 러너가 살아 있으면 그 트랩이 실험 중에 파티션을 날릴 수 있다. 그러면 "죽었다" 가
# 우리 타입 탓인지 트랩 탓인지 **구별할 수 없다** — 그게 2026-08-12 의 오진 그 자체다.
runners="$(pgrep -cf '[r]un_partitions\.sh' 2>/dev/null)"
case "$runners" in ''|*[!0-9]*) runners=0 ;; esac
if [ "$runners" -gt 0 ]; then
    echo "SKIP: 상류 run_partitions.sh 러너가 ${runners}개 살아 있다."
    echo "      그 EXIT 트랩이 실험 중 파티션을 내리면 사망 원인을 가릴 수 없다."
    echo "      정리:  pkill -f '[r]un_partitions\\.sh'   (프로세스 그룹째 죽여야 트랩이 돈다)"
    exit 2
fi

alive=0
for p in "${parts[@]}"; do
    [ -n "$(cid_of "adsw-${p}-${DOMAIN}")" ] && alive=$((alive + 1))
done
if [ "$alive" -ne 3 ]; then
    echo "SKIP: 파티션 3종 중 ${alive}개만 떠 있다. 잴 대상이 없다."
    echo "      기동:  tools/sils/ss2/run_partitions_detached.sh --domain-id ${DOMAIN} \\"
    echo "                 --map-path \"\$HOME/autoware_map/kashiwanoha\""
    exit 2
fi

SRC="${REPO_ROOT}/output/x86_64/Debug/extension/ros2/bin/sils_ego_source"
[ -x "$SRC" ] || SRC="${REPO_ROOT}/bin/sils_ego_source"
if [ ! -x "$SRC" ]; then
    echo "SKIP: sils_ego_source 가 없다.  ./build.sh --build-sils"
    exit 2
fi

# 바이너리가 스위치를 아는가. 모르는 바이너리로 --unpatched 를 돌리면 조용히 패치본을
# 재게 되고, 그 0건을 "재현 실패" 로 읽으면 정확히 틀린 결론이 된다.
# ⚠ 여기서 `strings ... | grep -q` 를 쓰면 **매치했을 때 실패로 판정된다.** grep -q 는 첫
# 매치에서 즉시 끝나고, 그러면 strings 가 SIGPIPE 로 141 을 반환하며 `set -o pipefail` 이
# 파이프라인 전체를 실패로 만든다. 스위치가 있을수록 SKIP 이 뜬다(실제로 밟았다).
# grep -c 는 입력을 끝까지 읽으므로 SIGPIPE 가 없다.
switch_hits="$(strings "$SRC" 2>/dev/null | grep -c 'SILS_TYPE_PROPAGATION')"
case "$switch_hits" in ''|*[!0-9]*) switch_hits=0 ;; esac
if [ "$switch_hits" -eq 0 ]; then
    echo "SKIP: 이 sils_ego_source 는 SILS_TYPE_PROPAGATION 을 모른다 (구버전 바이너리)."
    echo "      A/B 가 성립하지 않는다.  ./build.sh --build-sils 로 다시 만들 것."
    exit 2
fi

# ---------------------------------------------------------------- 기준선

# ⚠ 로그를 지우지 말 것. 컨테이너가 리다이렉트로 붙잡고 있어서 지우면 이후 기록이
#   통째로 사라진다(2026-08-12 에 측정 하나를 이렇게 날렸다). 반드시 기준선/델타로 잰다.
echo "==> 기준선 (도메인 ${DOMAIN})"
declare -A base_err base_seg base_cid
for p in "${parts[@]}"; do
    f="${LOG_DIR}/${p}_log.txt"
    base_err[$p]="$(count_in "$f" 'invalid type object')"
    base_seg[$p]="$(count_in "$f" 'Segmentation fault')"
    base_cid[$p]="$(cid_of "adsw-${p}-${DOMAIN}")"
    printf '    %-11s err=%-6s segv=%-3s cid=%s\n' \
        "$p" "${base_err[$p]}" "${base_seg[$p]}" "${base_cid[$p]}"
done

# ---------------------------------------------------------------- 투입

LIBDIR="${REPO_ROOT}/output/x86_64/lib"     # ⚠ 빼면 vECU 계열이 1초 만에 죽는다
OUT="${REPO_ROOT}/runs/ego-source-typeprop-${MODE}.jsonl"
mkdir -p "${REPO_ROOT}/runs"

echo
echo "==> sils_ego_source 투입  (type_propagation=${MODE}, ${WATCH}s)"
[ "$MODE" = "enabled" ] && echo "    ⚠ 미패치 재현 모드 — TypeObject 를 실제로 와이어에 올린다"

SILS_TYPE_PROPAGATION="$MODE" \
VISS_DDS_DOMAIN_ID="$DOMAIN" \
LD_LIBRARY_PATH="${LIBDIR}:${LD_LIBRARY_PATH:-}" \
    "$SRC" --rate-ms "$RATE_MS" --count $(( WATCH * 1000 / RATE_MS )) \
    > "$OUT" 2>&1 &
SRC_PID=$!
echo "    pid ${SRC_PID}  log ${OUT#"${REPO_ROOT}/"}"

# ---------------------------------------------------------------- 나중에 합류하는 리더
#
# ★ 이게 크래시 재현의 핵심 변형이다.
#
# 2026-08-10 에 실제로 segfault 한 것은 **파티션이 아니라 우리 쪽 Cyclone 리더**였다
# (어댑터 로그와 순수 rclpy stub 로그 양쪽에 `[ros2run]: Segmentation fault`).
# 파티션은 이미 자리를 잡고 도는 리더라서, "이미 오염된 도메인에 **새로** 합류하는 리더"
# 라는 조건이 빠져 있다. 그 조건을 여기서 만든다:
#
#     sils_ego_source 가 TypeObject 를 뿌리는 중  →  ${READER_DELAY}s 후  →  리더 합류
#
# ⚠ `--rm` 을 쓰지 않는다. 죽었을 때 **종료코드를 봐야** 하는데 --rm 이면 컨테이너가
#   흔적 없이 사라져서 "사라짐" 과 "정상 종료" 를 구별할 수 없다. 끝나고 직접 지운다.
#   (파티션이 --rm 이라 사망 진단이 어려웠던 것과 같은 함정이다.)
reader_started=0
if [ "$JOIN_READER" -eq 1 ]; then
    sleep "$READER_DELAY"
    docker rm -f "$READER_NAME" >/dev/null 2>&1 || true
    if docker run -d --name "$READER_NAME" --net=host --ipc=host \
            -e "ROS_DOMAIN_ID=${DOMAIN}" \
            -e "RMW_IMPLEMENTATION=rmw_cyclonedds_cpp" \
            -e "ROS_LOCALHOST_ONLY=0" \
            -e "VISS_DDS_DOMAIN_ID=${DOMAIN}" \
            -e "SILS_DURABILITY=transient-local" \
            "$READER_IMAGE" ros2 run ss2_ego_bridge "$READER_EXE" >/dev/null 2>&1; then
        reader_started=1
        echo "    리더 합류: ${READER_NAME} (${READER_EXE}, Cyclone, 오염 ${READER_DELAY}s 뒤)"
    else
        echo "    ⚠ 리더 기동 실패 — 이미지 ${READER_IMAGE} 확인. 리더 없이 계속한다."
    fi
    sleep $(( WATCH - READER_DELAY ))
else
    sleep "$WATCH"
fi

# ---------------------------------------------------------------- 판정

src_alive=no
kill -0 "$SRC_PID" 2>/dev/null && src_alive=yes
kill "$SRC_PID" 2>/dev/null

echo
echo "==> 결과"
printf '    %-11s %8s %8s %8s  %s\n' 파티션 에러증가 segv증가 생존 "컨테이너ID"
total_err=0; total_seg=0; died=0; replaced=0
for p in "${parts[@]}"; do
    f="${LOG_DIR}/${p}_log.txt"
    e="$(count_in "$f" 'invalid type object')"; d_err=$(( e - ${base_err[$p]} ))
    s="$(count_in "$f" 'Segmentation fault')"; d_seg=$(( s - ${base_seg[$p]} ))
    now="$(cid_of "adsw-${p}-${DOMAIN}")"
    if [ -z "$now" ]; then                       live="사라짐"; died=$((died+1))
    elif [ "$now" != "${base_cid[$p]}" ]; then   live="교체됨"; replaced=$((replaced+1))
    else                                          live="동일"
    fi
    total_err=$(( total_err + d_err )); total_seg=$(( total_seg + d_seg ))
    printf '    %-11s %8s %8s %8s  %s\n' "$p" "$d_err" "$d_seg" "$live" "${now:-—}"
done

echo
echo "    A. 오염   invalid type object  +${total_err}"
echo "    B. 사망   segfault +${total_seg} · 사라짐 ${died}/3 · 교체 ${replaced}/3"
echo "    C. 가해자 sils_ego_source 생존: ${src_alive}"

reader_verdict=""
if [ "$reader_started" -eq 1 ]; then
    r_state="$(docker inspect -f '{{.State.Status}}' "$READER_NAME" 2>/dev/null)"
    r_code="$(docker inspect -f '{{.State.ExitCode}}' "$READER_NAME" 2>/dev/null)"
    r_segv="$(docker logs "$READER_NAME" 2>&1 | grep -c 'Segmentation fault')"
    case "$r_segv" in ''|*[!0-9]*) r_segv=0 ;; esac
    r_err="$(docker logs "$READER_NAME" 2>&1 | grep -c 'invalid type object')"
    case "$r_err" in ''|*[!0-9]*) r_err=0 ;; esac
    echo "    D. 리더   ${READER_EXE}: ${r_state} (exit ${r_code}) · segfault ${r_segv} · 오염 ${r_err}"
    # 종료코드 245 = 08-10/08-11 에 관측된 그 값. 139(=128+SIGSEGV) 도 같은 계열이다.
    if [ "$r_segv" -gt 0 ] || [ "$r_code" = "245" ] || [ "$r_code" = "139" ]; then
        reader_verdict="crash"
    elif [ "$r_state" = "running" ]; then
        reader_verdict="alive"
    else
        reader_verdict="exited"
    fi
    docker logs "$READER_NAME" > "${REPO_ROOT}/runs/reader-${MODE}.log" 2>&1 || true
    docker rm -f "$READER_NAME" >/dev/null 2>&1 || true
fi
echo

# 판정문은 **명제를 분리해서** 쓴다. "안 죽었다" 를 "문제없다" 로 읽지 않게.
if [ "$MODE" = "enabled" ]; then
    [ "$total_err" -gt 0 ] \
        && echo "  → 오염 재현됨. 우리 타입도 MMF 타입과 같이 Cyclone 로그를 더럽힌다." \
        || echo "  → 오염 재현 안 됨. 순서(파티션 먼저)와 스위치 적용을 먼저 의심할 것."
    if [ "$total_seg" -gt 0 ] || [ "$died" -gt 0 ]; then
        echo "  → ★ 파티션 사망 재현됨. 2026-08-11 의 'decision segfault' 주장이 지지된다."
    else
        echo "  → ★ 파티션 사망 재현 안 됨 (${WATCH}s). 오염은 사실이나 **죽인다는 주장은 미지지**."
        echo "       MMF 미패치 때와 같은 결과다 — 오염 ≠ 사망."
    fi
    case "$reader_verdict" in
        crash) echo "  → ★★ 나중에 합류한 Cyclone 리더가 죽었다. 2026-08-10 의 어댑터 segfault 와 일치." ;;
        alive) echo "  → 나중에 합류한 Cyclone 리더도 멀쩡하다. 크래시 주장은 이 조건에서도 미지지." ;;
        exited) echo "  → 리더가 크래시 아닌 사유로 종료했다. runs/reader-${MODE}.log 를 볼 것 (판정 보류)." ;;
    esac
else
    [ "$total_err" -eq 0 ] \
        && echo "  → 대조군 정상: 패치본은 오염 0. 위 --unpatched 결과와의 차이가 곧 효과다." \
        || echo "  → ⚠ 패치본인데 오염이 났다. 다른 미패치 참가자(MMF 등)가 도메인에 있는지 볼 것."
fi
