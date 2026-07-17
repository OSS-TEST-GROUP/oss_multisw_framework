#!/bin/bash
# =============================================================================
# run-ext-agl.sh — arm64 HMI 보드에서 ext-agl 런타임 컨테이너 기동
#
# 컨테이너 안에서 agl_entrypoint.sh 가 agl_ota_client(&) + agl_adsw_manager 를
# 실행한다. docker.sock(DooD)로 호스트 docker 데몬을 통해 hmi_launcher 를 sibling
# 컨테이너로 start/stop/recovery 관리한다. (docker/compose/docker-compose-agl.yaml
# 의 설계를 보드 기동 스크립트로 옮긴 것)
#
# 배포:
#   scp docker/run-ext-agl.sh <user>@<hmi-board>:~/
#   ssh <user>@<hmi-board> 'bash ~/run-ext-agl.sh'
#
# 사용 예:
#   bash run-ext-agl.sh                                  # domain 0, :latest
#   ROS_DOMAIN_ID=1 bash run-ext-agl.sh                  # 보드별 DDS domain 격리
#   IMAGE=oss-mmf-ext-agl:arm64-local bash run-ext-agl.sh  # 로컬 이미지 오버라이드
#   HMI_RUNTIME_UID=1001 bash run-ext-agl.sh             # HMI 사용자 uid(호스트) 지정
#
# ⚠️ docker.sock + --group-add <docker-gid> 가 없으면 adsw_manager 가 hmi_launcher
#    를 docker run 하지 못한다 (ext-ros2 와 동일한 전제조건).
# =============================================================================
set -e

IMAGE="${IMAGE:-registry.obigo.com/oss/oss-mmf-ext-agl:latest}"
DOMAIN="${ROS_DOMAIN_ID:-0}"
DOCKER_GID="$(getent group docker | cut -d: -f3)"

# HMI 런타임 사용자 uid (호스트) — 컨테이너 내부 uid 1000 으로 매핑된다.
# XDG_RUNTIME_DIR(웨이랜드/컴포지터 소켓) 공유용. 보드 HMI 계정 uid 에 맞춰 조정.
HMI_RUNTIME_UID="${HMI_RUNTIME_UID:-1001}"

# ext-agl 서비스목록(hmi_launcher 등)을 보드 로컬 yaml 로 오버라이드
SVC_LIST="${AGL_SERVICE_LIST:-/opt/adsw/config/agl-service-list.yaml}"

echo "[run-ext-agl] image=$IMAGE  domain=$DOMAIN  docker_gid=$DOCKER_GID  hmi_uid=$HMI_RUNTIME_UID"

# 최신 이미지 확보 (registry 인증 필요; 로컬 태그/오프라인/미인증이면 스킵).
if [ "${PULL:-1}" = "1" ]; then
  if docker pull "$IMAGE" >/dev/null 2>&1; then
    echo "[run-ext-agl] pulled $IMAGE"
  else
    echo "[run-ext-agl] pull 스킵/실패 → 로컬 이미지 사용: $IMAGE"
  fi
fi

# 기존 컨테이너 정리 (hook 정책상 rm -f 대신 stop+rm)
docker stop oss-mmf-ext-agl 2>/dev/null || true
docker rm oss-mmf-ext-agl 2>/dev/null || true

# stale-SHM sweep (보드엔 ext-agl 만 fastdds SHM 사용 → 정지 후 전량 orphan)
rm -f /dev/shm/fastdds_* /dev/shm/fast_datasharing_* 2>/dev/null || true
echo "[run-ext-agl] stale-SHM sweep done"

SVC_MOUNT=()
if [ -f "${SVC_LIST}" ]; then
  SVC_MOUNT=(-v "${SVC_LIST}:/oss/bin/config/agl-service-list.yaml:ro")
  echo "[run-ext-agl] agl-service-list: ${SVC_LIST}"
fi

docker run -d --name oss-mmf-ext-agl \
  --restart unless-stopped \
  --privileged --network host \
  --cap-add NET_RAW --cap-add NET_ADMIN \
  --group-add "$DOCKER_GID" \
  -v /dev/shm:/dev/shm \
  -v "/run/user/${HMI_RUNTIME_UID}:/run/user/1000" \
  -v /var/run/docker.sock:/var/run/docker.sock \
  "${SVC_MOUNT[@]}" \
  -e ROS_DOMAIN_ID="$DOMAIN" \
  -e MMF_DDS_DOMAIN_ID="$DOMAIN" \
  "$IMAGE"

echo "[run-ext-agl] done — oss-mmf-ext-agl 시작됨 (adsw_manager + ota_client 내장)."
echo "               로그: docker logs -f oss-mmf-ext-agl"
docker ps --format "  {{.Names}}\t{{.Image}}\t{{.Status}}" | grep -E "oss-mmf-ext-agl" || true
