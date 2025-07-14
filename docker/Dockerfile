# Multi Platform
ARG TARGETPLATFORM
ARG BUILDPLATFORM
ARG TARGETOS
ARG TARGETARCH

###################################################################################
# Base image
###################################################################################
FROM --platform=${BUILDPLATFORM} ubuntu:22.04 AS base-setup
#LABEL org.opencontainers.image.authors="Brian.doh <brian.doh@obigo.com>"

# Show platform information
RUN echo "Building on $BUILDPLATFORM, targeting $TARGETPLATFORM"

# Set base environment variables
ENV DEBIAN_FRONTEND=noninteractive
ENV LANG=C.UTF-8
ENV LC_ALL=C.UTF-8
ENV PYTHONUNBUFFERED=1

RUN apt-get update && apt-get upgrade -y

###################################################################################
# Install Build Environment
###################################################################################
FROM base-setup AS builder

LABEL maintainer="Brian.doh <brian.doh@obigo.com>"
LABEL description="C++ Static Analysis and Build environment"
LABEL version="0.0.1"
LABEL platform=$TARGETPLATFORM

ARG LIBRARIES_INSTALL_DIR=/opt

RUN apt-get install -y --no-install-recommends \
    build-essential \
    wget \
    git \
    cmake \
    libasio-dev \
    ccache \
    libtinyxml2-dev \
    libssl-dev \
    libp11-dev \
    softhsm2 \
    libengine-pkcs11-openssl \
    python3 \
    python3-pip \
    openjdk-11-jre \
    libyaml-cpp-dev \
    ninja-build \
    curl \
    unzip \
    cppcheck \
    clang \
    clang-tools \
    clang-tidy \
    clang-format \
    iwyu \
    lcov \
    gcovr \
    valgrind \
    perf-tools-unstable \
    gdb \
    gdbserver \
    netcat \
    telnet \
    vim \
    htop \
    tree \
    jq \
    && rm -rf /var/lib/apt/lists/* 

# Install Python packages
RUN pip3 install --no-cache-dir \
    colcon-common-extensions \
    vcstool \
    # Static Analysis
    cpplint==1.6.1 \
    cmake-format==0.6.13 \
    cmakelang==0.6.13 \
    pre-commit==3.5.0 \
    black==23.9.1 \
    isort==5.12.0

# Build and Install Fast-DDS
RUN /bin/bash -c "mkdir -p ${LIBRARIES_INSTALL_DIR}/Fast-DDS \
    && cd ${LIBRARIES_INSTALL_DIR}/Fast-DDS \
    && wget https://raw.githubusercontent.com/eProsima/Fast-DDS/3.2.2/fastdds.repos \
    && mkdir src \
    && vcs import src < fastdds.repos \
    && colcon build --packages-up-to fastdds \
    && source ${LIBRARIES_INSTALL_DIR}/Fast-DDS/install/setup.bash \
    && cd ${LIBRARIES_INSTALL_DIR} \
    && git clone https://github.com/eProsima/Fast-DDS-Gen.git \
    && cd Fast-DDS-Gen \
    && chmod +x gradlew \
	&& ls -al \
    && ./gradlew clean \
    && ./gradlew build ; ./gradlew install"

ARG TARGETARCH
RUN case "${TARGETARCH}" in \
        amd64) echo "Optimizing for x86_64" ;; \
        arm64) echo "Optimizing for aarch64" && \
               echo "export CFLAGS='-march=armv8-a'" >> /etc/environment ;; \
        *) echo "Unsupported architecture: ${TARGETARCH}" && exit 1 ;; \
    esac

ENV CCACHE_DIR=/tmp/ccache
RUN mkdir -p ${CCACHE_DIR} && \
    ccache --set-config max_size=2G && \
    ccache --set-config compression=true && \
    ccache --show-stats

WORKDIR /workspace

###################################################################################
# Build stage
###################################################################################
FROM builder AS build-stage

ARG BUILD_UNITTEST=FALSE
ARG EXTENSION_PLATFORM=ROS2
COPY . .

ENV CFLAGS="-O2"
ENV CXXFLAGS="-O2"
ENV LDFLAGS="-O1"
ENV CMAKE_BUILD_PARALLEL_LEVEL=4
ENV MAKEFLAGS="$(nproc)"
ENV BUILD_MODE=${BUILD_UNITTEST}

RUN mkdir -p /workspace/.build
RUN /bin/bash -c "source $LIBRARIES_INSTALL_DIR/Fast-DDS/install/setup.bash \
    && cd /workspace/.build \
    && cmake ../ \
        -DBUILD_UNITTEST=${BUILD_UNITTEST} \
        -DEXTENSION_PLATFORM=${EXTENSION_PLATFORM} \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
        -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
        -DCMAKE_C_COMPILER_LAUNCHER=ccache \
    && make -j${MAKEFLAGS} \
    && make install \
    && cd -"
