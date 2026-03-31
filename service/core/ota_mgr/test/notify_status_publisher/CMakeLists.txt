# CMakeLists.txt for notify_status_publisher
# TC-3 테스트용: NotifyAppUpdateStatus DDS Publisher
#
# 독립 빌드:
#   mkdir build && cd build
#   cmake .. -DFASTDDS_ROOT=/home/obigo/obigo-sdk/Fast-DDS/output/ros2_x86_64
#   make
#
# 또는 메인 프로젝트 빌드 시 test/ 의 add_subdirectory(notify_status_publisher) 로 포함

cmake_minimum_required(VERSION 3.5)
project(notify_status_publisher CXX)

set(CMAKE_CXX_STANDARD 17)

# ── IDL 생성 파일 경로 ──────────────────────────────────────────────────────
# 프로젝트 루트 기준: output/x86_64/Debug/extension/ros2/generated/idl
get_filename_component(PROJECT_ROOT
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../"
    ABSOLUTE)

set(IDL_BASE_DIR
    "${PROJECT_ROOT}/output/x86_64/Debug/extension/ros2/generated/idl")
set(IDL_OTA_MGR_DIR "${IDL_BASE_DIR}/ota_mgr")
set(IDL_COMMON_DIR  "${IDL_BASE_DIR}/common")

# ── FastDDS 경로 ─────────────────────────────────────────────────────────────
if(NOT DEFINED FASTDDS_ROOT)
    # 시스템 설치 확인 후 fallback으로 obigo SDK 사용
    if(EXISTS "/usr/local/lib/libfastdds.so")
        set(FASTDDS_ROOT "/usr/local")
    else()
        set(FASTDDS_ROOT "/home/obigo/obigo-sdk/Fast-DDS/output/ros2_x86_64")
    endif()
endif()

message(STATUS "FASTDDS_ROOT = ${FASTDDS_ROOT}")
message(STATUS "IDL_BASE_DIR = ${IDL_BASE_DIR}")

# ── include 경로 ─────────────────────────────────────────────────────────────
include_directories(
    ${IDL_BASE_DIR}          # "common/ReturnValue.hpp" 등 하위 경로 참조용
    ${IDL_OTA_MGR_DIR}       # "OtamClientServiceMsg.hpp" 직접 참조용
    ${FASTDDS_ROOT}/include
)

link_directories(
    ${FASTDDS_ROOT}/lib
)

# ── IDL PubSubTypes 소스 (직접 컴파일) ───────────────────────────────────────
set(IDL_SOURCES
    ${IDL_OTA_MGR_DIR}/OtamClientServiceMsgPubSubTypes.cxx
    ${IDL_OTA_MGR_DIR}/OtamClientServiceMsgTypeObjectSupport.cxx
    ${IDL_COMMON_DIR}/ReturnValuePubSubTypes.cxx
    ${IDL_COMMON_DIR}/ReturnValueTypeObjectSupport.cxx
)

# ── 실행 파일 ─────────────────────────────────────────────────────────────────
add_executable(${PROJECT_NAME}
    main.cc
    ${IDL_SOURCES}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    fastdds
    fastcdr
    pthread
)

# ── 설치 ─────────────────────────────────────────────────────────────────────
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin/test
)
