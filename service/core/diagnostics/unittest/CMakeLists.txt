cmake_minimum_required(VERSION 3.5)

project ( "diagnostics_unittest" )

## Compiler Options
add_compile_options(
    -fno-access-control
    -ftest-coverage
)


# IDL generation
# Set path to fastrtpsgen or use RTI generator
set(FASTDDSGEN "fastddsgen")  # Adjust this path
set(IDL_FILE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/idl)

# Output directory for generated files
set(GENERATED_DIR "${CMAKE_INSTALL_PREFIX}/generated/idl")
set(BUILD_CHECK_FILE ${GENERATED_DIR}/.build_idl_diag_unittest)

if (NOT EXISTS ${GENERATED_DIR})
    file(MAKE_DIRECTORY ${GENERATED_DIR})
endif()
if (NOT EXISTS ${BUILD_CHECK_FILE})
    execute_process(
        COMMAND touch ${BUILD_CHECK_FILE}
    )

    # Function to generate sources from IDL files
    function(generate_dds_from_idl IDL_DIR IDL_FILE)
        execute_process(
            COMMAND ${FASTDDSGEN} -I ${IDL_DIR} -replace -d ${GENERATED_DIR} ${IDL_DIR}/${IDL_FILE}
        )
    endfunction()

    # Find all .idl files in service directories
    file(GLOB_RECURSE IDL_FILES RELATIVE ${IDL_FILE_DIR} "*.idl")

    foreach(IDL_FILE ${IDL_FILES})
        generate_dds_from_idl(${IDL_FILE_DIR} ${IDL_FILE})
    endforeach()
endif()


## Source
include_directories (
    ..
)

## Library
link_libraries ( gtest gmock fastcdr fastdds yaml-cpp )

## Target binary
add_executable ( DiagInfoRegistryTest DiagInfoRegistryTest.cpp )
add_executable ( DiagInfoTest DiagInfoTest.cpp )
add_executable ( ServiceExecutorTest ServiceExecutorTest.cpp )
add_executable ( ServiceTaskTest ServiceTaskTest.cpp )
add_executable ( RequestBaseTest RequestBaseTest.cpp )
add_executable ( VariantDataTest VariantDataTest.cpp )
add_executable ( MessageTransmitterTest MessageTransmitterTest.cpp )
add_executable ( MessageTest MessageTest.cpp )
add_executable ( DDSProtocolTest DDSProtocolTest.cpp )
add_executable ( DDSDynamicIDLParserTest DDSDynamicIDLParserTest.cpp )
add_executable ( ReferenceMessageTest ReferenceMessageTest.cpp )
add_executable ( DiagPolicyRuleTest DiagPolicyRuleTest.cpp )
add_executable ( DiagPolicyTest DiagPolicyTest.cpp )
add_executable ( PolicyManagerTest PolicyManagerTest.cpp )
add_executable ( DataManagerTest DataManagerTest.cpp )
add_executable ( DiagnosticAnalyzerTest DiagnosticAnalyzerTest.cpp )
add_executable ( ConditionEvalulatorTest ConditionEvalulatorTest.cpp )

## Install
install ( TARGETS DiagInfoRegistryTest
                  DiagInfoTest
                  ServiceExecutorTest
                  ServiceTaskTest
                  RequestBaseTest
                  VariantDataTest
                  MessageTransmitterTest
                  MessageTest
                  DDSProtocolTest
                  DDSDynamicIDLParserTest
                  ReferenceMessageTest
                  DiagPolicyRuleTest
                  DiagPolicyTest
                  PolicyManagerTest
                  DataManagerTest
                  DiagnosticAnalyzerTest
                  ConditionEvalulatorTest
    RUNTIME DESTINATION bin/unittest/diagnostics
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    )


# Sample app for TEST
set(GENERATED_DDS_FILES ${GENERATED_DIR}/DataTypeTestPubSubTypes.cxx
                        ${GENERATED_DIR}/DataTypeTestSimpleTypeObjectSupport.cxx
                        ${GENERATED_DIR}/LifecycleDiagReportPubSubTypes.cxx
                        ${GENERATED_DIR}/DataTypeTestSimplePubSubTypes.cxx
                        ${GENERATED_DIR}/DataTypeTestTypeObjectSupport.cxx
                        ${GENERATED_DIR}/LifecycleDiagReportTypeObjectSupport.cxx
)

add_executable ( HelloWorldPublisher test/HelloWorldPublisher.cpp ${GENERATED_DDS_FILES} )
target_include_directories ( HelloWorldPublisher PUBLIC ${GENERATED_DIR} )
add_executable ( HelloWorldPublisherNoDynamic test/HelloWorldPublisherNoDynamic.cpp ${GENERATED_DDS_FILES} )
target_include_directories ( HelloWorldPublisherNoDynamic PUBLIC ${GENERATED_DIR} )
add_executable ( LifecycleDiagReportPublisher test/LifecycleDiagReportPublisher.cpp ${GENERATED_DDS_FILES} )
target_include_directories ( LifecycleDiagReportPublisher PUBLIC ${GENERATED_DIR} )

install ( TARGETS HelloWorldPublisher HelloWorldPublisherNoDynamic LifecycleDiagReportPublisher
    RUNTIME DESTINATION bin/unittest/diagnostics/test
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    )

# Test IDL
install(
    DIRECTORY
        idl
    DESTINATION
        bin/unittest/diagnostics
    FILES_MATCHING PATTERN "*Simple.idl"
)

# Test Diag config
install(
    DIRECTORY
        config
    DESTINATION
        bin/unittest/diagnostics
    FILES_MATCHING PATTERN "*.yaml"
)
