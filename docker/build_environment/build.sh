#!/bin/bash

WORKDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Build options
export CMAKE_TEMP_DIR=".build"
export BUILD_MODE="Debug"
export CPU_CORES=`nproc`

export CROSSCOMPILING="FALSE"
export BUILD_UNITTEST="FALSE"
export EXTENSION_PLATFORM="ROS2"
export SDK_DIR=/opt
export TARGET_MF_NAME="ros2"
export TARGET_ARCH="x86_64"
export CMAKE_TOOLCHAIN_FILE=""


show_help() {
	cat << EOF
Usage: build.sh [OPTIONS]

Available options:
  --release                    Set the build mode to Release
  --build-unittest             Enable building unit tests
  --set-extension-platform={PLATFORM}
							   Set the target platform for extension
							   Supported platform:
								 - AAutoSAR
								 - AGL
								 - ROS2
  --help                       Show this help message

Examples:
  ./build.sh --release
  ./build.sh --build-unittest --set-extension-platform=ROS2
  ./build.sh --build-unittest --set-extension-platform=AGL
EOF
}

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
			--help)
				show_help
				exit 0
				;;
			--release)
				BUILD_MODE="Release"
				;;
			--build-unittest)
				BUILD_UNITTEST="TRUE"
				;;
			--set-extension-platform=*)
				EXTENSION_PLATFORM="${arg#*=}"
				;;
			--aarch64)
				#EXTENSION_PLATFORM="ROS2"
				CROSSCOMPILING="TRUE"
				TARGET_ARCH="aarch64"
				;;
			#--agl)
			#	EXTENSION_PLATFORM="AGL"
			#	TARGET_MF_NAME="agl"
			#	CROSSCOMPILING="TRUE"
			#	TARGET_ARCH="aarch64"
			#	;;
			*)
				echo "Unknown option. ${arg}"
				echo "Use --help to see available options."
				kill -SIGINT $$
				;;
		esac
	done
}

do_prepare_build_environment()
{
	if [[ ${CROSSCOMPILING} = TRUE ]]; then
		if [ "${EXTENSION_PLATFORM}" == "AGL" ]; then
			TARGET_MF_NAME="agl"
		fi
		#if [[ ${TARGET_MF_NAME} = "agl" ]]; then
		#	if [[ ! -d ${SDK_DIR}/agl-adt ]]; then
		#		curl -v -u "builder:obigo@se" \
		#			-o /tmp/oss-agl-ivi-flutter-aarch64-toolchain-18.94.0.sh \
		#			http://172.16.10.72:8081/repository/yocto-adt/agl/oss-agl-ivi-flutter-aarch64-toolchain-18.94.0.sh
		#		do_error_check

		#		chmod a+x /tmp/oss-agl-ivi-flutter-aarch64-toolchain-18.94.0.sh
		#		/tmp/oss-agl-ivi-flutter-aarch64-toolchain-18.94.0.sh -y -d ${SDK_DIR}/agl-adt
		#		do_error_check
		#	fi
		#	source ${SDK_DIR}/agl-adt/environment-setup-aarch64-agl-linux
		#	CMAKE_TOOLCHAIN_FILE="${OECORE_NATIVE_SYSROOT}/usr/share/cmake/OEToolchainConfig.cmake"
		#else
		CMAKE_TOOLCHAIN_FILE="${SDK_DIR}/cmake/toolchain-ubuntu-aarch64.cmake"
		#export PKG_CONFIG_PATH="${SDK_DIR}/sysroot-aarch64/usr/lib/aarch64-linux-gnu/pkgconfig"
		#export PKG_CONFIG_SYSROOT_DIR="${SDK_DIR}/sysroot-aarch64"
		#fi
	fi
	do_build_fastdds
	#do_build_fastdds_gen
}

