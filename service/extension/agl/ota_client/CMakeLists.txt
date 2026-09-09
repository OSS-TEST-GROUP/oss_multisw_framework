cmake_minimum_required(VERSION 3.5)
project(agl_ota_client)

find_package(fastcdr REQUIRED)
find_package(fastdds REQUIRED)
find_package(nlohmann_json QUIET)

set(GENERATED_OTA_DIR ${CMAKE_INSTALL_PREFIX}/generated/idl)
file(GLOB GENERATED_OTA_FILES "${GENERATED_OTA_DIR}/ota_mgr/*.cxx")
# NotifyOtaMgrHeartbeat 는 OtamDiagnosticServiceMsg.idl 에 정의되어 그 combined
# 생성물(OtamDiagnosticServiceMsg*.cxx)에 이미 포함된다. fastddsgen 이 참조용
# per-type 파일(NotifyOtaMgrHeartbeat*.cxx)도 별도 생성하므로 GLOB 이 둘 다 잡아
# unittest 실행파일 링크 시 multiple-definition 을 유발한다 → 중복인 standalone
# per-type 파일을 제외한다(심볼은 combined 에서 그대로 제공).
list(FILTER GENERATED_OTA_FILES EXCLUDE REGEX "NotifyOtaMgrHeartbeat")

add_library(agl_ota_core STATIC
            otac_main/OtacMain.cpp
            otac_manager_service/OtacManagerService.cpp
            common/OtacManagerDdsImpl.cpp
            otac_store/OtacStatusStore.cpp
            ${GENERATED_OTA_FILES})

target_include_directories(agl_ota_core PUBLIC
                           .
                           ${GENERATED_OTA_DIR}
                           otac_main
                           otac_manager_service
                           otac_store
                           common)

target_link_libraries(agl_ota_core PUBLIC
                      fastcdr
                      fastdds)

if(nlohmann_json_FOUND)
    target_link_libraries(agl_ota_core PUBLIC nlohmann_json::nlohmann_json)
endif()

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE agl_ota_core)

option(BUILD_UNITTEST "Build unit tests" OFF)
if (BUILD_UNITTEST)
  add_subdirectory(unittest)
endif()

install ( TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install ( FILES config/ota_client_config.json
    DESTINATION bin/config
)
