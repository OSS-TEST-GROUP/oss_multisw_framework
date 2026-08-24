#!/usr/bin/env bash
#
# check_mmf_typeobject.sh -- MMF 가 같은 도메인의 Cyclone 로그를 오염시키는지 판정한다.
#
# ★ 판정은 MMF 가 아니라 **파티션 로그의 에러 건수**로 한다.
#
# TypeObject 전파의 실패는 **리더에서** 난다. 규칙을 어긴 프로세스는 멀쩡하고 남의
# 로그가 더러워진다. "MMF 가 잘 뜬다" 는 아무것도 증명하지 않는다.
# 근거: docs/sils/evidence/mmf-typeobject-2026-08-12.md
#
# ⚠ **파티션 생존은 판정 기준이 아니다.** 미패치 MMF 로도 파티션은 안 죽는다
#   (실측: 1260건 나면서 160초 생존). 초판은 사망을 이 문제 탓으로 돌렸는데 틀렸고,
#   진짜 사망 원인은 중복된 run_partitions.sh 의 EXIT 트랩이었다(그 문서 §4).
#   그래서 여기서는 파티션이 살아 있는지는 **전제 확인**이고, 판정은 에러 건수로만 한다.
#
# 이 스크립트가 보는 것
#   1) Autoware 파티션 3종이 살아 있나            (전제. 없으면 잴 것이 없다)
#   2) 로그에 `invalid type object` 가 몇 건인가   ★ 판정 (0 이어야 한다)
#   3) MMF 참가자가 실제로 도메인에 있나            (없으면 시험 자체가 성립 안 한다)
#
# ⚠ 3) 이 중요하다. MMF 를 안 띄우고 돌리면 당연히 0 이 나오고 "고쳤다" 로 오독된다.
#    그래서 MMF 가 없으면 PASS 를 주지 않고 SKIP 을 낸다.
#
# ⚠⚠ **순서가 결과를 바꾼다 — 이것도 거짓 PASS 의 원인이다.**
#    에러는 리더-라이터가 매칭되는 시점의 타입 교환에서 난다. 그래서
#      MMF 가 먼저 떠 있고 파티션이 나중에 뜨면  →  0건   (미패치인데도!)
#      파티션이 먼저 떠 있고 MMF 가 나중에 오면  →  수백~천 건
#    실측 2026-08-12, 같은 미패치 이미지로 0건 / 354건 둘 다 나왔다.
#    그러므로 **--restart-mmf 로 MMF 를 파티션 뒤에 다시 넣고 재는 것이 정직한 시험**이다.
#    그냥 돌리면 "지금 이 상태에 에러가 쌓였나" 만 보는 것이고, 그건 약한 확인이다.
#
# Usage:
#   tools/sils/ss2/check_mmf_typeobject.sh --restart-mmf   # ★ 정직한 시험 (권장)
#   tools/sils/ss2/check_mmf_typeobject.sh                 # 지금 쌓인 것만 (약한 확인)
#   tools/sils/ss2/check_mmf_typeobject.sh --watch 120     # 120초 관찰 후 판정
# Exit: 0 PASS · 1 FAIL · 2 SKIP(전제 미충족)

set -uo pipefail

LOG_DIR="${ADSW_LOG_DIR:-$HOME/oss/oss_adsw/log}"
WATCH=0
RESTART=0

while [ "${1:-}" != "" ]; do
    case "$1" in
        --watch)   WATCH="$2"; shift ;;
        --restart-mmf) RESTART=1 ;;
        --log-dir) LOG_DIR="$2"; shift ;;
        --help|-h) sed -n '2,26p' "${BASH_SOURCE[0]}"; exit 0 ;;
        *) echo "check_mmf_typeobject: unknown option: $1" >&2; exit 2 ;;
    esac
    shift
done

parts=(perception decision control)

