cmake_minimum_required(VERSION 3.14)
project(vehicle_info_manager CXX)

# Set the default installation directory to be inside the build directory
set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE STRING "Default installation directory")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Release)

# Define project directories
set(PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(PROJECT_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
#set(PROJECT_BUILD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/build)
set(PROJECT_TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests)
set(PROJECT_CONFIG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/config)
set(PROJECT_PROTO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/proto)
set(PROJECT_IDL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/idl)


# Using target_include_directories instead
# include_directories(${PROJECT_INCLUDE_DIR})

# Find Dependencies using PkgConfig and CMake's find_package
find_package(PkgConfig REQUIRED)
if (PkgConfig_FOUND)
    message(STATUS "PkgConfig module found successfully.")
    message(STATUS "  PkgConfig executable: ${PKG_CONFIG_EXECUTABLE}")
    message(STATUS "  PkgConfig version: ${PKG_CONFIG_VERSION_STRING}")
else()
    message(WARNING "PkgConfig module not found.")
    message(WARNING "  Please ensure pkg-config is installed and in your system's PATH.")
endif()



find_package(Protobuf REQUIRED)
message(STATUS "Protobuf_FOUND: ${Protobuf_FOUND}")
message(STATUS "Protobuf_VERSION: ${Protobuf_VERSION}")
message(STATUS "Protobuf_INCLUDE_DIRS: ${Protobuf_INCLUDE_DIRS}")
message(STATUS "Protobuf_LIBRARIES: ${Protobuf_LIBRARIES}")
message(STATUS "Protobuf_PROTOC_EXECUTABLE: ${Protobuf_PROTOC_EXECUTABLE}")


pkg_check_modules(GRPC REQUIRED grpc++)
if (GRPC_FOUND)
   message(STATUS "Found gRPC via pkg-config.")
   message(STATUS "  Includes: ${GRPC_INCLUDE_DIRS}")
   message(STATUS "  Libraries: ${GRPC_LIBRARIES}")
else()
   message(FATAL_ERROR "Could not find gRPC via pkg-config.")
endif()


if(NOT fastcdr_FOUND)
    find_package(fastcdr 2 REQUIRED)
endif()

if(NOT fastdds_FOUND)
    find_package(fastdds 3 REQUIRED)
endif()


# Find fastddsgen executable
find_program(FASTDDSGEN_EXECUTABLE NAMES fastddsgen)
if(NOT FASTDDSGEN_EXECUTABLE)
    message(FATAL_ERROR "fastddsgen not found. Please ensure it is in your PATH.")
endif()




# Using target_include_directories instead
# include_directories(${Protobuf_INCLUDE_DIRS})
# include_directories(${GRPC_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR}) # For generated headers from proto and idl

# Generate C++ sources from .proto file
#set(PROTO_FILE ${PROJECT_PROTO_DIR}/VISSv3.0.proto)

# Find the gRPC C++ plugin
find_program(GRPC_CPP_PLUGIN_EXECUTABLE NAMES grpc_cpp_plugin
	PATHS /usr/local/bin /usr/bin
	NO_CMAKE_FIND_ROOT_PATH)
if(NOT GRPC_CPP_PLUGIN_EXECUTABLE)
    message(FATAL_ERROR "grpc_cpp_plugin not found.")
endif()



# proto 파일 리스트
file(GLOB PROTO_FILES "${PROJECT_PROTO_DIR}/*.proto")

# 출력 파일 리스트
set(GENERATED_PROTO_SRCS "")
set(GENERATED_PROTO_HDRS "")
set(GENERATED_GRPC_SRCS "")
set(GENERATED_GRPC_HDRS "")

foreach(PROTO ${PROTO_FILES})
    get_filename_component(FILENAME ${PROTO} NAME)  # ex: myfile.proto → myfile
    string(REPLACE ".proto" "" BASE_NAME_WITHOUT_PROTO "${FILENAME}")

    list(APPEND GENERATED_PROTO_SRCS "${CMAKE_CURRENT_BINARY_DIR}/${BASE_NAME_WITHOUT_PROTO}.pb.cc")
    list(APPEND GENERATED_PROTO_HDRS "${CMAKE_CURRENT_BINARY_DIR}/${BASE_NAME_WITHOUT_PROTO}.pb.h")
    list(APPEND GENERATED_GRPC_SRCS "${CMAKE_CURRENT_BINARY_DIR}/${BASE_NAME_WITHOUT_PROTO}.grpc.pb.cc")
    list(APPEND GENERATED_GRPC_HDRS "${CMAKE_CURRENT_BINARY_DIR}/${BASE_NAME_WITHOUT_PROTO}.grpc.pb.h")

    add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${BASE_NAME_WITHOUT_PROTO}.pb.cc"
               "${CMAKE_CURRENT_BINARY_DIR}/${BASE_NAME_WITHOUT_PROTO}.pb.h"
               "${CMAKE_CURRENT_BINARY_DIR}/${BASE_NAME_WITHOUT_PROTO}.grpc.pb.cc"
               "${CMAKE_CURRENT_BINARY_DIR}/${BASE_NAME_WITHOUT_PROTO}.grpc.pb.h"
        COMMAND ${Protobuf_PROTOC_EXECUTABLE}
                --proto_path=${CMAKE_CURRENT_SOURCE_DIR}/proto           # .proto 파일 검색 경로
                --cpp_out=${CMAKE_CURRENT_BINARY_DIR}                    # C++ 코드 출력 디렉토리
                --grpc_out=${CMAKE_CURRENT_BINARY_DIR}                   # gRPC 코드 출력 디렉토리
                --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN_EXECUTABLE}   # gRPC 플러그인 설정
                --experimental_allow_proto3_optional                     # proto3 optional 플래그
                ${PROTO}                                                 # 입력 .proto 파일
        DEPENDS ${PROTO}                                                 # 이 명령이 실행되기 위해 필요한 파일
        COMMENT "Generating ${FILENAME}.pb.cc and .pb.h from ${PROTO}"
        VERBATIM
    )
