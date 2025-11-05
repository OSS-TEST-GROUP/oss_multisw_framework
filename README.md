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

For Aarch64(AGL, AADP, etc)
```
.build.sh --aarch64 --set-extension-platform=ROS2
or
.build.sh --aarch64 --set-extension-platform=AGL
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
