#!/bin/bash

export LD_LIBRARY_PATH=/oss/lib
export SAVED_REGISTRY_BACKUP=/oss/bin/config

# Source ROS2 Humble setup
source /opt/ros/humble/setup.bash

# Start rosbridge_server in background
ros2 launch rosbridge_server rosbridge_websocket_launch.xml &

# Start main application
exec /oss/bin/adsw_manager

