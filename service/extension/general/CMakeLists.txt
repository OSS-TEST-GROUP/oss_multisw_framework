cmake_minimum_required(VERSION 3.5)

project ( "general" )

# Middleware-neutral extension platform: container-service management without ROS2.
# Selected with -DEXTENSION_PLATFORM=GENERAL. Currently just the adsw_manager variant
# (docker/K3s container control via the lifecycle DDS control plane), for non-ROS2
# workloads such as the SiLS vECU containers.
add_subdirectory ( adsw_manager )
