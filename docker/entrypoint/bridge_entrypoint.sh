#!/bin/bash
set -e

# setup ros2 environment
source /opt/ros/humble/setup.bash
source /opt/ros/humble/share/tier4_autoware/setup.bash
source /opt/ros/humble/share/diagnosticmgr/setup.bash

ros2 launch rosbridge_server rosbridge_websocket_launch.xml

exec "$@"
