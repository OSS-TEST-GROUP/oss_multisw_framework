cmake_minimum_required(VERSION 3.14)
project(viss-server CXX)

# Set the default installation directory to be inside the build directory
set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE STRING "Default installation directory")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Release)

if(CMAKE_SYSTEM_NAME STREQUAL "Android")
    # Fix for NDK 27+ strict version script symbol checks (lld linker)
    # This must be set before any FetchContent or add_subdirectory calls
    string(APPEND CMAKE_SHARED_LINKER_FLAGS " -Wl,--undefined-version")
    string(APPEND CMAKE_MODULE_LINKER_FLAGS " -Wl,--undefined-version")
    string(APPEND CMAKE_EXE_LINKER_FLAGS " -Wl,--undefined-version")
endif()


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


    # --- gRPC and Protobuf Integration ---
    # Try to find system-installed gRPC and Protobuf first.
    # If not found, fall back to FetchContent.
    #find_package(Protobuf)
    #find_package(gRPC)
if (NOT BUILD_FOR_ANDROID)
    # --- Using System-installed gRPC and Protobuf ---
    #find_package(Protobuf REQUIRED)
    find_package(Protobuf 3.12 REQUIRED)

    message(STATUS "Protobuf_FOUND: ${Protobuf_FOUND}")
    message(STATUS "Protobuf_VERSION: ${Protobuf_VERSION}")
    message(STATUS "Protobuf_INCLUDE_DIRS: ${Protobuf_INCLUDE_DIRS}")
    message(STATUS "Protobuf_LIBRARIES: ${Protobuf_LIBRARIES}")
    message(STATUS "Protobuf_PROTOC_EXECUTABLE: ${Protobuf_PROTOC_EXECUTABLE}")

    find_package(PkgConfig REQUIRED)
    if (PkgConfig_FOUND)
        message(STATUS "PkgConfig module found successfully.")
        message(STATUS "  PkgConfig executable: ${PKG_CONFIG_EXECUTABLE}")
        message(STATUS "  PkgConfig version: ${PKG_CONFIG_VERSION_STRING}")
    else()
        message(WARNING "PkgConfig module not found.")
        message(WARNING "  Please ensure pkg-config is installed and in your system's PATH.")
    endif()

    pkg_check_modules(GRPC REQUIRED grpc++)
    if (GRPC_FOUND)
    message(STATUS "Found gRPC via pkg-config.")
    message(STATUS "  Includes: ${GRPC_INCLUDE_DIRS}")
    message(STATUS "  Libraries: ${GRPC_LIBRARIES}")
    else()
    message(FATAL_ERROR "Could not find gRPC via pkg-config.")
    endif()


    message(STATUS "Found and using system-installed gRPC ${gRPC_VERSION}")
    message(STATUS "Found and using system-installed Protobuf ${Protobuf_VERSION}")

    set(HOST_PROTOC_EXECUTABLE ${Protobuf_PROTOC_EXECUTABLE})
    find_program(GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)
    if(NOT GRPC_CPP_PLUGIN_EXECUTABLE)
        message(FATAL_ERROR "grpc_cpp_plugin not found. Please ensure it is in your PATH.")
    endif()

    # Set variables for linking using modern CMake targets
    set(GRPC_LIBRARY ${GRPC_LIBRARIES})
    set(PROTOBUF_LIBRARY ${Protobuf_LIBRARIES})
    include_directories(${Protobuf_INCLUDE_DIRS})

