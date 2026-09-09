cmake_minimum_required(VERSION 3.5)

project ( "ota_client" )

find_package(fastcdr REQUIRED)
find_package(fastdds REQUIRED)
find_package(nlohmann_json QUIET)

## Source

set(GENERATED_OTA_DIR ${CMAKE_INSTALL_PREFIX}/generated/idl)
file(GLOB GENERATED_OTA_FILES "${GENERATED_OTA_DIR}/ota_mgr/*.cxx")
# NotifyOtaMgrHeartbeat 는 OtamDiagnosticServiceMsg.idl 에 정의되어 그 combined
# 생성물(OtamDiagnosticServiceMsg*.cxx)에 이미 포함된다. fastddsgen 이 참조용
# per-type 파일(NotifyOtaMgrHeartbeat*.cxx)도 별도 생성하므로 GLOB 이 둘 다 잡아
# unittest 실행파일 링크 시 multiple-definition 을 유발한다 → 중복인 standalone
# per-type 파일을 제외한다(심볼은 combined 에서 그대로 제공).
list(FILTER GENERATED_OTA_FILES EXCLUDE REGEX "NotifyOtaMgrHeartbeat")

set ( MAIN_SRC main.cc )

set ( OTAC_MAIN otac_main/OtacMain.cpp )

set ( OTAC_MANAGER_SERVICE otac_manager_service/OtacManagerService.cpp )

set ( OTAC_STORE otac_store/OtacStatusStore.cpp )

set ( COMMON common/OtacManagerDdsImpl.cpp )


add_library(ota_client_core STATIC
            ${OTAC_MAIN}
            ${OTAC_MANAGER_SERVICE}
            ${OTAC_STORE}
            ${COMMON}
            ${GENERATED_OTA_FILES})

target_include_directories(ota_client_core PUBLIC
                           .
                           ${GENERATED_OTA_DIR}
                           otac_main
                           otac_manager_service
                           otac_store
                           common)

target_link_libraries(ota_client_core PUBLIC
                      fastcdr
                      fastdds)

if(nlohmann_json_FOUND)
    target_link_libraries(ota_client_core PUBLIC nlohmann_json::nlohmann_json)
endif()

                      
add_executable ( ${PROJECT_NAME} ${MAIN_SRC} )
target_link_libraries(${PROJECT_NAME} PRIVATE ota_client_core)

## Install
install ( TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install ( FILES config/ota_client_config.json
    DESTINATION config
)

option(BUILD_UNITTEST "Build unit tests" OFF)
if (BUILD_UNITTEST)
  add_subdirectory(unittest)
endif()