endforeach()

# --- Fast DDS IDL Code Generation ---
set(GENERATED_IDL_DIR ${CMAKE_CURRENT_BINARY_DIR}/idl)
file(MAKE_DIRECTORY ${GENERATED_IDL_DIR})
set(GENERATED_IDL_SRCS "")
set(GENERATED_IDL_HDRS "")

# file(GLOB IDL_FILES "${PROJECT_IDL_DIR}/*.idl")
set(IDL_FILES
    "${PROJECT_IDL_DIR}/autoware/Time.idl"
    "${PROJECT_IDL_DIR}/autoware/Header.idl"
    "${PROJECT_IDL_DIR}/autoware/Quaternion.idl"
    "${PROJECT_IDL_DIR}/VehicleData.idl"
    "${PROJECT_IDL_DIR}/VissRequest.idl"
    "${PROJECT_IDL_DIR}/autoware/GnssInsOrientation.idl"
    "${PROJECT_IDL_DIR}/autoware/GnssInsOrientationStamped.idl"
    "${PROJECT_IDL_DIR}/autoware/NavSatFix.idl"
    "${PROJECT_IDL_DIR}/autoware/PointCloud2.idl"
    "${PROJECT_IDL_DIR}/autoware/TrajectoryPoint.idl"
    "${PROJECT_IDL_DIR}/autoware/TrafficLight.idl"
    "${PROJECT_IDL_DIR}/autoware/Trajectory.idl"
    "${PROJECT_IDL_DIR}/autoware/Duration.idl"
    "${PROJECT_IDL_DIR}/autoware/Pose.idl"
    "${PROJECT_IDL_DIR}/autoware/Point.idl"
    
)