else()
    # --- Using FetchContent to build gRPC and Protobuf ---
    # We let gRPC build its own BoringSSL submodule, which we will later share with Libwebsockets.
    message(STATUS "System gRPC not found. Falling back to FetchContent.")
    include(FetchContent)
    FetchContent_Declare(
        grpc
        GIT_REPOSITORY https://github.com/grpc/grpc.git
        GIT_TAG        v1.30.2 # Compatible with Protobuf 3.12.4
        GIT_SHALLOW    ON
        GIT_SUBMODULES_RECURSE ON
    )

    # Set gRPC and its dependencies' build options. This must be done BEFORE FetchContent_MakeAvailable.
    set(gRPC_INSTALL OFF CACHE BOOL "Disable gRPC installation" FORCE)
    set(protobuf_INSTALL OFF CACHE BOOL "Disable Protobuf installation" FORCE)
    set(gRPC_BUILD_TESTS OFF CACHE BOOL "Build gRPC tests" FORCE)
    set(gRPC_BUILD_CSHARP_EXT OFF CACHE BOOL "Build C# extension" FORCE)
    set(gRPC_BUILD_CODEGEN OFF CACHE BOOL "Build codegen" FORCE)
    
    set(gRPC_BUILD_GRPC_CSHARP_PLUGIN OFF CACHE BOOL "" FORCE)
    set(gRPC_BUILD_GRPC_NODE_PLUGIN OFF CACHE BOOL "" FORCE)
    set(gRPC_BUILD_GRPC_OBJECTIVE_C_PLUGIN OFF CACHE BOOL "" FORCE)
    set(gRPC_BUILD_GRPC_PHP_PLUGIN OFF CACHE BOOL "" FORCE)
    set(gRPC_BUILD_GRPC_PYTHON_PLUGIN OFF CACHE BOOL "" FORCE)
    set(gRPC_BUILD_GRPC_RUBY_PLUGIN OFF CACHE BOOL "" FORCE)
    set(gRPC_SSL_PROVIDER "module" CACHE STRING "Provider of ssl library" FORCE) # Use gRPC's built-in BoringSSL

    FetchContent_MakeAvailable(grpc)

    find_program(HOST_PROTOC_EXECUTABLE protoc)
    find_program(GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)

    # Set variables for linking using legacy FetchContent names
    set(GRPC_LIBRARY grpc++)
    set(PROTOBUF_LIBRARY protobuf::libprotobuf)
endif()




# --- Libwebsockets Library Integration using FetchContent ---
include(FetchContent)

FetchContent_Declare(
    libwebsockets
    GIT_REPOSITORY https://github.com/warmcat/libwebsockets.git
    GIT_TAG        v4.5.8
)

# Set LWS options before FetchContent_MakeAvailable
set(LWS_WITH_SSL ON CACHE BOOL "" FORCE)
set(LWS_WITH_HTTP2 ON CACHE BOOL "" FORCE)
set(LWS_ROLE_MQTT ON CACHE BOOL "" FORCE)
set(LWS_WITHOUT_TESTAPPS ON CACHE BOOL "" FORCE)
set(LWS_WITHOUT_TEST_SERVER ON CACHE BOOL "" FORCE)
set(LWS_WITHOUT_TEST_CLIENT ON CACHE BOOL "" FORCE)
set(LWS_WITHOUT_TEST_PING ON CACHE BOOL "" FORCE)
set(LWS_WITH_SHARED ON CACHE BOOL "" FORCE) # Build shared
set(LWS_WITH_STATIC OFF CACHE BOOL "" FORCE)

# Disable -Werror so minor type-qualifier warnings don't stop the build
set(DISABLE_WERROR ON CACHE BOOL "Do not treat warnings as errors" FORCE)
add_compile_options(-Wno-deprecated-declarations)

if (BUILD_FOR_ANDROID)
# Point libwebsockets to the BoringSSL built by gRPC
set(LWS_WITH_BORINGSSL ON CACHE BOOL "Use BoringSSL" FORCE)
set(LWS_OPENSSL_INCLUDE_DIRS "$<BUILD_INTERFACE:${grpc_SOURCE_DIR}/third_party/boringssl-with-bazel/src/include>" CACHE PATH "BoringSSL Include" FORCE)
set(LWS_OPENSSL_LIBRARIES "ssl;crypto" CACHE STRING "BoringSSL Libraries" FORCE)

# Disable exporting libwebsockets targets to avoid CMake errors about missing 'ssl'/'crypto' exports
set(LWS_WITH_EXPORT_LWSTARGETS OFF CACHE BOOL "Disable export" FORCE)


