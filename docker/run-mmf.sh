#!/bin/bash
# =============================================================================
# run-mmf.sh — arm64 실보드에서 core + ext-ros2 런타임 컨테이너 기동
#
#   - registry.obigo.com 의 :latest 이미지 사용 (CORE_IMG/EXTROS2_IMG 로 오버라이드)
#   - --restart unless-stopped + docker 데몬 enabled → 부팅 시 자동 재기동
#   - core → (DDS 준비 대기) → ext-ros2 순서로 기동
#   - 재기동 시 stale /dev/shm/fastdds_* 세그먼트 청소 (SHM 좀비 → 재등록 오염 방지)
#
# 배포: 이 스크립트를 보드로 복사해 실행한다.
#   scp docker/run-mmf.sh <user>@<board>:~/
#   ssh <user>@<board> 'bash ~/run-mmf.sh'
#
# 사용 예:
#   bash run-mmf.sh                          # domain 0, :latest
#   ROS_DOMAIN_ID=1 bash run-mmf.sh          # 보드별 DDS domain 격리 (전 스택 동일 도메인)
#   EXTROS2_IMG=oss-mmf-ext-ros2:diag2-local bash run-mmf.sh   # 로컬 이미지 오버라이드
#   SAVED_SERVICE_LIST=~/adsw-service-list.yaml bash run-mmf.sh  # ext-ros2 서비스목록 bind-mount
#
# ⚠️ ext-ros2 는 반드시 아래 옵션과 함께 기동해야 한다:
#   -v /var/run/docker.sock + --group-add <docker-gid>
#     → 없으면 adsw_manager 가 컨테이너(autoware 등)를 docker run 하지 못한다.
#       StartService 가 lifecycle 레벨에선 "Success" 로 보여도 실제 생성은 실패한다.
#   운영 상세: docs/ext-ros2-adsw-operation-guide.md
# =============================================================================
set -e

CORE_IMG="${CORE_IMG:-registry.obigo.com/oss/oss-mmf-core:latest}"
EXTROS2_IMG="${EXTROS2_IMG:-registry.obigo.com/oss/oss-mmf-ext-ros2:latest}"

# 보드별 DDS 도메인 격리. ROS_DOMAIN_ID 는 ROS2 노드/rmw 만 격리하므로, raw Fast-DDS
# 를 쓰는 lifecycle_mgr/diagnostics/adsw/ota(MMF_DDS_DOMAIN_ID) 와 vehicle_info/viss
# (VISS_DDS_DOMAIN_ID) 도 같은 값으로 맞춰야 전 스택이 실제로 한 도메인에 격리된다.
#
# 도메인 결정 우선순위: ① 명시한 ROS_DOMAIN_ID env  ② 도메인 파일  ③ 기본값 42.
# 도메인 파일(`ROS_DOMAIN_ID=<n>` 한 줄, compose `--env-file` 과 같은 형식)은
# HMI 런처 Settings 가 기록한다 → **다음 기동 때** 반영된다(ROS_DOMAIN_ID 는 프로세스
# 시작 시점에만 읽히므로 실행 중 변경은 불가).
# 기본값 42 인 이유: Autoware adsw 파티션(saved-service-list.yaml)이 42 를 쓰므로,
# 기본이 0 이면 adsw_monitor 가 구독만 하고 발행자가 없어 화면이 전부 null 이 된다(실측).
MMF_DOMAIN_ENV_FILE="${MMF_DOMAIN_ENV_FILE:-/opt/mmf/config/domain.env}"
_file_domain=""
if [ -f "$MMF_DOMAIN_ENV_FILE" ]; then
  _file_domain="$(sed -n 's/^[[:space:]]*ROS_DOMAIN_ID[[:space:]]*=[[:space:]]*\([0-9]\{1,3\}\).*/\1/p' \
                    "$MMF_DOMAIN_ENV_FILE" | tail -1)"
fi
DOMAIN="${ROS_DOMAIN_ID:-${_file_domain:-42}}"
# ROS 2 도메인 유효 범위는 0~232. 벗어나면 조용히 잘못 도는 대신 기본값으로 되돌린다.
if ! [ "$DOMAIN" -ge 0 ] 2>/dev/null || [ "$DOMAIN" -gt 232 ]; then
  echo "[run-mmf] ⚠️ 잘못된 도메인 '$DOMAIN' → 42 사용 (유효 범위 0~232)" >&2
  DOMAIN=42
fi

