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

# Start rosbridge_server in background
ros2 launch rosbridge_server rosbridge_websocket_launch.xml &

# Start main application
exec /oss/bin/adsw_manager

