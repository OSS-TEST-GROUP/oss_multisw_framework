#!/bin/bash

# Docker
export TARGET_PLATFORM="linux/amd64"
export BUILDX_BUILDER_NAME="multiarch-builder"
export DOCKER_IMAGE_NAME="oss-mmwf-ros2-x86_64"
export DOCKER_TAG=""
export DOCKER_TARGET="build-stage"

# Build options
export CMAKE_TEMP_DIR=".build"
export BUILD_MODE="Debug"
export CPU_CORES=`nproc`

export CROSSCOMPILING=false
export BUILD_UNITTEST="FALSE"
export EXTENSION_PLATFORM="ROS2"

show_help() {
  cat << EOF
Usage: build.sh [OPTIONS]

Available options:
  --release                    Set the build mode to Release
  --build-unittest             Enable building unit tests
  --build-extension            Enable building extension framework
  --set-extension-platform={PLATFORM}
                               Set the target platform for extension
                               Supported platform:
                                 - AAutoSAR
                                 - AGL
                                 - ROS2
  --aarch64                    Set the target platform to Aarch64
  --help                       Show this help message

Examples:
  ./build.sh --release
  ./build.sh --build-unittest
  ./build.sh --aarch64 --set-extension-platform=ROS2
  ./build.sh --aarch64 --set-extension-platform=AGL
EOF
}

# Sudoer
setup_sudo_nopasswd() {
	if [ ! -f /etc/sudoers.d/nopasswd_cmds ]; then
		 cat << EOF > /tmp/sudoers_temp
# Allow $USER to run specific commands without password
$USER ALL=(ALL) NOPASSWD: /usr/bin/apt-get, /bin/systemctl, /bin/mkdir, /bin/chown, /usr/sbin/chroot, /bin/bash
EOF
		visudo -c -f /tmp/sudoers_temp
		if [ ${?} -eq 0 ]; then
			sudo cp /tmp/sudoers_temp /etc/sudoers.d/nopasswd_cmds
		else
			echo "Erros in sudoers configuration"
		fi
		rm /tmp/sudoers_cmds
	fi
}

# Git / Docker Tags
do_parse_git_info() {
	export GIT_HASH=$(git rev-parse --short HEAD)
	export GIT_BRANCH=$(git rev-parse --abbrev-ref HEAD)
	export GIT_VERSION=$(git describe --tags --always --dirty)
	export GIT_TAG=$(git describe --tags --abbrev=0 2>/dev/null || echo "v0.0.0")
	export BUILD_TIME=$(date +%Y%m%d%H%M)
	export IS_DIRTY=$(git diff --quiet || echo "-dirty")
	export COMMIT_COUNT=$(git rev-list --count HEAD)
}

do_sanitize_branch() {
	local branch="$1"

	branch=$(echo "$branch" | tr '[:upper:]' '[:lower:]')
	branch=$(echo "$branch" | sed 's/[^a-z0-9._-]/-/g')
	branch=$(echo "$branch" | sed 's/--*/-/g')
	branch=$(echo "$branch" | sed 's/^-\|-$//g')
	branch=$(echo "$branch" | cut -c1-50)
	branch=$(echo "$branch" | sed 's/-$//g')

	echo "${branch}"
}