do_build_fastdds() {
	set -e

	pushd ${SDK_DIR}/Fast-DDS >/dev/null 2>&1
	build_args=(
		--merge-install
		--parallel-workers ${CPU_CORES}
		--build-base build/${TARGET_MF_NAME}_${TARGET_ARCH}
		--install-base output/${TARGET_MF_NAME}_${TARGET_ARCH}
	)
	if [[ ${CROSSCOMPILING} = TRUE ]]; then
		build_args+=(
			--event-handlers console_direct+
			--cmake-force-configure
			--cmake-args -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
			--cmake-args -DCMAKE_VERBOSE_MAKEFILE=ON
		)
		# include AGL
		#if [[ ${TARGET_MF_NAME} = "ros2" ]]; then
		build_args+=(
			--cmake-args -DOPENSSL_INCLUDE_DIR=${SDK_DIR}/sysroot-aarch64/usr/include/aarch64-linux-gnu
		)
		#fi
	fi
	colcon build "${build_args[@]}"
	do_error_check
	mkdir -p ${WORKDIR}/output/${TARGET_ARCH}/lib
	cp -P output/${TARGET_MF_NAME}_${TARGET_ARCH}/lib/*.so* ${WORKDIR}/output/${TARGET_ARCH}/lib
	do_error_check
	popd >/dev/null 2>&1
}

#do_build_fastdds_gen() {
#	if [[ ! -z $(which fastddsgen) ]]; then return; fi
#	source ${SDK_DIR}/Fast-DDS/output/ros2_x86-64/setup.bash
#
#	pushd ${SDK_DIR}/Fast-DDS-Gen >/dev/null 2>&1
#	./gradlew clean &&./gradlew build -x test
#	do_error_check
#	./gradlew install
#	do_error_check
#	popd >/dev/null 2>&1
#}

do_build_cmd() {
	set -e

	build_ext="FALSE"
	if [ "${1}" == "ext" ]; then build_ext="TRUE"; fi

	echo ${WORKDIR}
	cd ${WORKDIR}	

	CMAKE_TEMP_DIR=".build/${BUILD_MODE}/${TARGET_MF_NAME}_${TARGET_ARCH}"
	mkdir -p ${CMAKE_TEMP_DIR}

	# Build with cmake
	pushd "${CMAKE_TEMP_DIR}" 1>/dev/null

	cmake_args=(
		-DBUILD_UNITTEST=${BUILD_UNITTEST}
		-DBUILD_EXTENSION=${build_ext}
		-DEXTENSION_PLATFORM=${EXTENSION_PLATFORM}
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON
		-DCMAKE_CXX_COMPILER_LAUNCHER=ccache
		-DCMAKE_C_COMPILER_LAUNCHER=ccache
		-DCMAKE_VERBOSE_MAKEFILE=ON
	)
	if [[ ${CROSSCOMPILING} = TRUE ]]; then
		cmake_args+=(
			-DCMAKE_CROSSCOMPILING=${CROSSCOMPILING}
			-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
		)
		# include AGL
		#if [[ ${TARGET_MF_NAME} = "ros2" && ${1} = "core" ]]; then
		if [[ ${1} = "core" ]]; then
			libs_dir="${SDK_DIR}/sysroot-aarch64/usr/lib/aarch64-linux-gnu"
			cmake_args+=(
				-DOPENSSL_INCLUDE_DIR=${SDK_DIR}/sysroot-aarch64/usr/include/aarch64-linux-gnu
				-DGRPC_INCLUDE_DIRS=${SDK_DIR}/sysroot-aarch64/usr/include
				-DGRPC_LIBRARIES="${libs_dir}/libgrpc++.so;${libs_dir}/libgrpc.so"
			)
		fi
	fi
	source ${SDK_DIR}/Fast-DDS/output/${TARGET_MF_NAME}_${TARGET_ARCH}/setup.bash

	cmake ${WORKDIR} "${cmake_args[@]}"
	do_error_check

	make -j${CPU_CORES}
	do_error_check

	make install -j${CPU_CORES}
	do_error_check

	popd 1>/dev/null
}

do_build()
{
	do_parse_option $*
	do_prepare_build_environment

	if [ "${EXTENSION_PLATFORM}" == "ROS2" ]; then
		do_build_cmd "core"
	fi
	do_build_cmd "ext"
	
	if [[ ${CROSSCOMPILING} = FALSE && ${BUILD_UNITTEST} = TRUE ]]; then
		EXTENSION_PLATFORM="AGL"
		do_build_cmd "ext"
	fi

	echo "Finish."
}

do_build $*