# [CRITICAL FIX] Pre-seed BoringSSL feature flags
# Since BoringSSL is built from source alongside Libwebsockets, try_compile feature checks
# fail during configuration. We must tell LWS that BoringSSL supports these modern features
# so it doesn't try to polyfill them (which causes redefinition errors).
set(LWS_HAVE_ECDSA_SIG_set0 ON CACHE BOOL "Pre-seed BoringSSL feature" FORCE)
set(LWS_HAVE_HMAC_CTX_new ON CACHE BOOL "Pre-seed BoringSSL feature" FORCE)
set(LWS_HAVE_EVP_MD_CTX_free ON CACHE BOOL "Pre-seed BoringSSL feature" FORCE)
set(LWS_HAVE_EVP_MD_CTX_new ON CACHE BOOL "Pre-seed BoringSSL feature" FORCE)
set(LWS_HAVE_RSA_SET0_KEY ON CACHE BOOL "Pre-seed BoringSSL feature" FORCE)
set(LWS_HAVE_X509_get_key_usage ON CACHE BOOL "Pre-seed BoringSSL feature" FORCE)
set(LWS_HAVE_SSL_CTX_get0_certificate ON CACHE BOOL "Pre-seed BoringSSL feature" FORCE)
set(LWS_HAVE_SSL_get0_alpn_selected ON CACHE BOOL "Pre-seed BoringSSL feature" FORCE)
set(LWS_HAVE_SSL_set_alpn_protos ON CACHE BOOL "Pre-seed BoringSSL feature" FORCE)
set(LWS_HAVE_EVP_PKEY_new_raw_private_key ON CACHE BOOL "Pre-seed BoringSSL feature" FORCE)

# Fix for BN_BYTES error: LWS tries to access opaque BIGNUM fields if it thinks BN_bn2binpad is missing
set(LWS_HAVE_BN_bn2binpad ON CACHE BOOL "Pre-seed BoringSSL feature" FORCE)
set(LWS_HAVE_EC_KEY_new_by_curve_name ON CACHE BOOL "Pre-seed BoringSSL feature" FORCE)

# Fix for RSA_verify_PKCS1_PSS missing: tell LWS to use RSA_verify_pss_mgf1 instead
set(LWS_HAVE_RSA_verify_pss_mgf1 ON CACHE BOOL "Pre-seed BoringSSL feature" FORCE)
endif()
FetchContent_MakeAvailable(libwebsockets)


    # Using target_include_directories instead
    # include_directories(${Protobuf_INCLUDE_DIRS})
    # include_directories(${GRPC_INCLUDE_DIRS})
    include_directories(${CMAKE_CURRENT_BINARY_DIR}) # For generated headers from proto and idl

    # Generate C++ sources from .proto file
    #set(PROTO_FILE ${PROJECT_PROTO_DIR}/VISSv3.0.proto)



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
            COMMAND ${HOST_PROTOC_EXECUTABLE}
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

    # Create an OBJECT library from the generated protobuf and grpc files.
    # Using OBJECT library ensures 'cmake --build . --target proto_gen_files' works in CI
    # while still being linkable via target_link_libraries.
    add_library(proto_gen_files OBJECT
        ${GENERATED_PROTO_SRCS}
        ${GENERATED_GRPC_SRCS}
        ${GENERATED_PROTO_HDRS}
        ${GENERATED_GRPC_HDRS}
    )
    target_include_directories(proto_gen_files PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}
        ${Protobuf_INCLUDE_DIRS}
        ${GRPC_INCLUDE_DIRS}
    )

    if(TARGET grpc_cpp_plugin)
        add_dependencies(proto_gen_files protobuf::protoc grpc_cpp_plugin)
    endif()

if(NOT BUILD_FOR_ANDROID)

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage")
        link_libraries(gcov)
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


# --- Fast DDS IDL Code Generation ---
set(GENERATED_IDL_DIR ${CMAKE_CURRENT_BINARY_DIR}/idl)
file(MAKE_DIRECTORY ${GENERATED_IDL_DIR})
set(GENERATED_IDL_SRCS "")
set(GENERATED_IDL_HDRS "")
set(PREV_GENERATED_FILE "")

