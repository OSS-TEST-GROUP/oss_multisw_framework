#!/bin/bash -

export LD_LIBRARY_PATH=/oss/lib

# 아치별 기본 서비스 목록: amd64(x86_64) 는 hmi_launcher 를 GTK/X11 인자로 띄우는
# agl-service-list.x86.yaml, arm64/AGL 보드는 flutter-auto/Wayland 기본을 쓴다.
# ADSW_CONFIG 를 명시(또는 파일 마운트)하면 항상 그게 우선한다.
DEFAULT_ADSW_CONFIG=/oss/bin/config/agl-service-list.yaml
if [ "$(uname -m)" = "x86_64" ] && [ -f /oss/bin/config/agl-service-list.x86.yaml ]; then
  DEFAULT_ADSW_CONFIG=/oss/bin/config/agl-service-list.x86.yaml
fi
export ADSW_CONFIG=${ADSW_CONFIG:-$DEFAULT_ADSW_CONFIG}
export OTA_CLIENT_CONFIG=${OTA_CLIENT_CONFIG:-/oss/bin/config/ota_client_config.json}
echo "[agl_entrypoint] arch=$(uname -m)  ADSW_CONFIG=$ADSW_CONFIG"

# 호스트에 oss-domain-apply 설치 (docker.sock 경유, 실패는 무시).
# 도메인 변경 반영은 호스트에서 돌아야 하고, 이미지에만 담으면 재플래싱까지 미뤄진다.
# 자세한 근거는 install-domain-apply.sh 헤더 참조.
[ -x /oss/install-domain-apply.sh ] && /oss/install-domain-apply.sh /oss/host-install || true

/oss/bin/agl_ota_client &

exec /oss/bin/agl_adsw_manager
