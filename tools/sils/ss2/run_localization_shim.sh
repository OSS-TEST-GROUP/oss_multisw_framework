#!/usr/bin/env bash
#
# run_localization_shim.sh -- localization_initialize_shim.py 를 제어 파티션 컨테이너 안에서 띄운다.
#
# 왜 컨테이너 안인가: rclpy 와 tier4_localization_msgs 가 거기 있다. 우리 어댑터 이미지에는
# 없다(그 이미지는 traffic_simulator_msgs/autoware_control_msgs 계열만 갖는다).
#
# 컨테이너는 ~/oss/oss_adsw 를 /workspace 로 마운트하고 있으므로, 그리로 복사해서 실행한다.
# 이미지를 건드리지 않는다 -- 벤더 이미지는 읽기만 한다는 이 저장소의 규칙 그대로다.
#
# ★ 임시방편이다. docs/sils/sils-autoware-partition-vendor-requests.md §1-A 가 반영되면
#   이 스크립트는 필요 없어진다.
#
# Usage: tools/sils/ss2/run_localization_shim.sh [--domain-id 42] [--stop]
set -uo pipefail
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DOMAIN="${ROS_DOMAIN_ID:-42}"
WORKSPACE="${ADSW_WORKSPACE:-$HOME/oss/oss_adsw}"
STOP=0
while [ "${1:-}" != "" ]; do
    case "$1" in
        --domain-id) DOMAIN="$2"; shift ;;
        --stop) STOP=1 ;;
        *) echo "run_localization_shim: unknown option: $1" >&2; exit 2 ;;
    esac
    shift
done
NAME="adsw-control-${DOMAIN}"
docker ps --format '{{.Names}}' | grep -qx "$NAME" || { echo "run_localization_shim: ${NAME} 가 안 떠 있다" >&2; exit 2; }

if [ "$STOP" = "1" ]; then
    docker exec "$NAME" bash -lc 'pkill -f localization_initialize_shim' 2>/dev/null
    echo "run_localization_shim: 내렸다"; exit 0
fi

cp "${here}/localization_initialize_shim.py" "${WORKSPACE}/localization_initialize_shim.py"
docker exec "$NAME" bash -lc 'pkill -f localization_initialize_shim' 2>/dev/null || true
docker exec -d "$NAME" bash -lc \
  'source /opt/ros/humble/setup.bash && source /opt/autoware/setup.bash && \
   python3 /workspace/localization_initialize_shim.py > /workspace/log/localization_shim.txt 2>&1'
echo "==> localization initialize shim 기동 (${NAME}, 도메인 ${DOMAIN})"
echo "    로그: ${WORKSPACE}/log/localization_shim.txt"
echo "    확인: ros2 service call /localization/initialize tier4_localization_msgs/srv/InitializeLocalization '{}'"
