cmake_minimum_required(VERSION 3.5)

project ( "ota_client" )

find_package(fastcdr REQUIRED)
find_package(fastdds REQUIRED)

## Source

set(GENERATED_OTA_DIR ${CMAKE_INSTALL_PREFIX}/generated/idl)
file(GLOB GENERATED_OTA_FILES "${GENERATED_OTA_DIR}/ota_mgr/*.cxx")

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

                      
add_executable ( ${PROJECT_NAME} ${MAIN_SRC} )
target_link_libraries(${PROJECT_NAME} PRIVATE ota_client_core)

## Install
install ( TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    )

option(BUILD_UNITTEST "Build unit tests" OFF)
if (BUILD_UNITTEST)
  add_subdirectory(unittest)
endif()