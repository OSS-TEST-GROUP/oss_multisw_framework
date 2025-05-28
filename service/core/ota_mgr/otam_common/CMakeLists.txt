# in ota_mgr/otam_common/CMakeLists.txt

# Do not call project() here, it's defined in the top-level CMakeLists.txt

# --- OtamDdsImpl Library ---
set(OTAM_CLIENT_DDS_IMPL_SRCS
    otam_dds_impl.cc
)
message(STATUS "Configuring OtamDdsImpl static library.")
add_library(OtamDdsImpl STATIC ${OTAM_CLIENT_DDS_IMPL_SRCS})

# target_include_directories(OtamDdsImpl PUBLIC
#     $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> # For otam_client_service.h
#     $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..> # For header files in otam_common
# )

# target_link_libraries(OtamDdsImpl PUBLIC 
#     fastcdr fastdds
# )
