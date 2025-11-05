# oss_multisw_framework


## Fetch the repository

```
git clone git@github.com:obigo-gov/oss_multisw_framework.git
```

## How to build the OSS MW Framework

For x86-64(unittest, static analysis, debugging)
```
.build.sh
or
.build.sh --build-unittest
```

For aarch64(AGL, AADP, etc)
```
.build.sh --aarch64 --set-extension-platform=ROS2
or
.build.sh --aarch64 --set-extension-platform=AGL
```

## How to run containers

For testing or debugging Core and Extension for ROS2 : x86-64
```
docker run --privileged --gpus all --rm -it -u 1000:1000 \
           -w /home/ossuser/workspace \
           -v output/x86_64:/home/ossuser/workspace/output \
           -v /tmp/.X11-unix:/tmp/.X11-unix -v \
           ${HOME}/.ssh:/root/.ssh \
           -e DISPLAY=unix:1 \
           -e LD_LIBRARY_PATH=/home/ossuser/workspace/output/lib \
           --network=host --cap-add=NET_RAW --cap-add=NET_ADMIN \
           172.16.10.216:80/oss/oss-mmf-core:latest \
           /bin/bash
```

For MMF Core and Extention for ROS2 on AADP : aarch64
```
docker run -d --gpus all \
           --privileged \
           --network=host --cap-add=NET_RAW --cap-add=NET_ADMIN \
           -v /dev/shm:/dev/shm \
           172.16.10.216:80/oss/oss-mmf-core:latest
and
docker run -d --gpus all \
           --privileged \
           --network=host --cap-add=NET_RAW --cap-add=NET_ADMIN \
           172.16.10.216:80/oss/oss-mmf-ext-ros2:latest
```

For Extention for AGL and HMI on Orin using docker compose: aarch64
```
cd /root/oss
docker compose -f oss-hmi.yaml up -d
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
