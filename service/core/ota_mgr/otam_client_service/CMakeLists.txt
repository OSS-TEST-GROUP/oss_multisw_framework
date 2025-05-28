# in ota_mgr/otam_client_service/CMakeLists.txt

# Do not call project() here, it's defined in the top-level CMakeLists.txt

# Add source files for OtamClientService
set(GENERATED_DDS_FILES ${GENERATED_OTA_MGR_DIR}/OtamClientServiceMsgPubSubTypes.cxx
                        ${GENERATED_OTA_MGR_DIR}/OtamClientServiceMsgTypeObjectSupport.cxx
                        ${GENERATED_DIR}/common/ReturnValueTypeObjectSupport.cxx
                        ${GENERATED_DIR}/common/ReturnValuePubSubTypes.cxx
)

message(STATUS GENERATED_OTA_MGR_DIR=${GENERATED_OTA_MGR_DIR})

set(OTAM_CLIENT_SERVICE_SRCS
    otam_client_service.cc
    ${GENERATED_DDS_FILES}
)

# Create a static library for OtamClientService
# Static libraries are often preferred for internal components within a project
# as they get linked directly into the final executable.
add_library(OtamClientService STATIC ${OTAM_CLIENT_SERVICE_SRCS})

# Specify include directories for OtamClientService.
# PUBLIC means that any target linking to OtamClientService will also get these include paths.
# target_include_directories(OtamClientService PUBLIC
#     $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> # For otam_client_service.h
#     $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..> # For header files in otam_common
# )

# (Optional) If OtamClientService itself depends on other libraries, link them here.
# For now, it's self-contained with standard library includes.
target_link_libraries(OtamClientService PRIVATE 
    OtamDdsImpl
)

message(STATUS "CMAKE_CURRENT_SOURCE_DIR = ${CMAKE_CURRENT_SOURCE_DIR}")