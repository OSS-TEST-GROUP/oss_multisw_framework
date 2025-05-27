#!/bin/bash

# Toolchain options
#TOOLCHAIN_PATH="/home/tori/toolchain/arm/bin"
#TOOLCHAIN_NAME="arm-none-linux-gnueabi"
#TOOLCHAIN_PREFIX="${TOOLCHAIN_PATH}/${TOOLCHAIN_NAME}"

#ARCH_OPTION="-march=armv7-a -mfpu=vfpv3-d16 -mfloat-abi=softfp -mcpu=cortex-a9"

#export SYSROOT_PATH="/home/tori/sysroots"
#export PKG_CONFIG_PATH=${SYSROOT_PATH}/usr/lib/pkgconfig

# Compiler target system options
#export PKG_CONFIG_SYSROOT_DIR=${SYSROOT_PATH}
#export PKG_CONFIG_LIBDIR=${PKG_CONFIG_PATH}

#export CC="${TOOLCHAIN_PREFIX}-gcc ${ARCH_OPTION} --sysroot=${SYSROOT_PATH}"
#export CXX="${TOOLCHAIN_PREFIX}-g++ ${ARCH_OPTION} --sysroot=${SYSROOT_PATH}"
#export AS="${TOOLCHAIN_PREFIX}-as"
#export LD="${TOOLCHAIN_PREFIX}-ld --sysroot=${SYSROOT_PATH}"

# Compiler options
export CFLAGS="-O2" # -Wno-psabi -fexceptions"
export CXXFLAGS="-O2" # -fcheck-new -Wno-psabi -fexceptions -fpermissive"
export LDFLAGS="-O1" # -Wl,-O1 -Wl,--hash-style=gnu -Wl,--gc-sections"


# Build options
export CMAKE_TEMP_DIR=".build"
export BUILD_MODE="Debug"
export CPU_CORES=`nproc`

export BUILD_UNITTEST="FALSE"

do_error_check()
{
	if [ "${?}" != "0" ]; then
		echo "An error happend in ${FUNCNAME[1]}."
		kill -SIGINT $$
	fi
}

do_parse_option()
{
  for arg in $*
  do
    case ${arg} in
      --release)
        BUILD_MODE="Release"
        ;;
      --build-unittest)
        BUILD_UNITTEST="TRUE"
        ;;
      *)
        echo "Unknown option. ${arg}"
        kill -SIGINT $$
        ;;
    esac
  done
}

do_prepare_build_environment()
{
  if [ "${BUILD_MODE}" == "Release" ]; then
    CMAKE_TEMP_DIR="${CMAKE_TEMP_DIR}_release"
  else
    CMAKE_TEMP_DIR="${CMAKE_TEMP_DIR}_debug"
    export CXXFLAGS="${CXXFLAGS} -g"
  fi

  # Prepare cmake cache.
  if [ ! -d "${CMAKE_TEMP_DIR}" ]; then
    mkdir "${CMAKE_TEMP_DIR}"
  fi
}

do_build()
{
  echo "Start build ${PWD}"

  do_parse_option $*
  do_prepare_build_environment

  # Build with cmake
  pushd "${CMAKE_TEMP_DIR}" 1>/dev/null

  cmake ../ -DBUILD_UNITTEST=${BUILD_UNITTEST}
  do_error_check

  make -j${CPU_CORES}
  do_error_check

  make install
  #do_error_check

  popd 1>/dev/null

  echo "Finish."
}

do_build $*
