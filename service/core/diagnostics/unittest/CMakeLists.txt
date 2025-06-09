cmake_minimum_required(VERSION 3.5)

project ( "diagnostics_unittest" )

## Compiler Options
add_compile_options(
    -fno-access-control
    -ftest-coverage
)

# IDL generation
set(GENERATED_DIR "${CMAKE_INSTALL_PREFIX}/generated/idl")
file(MAKE_DIRECTORY ${GENERATED_DIR})

set(TEST_IDL "${CMAKE_CURRENT_SOURCE_DIR}/idl/DataTypeTest.idl")
add_custom_command(
    OUTPUT ${GENERATED_DIR}/DataTypeTestPubSubTypes.cxx ${GENERATED_DIR}/DataTypeTestTypeObjectSupport.cxx
    COMMAND fastddsgen -d ${GENERATED_DIR} ${TEST_IDL}
    DEPENDS ${TEST_IDL}
    COMMENT "Generating Fast DDS code from ${TEST_IDL}"
)

set(TEST_IDL2 "${CMAKE_CURRENT_SOURCE_DIR}/idl/DataTypeTestSimple.idl")
add_custom_command(
    OUTPUT ${GENERATED_DIR}/DataTypeTestSimplePubSubTypes.cxx
    COMMAND fastddsgen -no-typeobjectsupport -d ${GENERATED_DIR} ${TEST_IDL2}
    DEPENDS ${TEST_IDL2}
    COMMENT "Generating Fast DDS code from ${TEST_IDL2}"
)

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
    RUNTIME DESTINATION bin/unittest/diagnostics
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    )

# Sample app for TEST
set(GENERATED_DDS_FILES ${GENERATED_DIR}/DataTypeTestPubSubTypes.cxx
                        ${GENERATED_DIR}/DataTypeTestTypeObjectSupport.cxx
                        ${GENERATED_DIR}/DataTypeTestSimplePubSubTypes.cxx
)
add_executable ( HelloWorldPublisher test/HelloWorldPublisher.cpp ${GENERATED_DDS_FILES} )
add_executable ( HelloWorldPublisherNoDynamic test/HelloWorldPublisherNoDynamic.cpp ${GENERATED_DDS_FILES} )
target_include_directories ( HelloWorldPublisher PUBLIC ${GENERATED_DIR} )
target_include_directories ( HelloWorldPublisherNoDynamic PUBLIC ${GENERATED_DIR} )
install ( TARGETS HelloWorldPublisher HelloWorldPublisherNoDynamic
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
