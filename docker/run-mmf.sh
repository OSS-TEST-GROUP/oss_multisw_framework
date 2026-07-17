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
DOMAIN="${ROS_DOMAIN_ID:-0}"
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
  "${SVC_MOUNT[@]}" \
  -e ROS_DOMAIN_ID="$DOMAIN" \
  -e MMF_DDS_DOMAIN_ID="$DOMAIN" \
  "$EXTROS2_IMG"

echo "[run-mmf] done."
docker ps --format "  {{.Names}}\t{{.Image}}\t{{.Status}}" | grep -E "oss-mmf"
