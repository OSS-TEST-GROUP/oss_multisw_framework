cmake_minimum_required(VERSION 3.5)

project ( "diagnostics" )

## Unittest
if (BUILD_UNITTEST)
    add_subdirectory ( unittest )
endif()

## Source
set ( DIAGNOSTIC_SRC
    diagnostic/DiagInfo.cpp
    diagnostic/DiagInfoRegistry.cpp
    diagnostic/VariantData.cpp
    diagnostic/DiagnosticPublisher.cpp
)
set ( EXECUTOR_SRC
    executor/RequestBase.cpp
    executor/ServiceExecutor.cpp
    executor/ServiceTask.cpp
)
set ( TRANSMITTER_SRC
    transmitter/Message.cpp
    transmitter/request_msg/IncomingMessageBase.cpp
    transmitter/MessageTransmitter.cpp
    transmitter/MessageProtocol.cpp
    transmitter/dds/DDSProtocol.cpp
    transmitter/dds/DDSDynamicIDLParser.cpp
)

set ( DIAGNOSTIC_ANALYZER_SRC
    diagnostic_analyzer/DataManager.cpp
    diagnostic_analyzer/DiagPolicyRule.cpp
    diagnostic_analyzer/PolicyManager.cpp
    diagnostic_analyzer/DiagPolicy.cpp
    diagnostic_analyzer/DiagnosticAnalyzer.cpp
    diagnostic_analyzer/ConditionEvaluator.cpp
    diagnostic_analyzer/ReferenceMessage.cpp
)


set ( MAIN_SRC main.cc
               DiagnosticManager.cpp
               ${DIAGNOSTIC_SRC}
               ${EXECUTOR_SRC}
               ${TRANSMITTER_SRC}
               ${DIAGNOSTIC_ANALYZER_SRC}
               logger/Logger.cpp
)

## Library
include_directories ( . )
link_libraries ( pthread fastcdr fastdds yaml-cpp )
#link_directories ( ${CMAKE_INSTALL_PREFIX} )

## Target binary
#add_library ( abc_static STATIC or SHARED or OBJECT $<TARGET_OBJECTS:abcd_obj> )
#set_target_properties ( abc_static PROPERTIES OUTPUT_NAME abc )
#target_link_libraries ( abc_static ${} )

#add_executable ( ${PROJECT_NAME} ${MAIN_SRC} $<TARGET_OBJECTS:abc_obj> )
add_executable ( ${PROJECT_NAME} ${MAIN_SRC} )

## Install
install ( TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    )

# Diag config
install(
    DIRECTORY
        config
    DESTINATION
        bin
    FILES_MATCHING PATTERN "*.yaml"
)

# Diag idl
install(DIRECTORY
    ${CMAKE_SOURCE_DIR}/idl/dds/common
    ${CMAKE_SOURCE_DIR}/idl/dds/diagnostic
    ${CMAKE_SOURCE_DIR}/idl/dds/security_mgr
    DESTINATION bin/idl
)
