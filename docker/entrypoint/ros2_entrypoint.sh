#!/bin/bash
# Container entrypoint for runtime-mmf-ext-ros2.
#
# F010 (oss-agent3 통합검증): boot ota_client alongside rosbridge_server
# and adsw_manager so that any OTA Monitor running on the host has a real
# Client-service counterparty inside the container. The container's life
# cycle remains tied to adsw_manager (PID 1 via exec); ota_client and
# rosbridge run as background children — if either dies, the container
# stays up and the failure is logged on stdout.
#
# Intentionally no `set -e` here: a failed background spawn must NOT take
# down the container. Each launch is wrapped so its rc surfaces in the
# log without propagating.

export LD_LIBRARY_PATH=/oss/lib
export SAVED_REGISTRY_BACKUP=${SAVED_REGISTRY_BACKUP:-/oss/bin/config}

# Source ROS2 Humble setup
source /opt/ros/humble/setup.bash

# colcon overlay containing all auto-generated per-module packages
# (diagnosticmgr, lifecyclemgr, otamanager, adswmgr, security). Built
# in docker/Dockerfile.rel's ros-mmf-base stage via idl_to_msg.py +
# colcon. Required so rosbridge_server can resolve `<pkg>/msg/<Type>`
# advertise / subscribe requests; without it every typed advertise
# fails with "No module named '<pkg>'".
if [ -f /opt/ros/humble/share/mmf_interfaces/setup.bash ]; then
  source /opt/ros/humble/share/mmf_interfaces/setup.bash
fi

# tier4_autoware_msgs overlay (built in same ros-mmf-base stage). Only
# needed if any topic uses tier4_*_msgs / autoware_*_msgs types.
if [ -f /opt/ros/humble/share/tier4_autoware/setup.bash ]; then
  source /opt/ros/humble/share/tier4_autoware/setup.bash
fi

# F010: ota_client background launch.
# The binary is shipped by Dockerfile.rel runtime-mmf-ext-ros2 (it copies
# output/<arch>/<type>/extension/ros2/bin/ → /oss/bin/). We still guard
# `-x` for cases where a developer mounts a stripped image and forgets
# to rebuild ext-ros2.
if [ -x /oss/bin/ota_client ]; then
  echo "[ros2_entrypoint] launching /oss/bin/ota_client in background"
  ( /oss/bin/ota_client; rc=$?
    echo "[ros2_entrypoint] ota_client exited rc=$rc (container stays up)"
  ) &
else
  echo "[ros2_entrypoint] WARN: /oss/bin/ota_client not found, skipping (rebuild ext-ros2 to populate it)" >&2
fi

# dds_bridge: dynamic-IDL adapter that forwards every OtaManager
# topic between the raw bare-DDS layer (otamanager::X) and the
# ROS-canonical layer (otamanager::msg::dds_::X_) so rosbridge_server
# and ota_monitor can talk to the host OtaManager without TypeObject
# matching pain. Topic table + design rationale live in
# service/extension/ros2/dds_bridge/main.cc.
if [ -x /oss/bin/dds_bridge ]; then
  echo "[ros2_entrypoint] launching /oss/bin/dds_bridge in background"
  ( /oss/bin/dds_bridge /oss/bin/idl; rc=$?
    echo "[ros2_entrypoint] dds_bridge exited rc=$rc (container stays up)"
  ) &
else
  echo "[ros2_entrypoint] WARN: /oss/bin/dds_bridge not found, skipping" >&2
fi

# domain_agent: 원격(HMI 보드) Settings 가 정한 도메인을 이 호스트의 도메인 파일에
# 반영한다. hmi_launcher 는 자기 호스트의 파일만 쓰기 때문에, Autoware/core/ext-ros2
# 가 다른 호스트에 있으면 그쪽은 옛 값으로 남는다 — 그 경로를 잇는 데몬이다.
# 컨테이너 재기동은 하지 않는다(경계). 설계: domain_agent.h
if [ -x /oss/bin/domain_agent ]; then
  echo "[ros2_entrypoint] launching /oss/bin/domain_agent in background"
  ( /oss/bin/domain_agent /oss/bin/idl; rc=$?
    echo "[ros2_entrypoint] domain_agent exited rc=$rc (container stays up)" ) &
else
  echo "[ros2_entrypoint] WARN: /oss/bin/domain_agent not found, skipping" >&2
fi

# Start rosbridge_server in background
ros2 launch rosbridge_server rosbridge_websocket_launch.xml &

# Start main application
# 호스트에 oss-domain-apply 설치 (docker.sock 경유, 실패는 무시).
# 도메인 변경 반영은 호스트에서 돌아야 하고, 이미지에만 담으면 재플래싱까지 미뤄진다.
# 자세한 근거는 install-domain-apply.sh 헤더 참조.
[ -x /oss/install-domain-apply.sh ] && /oss/install-domain-apply.sh /oss/host-install || true

exec /oss/bin/adsw_manager