foreach(IDL_FILE ${IDL_FILES})
    get_filename_component(FILENAME ${IDL_FILE} NAME_WE)
    

    set(GENERATED_HDR "${GENERATED_IDL_DIR}/${FILENAME}.h")
    set(GENERATED_PUBSUB_SRC "${GENERATED_IDL_DIR}/${FILENAME}PubSubTypes.cxx")
    set(GENERATED_PUBSUB_HDR "${GENERATED_IDL_DIR}/${FILENAME}PubSubTypes.h")
    set(GENERATED_TYPE_OBJECT_SRC "${GENERATED_IDL_DIR}/${FILENAME}TypeObjectSupport.cxx")
    set(GENERATED_TYPE_OBJECT_HDR "${GENERATED_IDL_DIR}/${FILENAME}TypeObjectSupport.h")

    add_custom_command(
        OUTPUT ${GENERATED_HDR} ${GENERATED_PUBSUB_SRC} ${GENERATED_PUBSUB_HDR} ${GENERATED_TYPE_OBJECT_SRC} ${GENERATED_TYPE_OBJECT_HDR}
        COMMAND ${FASTDDSGEN_EXECUTABLE} -typeros2 -d ${GENERATED_IDL_DIR} -I ${PROJECT_IDL_DIR}/autoware -replace ${IDL_FILE}
        DEPENDS ${IDL_FILE}
        COMMENT "Generating C++ from ${IDL_FILE}"
    )

    list(APPEND GENERATED_IDL_SRCS ${GENERATED_PUBSUB_SRC} ${GENERATED_TYPE_OBJECT_SRC})
    list(APPEND GENERATED_IDL_HDRS ${GENERATED_HDR} ${GENERATED_PUBSUB_HDR} ${GENERATED_TYPE_OBJECT_HDR})
endforeach()

# Create a static library from all generated IDL sources
add_library(idl_types STATIC ${GENERATED_IDL_SRCS})

target_include_directories(idl_types
    PRIVATE
    ${fastdds_INCLUDE_DIR}
    ${fastcdr_INCLUDE_DIR}
)

# --- Googletest Integration using FetchContent ---
include(FetchContent)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.17.0
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# --- MQTT Paho Library Integration using FetchContent ---
include(FetchContent)

# Find OpenSSL, which is a dependency for Paho MQTT C library for TLS support
find_package(OpenSSL)
if(OPENSSL_FOUND)
    message(STATUS "Found OpenSSL: ${OPENSSL_VERSION}")
    message(STATUS "  Includes: ${OPENSSL_INCLUDE_DIR}")
    message(STATUS "  Libraries: ${OPENSSL_LIBRARIES}")
else()
    message(WARNING "OpenSSL not found. MQTT TLS support will be disabled.")
endif()

# Declare Paho C++ library. This will also fetch the C library as a submodule.
FetchContent_Declare(
    paho-mqtt-cpp
    GIT_REPOSITORY https://github.com/eclipse/paho.mqtt.cpp.git
    GIT_TAG        v1.5.3
    GIT_SUBMODULES_RECURSE ON
)

# Set options BEFORE FetchContent_MakeAvailable
set(PAHO_WITH_MQTT_C ON CACHE BOOL "Build Paho C library from submodule")
set(PAHO_BUILD_STATIC ON CACHE BOOL "Build Paho libraries as static")
set(PAHO_BUILD_SAMPLES OFF CACHE BOOL "Don't build samples")
set(PAHO_ENABLE_TESTING OFF CACHE BOOL "Don't build tests")
if(OPENSSL_FOUND)
    set(PAHO_MQTT_C_USE_OPENSSL ON CACHE BOOL "Use OpenSSL in Paho C library")
endif()

FetchContent_MakeAvailable(paho-mqtt-cpp)


# Add the main executable with all sources
add_executable(vehicle_info_manager_app
    src/app/VissServerApp.cpp
    src/core/VssTreeParser.cpp
    src/core/ServiceManager.cpp
    src/core/VssSubscriptionManager.cpp
    src/core/VssDataStore.cpp
    src/core/JsonSchemaValidator.cpp
    src/transport/mqtt_transport.cpp
    src/transport/grpc_transport.cpp
    src/transport/dds_transport.cpp
    # src/transport/ws_server.cpp
    src/providers/DDSProvider.cpp
    src/providers/CANProvider.cpp
    src/providers/NetworkProvider.cpp
    src/common/StatusCodes.cpp
    src/common/Utils.cpp
    ${GENERATED_PROTO_SRCS}
    ${GENERATED_GRPC_SRCS}
)

