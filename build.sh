#!/bin/bash

WORKDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Docker
export TARGET_PLATFORM="linux/amd64"
export BUILDX_BUILDER_NAME="multiarch-builder"
export BUILDER_CONTAINER_NAME="oss-mmf-builder"
export BUILDER_CONTAINER_TAG="latest"
export CONTAINER_IMAGE_NAME="oss-mmwf-ros2-x86_64"
export CONTAINER_IMAGE_TAG=""
export REGISTRY_ADDRESS="registry.obigo.com"
export REPO_ADDRESS="repo.obigo.com"
export S3_ARTIFACT_URL="s3://oss-artifact"
export AGL_SDK_SOURCE="${AGL_SDK_SOURCE:-nexus}"
# 레지스트리 자격증명은 하드코딩하지 않는다 (공개 저장소로 전달되는 파일).
#   · pull  : oss 프로젝트가 public 이므로 인증 불필요
#   · push  : CI 는 Harbor 로봇 계정(oss-ci) 을 REGISTRY_ID/REGISTRY_PWD 로 주입,
#             로컬은 미리 `docker login registry.obigo.com` (docker/registry-auth.sh 참조)
export REGISTRY_ID="${REGISTRY_ID:-}"
export REGISTRY_PWD="${REGISTRY_PWD:-}"
export DOCKER_TARGET="build-stage"

# Build options
export CMAKE_TEMP_DIR=".build"
export BUILD_MODE="Debug"
export CPU_CORES=`nproc`

export CROSSCOMPILING=false
export BUILD_UNITTEST="FALSE"
export BUILD_SILS="FALSE"
export EXTENSION_PLATFORM="ROS2"
export SDK_DIR=${HOME}/obigo-sdk
#export TARGET_MF_NAME="aadp"

# 레지스트리 로그인 헬퍼 (ossuser 단일 계정, pull·push 공용).
source "${WORKDIR}/docker/registry-auth.sh"

show_help() {
	cat << EOF
Usage: build.sh [OPTIONS]

Available options:
  --release                    Set the build mode to Release
  --build-unittest             Enable building unit tests
  --build-sils                 Enable building the SiLS vECU Phase 1 PoC
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
$USER ALL=(ALL) NOPASSWD: ALL
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

	CONTAINER_IMAGE_TAG=${branch}
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
				BUILD_MODE="Debug"
				;;
			--build-sils)
				BUILD_SILS="TRUE"
				;;
			--set-extension-platform=*)
				EXTENSION_PLATFORM="${arg#*=}"
				;;
			--aarch64)
				TARGET_PLATFORM="linux/arm64"
				CROSSCOMPILING=true
				;;
			#--agl)
			#	TARGET_PLATFORM="linux/arm64"
			#	#TARGET_MF_NAME="agl"
			#	EXTENSION_PLATFORM="AGL"
			#	CROSSCOMPILING=true
			#	;;
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

do_set_private_registry() {
	if cat ~/.docker/config.json 2>/dev/null \
		| grep "${REGISTRY_ADDRESS}:80"; then
		docker logout "${REGISTRY_ADDRESS}:80"
	fi

	# oss 프로젝트는 public 이므로 pull 은 인증이 필요 없다. 인증이 필요한 것은 push 뿐이다.
	# 따라서 자격증명이 주입되지 않았으면 로그인을 건너뛴다 (자격증명 없이도 로컬 빌드 가능).
	if cat ~/.docker/config.json 2>/dev/null | grep -q "${REGISTRY_ADDRESS}"; then
		:  # 이미 로그인되어 있음
	elif [[ -z "${REGISTRY_ID}" || -z "${REGISTRY_PWD}" ]]; then
		echo "INFO: 레지스트리 자격증명이 없어 로그인을 건너뜁니다 (익명 pull 사용)."
		echo "      push 가 필요하면 REGISTRY_ID/REGISTRY_PWD 를 주입하거나"
		echo "      'docker login ${REGISTRY_ADDRESS}' 를 먼저 실행하세요."
	else
		printf '%s' "${REGISTRY_PWD}" | docker login ${REGISTRY_ADDRESS} -u "${REGISTRY_ID}" --password-stdin
		do_error_check
	fi
	## we can access https://registry.obigo.com now. so we don't need below login process anymore.
	#if ! docker info --format '{{json .RegistryConfig}}' 2>/dev/null \
	#	| grep -q "${REGISTRY_ADDRESS}"; then
	#	if [[ ! -f /etc/docker/daemon.json ]]; then
	#		sudo mkdir -p /etc/docker
	#		echo '{"insecure-registries": []}' | sudo tee /etc/docker/daemon.json > /dev/null
	#	else
	#		jq 'if has("insecure-registries") then . else . + {"insecure-registries": []} end' /etc/docker/daemon.json > /tmp/daemon.json
	#		sudo cp /etc/docker/daemon.json /etc/docker/daemon.json.bak
	#		sudo cp /tmp/daemon.json /etc/docker/daemon.json
	#	fi
	#	sudo sed -i "s/172.16.10.216:80/${REGISTRY_ADDRESS}/" /etc/docker/daemon.json
	#	sudo sed -i "s/\[\]/[\"${REGISTRY_ADDRESS}\"]/" /etc/docker/daemon.json
	#	sudo systemctl restart docker
	#
	#	echo "${REGISTRY_PWD}" | docker login ${REGISTRY_ADDRESS} -u ${REGISTRY_ID} --password-stdin
	#	do_error_check
	#fi
}

