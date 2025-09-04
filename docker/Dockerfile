# Gobal 
ARG SDKDIR=/opt
ARG TARGET_ARCH="x86_64"

###################################################################################
# Base image
###################################################################################
FROM --platform=$BUILDPLATFORM ubuntu:22.04 AS base-setup
#LABEL org.opencontainers.image.authors="Brian.doh <brian.doh@obigo.com>"

# Show platform information
RUN echo "Building on ${BUILDPLATFORM}"

# Set base environment variables
ENV DEBIAN_FRONTEND=noninteractive
ENV LANG=C.UTF-8
ENV LC_ALL=C.UTF-8
ENV PYTHONUNBUFFERED=1

ENV TZ=Asia/Seoul
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

RUN rm -rf /var/lib/apt/lists/*
RUN apt-get update && apt-get upgrade -y

###################################################################################
# Install Build Environment
###################################################################################
FROM base-setup AS builder-x86_64

ARG SDKDIR="/opt"
ARG TARGET_ARCH="x86_64"

LABEL maintainer="Brian.doh <brian.doh@obigo.com>"
LABEL description="Setup Build environment and build/install Fast-DDS"
LABEL version="0.0.1"

WORKDIR /root/workspace

RUN apt-get install -y --no-install-recommends \
    # Base
    build-essential \
    gdb \
    wget \
    git \
    cmake \
    ccache \
    ninja-build \
    curl \
    file \
    unzip \
    rsync \
    # Dependencies
    libasio-dev \
    libtinyxml2-dev \
    libssl-dev \
    libp11-dev \
    libyaml-cpp-dev \
    protobuf-compiler \
    protobuf-compiler-grpc \
    libprotobuf-dev \
    libgrpc-dev \
    libgrpc++-dev \
    softhsm2 \
    libengine-pkcs11-openssl \
    libgtest-dev \
    python3 \
    python3-pip \
    openjdk-11-jre \
    gcc-aarch64-linux-gnu \
    g++-aarch64-linux-gnu \
    # Static Analysis
    cppcheck \
    sloccount \
    clang \
    clang-tools \
    clang-tidy \
    clang-format \
    iwyu \
    lcov \
    gcovr \
    perf-tools-unstable \
    && rm -rf /var/lib/apt/lists/*

# Install Python packages
RUN pip3 install --no-cache-dir \
    # Build Environment
    colcon-common-extensions \
    vcstool \
    # Static Analysis
    lcov_cobertura \
    cpplint \
    cmake-format \
    cmakelang \
    pre-commit \
    black \
    isort

ENV CCACHE_DIR=/tmp/ccache

RUN mkdir -p ${CCACHE_DIR} && \
    ccache --set-config max_size=2G && \
    ccache --set-config compression=true && \
    ccache --show-stats

RUN /bin/bash -c "mkdir -p ${SDKDIR}/Fast-DDS \
    && cd ${SDKDIR}/Fast-DDS \
    && wget https://raw.githubusercontent.com/eProsima/Fast-DDS/3.2.2/fastdds.repos \
    && mkdir src \
    && vcs import src < fastdds.repos \
    && colcon build \
        --merge-install \
        --build-base build_x86_64 \
        --install-base install_x86_64 \
    && cd - \
"

RUN mkdir -p /tmp/oss-x86_64/lib \
	&& cp -rpa ${SDKDIR}/Fast-DDS/install_x86_64/lib/*.so* /tmp/oss-x86_64/lib

RUN /bin/bash -c " \
    source ${SDKDIR}/Fast-DDS/install_x86_64/setup.bash \
    && cd ${SDKDIR} \
    && git clone https://github.com/eProsima/Fast-DDS-Gen.git \
    && cd Fast-DDS-Gen \
    && chmod +x gradlew \
    && ls -al \
    && ./gradlew clean \
    && ./gradlew build -x test; ./gradlew install \
"

###################################################################################
# Sysroot
###################################################################################
FROM builder-x86_64 AS builder-aarch64

LABEL maintainer="Brian.doh <brian.doh@obigo.com>"
LABEL description="Create a sysroot for Aarch64 platform using debootstrap"
LABEL version="0.0.1"

RUN sed -i 's|http://archive.ubuntu.com|http://mirror.kakao.com|g' /etc/apt/sources.list \
	&& apt update \
	&& apt install -y --no-install-recommends \
	debootstrap \
	qemu-user-static \
	&& rm -rf /var/lib/apt/lists/*

RUN debootstrap --arch=arm64 \
	jammy \
	${SDKDIR}/sysroot-aarch64 \
	http://kr.ports.ubuntu.com/ubuntu-ports/

RUN cat <<EOF > ${SDKDIR}/sysroot-aarch64/etc/apt/sources.list.d/ubuntu-aarch64.list
deb http://ports.ubuntu.com/ubuntu-ports jammy main universe multiverse restricted
deb http://ports.ubuntu.com/ubuntu-ports jammy-updates main universe multiverse restricted
deb http://ports.ubuntu.com/ubuntu-ports jammy-security main universe multiverse restricted
EOF

RUN chroot ${SDKDIR}/sysroot-aarch64 /bin/bash -c "\
	DEBIAN_FRONTEND=noninteractive apt update \
	&& apt install -y --no-install-recommends \
	libasio-dev \
	libtinyxml2-dev \
	libyaml-cpp-dev \
	protobuf-compiler \
	protobuf-compiler-grpc \
	libprotobuf-dev \
	libgrpc-dev \
	libgrpc++-dev \
	libssl-dev \
	&& rm -rf /var/lib/apt/lists/* \
"

RUN cat <<EOF > ${SDKDIR}/toolchain-ros2-aarch64.cmake
# Target system and processor architecture
SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_SYSTEM_PROCESSOR aarch64)

# Cross compiler location
SET(CMAKE_C_COMPILER /usr/bin/aarch64-linux-gnu-gcc)
SET(CMAKE_CXX_COMPILER /usr/bin/aarch64-linux-gnu-g++)

# Where target environment includes and libraries are
SET(CMAKE_SYSROOT /opt/sysroot-aarch64/)
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

# Fast-DDS 
RUN /bin/bash -c "\
    cd ${SDKDIR}/Fast-DDS \
    && colcon build \
        --merge-install \
        --build-base build_aarch64 \
        --install-base install_aarch64 \
        --cmake-force-configure \
        --cmake-args \
        -DCMAKE_TOOLCHAIN_FILE=${SDKDIR}/toolchain-ros2-aarch64.cmake \
    && cd - \
"

RUN mkdir -p /tmp/oss-aarch64/lib \
	&& cp -rpa ${SDKDIR}/Fast-DDS/install_aarch64/lib/*.so* /tmp/oss-aarch64/lib

###################################################################################
# Build stage
###################################################################################
FROM builder-${TARGET_ARCH} AS build-stage

ARG BUILD_UNITTEST=FALSE
ARG BUILD_EXTENSION=FALSE
ARG EXTENSION_PLATFORM=ROS2
ARG BUILD_TYPE=Release
ARG CROSSCOMPILING=FALSE
ARG TOOLCHAIN_FILE=

COPY . .

ENV BUILD_MODE=${BUILD_TYPE}
ENV MAKEFLAGS="$(nproc)"

RUN ccache -C

SHELL ["/bin/bash", "-c"]

RUN CMAKE_ARGS=(\
		"-DEXTENSION_PLATFORM=${EXTENSION_PLATFORM}" \
		"-DBUILD_EXTENSION=${BUILD_EXTENSION}" \
		"-DBUILD_UNITTEST=${BUILD_UNITTEST}" \
		"-DCMAKE_EXPORT_COMPILE_COMMANDS=ON" \
		"-DCMAKE_CXX_COMPILER_LAUNCHER=ccache" \
		"-DCMAKE_C_COMPILER_LAUNCHER=ccache" \
	) \
	&& if [ ${CROSSCOMPILING} = TRUE ]; then \
		CMAKE_ARGS+=(\
			"-DCMAKE_CROSSCOMPILING=${CROSSCOMPILING}" \
			"-DCMAKE_TOOLCHAIN_FILE=${SDKDIR}/${TOOLCHAIN_FILE}" \
		); \
	fi \
    && source ${SDKDIR}/Fast-DDS/install_${TARGET_ARCH}/setup.bash \
    && mkdir -p /root/workspace/.build_${TARGET_ARCH} \
    && cd /root/workspace/.build_${TARGET_ARCH} \
	&& cmake ../ "${CMAKE_ARGS[@]}" \
    && make install -j $(nproc) \
    && cd /root/workspace

RUN mkdir -p /tmp/oss-${TARGET_ARCH} \
	&& cp -rpa /root/workspace/output/${TARGET_ARCH}/${BUILD_TYPE}/* /tmp/oss-${TARGET_ARCH}

###################################################################################
# Runtime
###################################################################################
FROM ubuntu:22.04 AS runtime

LABEL maintainer="Brian.doh <brian.doh@obigo.com>"
LABEL description="Runtime Image"
LABEL version="0.0.1"

# Set base environment variables
ENV DEBIAN_FRONTEND=noninteractive
ENV LANG=C.UTF-8
ENV LC_ALL=C.UTF-8

ENV TZ=Asia/Seoul
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone
RUN sed -i 's|http://archive.ubuntu.com|http://mirror.kakao.com|g' /etc/apt/sources.list \
	&& apt-get update && apt-get upgrade -y

RUN apt-get install -y --no-install-recommends \
    libtinyxml2-9 \
    libssl3 \
    libp11-3 \
    softhsm2 \
    libengine-pkcs11-openssl \
    libyaml-cpp0.7 \
    && rm -rf /var/lib/apt/lists/*

RUN groupadd -r -g 1000 ossuser \
    && useradd -r -u 1000 -g ossuser -d /oss -s /bin/bash ossuser \
    && mkdir -p /oss \
    && mkdir -p /oss/lib \
    && chown -R ossuser:ossuser /oss

###################################################################################
# ROS2 x86_64
###################################################################################
# Core
###################################################################################
FROM runtime AS runtime-core-x86_64

COPY --from=build-stage --chown=ossuser:ossuser /tmp/oss-x86_64/core/bin /oss
COPY --from=build-stage --chown=ossuser:ossuser /tmp/oss-x86_64/lib /oss/lib

WORKDIR /oss
USER ossuser

ENV LD_LIBRARY_PATH=/oss/lib

CMD ["./lifecycle_mgr"]
###################################################################################
# Extension
###################################################################################
FROM runtime AS runtime-ext-x86_64

RUN mkdir -p /oss

COPY --from=build-stage --chown=ossuser:ossuser /tmp/oss-x86_64/extension/ros2/bin /oss
COPY --from=build-stage --chown=ossuser:ossuser /tmp/oss-x86_64/lib /oss/lib

WORKDIR /oss
USER ossuser

ENV LD_LIBRARY_PATH=/oss/lib

###################################################################################
# ROS2 Aarch64
###################################################################################
# Core
###################################################################################
FROM runtime AS runtime-core-ros2

COPY --from=build-stage --chown=ossuser:ossuser /tmp/oss-aarch64/core/bin /oss
COPY --from=build-stage --chown=ossuser:ossuser /tmp/oss-aarch64/lib /oss/lib

WORKDIR /oss
USER ossuser

ENV LD_LIBRARY_PATH=/oss/lib

CMD ["./lifecycle_mgr"]

###################################################################################
# Extension
###################################################################################
FROM runtime AS runtime-ext-ros2

COPY --from=build-stage --chown=ossuser:ossuser /tmp/oss-aarch64/extension/ros2/bin /oss
COPY --from=build-stage --chown=ossuser:ossuser /tmp/oss-aarch64/lib /oss/lib

WORKDIR /oss
USER ossuser

ENV LD_LIBRARY_PATH=/oss/lib

###################################################################################
# Static Analysis stage
###################################################################################
FROM build-stage AS static-analysis

#RUN mkdir -p /root/workspace/reports \
#    && find /root/workspace/service/ \
#        -not -path "*/unittest/*" \
#        -name "*.cpp" \
#        > /root/workspace/reports/files_to_cppcheck.txt || true
#
## Run CppCheck
#RUN cppcheck --version \
#    && cppcheck --enable=all \
#        --xml \
#        --xml-version=2 \
#        --output-file=/root/workspace/reports/cppcheck-result.xml \
#        --suppress=missingIncludeSystem \
#        --suppress=unmatchedSuppression \
#        --suppress=unusedFunction \
#        --std=c++20 \
#        --file-list=/root/workspace/files_to_cppcheck.txt || true

# Run SlocCount
#RUN mkdir -p /tmp/sloccount-clean \
#    && find /root/workspace/service/ \
#        -not -path "*/unittest/*" \
#        \( -name "*.cpp" -o -name "*.hpp" -o -name "*.h" \) \
#        -exec cp --parents {} /tmp/sloccount-clean/ \\; 2>/dev/null || true
#
#RUN cd /tmp/sloccount-clean \
#    && sloccount --duplicates --wide --details . \
#        > /root/workspace/reports/sloccount-detailed.xml || true \
#    && sloccount . > /root/workspace/reports/sloccount-summary.txt || true \
#    && cd /root/workspace

# CppLint
#RUN find /root/workspace/service/ \
#        -not -path "*/unittest/*" \
#        \( -name "*.cpp" -o -name "*.hpp" -o -name "*.h" \) \
#        > /root/workspace/reports/files_to_lintcheck.txt || true
#
#RUN if [ -f /root/workspace/files_to_lintcheck.txt ]; then \
#        cat /root/workspace/files_to_lintcheck.txt | xargs cpplint \
#            --filter=-whitespace/tab,-legal/copyright,-build/include_order \
#            --linelength=120 \
#            --extensions=cpp,cc,h,hpp \
#            --output=vs7 \
#            --counting=detailed \
#            > /root/workspace/reports/cpplint-result.txt 2>&1 || true \
#    else \
#        echo "No files to check" > /root/workspace/reports/cpplint-result.txt \
#    fi

###################################################################################
# Debug
###################################################################################
FROM --platform=$BUILDPLATFORM build-stage AS build-debug
