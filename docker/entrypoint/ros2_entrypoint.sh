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
export SAVED_REGISTRY_BACKUP=/oss/bin/config

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

# ota_dds_bridge: dynamic-IDL adapter that forwards every OtaManager
# topic between the raw bare-DDS layer (otamanager::X) and the
# ROS-canonical layer (otamanager::msg::dds_::X_) so rosbridge_server
# and ota_monitor can talk to the host OtaManager without TypeObject
# matching pain. Topic table + design rationale live in
# service/extension/ros2/dds_bridge/main.cc.
if [ -x /oss/bin/ota_dds_bridge ]; then
  echo "[ros2_entrypoint] launching /oss/bin/ota_dds_bridge in background"
  ( /oss/bin/ota_dds_bridge /oss/bin/idl; rc=$?
    echo "[ros2_entrypoint] ota_dds_bridge exited rc=$rc (container stays up)"
  ) &
else
  echo "[ros2_entrypoint] WARN: /oss/bin/ota_dds_bridge not found, skipping" >&2
fi

# Start rosbridge_server in background
ros2 launch rosbridge_server rosbridge_websocket_launch.xml &

# Start main application
exec /oss/bin/adsw_manager