# file(GLOB IDL_FILES "${PROJECT_IDL_DIR}/*.idl")
set(IDL_FILES
    "${PROJECT_IDL_DIR}/autoware/unique_identifier_msgs/msg/UUID.idl"
    "${PROJECT_IDL_DIR}/autoware/shape_msgs/msg/SolidPrimitive.idl"
    "${PROJECT_IDL_DIR}/autoware/builtin_interfaces/msg/Time.idl"
    "${PROJECT_IDL_DIR}/autoware/builtin_interfaces/msg/Duration.idl"
    "${PROJECT_IDL_DIR}/autoware/std_msgs/msg/Header.idl"
    "${PROJECT_IDL_DIR}/autoware/geometry_msgs/msg/Vector3.idl"
    "${PROJECT_IDL_DIR}/autoware/geometry_msgs/msg/Quaternion.idl"
    "${PROJECT_IDL_DIR}/autoware/geometry_msgs/msg/Point.idl"
    "${PROJECT_IDL_DIR}/autoware/geometry_msgs/msg/Point32.idl"
    "${PROJECT_IDL_DIR}/autoware/geometry_msgs/msg/Pose.idl"
    "${PROJECT_IDL_DIR}/autoware/geometry_msgs/msg/PoseWithCovariance.idl"
    "${PROJECT_IDL_DIR}/autoware/geometry_msgs/msg/PoseWithCovarianceStamped.idl"
    "${PROJECT_IDL_DIR}/autoware/geometry_msgs/msg/Twist.idl"
    "${PROJECT_IDL_DIR}/autoware/geometry_msgs/msg/TwistWithCovariance.idl"
    "${PROJECT_IDL_DIR}/autoware/geometry_msgs/msg/TwistWithCovarianceStamped.idl"
    "${PROJECT_IDL_DIR}/autoware/geometry_msgs/msg/Accel.idl"
    "${PROJECT_IDL_DIR}/autoware/geometry_msgs/msg/AccelWithCovariance.idl"
    "${PROJECT_IDL_DIR}/autoware/geometry_msgs/msg/AccelWithCovarianceStamped.idl"
    "${PROJECT_IDL_DIR}/autoware/geometry_msgs/msg/Polygon.idl"
    "${PROJECT_IDL_DIR}/autoware/geographic_msgs/msg/GeoPoint.idl"
    "${PROJECT_IDL_DIR}/autoware/geographic_msgs/msg/GeoPointStamped.idl"
    "${PROJECT_IDL_DIR}/VehicleData.idl"
    "${PROJECT_IDL_DIR}/VissRequest.idl"
    "${PROJECT_IDL_DIR}/autoware/GnssInsOrientation.idl"
    "${PROJECT_IDL_DIR}/autoware/GnssInsOrientationStamped.idl"
    "${PROJECT_IDL_DIR}/autoware/NavSatFix.idl"
    "${PROJECT_IDL_DIR}/autoware/PointCloud2.idl"
    "${PROJECT_IDL_DIR}/autoware/autoware_planning_msgs/msg/TrajectoryPoint.idl"
    "${PROJECT_IDL_DIR}/autoware/autoware_planning_msgs/msg/Trajectory.idl"
    "${PROJECT_IDL_DIR}/autoware/autoware_planning_msgs/msg/Path.idl"
    "${PROJECT_IDL_DIR}/autoware/autoware_planning_msgs/msg/PathPoint.idl"
    "${PROJECT_IDL_DIR}/autoware/autoware_adapi_v1_msgs/msg/VehicleKinematics.idl"
    "${PROJECT_IDL_DIR}/autoware/autoware_adapi_v1_msgs/msg/Gear.idl"
    "${PROJECT_IDL_DIR}/autoware/autoware_adapi_v1_msgs/msg/TurnIndicators.idl"
    "${PROJECT_IDL_DIR}/autoware/autoware_adapi_v1_msgs/msg/HazardLights.idl"
    "${PROJECT_IDL_DIR}/autoware/autoware_adapi_v1_msgs/msg/VehicleStatus.idl"
    "${PROJECT_IDL_DIR}/autoware/autoware_adapi_v1_msgs/msg/Route.idl"
    "${PROJECT_IDL_DIR}/autoware/autoware_adapi_v1_msgs/msg/RouteData.idl"
    "${PROJECT_IDL_DIR}/autoware/autoware_adapi_v1_msgs/msg/RoutePrimitive.idl"
    "${PROJECT_IDL_DIR}/autoware/autoware_adapi_v1_msgs/msg/RouteSegment.idl"
    "${PROJECT_IDL_DIR}/autoware/autoware_adapi_v1_msgs/msg/RouteState.idl"
    "${PROJECT_IDL_DIR}/autoware/autoware_adapi_v1_msgs/msg/MrmState.idl"
    "${PROJECT_IDL_DIR}/autoware/autoware_adapi_v1_msgs/msg/OperationModeState.idl"
    "${PROJECT_IDL_DIR}/autoware/autoware_adapi_v1_msgs/msg/DynamicObject.idl"
    "${PROJECT_IDL_DIR}/autoware/autoware_adapi_v1_msgs/msg/DynamicObjectArray.idl"
    "${PROJECT_IDL_DIR}/autoware/autoware_adapi_v1_msgs/msg/DynamicObjectKinematics.idl"
    "${PROJECT_IDL_DIR}/autoware/autoware_adapi_v1_msgs/msg/ObjectClassification.idl"
    "${PROJECT_IDL_DIR}/autoware/autoware_adapi_v1_msgs/msg/DynamicObjectPath.idl"
    "${PROJECT_IDL_DIR}/autoware/autoware_adapi_v1_msgs/msg/CooperationDecision.idl"
    "${PROJECT_IDL_DIR}/autoware/autoware_adapi_v1_msgs/msg/CooperationStatus.idl"
    "${PROJECT_IDL_DIR}/autoware/autoware_adapi_v1_msgs/msg/VelocityFactor.idl"
    "${PROJECT_IDL_DIR}/autoware/autoware_adapi_v1_msgs/msg/VelocityFactorArray.idl"
    "${PROJECT_IDL_DIR}/autoware/autoware_adapi_v1_msgs/msg/SteeringFactor.idl"
    "${PROJECT_IDL_DIR}/autoware/autoware_adapi_v1_msgs/msg/SteeringFactorArray.idl"
    "${PROJECT_IDL_DIR}/autoware/autoware_adapi_v1_msgs/msg/DiagGraphStatus.idl"
    "${PROJECT_IDL_DIR}/autoware/autoware_adapi_v1_msgs/msg/DiagNodeStatus.idl"
    "${PROJECT_IDL_DIR}/autoware/autoware_adapi_v1_msgs/msg/DiagGraphStruct.idl"
    "${PROJECT_IDL_DIR}/autoware/autoware_adapi_v1_msgs/msg/DiagLinkStruct.idl"
    "${PROJECT_IDL_DIR}/autoware/autoware_adapi_v1_msgs/msg/DiagNodeStruct.idl"
    "${PROJECT_IDL_DIR}/autoware/tier4_external_api_msgs/msg/Emergency.idl"
    "${PROJECT_IDL_DIR}/autoware/autoware_system_msgs/msg/HazardStatus.idl"
    "${PROJECT_IDL_DIR}/autoware/autoware_system_msgs/msg/HazardStatusStamped.idl"
    "${PROJECT_IDL_DIR}/autoware/diagnostic_msgs/msg/DiagnosticStatus.idl"
    "${PROJECT_IDL_DIR}/autoware/diagnostic_msgs/msg/KeyValue.idl"
    "${PROJECT_IDL_DIR}/autoware/tier4_system_msgs/msg/OperationModeAvailability.idl"
    "${PROJECT_IDL_DIR}/autoware/autoware_perception_msgs/msg/DetectedObject.idl"
    "${PROJECT_IDL_DIR}/autoware/autoware_perception_msgs/msg/DetectedObjectKinematics.idl"
    "${PROJECT_IDL_DIR}/autoware/autoware_perception_msgs/msg/DetectedObjects.idl"
    "${PROJECT_IDL_DIR}/autoware/autoware_perception_msgs/msg/ObjectClassification.idl"
    "${PROJECT_IDL_DIR}/autoware/autoware_perception_msgs/msg/Shape.idl"
    "${PROJECT_IDL_DIR}/autoware/autoware_perception_msgs/msg/TrafficLightElement.idl"
    "${PROJECT_IDL_DIR}/autoware/autoware_perception_msgs/msg/TrafficLightGroup.idl"
    "${PROJECT_IDL_DIR}/autoware/autoware_perception_msgs/msg/TrafficLightGroupArray.idl"
)