do_install_agl_build_environment() {
	if [[ ${CROSSCOMPILING} != true ]]; then return; fi
	#if [[ "${TARGET_MF_NAME}" != "agl" ]]; then return; fi

	if ! dpkg -s curl &>/dev/null; then
		sudo apt-get update
		sudo apt install -y --no-install-recommends curl
	fi

	if [[ ! -d ${SDK_DIR}/agl-adt ]]; then
		case "${AGL_SDK_SOURCE}" in
			nexus)
				# AGL SDK 다운로드 자격증명은 환경변수로 주입한다 (하드코딩 금지).
				#   예: REPO_ID=<id> REPO_PWD=<pw> ./build.sh --aarch64 ...
				: "${REPO_ID:?REPO_ID 를 지정하세요 (AGL SDK 다운로드 계정)}"
				: "${REPO_PWD:?REPO_PWD 를 지정하세요 (AGL SDK 다운로드 비밀번호)}"
				echo "==> AGL SDK 취득: Nexus (https://${REPO_ADDRESS}/repository/yocto-adt/agl/oss-agl-ivi-flutter-aarch64-toolchain-18.94.0.sh)"
				curl -v -u "${REPO_ID}:${REPO_PWD}" \
					-o /tmp/oss-agl-ivi-flutter-aarch64-toolchain-18.94.0.sh \
					https://${REPO_ADDRESS}/repository/yocto-adt/agl/oss-agl-ivi-flutter-aarch64-toolchain-18.94.0.sh
				do_error_check
				;;
			s3)
				# AWS CLI 로그인(인증) 상태 확인
				if ! aws sts get-caller-identity >/dev/null 2>&1 && ! aws s3 ls s3://oss-artifact >/dev/null 2>&1; then
					if [[ -n "${AWS_ACCESS_KEY_ID}" && -n "${AWS_SECRET_ACCESS_KEY}" ]]; then
						echo "==> AWS CLI credentials 환경변수가 발견되어 자동 설정을 진행합니다."
						aws configure set aws_access_key_id "${AWS_ACCESS_KEY_ID}"
						aws configure set aws_secret_access_key "${AWS_SECRET_ACCESS_KEY}"
						aws configure set region "${AWS_DEFAULT_REGION:-ap-northeast-2}"
					else
						echo "ERROR: AWS CLI 인증 정보가 설정되어 있지 않습니다 (AGL_SDK_SOURCE=s3)."
						echo "       사전에 'aws configure' 명령으로 로그인하거나,"
						echo "       AWS_ACCESS_KEY_ID=... AWS_SECRET_ACCESS_KEY=... ./build.sh 형식으로 환경변수를 전달하세요."
						echo "       기본 경로(Nexus)를 쓰려면 AGL_SDK_SOURCE 변수를 지정하지 마세요."
						exit 1
					fi
				fi
				echo "==> AGL SDK 취득: S3 (${S3_ARTIFACT_URL}/yocto-adt/agl/oss-agl-ivi-flutter-aarch64-toolchain-18.94.0.sh)"
				aws s3 cp ${S3_ARTIFACT_URL}/yocto-adt/agl/oss-agl-ivi-flutter-aarch64-toolchain-18.94.0.sh \
					/tmp/oss-agl-ivi-flutter-aarch64-toolchain-18.94.0.sh
				do_error_check
				;;
			*)
				echo "ERROR: AGL_SDK_SOURCE='${AGL_SDK_SOURCE}' — nexus 또는 s3 만 가능합니다."
				exit 1
				;;
		esac

		chmod a+x /tmp/oss-agl-ivi-flutter-aarch64-toolchain-18.94.0.sh
		/tmp/oss-agl-ivi-flutter-aarch64-toolchain-18.94.0.sh -y -d ${SDK_DIR}/agl-adt
		do_error_check
	fi
}