# ⚠ `grep -c` 는 0건일 때 "0" 을 출력하고 **exit 1** 이다. `|| echo 0` 을 붙이면
# "0" 이 두 줄이 되어 산술이 깨진다(실제로 밟았다). 종료코드는 버리고 출력만 쓴다.
count_in() {  # <파일>
    local c
    c=$(grep -c 'invalid type object' "$1" 2>/dev/null)
    case "$c" in ''|*[!0-9]*) echo 0 ;; *) echo "$c" ;; esac
}
alive() { docker ps --format '{{.Names}}' | grep -c "^adsw-.*-" ; }
errs()  { local n=0 c; for p in "${parts[@]}"; do
            c=$(count_in "${LOG_DIR}/${p}_log.txt"); n=$((n + c)); done; echo "$n"; }

mmf_up=$(docker ps --format '{{.Names}}' | grep -c 'mmf' || true)

echo "==> 전제"
printf '    MMF 컨테이너      %s\n' "$([ "$mmf_up" -gt 0 ] && echo '있음' || echo '없음  ← 시험 성립 안 함')"
printf '    파티션 살아있음    %s / 3\n' "$(alive)"
printf '    로그 디렉터리     %s\n' "$LOG_DIR"

if [ "$mmf_up" -eq 0 ]; then
    echo
    echo "SKIP: MMF 가 떠 있지 않다. 이 상태의 0건은 고쳐졌다는 증거가 아니다."
    exit 2
fi

if [ "$RESTART" = "1" ]; then
    # ★ 파티션이 이미 도는 도메인에 MMF 를 나중에 넣는다 = 에러가 나는 조건.
    # 이걸 안 하면 MMF 가 먼저 떠 있던 rig 에서는 미패치라도 0건이 나온다.
    echo
    echo "==> MMF 를 파티션 뒤로 재투입 (에러가 나는 조건을 만든다)"
    before=$(errs)
    docker restart oss-mmf-core >/dev/null 2>&1 || { echo "    ⛔ oss-mmf-core 재기동 실패" >&2; exit 2; }
    sleep 25
    echo "    재투입 전 ${before}건 → 후 $(errs)건"
fi

if [ "$WATCH" -gt 0 ]; then
    echo
    echo "==> ${WATCH}초 관찰 (크래시는 기동이 아니라 리더-라이터 매칭 시점에 난다)"
    before=$(errs)
    for i in $(seq 1 "$WATCH"); do
        sleep 1
        [ $((i % 15)) -eq 0 ] && printf '    %3ds  파티션 %s/3  에러 %s\n' "$i" "$(alive)" "$(errs)"
    done
    echo "    관찰 전 ${before}건 → 후 $(errs)건"
fi

echo
echo "==> 판정"
n=$(errs); a=$(alive)
for p in "${parts[@]}"; do
    f="${LOG_DIR}/${p}_log.txt"
    if [ -f "$f" ]; then printf '    %-12s %s 건\n' "$p" "$(count_in "$f")"
    else                printf '    %-12s (로그 없음)\n' "$p"; fi
done

if [ "$a" -lt 3 ]; then
    echo
    echo "SKIP: 파티션이 ${a}/3 뿐이다. 잴 대상이 없다."
    echo "      ⚠ 파티션이 흔적 없이 사라졌다면 이 문제가 아니라 run_partitions.sh 의"
    echo "        EXIT 트랩을 의심하라:  pgrep -cf '[r]un_partitions.sh'"
    exit 2
fi
if [ "$n" -eq 0 ]; then
    echo
    echo "PASS: MMF 가 도메인에 있는데 invalid type object 0건."
    exit 0
fi
echo
echo "FAIL: invalid type object ${n}건. MMF 가 TypeObject 를 전파하고 있다."
echo "      패치된 이미지인지 확인:"
echo "        docker exec oss-mmf-core sh -c 'strings /oss/bin/viss-server | grep -c fastdds.type_propagation'"
echo "      (0 이면 미패치. 1 이상이면 다른 참가자를 봐야 한다.)"
exit 1
