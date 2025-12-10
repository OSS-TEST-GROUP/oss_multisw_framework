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
set(TEST_IDL_LIFECYCLE_DIAG_REPORT "${CMAKE_CURRENT_SOURCE_DIR}/idl/LifecycleDiagReport.idl")
add_custom_command(
    OUTPUT ${GENERATED_DIR}/LifecycleDiagReportPubSubTypes.cxx ${GENERATED_DIR}/LifecycleDiagReportTypeObjectSupport.cxx
    COMMAND fastddsgen -d ${GENERATED_DIR} ${TEST_IDL_LIFECYCLE_DIAG_REPORT}
    DEPENDS ${TEST_IDL_LIFECYCLE_DIAG_REPORT}
    COMMENT "Generating Fast DDS code from ${TEST_IDL_LIFECYCLE_DIAG_REPORT}"
)
set(TEST_IDL_SECURITY_REPORT "${CMAKE_CURRENT_SOURCE_DIR}/idl/SecurityDetection.idl")
add_custom_command(
    OUTPUT ${GENERATED_DIR}/SecurityDetectionPubSubTypes.cxx ${GENERATED_DIR}/SecurityDetectionTypeObjectSupport.cxx
    COMMAND fastddsgen -d ${GENERATED_DIR} ${TEST_IDL_SECURITY_REPORT}
    DEPENDS ${TEST_IDL_SECURITY_REPORT}
    COMMENT "Generating Fast DDS code from ${TEST_IDL_SECURITY_REPORT}"
)

## Source
include_directories (
    ..
    ${GENERATED_DIR}
)

## Library
link_libraries ( gtest gmock fastcdr fastdds yaml-cpp )

enable_testing()
include (GoogleTest)

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

# Activate CTest
gtest_discover_tests ( DiagInfoRegistryTest )
gtest_discover_tests ( DiagInfoTest )
gtest_discover_tests ( ServiceExecutorTest )
gtest_discover_tests ( ServiceTaskTest )
gtest_discover_tests ( RequestBaseTest )
gtest_discover_tests ( VariantDataTest )
gtest_discover_tests ( MessageTransmitterTest )
gtest_discover_tests ( MessageTest )
gtest_discover_tests ( DDSProtocolTest )
gtest_discover_tests ( DDSDynamicIDLParserTest )
gtest_discover_tests ( ReferenceMessageTest )
gtest_discover_tests ( DiagPolicyRuleTest )
gtest_discover_tests ( DiagPolicyTest )
gtest_discover_tests ( PolicyManagerTest )
gtest_discover_tests ( DataManagerTest )
gtest_discover_tests ( DiagnosticAnalyzerTest )
gtest_discover_tests ( ConditionEvalulatorTest )

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
                        ${GENERATED_DIR}/DataTypeTestSimplePubSubTypes.cxx
                        ${GENERATED_DIR}/DataTypeTestTypeObjectSupport.cxx
                        ${GENERATED_DIR}/LifecycleDiagReportPubSubTypes.cxx
                        ${GENERATED_DIR}/LifecycleDiagReportTypeObjectSupport.cxx
                        ${GENERATED_DIR}/common/CommonPubSubTypes.cxx
                        ${GENERATED_DIR}/common/CommonTypeObjectSupport.cxx
                        ${GENERATED_DIR}/diagnostic/DiagnosticCommonPubSubTypes.cxx
                        ${GENERATED_DIR}/diagnostic/DiagnosticCommonTypeObjectSupport.cxx
                        ${GENERATED_DIR}/diagnostic/DiagnosticNotificationPubSubTypes.cxx
                        ${GENERATED_DIR}/diagnostic/DiagnosticNotificationTypeObjectSupport.cxx
                        ${GENERATED_DIR}/diagnostic/DiagnosticInfoPubSubTypes.cxx
                        ${GENERATED_DIR}/diagnostic/DiagnosticInfoTypeObjectSupport.cxx
                        ${GENERATED_DIR}/security_mgr/SecurityDetectionPubSubTypes.cxx
                        ${GENERATED_DIR}/security_mgr/SecurityDetectionTypeObjectSupport.cxx
)

add_executable ( HelloWorldPublisher test/HelloWorldPublisher.cpp ${GENERATED_DDS_FILES} )
target_include_directories ( HelloWorldPublisher PUBLIC ${GENERATED_DIR} )
add_executable ( HelloWorldPublisherNoDynamic test/HelloWorldPublisherNoDynamic.cpp ${GENERATED_DDS_FILES} )
target_include_directories ( HelloWorldPublisherNoDynamic PUBLIC ${GENERATED_DIR} )

add_executable ( LifecycleDiagReportPublisher test/LifecycleDiagReportPublisher.cpp ${GENERATED_DDS_FILES} )
target_include_directories ( LifecycleDiagReportPublisher PUBLIC ${GENERATED_DIR} )
add_executable ( DiagnosticNotificationReceiver test/DiagnosticNotificationReceiver.cpp ${GENERATED_DDS_FILES} )
target_include_directories ( DiagnosticNotificationReceiver PUBLIC ${GENERATED_DIR} )
add_executable ( DiagnosticInfoTestClient test/DiagnosticInfoTestClient.cpp ${GENERATED_DDS_FILES} )
target_include_directories ( DiagnosticInfoTestClient PUBLIC ${GENERATED_DIR} )
add_executable ( SecurityDetectionPublisher test/SecurityDetectionPublisher.cpp ${GENERATED_DDS_FILES} )
target_include_directories ( SecurityDetectionPublisher PUBLIC ${GENERATED_DIR} )

install ( TARGETS HelloWorldPublisher HelloWorldPublisherNoDynamic LifecycleDiagReportPublisher DiagnosticNotificationReceiver DiagnosticInfoTestClient SecurityDetectionPublisher
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
    FILES_MATCHING
        PATTERN "*Simple.idl"
        PATTERN "*Complex.idl"
)

# Test Diag config
install(
    DIRECTORY
        config
    DESTINATION
        bin/unittest/diagnostics
    FILES_MATCHING PATTERN "*.yaml"
)