foreach(IDL_FILE ${IDL_FILES})
    if(${IDL_FILE} MATCHES "${PROJECT_IDL_DIR}/autoware/")
        file(RELATIVE_PATH REL_IDL_FILE "${PROJECT_IDL_DIR}/autoware" ${IDL_FILE})
    else()
        file(RELATIVE_PATH REL_IDL_FILE ${PROJECT_IDL_DIR} ${IDL_FILE})
    endif()
    string(REPLACE ".idl" "" REL_IDL_WE ${REL_IDL_FILE})

    set(GENERATED_HDR "${GENERATED_IDL_DIR}/${REL_IDL_WE}.h")
    set(GENERATED_PUBSUB_SRC "${GENERATED_IDL_DIR}/${REL_IDL_WE}PubSubTypes.cxx")
    set(GENERATED_PUBSUB_HDR "${GENERATED_IDL_DIR}/${REL_IDL_WE}PubSubTypes.h")
    set(GENERATED_TYPE_OBJECT_SRC "${GENERATED_IDL_DIR}/${REL_IDL_WE}TypeObjectSupport.cxx")
    set(GENERATED_TYPE_OBJECT_HDR "${GENERATED_IDL_DIR}/${REL_IDL_WE}TypeObjectSupport.h")
    
    get_filename_component(GENERATED_OUT_DIR ${GENERATED_HDR} DIRECTORY)

    # 기본 의존성 목록 (IDL 소스 파일)
    set(IDL_DEPENDENCIES ${IDL_FILE})
    
    # 만약 이전 파일이 존재한다면, 의존성에 추가하여 순차 실행을 강제함
    if(PREV_GENERATED_FILE)
        list(APPEND IDL_DEPENDENCIES ${PREV_GENERATED_FILE})
    endif()

    add_custom_command(
        OUTPUT ${GENERATED_HDR} ${GENERATED_PUBSUB_SRC} ${GENERATED_PUBSUB_HDR} ${GENERATED_TYPE_OBJECT_SRC} ${GENERATED_TYPE_OBJECT_HDR}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${GENERATED_OUT_DIR}
        COMMAND ${FASTDDSGEN_EXECUTABLE} -typeros2 -d ${GENERATED_OUT_DIR} -I ${PROJECT_IDL_DIR}/autoware -replace ${IDL_FILE}
        DEPENDS ${IDL_DEPENDENCIES}
        COMMENT "Generating C++ from ${IDL_FILE}"
    )

    # 현재 파일의 결과물(헤더 파일 중 하나)을 다음 루프의 의존성 변수로 지정
    set(PREV_GENERATED_FILE ${GENERATED_HDR})

    list(APPEND GENERATED_IDL_SRCS ${GENERATED_PUBSUB_SRC} ${GENERATED_TYPE_OBJECT_SRC})
    list(APPEND GENERATED_IDL_HDRS ${GENERATED_HDR} ${GENERATED_PUBSUB_HDR} ${GENERATED_TYPE_OBJECT_HDR})