do_install_ros_build_environment() {
	if [[ ${CROSSCOMPILING} != true ]]; then return; fi
	#if [[ "${TARGET_MF_NAME}" != "aadp" ]]; then return; fi

	if ! dpkg -s debootstrap >/dev/null 2>&1; then
		sudo apt-get update
		sudo apt-get install -y --no-install-recommends \
			debootstrap \
			qemu-user-static
	fi	

	if [[ ! -f ${SDK_DIR}/.sysroot-aarch64.done ]]; then
		if [[ -d ${SDK_DIR}/sysroot-aarch64 ]]; then
			sudo rm -rf ${SDK_DIR}/sysroot-aarch64
		fi
	fi

	if [[ ! -d ${SDK_DIR}/sysroot-aarch64 ]]; then
		mkdir -p ${SDK_DIR}
		# Create sysroot for Ubuntu Jammy Arm64
		sudo debootstrap --arch=arm64 \
			jammy \
			${SDK_DIR}/sysroot-aarch64 \
			http://ports.ubuntu.com/ubuntu-ports/
		do_error_check
		source_dir=${SDK_DIR}/sysroot-aarch64/etc/apt/sources.list.d
		if [[ ! -f ${SDK_DIR}/ubuntu-arm64.list ]]; then
			cat <<EOF > ${SDK_DIR}/ubuntu-arm64.list
deb [arch=arm64] http://ports.ubuntu.com/ubuntu-ports jammy main universe multiverse restricted
deb [arch=arm64] http://ports.ubuntu.com/ubuntu-ports jammy-updates main universe multiverse restricted
deb [arch=arm64] http://ports.ubuntu.com/ubuntu-ports jammy-security main universe multiverse restricted
EOF
		fi
		sudo cp -rpa ${SDK_DIR}/ubuntu-arm64.list ${source_dir}
		do_error_check
	
		sudo chroot ${SDK_DIR}/sysroot-aarch64 /bin/bash -c '\
			DEBIAN_FRONTEND=noninteractive apt update \
			&& apt install -y --no-install-recommends \
				curl \
				libasio-dev \
				libtinyxml2-dev \
				libyaml-cpp-dev \
				protobuf-compiler \
				protobuf-compiler-grpc \
				libprotobuf-dev \
				libgrpc-dev \
				libgrpc++-dev \
				libssl-dev \
				libcurl4-openssl-dev \
				nlohmann-json3-dev \
			&& rm -rf /var/lib/apt/lists/*
		'
		do_error_check
		touch ${SDK_DIR}/.sysroot-aarch64.done
	fi

	if [[ ! -d ${SDK_DIR}/cmake ]]; then
		mkdir -p ${SDK_DIR}/cmake
		if [[ ! -f ${SDK_DIR}/cmake/toolchain-ubuntu-aarch64.cmake ]]; then
			cat <<EOF > ${SDK_DIR}/cmake/toolchain-ubuntu-aarch64.cmake
# Target system and processor architecture
SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_SYSTEM_PROCESSOR aarch64)

# Cross compiler location
SET(CMAKE_C_COMPILER /usr/bin/aarch64-linux-gnu-gcc)
SET(CMAKE_CXX_COMPILER /usr/bin/aarch64-linux-gnu-g++)

# Where target environment includes and libraries are
SET(CMAKE_FIND_ROOT_PATH /opt/sysroot-aarch64/)

# Search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# Search for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Optional: set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON )

set(CMAKE_CXX_FLAGS_INIT "-Wno-error")
EOF
		fi
	fi
}

do_prepare_build_environment()
{
	setup_sudo_nopasswd
	do_install_ros_build_environment
	#do_install_agl_build_environment
	do_import_dep_libraries
	do_set_private_registry
}

do_import_dep_libraries() {
	if [[ -d ${SDK_DIR}/Fast-DDS ]]; then return; fi

	if [[ ! -d ${SDK_DIR}/venv ]]; then
		python3 -m venv ${SDK_DIR}/venv
	fi
	source ${SDK_DIR}/venv/bin/activate
	if [[ -z $(which vcs) ]]; then
		pip3 install --no-cache-dir \
			colcon-common-extensions \
			vcstool \
			>/dev/null 2>&1
		do_error_check
	fi
	
	mkdir -p ${SDK_DIR}/Fast-DDS/src
	pushd ${SDK_DIR}/Fast-DDS >/dev/null 2>&1
	wget https://raw.githubusercontent.com/eProsima/Fast-DDS/3.2.2/fastdds.repos >/dev/null 2>&1
	vcs import src < fastdds.repos >/dev/null 2>&1
	do_error_check
	popd >/dev/null 2>&1
	sed -i "s#-Wpedantic##g" ${SDK_DIR}/Fast-DDS/src/fastdds/CMakeLists.txt
	do_error_check
	deactivate

	#pushd ${SDK_DIR} >/dev/null 2>&1
    #git clone https://github.com/eProsima/Fast-DDS-Gen.git >/dev/null 2>&1
	#popd >/dev/null 2>&1
}