do_abbreviate_branch() {
	local branch="$1"
	local max_length="${2:-20}"

	case "$branch" in
		dev_*|develop_*)
			echo "dev-$(echo $branch | cut -d'_' -f2 | cut -c1-15)"
			;;
		feature/*|feat/*)
			echo "feat-$(echo $branch | cut -d'/' -f2 | cut -c1-15)"
			;;
		feature-*|feat-*)
			echo "feat-$(echo $branch | cut -d'-' -f2 | cut -c1-15)"
			;;
		bugfix/*|fix/*)
			echo "fix-$(echo $branch | cut -d'/' -f2 | cut -c1-16)"
			;;
		hotfix/*)
			echo "hf-$(echo $branch | cut -d'/' -f2 | cut -c1-17)"
			;;
		release/*)
			echo "rel-$(echo $branch | cut -d'/' -f2 | sed 's/\./-/g' | cut -c1-15)"
			;;
		*)
			echo "$branch" | cut -c1-$max_length
			;;
	esac
}

do_parse_branch() {
	local branch=$(do_sanitize_branch ${1})
	branch=$(do_abbreviate_branch ${branch})

	if [[ ${branch} =~ ^dev.* ]]; then
		branch="develop-${GIT_HASH}"
	elif [[ ${branch} =~ ^feature.* ]]; then
		branch="feature-${GIT_HASH}"
	elif [[ ${branch} =~ ^hotfix.* ]]; then
		branch="hotfix-${GIT_HASH}"
	fi

	DOCKER_TAG=${branch}
}

# Build
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
        TARGET_PLATFORM="linux/arm64"
        CROSSCOMPILING=true
        ;;
      *)
        echo "Unknown option. ${arg}"
        echo "Use --help to see available options."
        kill -SIGINT $$
        ;;
    esac
  done
}

do_setup_docker_buildx()
{
	#docker buildx rm ${BUILDX_BUILDER_NAME} || true
	if ! $(docker buildx inspect ${BUILDX_BUILDER_NAME} >/dev/null 2>&1); then
		docker buildx create --name ${BUILDX_BUILDER_NAME} --driver docker-container --use || true

		echo "Check builder information..."
		docker buildx inspect --bootstrap

		echo "Check supported platform information..."
		docker buildx inspect | grep Platforms

		echo "Check Qemu support"
		docker run --privileged --rm tonistiigi/binfmt --install arm64
	fi
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
	
	mkdir -p ${PWD}/output

}

do_create_docker_image()
{
	build_ext="FALSE"
	target_arch="x86_64"
	toolchain_file=""

	if [ "${1}" == "ext" ]; then build_ext="TRUE"; fi

	ext=$(echo ${EXTENSION_PLATFORM} | tr '[:upper:]' '[:lower:]')
	if [ ${CROSSCOMPILING} = true ]; then
		target_arch="aarch64"
		toolchain_file="toolchain-${ext}-${target_arch}.cmake"
		DOCKER_TARGET="runtime-${1}-${ext}"
	else
		DOCKER_TARGET="runtime-${1}-x86_64"
	fi

	DOCKER_IMAGE_NAME="oss-mmwf-${1}-${ext}-${target_arch}"

	#if $(docker image inspect ${DOCKER_IMAGE_NAME} >/dev/null 2>&1); then
	#	return
	#fi

	#######################################################################
	# Debug
	#######################################################################
	#echo "Docker build target       : ${DOCKER_TARGET}"
	#echo "Docker image name         : ${DOCKER_IMAGE_NAME}"
	#echo "Docker tag name           : ${DOCKER_TAG}"
	#echo "Target Platform           : ${TARGET_PLATFORM}"
	#echo "Target Architecture       : ${target_arch}"
	#echo "Build Unittest            : ${BUILD_UNITTEST}"
	#echo "Build Type                : ${BUILD_MODE}"
	#echo "Build Extension           : ${build_ext}"
	#echo "Build Extension Platform  : ${ext}"
	#echo "Cross-compiling           : ${CROSSCOMPILING}"
	#######################################################################

	do_setup_docker_buildx

	docker buildx build \
		--platform ${TARGET_PLATFORM} \
		--build-arg TARGET_ARCH=${target_arch} \
		--build-arg SDKDIR="/opt" \
		--build-arg BUILD_UNITTEST=${BUILD_UNITTEST} \
		--build-arg BUILD_TYPE=${BUILD_MODE} \
		--build-arg BUILD_EXTENSION=${build_ext} \
		--build-arg EXTENSION_PLATFORM=${EXTENSION_PLATFORM} \
		--build-arg TOOLCHAIN_FILE=${toolchain_file} \
		--build-arg CROSSCOMPILING=$(if [ ${CROSSCOMPILING} = true ]; then echo "TRUE"; else echo "FLASE"; fi) \
		-f docker/Dockerfile \
		--target ${DOCKER_TARGET} \
		--load \
		-t ${DOCKER_IMAGE_NAME}:${DOCKER_TAG} \
		.
}

do_run_container()
{
	docker_cmd=
	tty_opts=

	if [ -t 1 ]; then
		tty_opts="-it"
	fi
}

do_build()
{
	echo "Start build ${PWD}"

	do_parse_git_info
	do_parse_branch ${GIT_BRANCH}

	do_parse_option $*
	do_prepare_build_environment

	if [ "${EXTENSION_PLATFORM}" == "ROS2" ]; then
		do_create_docker_image "core"
	fi
	do_create_docker_image "ext"
	
	echo "Finish."
}

do_build $*