endforeach()


# Create a static library from all generated IDL sources
add_library(idl_types STATIC ${GENERATED_IDL_SRCS})

target_include_directories(idl_types
    PUBLIC
    ${GENERATED_IDL_DIR}
    PRIVATE
    ${fastdds_INCLUDE_DIR}
    ${fastcdr_INCLUDE_DIR}
)
add_dependencies(proto_gen_files idl_types)

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

option(BUILD_FOR_ANDROID "Build for Android platform" OFF)
option(BUILD_TEST_CLIENT "Build VISS test client" OFF)

else()
    # --- Android Build Configuration ---
    # Protobuf sources are generated by the protobuf-gradle-plugin.
    # gRPC sources require additional setup.
endif()
# Add the main executable with all sources
set(VISS_APP_SOURCES
    src/app/VissServerApp.cpp
    src/core/VssTreeParser.cpp
    src/core/ServiceManager.cpp
    src/core/VssSubscriptionManager.cpp
    src/core/VssDataStore.cpp
    src/core/JsonSchemaValidator.cpp
    src/transport/grpc_transport.cpp
    src/transport/viss_websocket_transport.cpp
    src/providers/UdsProvider.cpp
    src/providers/GrpcProvider.cpp
    src/common/StatusCodes.cpp
    src/common/Utils.cpp
    src/common/ConfigManager.cpp
)

if(BUILD_FOR_ANDROID)
    list(APPEND VISS_APP_SOURCES src/app/jni_export.cpp)
    add_definitions(-DBUILD_FOR_ANDROID)
    # Protobuf/gRPC sources are added by the gradle plugin or need to be handled separately
else()
    list(APPEND VISS_APP_SOURCES
        src/app/main.cpp
        src/transport/dds_transport.cpp
        src/providers/DDSProvider.cpp
    )
