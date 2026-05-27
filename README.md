# oss_multisw_framework


## Fetch the repository

```
git clone git@github.com:obigo-gov/oss_multisw_framework.git
```

## How to build the OSS Multi-Middleware Framework

For x86-64(unittest, static analysis, debugging)
```
./build.sh
or
./build.sh --build-unittest
```

For aarch64(AGL, AADP, etc)
```
./build.sh --aarch64 --set-extension-platform=ROS2
or
./build.sh --aarch64 --set-extension-platform=AGL
```

## How to run containers

For testing or debugging Core and Extension for ROS2 : x86-64
```
docker run --privileged --gpus all \
    --rm -it -u 1000:1000 \
    -w /home/ossuser/workspace \
    -v ${PWD}/output/x86_64:/home/ossuser/workspace/output \
    -v /tmp/.X11-unix:/tmp/.X11-unix \
    -v /run/user/1000:/run/user/1000 \
    -v ${HOME}/.ssh:/root/.ssh \
    -e DISPLAY=unix:1 \
    -e LD_LIBRARY_PATH=/home/ossuser/workspace/output/lib \
    --network=host --cap-add=NET_RAW --cap-add=NET_ADMIN \
    registry.obigo.com/oss/oss-mmf-builder:latest
    /bin/bash
```
Binary path
```
/home/ossuser/workspace/output/Debug/core/bin # core
/home/ossuser/workspace/output/Debug/extension/ros2/bin # extension - ros2
/home/ossuser/workspace/output/Debug/extension/agl/bin # extensioin - agl
```
## How to build the container images to release

You can build the container images after build MMF to release
```
./build.sh --release --aarch64

docker buildx create --name mmf-buildx --driver docker-container --use || true

docker buildx build \
    --no-cache --platform="linux/arm64" \
    --build-arg BUILD_TYPE=Release \
    --build-arg TARGETARCH=aarch64 \
    -f docker/Dockerfile.rel \
    --target runtime-mmf-core \
    --load \
    -t registry.obigo.com/oss/oss-mmf-core:latest \
    .

docker buildx build \
    --no-cache --platform="linux/arm64" \
    --build-arg BUILD_TYPE=Release \
    --build-arg TARGETARCH=aarch64 \
    -f docker/Dockerfile.rel \
    --target runtime-mmf-ext-ros2 \
    --load \
    -t registry.obigo.com/oss/oss-mmf-ext-ros2:latest \
    .

./build.sh --release --aarch64 --set-extension-platform=AGL

docker buildx build \
    --no-cache --platform="linux/arm64" \
    --build-arg BUILD_TYPE=Release \
    --build-arg TARGETARCH=aarch64 \
    -f docker/Dockerfile.rel \
    --target runtime-mmf-ext-agl \
    --load \
    -t registry.obigo.com/oss/oss-mmf-ext-agl:latest \
    .
```

For MMF Core and Extention for ROS2 on AADP using docker compose: aarch64
```
cd ~/oss
docker compose up -d
```

For Extention for AGL and HMI on Orin using docker compose: aarch64
```
cd /root/oss
docker compose up -d
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
