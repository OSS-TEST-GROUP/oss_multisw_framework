# oss_multisw_framework


## Fetch the repository

```
git clone ssh://lotte-autops.com/oss_group1/oss_multisw_framework.git
```

## Setup the build environment

Build the docker image for the development environment.
```
docker build -t oss-mw-dev-test:0.1 -f docker/build_environment/Dockerfile .
```

Run the docker with the built image.
```
docker run -it --gpus all -v /${HOME}/workspace:/root/workspace -v /run/user/1000:/run/user/1000 -v /tmp/.X11-unix:/tmp/.X11-unix -v ${HOME}/.ssh:/root/.ssh -e DISPLAY=unix:1 oss-mw-dev-test:0.1 bash
```

## How to build the OSS MW Framework

```
source docker/build_environment/setup_environment.sh
.build.sh
or
.build.sh --build-unittest
```

## How to use dds idl files

You can add your idl files into the path below.
```
oss_multisw_framework/idl/dds
```

The dds generated source files will be generated at 'oss_multisw_framework/output/[debug/release]/generated/idl'.
You can refer it through CMakeLists.txt like the below.
```
link_libraries ( fastcdr fastdds )  // 라이브러리 추가
set(GENERATED_DIR "${CMAKE_INSTALL_PREFIX}/generated/idl")
set(GENERATED_DDS_FILES ${GENERATED_DIR}/DataTypeTestPubSubTypes.cxx
                        ${GENERATED_DIR}/DataTypeTestTypeObjectSupport.cxx
                        ${GENERATED_DIR}/DataTypeTestSimplePubSubTypes.cxx
```

## About the unittest
You can use gtest on each services by including the line below on your unittest's CMakeLists.txt
```
link_libraries ( gtest gmock )
```