endif()

# Configure output directories for all targets
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BUILD_DIR}/bin)
#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BUILD_DIR}/lib)
#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BUILD_DIR}/lib)

if(BUILD_FOR_ANDROID)
    add_library(${PROJECT_NAME} SHARED ${VISS_APP_SOURCES})
    target_link_libraries(${PROJECT_NAME} PRIVATE proto_gen_files)
    #set_target_properties(${PROJECT_NAME} PROPERTIES
    #    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/${ANDROID_ABI}")
else()
    add_executable(${PROJECT_NAME} ${VISS_APP_SOURCES})
    target_link_libraries(${PROJECT_NAME} PRIVATE proto_gen_files)
endif()


# Add include directories for dependencies
target_include_directories(${PROJECT_NAME}
    PRIVATE
    ${PROJECT_INCLUDE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${libwebsockets_SOURCE_DIR}/include
    ${libwebsockets_BINARY_DIR}
)

if(NOT BUILD_FOR_ANDROID)
    target_include_directories(${PROJECT_NAME}
        PRIVATE
        ${GENERATED_IDL_DIR}
        ${fastdds_INCLUDE_DIR}
    )
endif()


# Link libraries
# gRPC::grpc++ and protobuf::libprotobuf are the targets provided by FetchContent
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${GRPC_LIBRARY}
    ${PROTOBUF_LIBRARY}
    websockets_shared
    ${OPENSSL_LIBRARIES}
)

if(BUILD_FOR_ANDROID)
    find_library(log-lib log)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${log-lib}
    )
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE
        pthread
        idl_types
        fastcdr
        fastdds
    )
    # fastddsgen generates duplicate register_*_type_identifier symbols across
    # TypeObjectSupport files when multiple IDLs share the same typedef (e.g. double__36).
    # Allow multiple definitions to work around this fastddsgen bug.
    target_link_options(${PROJECT_NAME} PRIVATE -Wl,--allow-multiple-definition)
endif()


# Add gRPC test server executable
add_executable(grpc_test_server
    tests/grpc_test_server.cpp
    ${GENERATED_PROTO_SRCS}
    ${GENERATED_GRPC_SRCS}
)

target_link_libraries(grpc_test_server
    ${GRPC_LIBRARY}
    ${PROTOBUF_LIBRARY}
    pthread
)

# --- Mock UDS Provider Server ---
add_executable(mock_uds_provider_server tests/mock_uds_provider_server.cpp)
target_include_directories(mock_uds_provider_server PRIVATE ${PROJECT_INCLUDE_DIR})
target_link_libraries(mock_uds_provider_server PRIVATE pthread stdc++fs)

