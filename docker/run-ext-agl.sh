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
# amd64(x86_64) PC — 범용 실행:
#   adsw 가 hmi_launcher 를 GTK/X11 로 띄운다. 이 스크립트가 GPU(nvidia/mesa 자동)·
#   DISPLAY·uid 를 감지해 hmi_launcher config(agl-service-list.x86.yaml)를 호스트에
#   맞게 생성·마운트하므로 어느 x86 PC 에서도 별도 편집 없이 실행된다.
#   오버라이드: HMI_GPU=auto|nvidia|mesa|none, DISPLAY=:N
#
# ⚠️ docker.sock + --group-add <docker-gid> 가 없으면 adsw_manager 가 hmi_launcher
#    를 docker run 하지 못한다 (ext-ros2 와 동일한 전제조건).
# =============================================================================
set -e

IMAGE="${IMAGE:-registry.obigo.com/oss/oss-mmf-ext-agl:latest}"
DOMAIN="${ROS_DOMAIN_ID:-0}"
DOCKER_GID="$(getent group docker | cut -d: -f3)"
HMI_GPU="${HMI_GPU:-auto}"

# HMI 런타임 사용자 uid (호스트) — 컨테이너 내부 uid 1000 으로 매핑된다.
# XDG_RUNTIME_DIR(웨이랜드/컴포지터 소켓) 공유용. arm64 보드는 1001, x86 은 현재 uid.
if [ "$(uname -m)" = "x86_64" ]; then
  HMI_RUNTIME_UID="${HMI_RUNTIME_UID:-$(id -u)}"
else
  HMI_RUNTIME_UID="${HMI_RUNTIME_UID:-1001}"
fi

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

# amd64(x86_64) 범용 실행: 호스트 환경(GPU/DISPLAY/uid)을 감지해 hmi_launcher
# config 를 생성하고 컨테이너의 x86 config 위에 마운트한다. baked 기본값
# (--gpus all/:0/uid1000)에 얽매이지 않고 어느 x86 PC 에서도 동작하게 한다.
# entrypoint 가 x86_64 에서 agl-service-list.x86.yaml 을 자동 선택하므로 그 경로에
# 마운트하면 된다. arm64 보드(Wayland/flutter-auto)는 이 블록을 타지 않는다.
X86_MOUNT=()
if [ "$(uname -m)" = "x86_64" ]; then
  # GPU 감지: nvidia > mesa(/dev/dri) > software GL
  gpu="$HMI_GPU"
  if [ "$gpu" = "auto" ]; then
    if command -v nvidia-smi >/dev/null 2>&1 && docker info 2>/dev/null | grep -qiE "Runtimes:.*nvidia|nvidia"; then
      gpu="nvidia"
    elif [ -e /dev/dri ]; then gpu="mesa"; else gpu="none"; fi
  fi
  case "$gpu" in
    nvidia) GPU_YAML=$'  - "--gpus"\n  - "all"' ;;
    mesa)   GPU_YAML=$'  - "--device"\n  - "/dev/dri:/dev/dri"' ;;
    *)      GPU_YAML="" ;;
  esac
  DISP="${DISPLAY:-:0}"
  UIDN="$(id -u)"
  GEN_X86=/tmp/agl-service-list.x86.gen.yaml
  cat > "$GEN_X86" <<YAML
services:
- name: adsw_monitor
  service_type: NORMAL
  recovery_policy: ALWAYS
  is_enabled: true
  mw_type: AGL
  init_state: NOT_RUNNING
- name: ota_monitor
  service_type: NORMAL
  recovery_policy: R_DEFAULT
  is_enabled: true
  mw_type: AGL
  init_state: NOT_RUNNING
- name: lfc_monitor
  service_type: NORMAL
  recovery_policy: R_DEFAULT
  is_enabled: true
  mw_type: AGL
  init_state: NOT_RUNNING
- name: hmi_launcher
  service_type: NORMAL
  recovery_policy: ALWAYS
  is_enabled: true
  mw_type: AGL
  init_state: NOT_RUNNING
  image: registry.obigo.com/oss/oss-hmi-launcher:latest
  extra_args:
  - "-it"
  - --net=host
  - --privileged
  - --restart
  - unless-stopped
  - --cap-add
  - NET_RAW
  - --cap-add
  - NET_ADMIN
${GPU_YAML}
  - "-e"
  - DISPLAY=${DISP}
  - "-e"
  - XDG_RUNTIME_DIR=/run/user/${UIDN}
  - "-e"
  - LAUNCHER_BOT_ENABLED=0
  - "-v"
  - /tmp/.X11-unix:/tmp/.X11-unix
  - "-v"
  - /run/user/${UIDN}:/run/user/${UIDN}
  - "-v"
  - /usr/share/fonts:/usr/share/fonts:ro
  - "-v"
  - /etc/fonts:/etc/fonts:ro
YAML
  X86_MOUNT=(-v "${GEN_X86}:/oss/bin/config/agl-service-list.x86.yaml:ro")
  echo "[run-ext-agl] amd64 hmi_launcher config 생성: gpu=$gpu display=$DISP uid=$UIDN"
  if command -v xhost >/dev/null 2>&1; then
    xhost +local:root >/dev/null 2>&1 || xhost +local: >/dev/null 2>&1 || true
    echo "[run-ext-agl] xhost +local: (hmi_launcher X 접근 허용)"
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
  "${X86_MOUNT[@]}" \
  -e ROS_DOMAIN_ID="$DOMAIN" \
  -e MMF_DDS_DOMAIN_ID="$DOMAIN" \
  "$IMAGE"

echo "[run-ext-agl] done — oss-mmf-ext-agl 시작됨 (adsw_manager + ota_client 내장)."
echo "               로그: docker logs -f oss-mmf-ext-agl"
docker ps --format "  {{.Names}}\t{{.Image}}\t{{.Status}}" | grep -E "oss-mmf-ext-agl" || true