# Add include directories for dependencies
target_include_directories(vehicle_info_manager_app
    PRIVATE
    ${PROJECT_INCLUDE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${GENERATED_IDL_DIR}
    ${Protobuf_INCLUDE_DIRS}
    ${GRPC_INCLUDE_DIRS}
    ${fastdds_INCLUDE_DIR}
    ${paho-mqtt-cpp_SOURCE_DIR}/include
)

# Link libraries
target_link_libraries(vehicle_info_manager_app
    PRIVATE
    idl_types
    ${GRPC_LIBRARIES}
    ${Protobuf_LIBRARIES}
    paho-mqttpp3-static         # Paho C++ static library target
    paho-mqtt3as-static         # Paho C static library target (async, with SSL)
    fastdds fastcdr
    pthread
    ${OPENSSL_LIBRARIES}
)

# Configure output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "out/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "out/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "out/lib")

# Add test client executable
add_executable(grpc_client_test
    tests/grpc_client_test.cpp
    ${GENERATED_PROTO_SRCS}
    ${GENERATED_GRPC_SRCS}
)

# Link libraries for the test client
target_link_libraries(grpc_client_test
    ${GRPC_LIBRARIES}
    ${Protobuf_LIBRARIES}
    pthread
)

# Add MQTT test client executable
add_executable(mqtt_client_test
    tests/mqtt_client_test.cpp
)

target_include_directories(mqtt_client_test
    PRIVATE
    ${PROJECT_INCLUDE_DIR}
    ${paho-mqtt-cpp_SOURCE_DIR}/include
)

target_link_libraries(mqtt_client_test
    PRIVATE
    paho-mqttpp3-static
    paho-mqtt3as-static
    pthread
    ${OPENSSL_LIBRARIES}
)

# Add DDS test client executable
add_executable(dds_client_test
    tests/dds_client_test.cpp
)

target_include_directories(dds_client_test
    PRIVATE
    ${PROJECT_INCLUDE_DIR}
    ${GENERATED_IDL_DIR}
    ${fastdds_INCLUDE_DIR}
)

target_link_libraries(dds_client_test
    PRIVATE
    idl_types
    fastdds
    fastcdr
    pthread
)

# --- Installation Setup ---

# Install the main application executable
install(TARGETS vehicle_info_manager_app
    RUNTIME DESTINATION bin
)

# Install the configuration files into a 'config' directory within the install prefix
install(FILES
    config/vissv3.0-schema.json
    config/viss.him
    DESTINATION bin/config
)

# Install the forest directory
install(DIRECTORY
    config/forest/
    DESTINATION bin/config/forest
)

# --- Unit Testing Setup ---
if (BUILD_UNITTEST)
	enable_testing()
	
	add_executable(VehicleInfoMgrTest
	    unittest/vss_tree_test.cpp
	    unittest/utils_test.cpp
	    unittest/vss_data_store_test.cpp
	    unittest/vss_subscription_manager_test.cpp
	    unittest/service_manager_test.cpp
	    src/core/VssTreeParser.cpp
	    src/common/Utils.cpp
	    src/core/VssDataStore.cpp
	    src/core/VssSubscriptionManager.cpp
	    src/core/ServiceManager.cpp
	    src/common/StatusCodes.cpp
	)
	
	target_include_directories(VehicleInfoMgrTest
	    PRIVATE
	    ${PROJECT_INCLUDE_DIR}
	)
	
	target_link_libraries(VehicleInfoMgrTest
	    PRIVATE
	    gtest_main
	)
	
	include(GoogleTest)
	gtest_discover_tests(VehicleInfoMgrTest)

	install (TARGETS VehicleInfoMgrTest
		RUNTIME DESTINATION bin/unittest/vehicle_info_mgr
		LIBRARY DESTINATION lib
		ARCHIVE DESTINATION lib
	)
endif()