# ContainerController 등이 `docker compose -f <file> up -d` 를 `--env-file` 없이
# 호출하면 compose 는 **compose 파일 옆의 `.env`** 만 읽는다. 도메인 파일이 있으면
# 그 심링크를 만들어 두 경로(run 스크립트/compose)가 같은 값을 쓰게 한다(best-effort).
_compose_dir="$(cd "$(dirname "$0")/compose" 2>/dev/null && pwd || true)"
if [ -f "$MMF_DOMAIN_ENV_FILE" ] && [ -n "$_compose_dir" ] && [ -w "$_compose_dir" ]; then
  ln -sfn "$MMF_DOMAIN_ENV_FILE" "$_compose_dir/.env" 2>/dev/null \
    && echo "[run-mmf] compose .env → $MMF_DOMAIN_ENV_FILE"
fi
DOCKER_GID="$(getent group docker | cut -d: -f3)"

echo "[run-mmf] core=$CORE_IMG"
echo "[run-mmf] ext-ros2=$EXTROS2_IMG  domain=$DOMAIN  docker_gid=$DOCKER_GID"

# 최신 이미지 확보. registry 인증이 되어 있으면 pull, 로컬 전용 태그(:*-local)나
# 오프라인/미인증이면 스킵하고 로컬 이미지로 진행 (PULL=0 으로 완전 비활성 가능).
if [ "${PULL:-1}" = "1" ]; then
  for img in "$CORE_IMG" "$EXTROS2_IMG"; do
    if docker pull "$img" >/dev/null 2>&1; then
      echo "[run-mmf] pulled $img"
    else
      echo "[run-mmf] pull 스킵/실패 → 로컬 이미지 사용: $img"
    fi
  done
fi

# 기존 컨테이너 정리 (hook 정책상 rm -f 대신 stop+rm)
for c in oss-mmf-core oss-mmf-ext-ros2; do
  docker stop "$c" 2>/dev/null || true
  docker rm "$c" 2>/dev/null || true
done

# stale-SHM 청소 (컨테이너 정지 상태 → 남은 세그먼트는 orphan)
find /dev/shm -maxdepth 1 \
  \( -name 'fastdds_*' -o -name 'fast_datasharing_*' -o -name 'sem.fastdds_*' \) \
  -delete 2>/dev/null || true
echo "[run-mmf] stale-SHM sweep done"

echo "[run-mmf] starting oss-mmf-core ..."
docker run -d --name oss-mmf-core \
  --restart unless-stopped \
  --privileged --network host \
  --cap-add NET_RAW --cap-add NET_ADMIN \
  -v /dev/shm:/dev/shm \
  -e ROS_DOMAIN_ID="$DOMAIN" \
  -e MMF_DDS_DOMAIN_ID="$DOMAIN" \
  -e VISS_DDS_DOMAIN_ID="$DOMAIN" \
  -e HEARTBEAT_INTERVAL_SEC=3 \
  "$CORE_IMG"

echo "[run-mmf] core 기동, DDS 준비 대기(12s) ..."
sleep 12

# ext-ros2 서비스목록을 보드 로컬 yaml 로 오버라이드하려면 SAVED_SERVICE_LIST 지정
SVC_MOUNT=()
if [ -n "${SAVED_SERVICE_LIST:-}" ] && [ -f "${SAVED_SERVICE_LIST}" ]; then
  SVC_MOUNT=(-v "${SAVED_SERVICE_LIST}:/oss/bin/config/saved-service-list.yaml:ro")
  echo "[run-mmf] ext-ros2 saved-service-list 오버라이드: ${SAVED_SERVICE_LIST}"
fi

echo "[run-mmf] starting oss-mmf-ext-ros2 ..."
docker run -d --name oss-mmf-ext-ros2 \
  --restart unless-stopped \
  --privileged --network host \
  --cap-add NET_RAW --cap-add NET_ADMIN \
  --group-add "$DOCKER_GID" \
  -v /dev/shm:/dev/shm \
  -v /var/run/docker.sock:/var/run/docker.sock \
  ${SVC_MOUNT[@]+"${SVC_MOUNT[@]}"} \
  -e ROS_DOMAIN_ID="$DOMAIN" \
  -e MMF_DDS_DOMAIN_ID="$DOMAIN" \
  "$EXTROS2_IMG"

echo "[run-mmf] done."
docker ps --format "  {{.Names}}\t{{.Image}}\t{{.Status}}" | grep -E "oss-mmf"
