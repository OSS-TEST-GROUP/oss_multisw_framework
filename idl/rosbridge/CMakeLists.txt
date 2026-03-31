cmake_minimum_required(VERSION 3.8)
project(mmf_interfaces)

find_package(ament_cmake REQUIRED)
find_package(rosidl_default_generators REQUIRED)

# ── Auto-generate .msg files from DDS IDL ───────────────────────────────────
# idl_to_msg.py converts idl/dds/**/*.idl → msg/*.msg
# Re-runs at configure time whenever any IDL file changes.
file(GLOB_RECURSE DDS_IDL_FILES
  CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/../dds/**/*.idl"
)

set(IDL_TO_MSG_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/idl_to_msg.py")
set(MSG_OUTPUT_DIR    "${CMAKE_CURRENT_SOURCE_DIR}/msg")

execute_process(
  COMMAND ${CMAKE_COMMAND} -E echo "[mmf_interfaces] Generating .msg from DDS IDL..."
)
execute_process(
  COMMAND python3 ${IDL_TO_MSG_SCRIPT}
          --idl-root "${CMAKE_CURRENT_SOURCE_DIR}/../dds"
          --output-dir "${MSG_OUTPUT_DIR}"
  RESULT_VARIABLE IDL_GEN_RESULT
  OUTPUT_VARIABLE IDL_GEN_OUTPUT
  ERROR_VARIABLE  IDL_GEN_ERRORS
)
if(NOT IDL_GEN_RESULT EQUAL 0)
  message(FATAL_ERROR "[mmf_interfaces] idl_to_msg.py failed:\n${IDL_GEN_ERRORS}")
endif()
message(STATUS "${IDL_GEN_OUTPUT}")

# ── Collect generated .msg files ─────────────────────────────────────────────
file(GLOB MSG_FILES CONFIGURE_DEPENDS "${MSG_OUTPUT_DIR}/*.msg")

if(NOT MSG_FILES)
  message(FATAL_ERROR "[mmf_interfaces] No .msg files found in ${MSG_OUTPUT_DIR}")
endif()

# Make paths relative to CMAKE_CURRENT_SOURCE_DIR for rosidl
set(MSG_FILES_RELATIVE)
foreach(f ${MSG_FILES})
  file(RELATIVE_PATH rel "${CMAKE_CURRENT_SOURCE_DIR}" "${f}")
  list(APPEND MSG_FILES_RELATIVE "${rel}")
endforeach()

rosidl_generate_interfaces(${PROJECT_NAME}
  ${MSG_FILES_RELATIVE}
)

ament_export_dependencies(rosidl_default_runtime)
ament_package()
