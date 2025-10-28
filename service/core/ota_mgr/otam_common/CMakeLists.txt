# in ota_mgr/otam_common/CMakeLists.txt

# Do not call project() here, it's defined in the top-level CMakeLists.txt

# Add source files for DDS IDL
set(GENERATED_DDS_FILES ${GENERATED_OTA_MGR_DIR}/OtamClientServiceMsgPubSubTypes.cxx
                        ${GENERATED_OTA_MGR_DIR}/OtamClientServiceMsgTypeObjectSupport.cxx
                        ${GENERATED_OTA_MGR_DIR}/OtamDiagnosticServiceMsgPubSubTypes.cxx
                        ${GENERATED_OTA_MGR_DIR}/OtamDiagnosticServiceMsgTypeObjectSupport.cxx
                        ${GENERATED_OTA_MGR_DIR}/OtamVehicleServiceMsgPubSubTypes.cxx
                        ${GENERATED_OTA_MGR_DIR}/OtamVehicleServiceMsgTypeObjectSupport.cxx
                        ${GENERATED_OTA_MGR_DIR}/OtamActorServiceMsgPubSubTypes.cxx
                        ${GENERATED_OTA_MGR_DIR}/OtamActorServiceMsgTypeObjectSupport.cxx
                        ${GENERATED_LIFECYCLE_MGR_DIR}/SetServiceModeRequestPubSubTypes.cxx  
                        ${GENERATED_LIFECYCLE_MGR_DIR}/SetServiceModeRequestTypeObjectSupport.cxx                        
                        ${GENERATED_LIFECYCLE_MGR_DIR}/LifecycleCommonPubSubTypes.cxx  
                        ${GENERATED_LIFECYCLE_MGR_DIR}/LifecycleCommonTypeObjectSupport.cxx                        
                        ${GENERATED_VEHICLE_INFO_MGR_DIR}/VissRequestPubSubTypes.cxx  
                        ${GENERATED_VEHICLE_INFO_MGR_DIR}/VissRequestTypeObjectSupport.cxx                        
                        ${GENERATED_DIR}/common/ReturnValueTypeObjectSupport.cxx
                        ${GENERATED_DIR}/common/ReturnValuePubSubTypes.cxx
)

message(STATUS GENERATED_OTA_MGR_DIR=${GENERATED_OTA_MGR_DIR})

# --- OtamDdsImpl Library ---
# Note: otam_dds_impl.cc removed because OtamDdsImpl is now a template class
# Template implementations are in otam_dds_impl.h
set(OTAM_CLIENT_DDS_IMPL_SRCS
    ${GENERATED_DDS_FILES}
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