do_pull_builder_image()
{
	img_name="${REGISTRY_ADDRESS}/oss/${BUILDER_CONTAINER_NAME}:${BUILDER_CONTAINER_TAG}"
	if ! curl -u "${REGISTRY_ID}:${REGISTRY_PWD}" -s https://${REGISTRY_ADDRESS}/v2/_catalog 2>/dev/null \
		| grep -q "${BUILDER_CONTAINER_NAME}"; then
		docker build -t ${REGISTRY_ADDRESS}/oss/${BUILDER_CONTAINER_NAME}:${BUILDER_CONTAINER_TAG} \
			-f ${PWD}/docker/Dockerfile .
		do_error_check
		# 빌더 이미지 push (ossuser 단일 계정 — pull·push 공용).
		registry_login >/dev/null 2>&1 || true
		docker push ${REGISTRY_ADDRESS}/oss/${BUILDER_CONTAINER_NAME}:${BUILDER_CONTAINER_TAG} || \
			echo "[build_all] push to registry failed (502/auth) — using local image" >&2
	else
		docker pull ${img_name}
	fi
}

do_build_unittest_image()
{
	#build_ext="FALSE"
	#target_arch="x86_64"
	#toolchain_file=""

	#if [ "${1}" == "ext" ]; then build_ext="TRUE"; fi

	#ext=$(echo ${EXTENSION_PLATFORM} | tr '[:upper:]' '[:lower:]')
	#if [ ${CROSSCOMPILING} = true ]; then
	#	target_arch="aarch64"
	#	toolchain_file="toolchain-${ext}-${target_arch}.cmake"
	#	DOCKER_TARGET="runtime-${1}-${ext}"
	#else
	#	DOCKER_TARGET="runtime-${1}-x86_64"
	#fi

	#CONTAINER_IMAGE_NAME="oss-mmwf-${1}-${ext}-${target_arch}"

	#if $(docker image inspect ${CONTAINER_IMAGE_NAME} >/dev/null 2>&1); then
	#	return
	#fi

	#######################################################################
	# Debug
	#######################################################################
	#echo "Docker build target       : ${DOCKER_TARGET}"
	#echo "Docker image name         : ${CONTAINER_IMAGE_NAME}"
	#echo "Docker tag name           : ${CONTAINER_IMAGE_TAG}"
	#echo "Target Platform           : ${TARGET_PLATFORM}"
	#echo "Target Architecture       : ${target_arch}"
	#echo "Build Unittest            : ${BUILD_UNITTEST}"
	#echo "Build Type                : ${BUILD_MODE}"
	#echo "Build Extension           : ${build_ext}"
	#echo "Build Extension Platform  : ${ext}"
	#echo "Cross-compiling           : ${CROSSCOMPILING}"
	#######################################################################

	if [[ ${BUILD_UNITTEST} = FALSE ]]; then return; fi
	if [[ ! -d ${WORKDIR}/output/x86_64/Debug/core/bin/unittest ]]; then return; fi

	do_setup_docker_buildx

	docker buildx build \
		--platform "linux/amd64" \
		--build-arg TARGETARCH="x86_64" \
		-f docker/Dockerfile.rel \
		--target unittest-x86_64 \
		--load \
		-t oss-mmf-unittest:latest \
		.
}

do_start_build_mmf()
{
	set -e

	img_name="${REGISTRY_ADDRESS}/oss/${BUILDER_CONTAINER_NAME}:${BUILDER_CONTAINER_TAG}"
	build_args=$*

	mkdir -p .build
	mkdir -p output
	docker run --rm -i \
		--name mmf-builder \
		-u 1000:1000 \
		-w /home/ossuser/workspace \
		-e SILS_ENABLE_FMU \
		-v ${SDK_DIR}:/opt \
		-v ${PWD}/docker/scripts/build.sh:/home/ossuser/workspace/build.sh \
		-v ${PWD}/CMakeLists.txt:/home/ossuser/workspace/CMakeLists.txt \
		-v ${PWD}/idl:/home/ossuser/workspace/idl \
		-v ${PWD}/service:/home/ossuser/workspace/service \
		-v ${PWD}/output:/home/ossuser/workspace/output \
		-v ${PWD}/.build:/home/ossuser/workspace/.build \
		${img_name} \
		/home/ossuser/workspace/build.sh ${build_args}
}

do_build()
{
	do_parse_git_info
	do_parse_branch ${GIT_BRANCH}

	do_parse_option $*
	do_prepare_build_environment
	do_pull_builder_image

	do_start_build_mmf $*
	#do_build_unittest_image
}

do_build $*