# Add Integrated VISS test client
if(NOT BUILD_FOR_ANDROID AND BUILD_TEST_CLIENT)
    # --- NCurses setup for Integrated Client ---
    set(CURSES_NEED_NCURSES TRUE)
    set(CURSES_NEED_WIDE TRUE)
    find_package(Curses REQUIRED)
    include_directories(${CURSES_INCLUDE_DIRS})
    
    add_executable(viss_integrated_client
        tests/viss_integrated_client.cpp
        tests/GrpcVissClient.cpp
        ${GENERATED_PROTO_SRCS}
        ${GENERATED_GRPC_SRCS}
    )

    target_include_directories(viss_integrated_client
        PRIVATE
        ${PROJECT_INCLUDE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests
        ${libwebsockets_SOURCE_DIR}/include
        ${libwebsockets_BINARY_DIR}
        ${GENERATED_IDL_DIR}
        ${fastdds_INCLUDE_DIR}
    )

    target_link_libraries(viss_integrated_client
        PRIVATE
        websockets_shared
        ${OPENSSL_LIBRARIES}
        ${CURSES_LIBRARIES}
        panelw
        menuw
        pthread
        ${GRPC_LIBRARY}
        ${PROTOBUF_LIBRARY}
    )
endif()


    


# --- Vehicle Info Manager & Dummy REST Server ---
if(NOT BUILD_FOR_ANDROID)
    find_package(CURL REQUIRED)
    find_package(ZLIB REQUIRED)

if(BUILD_TEST_CLOUD)    
    
    # 1. Dummy REST Server
    add_executable(dummy_rest_server
        tests/dummy_rest_server.cpp
        src/common/Utils.cpp
    )
    target_include_directories(dummy_rest_server PRIVATE ${PROJECT_INCLUDE_DIR} ${ZLIB_INCLUDE_DIRS})
    target_link_libraries(dummy_rest_server PRIVATE websockets_shared pthread ${ZLIB_LIBRARIES})
endif()

    # 2. Vehicle Info Manager
    add_executable(vehicle_info_manager
        src/app/VehicleInfoManager.cpp
        src/app/GrpcVissClient.cpp
        src/common/Utils.cpp
        ${GENERATED_PROTO_SRCS}
        ${GENERATED_GRPC_SRCS}
    )
    target_include_directories(vehicle_info_manager PRIVATE ${PROJECT_INCLUDE_DIR} tests ${CURL_INCLUDE_DIRS} ${ZLIB_INCLUDE_DIRS})
    target_link_libraries(vehicle_info_manager 
        PRIVATE 
        ${GRPC_LIBRARY} 
        ${PROTOBUF_LIBRARY} 
        ${CURL_LIBRARIES} 
        ${ZLIB_LIBRARIES} 
        pthread
    )
endif()

# --- Installation Setup ---

install(TARGETS vehicle_info_manager
    RUNTIME DESTINATION bin
)


# Install the main application executable
install(TARGETS ${PROJECT_NAME}
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

# Install the forest directory
install(DIRECTORY
    config/security/
    DESTINATION bin/config/security
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
	    unittest/json_schema_validator_test.cpp
#	    unittest/can_provider_test.cpp
	    unittest/dds_provider_test.cpp
	    unittest/grpc_provider_test.cpp
	    unittest/uds_provider_test.cpp
#	    unittest/network_provider_test.cpp
	    unittest/grpc_transport_test.cpp
	    unittest/dds_transport_test.cpp
	    unittest/viss_websocket_transport_test.cpp
	    unittest/status_codes_test.cpp
	    unittest/logger_level_test.cpp
	    unittest/viss_server_app_test.cpp
        unittest/main_test.cpp
	    src/app/VissServerApp.cpp
	    src/core/VssTreeParser.cpp
	    src/common/Utils.cpp
	    src/core/VssDataStore.cpp
	    src/core/VssSubscriptionManager.cpp
	    src/core/ServiceManager.cpp
	    src/core/JsonSchemaValidator.cpp
	    src/providers/DDSProvider.cpp
	    src/providers/GrpcProvider.cpp
	    src/providers/UdsProvider.cpp
	    src/transport/grpc_transport.cpp
	    src/transport/dds_transport.cpp
	    src/transport/viss_websocket_transport.cpp
	    src/common/StatusCodes.cpp
        src/common/ConfigManager.cpp
	)
	
	file(GLOB_RECURSE IDL_MSG_DIRS LIST_DIRECTORIES true "${GENERATED_IDL_DIR}/*")
	set(IDL_INCLUDE_DIRS "${GENERATED_IDL_DIR}")
	foreach(DIR ${IDL_MSG_DIRS})
		if(IS_DIRECTORY ${DIR})
			list(APPEND IDL_INCLUDE_DIRS ${DIR})
		endif()
	endforeach()

	target_include_directories(VehicleInfoMgrTest
	    PRIVATE
	    ${PROJECT_INCLUDE_DIR}
	    ${IDL_INCLUDE_DIRS}
        ${libwebsockets_SOURCE_DIR}/include
        ${libwebsockets_BINARY_DIR}
	)
	
	target_link_libraries(VehicleInfoMgrTest
	    PRIVATE
	    gtest_main
	    gmock_main
            ${PROTOBUF_LIBRARY}
            ${GRPC_LIBRARY}
            fastcdr
            fastdds
            idl_types
            proto_gen_files
            websockets_shared
	)
	
	target_link_options(VehicleInfoMgrTest PRIVATE -Wl,--allow-multiple-definition)
	
	include(GoogleTest)
	gtest_discover_tests(VehicleInfoMgrTest)

	install (TARGETS VehicleInfoMgrTest
		RUNTIME DESTINATION bin/unittest/vehicle_info_mgr
		LIBRARY DESTINATION lib
		ARCHIVE DESTINATION lib
	)

    install(DIRECTORY
        unittest/testfiles/
        DESTINATION bin/config/testfiles
    )


endif()
