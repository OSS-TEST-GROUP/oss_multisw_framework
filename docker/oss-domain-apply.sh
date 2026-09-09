#!/bin/sh
# oss-domain-apply — 바뀐 도메인을 이 호스트의 컨테이너에 반영한다.
#
# 왜 호스트에서 도는가
# --------------------
# 도메인은 **프로세스 시작 시에만** 읽히므로 반영에는 컨테이너 재생성이 필요하다.
# 그런데 재생성해야 할 대상 안에 재생성을 실행할 주체가 들어 있다 (보드 실측 2026-09-08):
#
#   adsw_manager      → oss-mmf-ext-ros2 **안에서** 돈다 (151)
#   agl_adsw_manager  → oss-mmf-ext-agl  **안에서** 돈다 (81)
#
# 그래서 lfc_monitor 의 Stop/Start 로는 이 둘을 재생성할 수 없다 — 명령을 실행하는
# 주체가 함께 죽어 Start 를 수행할 것이 남지 않는다. 실행자가 대상 **밖**에 있어야
# 하므로 이 스크립트는 호스트에서 돈다.
#
# 순서가 중요한 이유
# ------------------
# adsw_manager 는 Autoware 를 `docker run` 할 때 서비스 목록의 `${ROS_DOMAIN_ID:-42}`
# 를 **자기 셸 env** 로 확장한다. 따라서 ext-ros2 를 먼저 새 도메인으로 재생성해야
# 그 다음의 Autoware Start 가 새 도메인을 물려받는다. 순서를 거꾸로 하면 Autoware 는
# 몇 번을 재시작해도 옛 도메인에 머문다 (실측: 2026-09-08 에 3회 재현).
#
#   ① core + ext-ros2 재생성      (151, compose)
#   ② ext-agl 재생성              (81,  compose)
#   ③ Autoware / rviz2            adsw·rviz2_panel 이 만드는 자식 — ①② 이후에 재시작
#
# 사용법
#   oss-domain-apply            # 이 호스트에 있는 플랫폼 컨테이너를 재생성
#   oss-domain-apply --dry-run  # 실행할 명령만 출력
#   oss-domain-apply --children # ③ 까지 시도 (Autoware stop+rm — Start 는 UI/adsw 몫)
#
# 종료코드: 0 성공(또는 이 호스트에 대상 없음) / 1 도메인 파일 없음 / 2 재생성 실패
set -e

DOMAIN_ENV="${DOMAIN_ENV:-/opt/mmf/config/domain.env}"
BOARD_ENV="${BOARD_ENV:-/etc/oss/compose/board.env}"
COMPOSE_DIR_81="${COMPOSE_DIR_81:-/etc/oss/compose}"
COMPOSE_DIR_151="${COMPOSE_DIR_151:-/home/beyhpc/oss}"
DRY=0
CHILDREN=0
for a in "$@"; do
    case "$a" in
        --dry-run)  DRY=1 ;;
        --children) CHILDREN=1 ;;
        -h|--help)  sed -n '2,40p' "$0"; exit 0 ;;
        *) echo "unknown option: $a" >&2; exit 1 ;;
    esac
done

run() {
    if [ "$DRY" = "1" ]; then echo "   [dry-run] $*"; else "$@"; fi
}

if [ ! -f "$DOMAIN_ENV" ]; then
    echo "oss-domain-apply: 도메인 파일이 없다: $DOMAIN_ENV" >&2
    exit 1
fi

# 도메인 파일이 **정본**이다. board.env 를 먼저 읽고 domain.env 를 나중에 읽어,
# board.env 에 남아 있는 ROS_DOMAIN_ID 같은 중복 키를 도메인 파일이 이기게 한다
# (실측: 순서를 반대로 하면 ROS 평면만 옛 값이 되어 반쪽 적용된다).
# 사용자 셸의 오염(예: ~/.bashrc 의 `export ROS_DOMAIN_ID=1`)도 이 순서로 덮인다.
set -a
# shellcheck disable=SC1090
[ -f "$BOARD_ENV" ] && . "$BOARD_ENV"
# shellcheck disable=SC1090
. "$DOMAIN_ENV"
set +a

echo "oss-domain-apply: 목표 도메인 DDS=${DDS_DOMAIN_ID:-?} ROS=${ROS_DOMAIN_ID:-?}"

compose_recreate() {
    dir="$1"; shift
    [ -d "$dir" ] || return 0
    files=""
    for f in docker-compose-integration.yaml docker-compose-agl-board.override.yaml; do
        [ -f "$dir/$f" ] && files="$files -f $dir/$f"
    done
    [ -n "$files" ] || return 0
    echo "▶ 재생성: $* ($dir)"
    # shellcheck disable=SC2086
    run docker compose $files up -d --force-recreate "$@" || return 2
}

RC=0

# ── ① 컴퓨트 보드 (core + ext-ros2) ─────────────────────────────────────────
if docker container inspect oss-mmf-core >/dev/null 2>&1 \
   || docker container inspect oss-mmf-ext-ros2 >/dev/null 2>&1; then
    ( cd "$COMPOSE_DIR_151" 2>/dev/null && compose_recreate "$COMPOSE_DIR_151" mmf-core mmf-ext-ros2 ) || RC=2
fi

# ── ② HMI 보드 (ext-agl) ────────────────────────────────────────────────────
if docker container inspect oss-mmf-ext-agl >/dev/null 2>&1; then
    ( cd "$COMPOSE_DIR_81" 2>/dev/null && compose_recreate "$COMPOSE_DIR_81" mmf-ext-agl ) || RC=2
fi

# ── ③ 자식 컨테이너 ─────────────────────────────────────────────────────────
# Autoware 는 adsw_manager 가, rviz2 는 rviz2_panel 이 만든다. 인자의 정본이 각각
# adsw 서비스 목록과 패널 설정이므로 **여기서 docker run 하지 않는다** — 그러면
# 정본이 둘로 갈라진다. 대신 지워 두고, 재생성은 그 소유자에게 맡긴다.
if [ "$CHILDREN" = "1" ]; then
    for c in autoware-perception autoware-decision autoware-control rviz2; do
        if docker container inspect "$c" >/dev/null 2>&1; then
            echo "▶ 제거(소유자가 재생성): $c"
            run docker stop "$c" >/dev/null 2>&1 || true
            run docker rm "$c" >/dev/null 2>&1 || true
        fi
    done
    echo "ℹ️  Autoware 는 lfc_monitor 에서 Start, rviz2 는 패널에서 Start 하면"
    echo "   방금 재생성된 플랫폼 컨테이너의 도메인(${ROS_DOMAIN_ID:-?})을 물려받는다."
fi

if [ "$RC" != "0" ]; then
    echo "oss-domain-apply: 일부 재생성이 실패했다" >&2
    exit "$RC"
fi

echo "✅ oss-domain-apply 완료 — 이 호스트의 플랫폼 컨테이너가 도메인 ${ROS_DOMAIN_ID:-?} 로 재생성됨"
