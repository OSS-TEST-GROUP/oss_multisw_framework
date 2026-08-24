cmake_minimum_required(VERSION 3.14)
project(ss2_ego_bridge CXX)

# SS_v2 <-> SiLS DDS adapter, BOTH directions.  See adapters/ss2/README.md.
#
#   ss2_ego_bridge      B2-1  SS_v2 /entity/status  ->  canonical EgoState (DDS)
#   ss2_control_bridge  B2-3  canonical VehicleControl (DDS) -> Autoware control topics
#
# Two executables, one package: they share the Fast DDS/fastddsgen setup below, which is the
# fiddly part, but they are separate PROCESSES on purpose -- they fail independently and a
# combined report line would not say which half is dead.
#
# This is a colcon/ament package because it needs rclcpp and traffic_simulator_msgs, which
# only exist inside a ROS 2 workspace overlaid on the scenario_simulator_v2 build.  It is
# NOT part of the top-level SiLS CMake tree on purpose: the adapter is outside SiLS
# (docs/sils/sils-simulator-loops.md §3), and pulling ROS 2 into service/sils/ would
# undo exactly the separation that lets us swap simulators.
#
# Build it in the adapter container: docker/sils/Dockerfile.ss2-adapter.
#
# The mapping logic (include/ss2_ego_mapping.hpp) has no ROS 2 and no DDS dependency and
# is tested standalone -- see run_tests.sh, which needs none of this.

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(traffic_simulator_msgs REQUIRED)
# The return path publishes exactly what concealer::AutowareUniverse subscribes to
# (external/concealer/src/autoware_universe.cpp).
find_package(autoware_control_msgs REQUIRED)
find_package(autoware_vehicle_msgs REQUIRED)
# Stage 3: the ADAPI vehicle-state surface (aw_ego_state_bridge). ADAPI rather than the raw
# /vehicle/status/* + /localization/* topics -- see aw_state_mapping.hpp.
find_package(autoware_adapi_v1_msgs REQUIRED)
# Stage 3: the localization surface simple_planning_simulator vacates (aw_localization_bridge).
# Plain ROS 2 message packages, not Autoware ones -- /localization/kinematic_state is a
# nav_msgs/Odometry and /localization/acceleration a geometry_msgs/AccelWithCovarianceStamped.
find_package(nav_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
# map -> base_link transform. With launch_localization=false nobody else broadcasts it, and
# its absence is invisible to a topic-level audit -- see the TF note in aw_localization_bridge.cpp.
find_package(tf2_ros REQUIRED)

# --- Fast DDS -----------------------------------------------------------------
#
# NO VERSION PIN here, unlike service/sils/vecu/CMakeLists.txt which requires fastdds 3.
# The adapter links whatever Fast DDS the ROS 2 distribution in this container ships
# (Humble bundles 2.x), because it must share a process with rclcpp.  Two Fast DDS major
# versions in one process is not a thing we want to debug.
#
# The consequence is that the canonical types must be GENERATED HERE with the matching
# fastddsgen rather than reused from ${CMAKE_INSTALL_PREFIX}/generated/idl -- that tree is
# produced by the top-level build against Fast DDS 3 and its generated sources will not
# compile against 2.x (namespaces and TypeObject support differ; 3.x emits *.hpp where
# 2.x emits *.h).  See README "Fast DDS 버전".
find_package(fastcdr REQUIRED)
find_package(fastrtps QUIET)      # Fast DDS 2.x CMake package name
if(NOT fastrtps_FOUND)
    find_package(fastdds REQUIRED)  # Fast DDS 3.x
    set(SS2_FASTDDS_TARGET fastdds)
else()
    set(SS2_FASTDDS_TARGET fastrtps)
endif()
message(STATUS "ss2_ego_bridge: linking Fast DDS target '${SS2_FASTDDS_TARGET}'")

# --- generate canonical EgoState ---------------------------------------------

# Both canonical contracts: EgoState goes out to the vECU, VehicleControl comes back.
set(SS2_IDL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../idl/dds/sils")
if(NOT EXISTS "${SS2_IDL_DIR}/EgoState.idl")
    # Allow an out-of-tree build (the container copies the IDL in). The env var is named
    # for EgoState for backward compatibility; it points at the DIRECTORY's file, and the
    # sibling VehicleControl.idl is expected next to it.
    get_filename_component(SS2_IDL_DIR "$ENV{SILS_EGOSTATE_IDL}" DIRECTORY)
endif()
set(SS2_IDL_SRCS
    "${SS2_IDL_DIR}/EgoState.idl"
    "${SS2_IDL_DIR}/VehicleControl.idl"
    # Stage 3: real Autoware's control command comes IN as this one.
    "${SS2_IDL_DIR}/ControlTarget.idl")
foreach(idl IN LISTS SS2_IDL_SRCS)
    if(NOT EXISTS "${idl}")
        message(FATAL_ERROR
            "ss2 adapter: cannot find ${idl}. Expected idl/dds/sils/ relative to this "
            "package, or SILS_EGOSTATE_IDL pointing into it. The adapter must generate the "
            "canonical types itself -- see the Fast DDS version note above.")
    endif()
endforeach()

set(SS2_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${SS2_GEN_DIR}")

find_program(FASTDDSGEN NAMES fastddsgen)
if(NOT FASTDDSGEN)
    message(FATAL_ERROR
        "ss2_ego_bridge: fastddsgen not found on PATH. It is a Java tool and is not part "
        "of the ROS 2 image; docker/sils/Dockerfile.ss2-adapter installs it. Install it or "
        "build this package in that container.")
endif()

# Generate at CONFIGURE time, not build time.
#
# The obvious shape -- add_custom_command to generate, then file(GLOB) the results -- does
# not work here, and fails in the worst way: the glob runs while CMake is configuring, when
# the generated directory is still empty, so add_executable() gets ONLY the .cpp. Everything
# compiles cleanly and then the link dies on undefined references to the type's methods,
# which reads like a missing library rather than "your sources were never added".
# CONFIGURE_DEPENDS does not rescue it either: the first build generates the files after
# the glob has already been evaluated.
#
# Generating here means the files exist before the glob, so the source list is right on the
# first pass and the filenames still do not have to be hardcoded (they differ across
# generator lines: 2.x emits .cxx/.h, 4.x emits .cxx/.hpp plus TypeObjectSupport files).
foreach(idl IN LISTS SS2_IDL_SRCS)
    execute_process(
        COMMAND ${FASTDDSGEN} -replace -d "${SS2_GEN_DIR}" "${idl}"
        RESULT_VARIABLE SS2_GEN_RESULT
        OUTPUT_VARIABLE SS2_GEN_OUTPUT
        ERROR_VARIABLE SS2_GEN_OUTPUT)
    if(NOT SS2_GEN_RESULT EQUAL 0)
        message(FATAL_ERROR "ss2 adapter: fastddsgen failed on ${idl}\n${SS2_GEN_OUTPUT}")
    endif()
    # Re-run CMake (and therefore regeneration) when a canonical contract changes.
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${idl}")
endforeach()

file(GLOB SS2_GENERATED_SRCS "${SS2_GEN_DIR}/*.cxx")
if(NOT SS2_GENERATED_SRCS)
    message(FATAL_ERROR
        "ss2 adapter: fastddsgen reported success but produced no .cxx under "
        "${SS2_GEN_DIR}. Generator output:\n${SS2_GEN_OUTPUT}")
endif()
message(STATUS "ss2 adapter: generated type sources: ${SS2_GENERATED_SRCS}")

# Both executables compile the full generated set. It is a few redundant objects per binary
# and it keeps the type registration self-contained -- splitting per-IDL would mean tracking
# which generated file belongs to which contract, which is exactly the bookkeeping the glob
# exists to avoid.
add_executable(ss2_ego_bridge src/ss2_ego_bridge.cpp ${SS2_GENERATED_SRCS})
target_include_directories(ss2_ego_bridge PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${SS2_GEN_DIR}")
target_link_libraries(ss2_ego_bridge ${SS2_FASTDDS_TARGET} fastcdr)
ament_target_dependencies(ss2_ego_bridge rclcpp traffic_simulator_msgs)

add_executable(ss2_control_bridge src/ss2_control_bridge.cpp ${SS2_GENERATED_SRCS})
target_include_directories(ss2_control_bridge PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${SS2_GEN_DIR}")
target_link_libraries(ss2_control_bridge ${SS2_FASTDDS_TARGET} fastcdr)
ament_target_dependencies(ss2_control_bridge rclcpp autoware_control_msgs autoware_vehicle_msgs)

# Stage 3 (B-full): REAL Autoware's control command -> canonical ControlTarget, for the
# vECU in the vehicle_interface seat. Autoware-side rather than SS_v2-side, so the name
# does not carry the ss2_ prefix; it lives in this package only because it needs exactly
# the Fast DDS + fastddsgen setup above, which is the fiddly part.
add_executable(aw_control_target_bridge src/aw_control_target_bridge.cpp ${SS2_GENERATED_SRCS})
target_include_directories(aw_control_target_bridge PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${SS2_GEN_DIR}")
target_link_libraries(aw_control_target_bridge ${SS2_FASTDDS_TARGET} fastcdr)
ament_target_dependencies(aw_control_target_bridge
    rclcpp autoware_control_msgs autoware_vehicle_msgs)

# Stage 3 (B-full): REAL Autoware's ADAPI vehicle state -> canonical EgoState, so a
# developer ECU can run against real Autoware without a simulator adapter. Stand-in for the
# plant's own state source (MORAI / SS_v2) -- see the note in aw_state_mapping.hpp.
add_executable(aw_ego_state_bridge src/aw_ego_state_bridge.cpp ${SS2_GENERATED_SRCS})
target_include_directories(aw_ego_state_bridge PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${SS2_GEN_DIR}")
target_link_libraries(aw_ego_state_bridge ${SS2_FASTDDS_TARGET} fastcdr)
ament_target_dependencies(aw_ego_state_bridge rclcpp autoware_adapi_v1_msgs)

# Stage 3 (B-full): the RETURN half of the vehicle_interface seat -- canonical EgoState
# out to Autoware's /vehicle/status/*. Needed by BOTH real configurations (MORAI and
# Web.Auto), because in both of them Autoware's own simple_planning_simulator is off and
# nothing else feeds those 25 internal subscriptions.
# See docs/sils/sils-integration-topology.md §3.
add_executable(aw_vehicle_status_bridge src/aw_vehicle_status_bridge.cpp ${SS2_GENERATED_SRCS})
target_include_directories(aw_vehicle_status_bridge PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${SS2_GEN_DIR}")
target_link_libraries(aw_vehicle_status_bridge ${SS2_FASTDDS_TARGET} fastcdr)
ament_target_dependencies(aw_vehicle_status_bridge rclcpp autoware_vehicle_msgs)

# Stage 3 (B-full): the OTHER half of what simple_planning_simulator was doing. Turning
# that simulator off does not only remove /vehicle/status/* -- in full_motion mode it is
# also the sole publisher of /localization/{kinematic_state,acceleration}, and
# obigo_launch/launch/planning_simulator.launch.xml hardcodes launch_localization=false, so
# nothing else fills them. Separate process from aw_vehicle_status_bridge because it is
# CONDITIONAL (planning_simulation mode only) while /vehicle/status/* is universal -- and
# because they must fail independently. See the header comment in the .cpp.
add_executable(aw_localization_bridge src/aw_localization_bridge.cpp ${SS2_GENERATED_SRCS})
target_include_directories(aw_localization_bridge PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${SS2_GEN_DIR}")
target_link_libraries(aw_localization_bridge ${SS2_FASTDDS_TARGET} fastcdr)
ament_target_dependencies(aw_localization_bridge rclcpp nav_msgs geometry_msgs tf2_ros)

install(TARGETS ss2_ego_bridge ss2_control_bridge aw_control_target_bridge aw_ego_state_bridge
                aw_vehicle_status_bridge aw_localization_bridge
        DESTINATION lib/${PROJECT_NAME})

ament_package()
