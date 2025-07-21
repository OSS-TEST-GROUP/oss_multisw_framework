# Multi Platform
ARG BUILDPLATFORM
ARG TARGETOS
ARG TARGETARCH

###################################################################################
# Base image
###################################################################################
FROM --platform=${BUILDPLATFORM} ubuntu:22.04 AS base-setup
#LABEL org.opencontainers.image.authors="Brian.doh <brian.doh@obigo.com>"

# Show platform information
RUN echo "Building on ${BUILDPLATFORM}"

# Set base environment variables
ENV DEBIAN_FRONTEND=noninteractive
ENV LANG=C.UTF-8
ENV LC_ALL=C.UTF-8
ENV PYTHONUNBUFFERED=1

RUN rm -rf /var/lib/apt/lists/*
RUN apt-get update && apt-get upgrade -y

###################################################################################
# Install Build Environment
###################################################################################
FROM base-setup AS builder

LABEL maintainer="Brian.doh <brian.doh@obigo.com>"
LABEL description="C++ Static Analysis and Build environment"
LABEL version="0.0.1"

ARG LIBRARIES_INSTALL_DIR=/opt

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
    unzip \
    rsync \
    # Dependencies
    libasio-dev \
    libtinyxml2-dev \
    libssl-dev \
    libp11-dev \
    libyaml-cpp-dev \
    softhsm2 \
    libengine-pkcs11-openssl \
    python3 \
    python3-pip \
    openjdk-11-jre \
    gcc-aarch64-linux-gnu \
    g++-aarch64-linux-gnu \
    && rm -rf /var/lib/apt/lists/*

# Install Python packages
RUN pip3 install --no-cache-dir \
    # Build Environment
    colcon-common-extensions \
    vcstool

# Build and Install Fast-DDS
RUN mkdir -p /opt
COPY sysroot-aarch64 /opt/sysroot-aarch64
COPY toolchain-aarch64.cmake /opt

RUN /bin/bash -c "mkdir -p ${LIBRARIES_INSTALL_DIR}/Fast-DDS \
    && cd ${LIBRARIES_INSTALL_DIR}/Fast-DDS \
    && wget https://raw.githubusercontent.com/eProsima/Fast-DDS/3.2.2/fastdds.repos \
    && mkdir src \
    && vcs import src < fastdds.repos \
    # AMD64
    && colcon build \
        --build-base build_amd64 \
        --install-base install_amd64 \
        --packages-up-to fastdds \
    # ARM64
    && colcon build --merge-install \
        --build-base build_aarch64 \
        --install-base install_aarch64 \
        --cmake-force-configure \
        --cmake-args \
        -DCMAKE_TOOLCHAIN_FILE=/opt/toolchain-aarch64.cmake \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    && cd - \
"

RUN /bin/bash -c "\
    source ${LIBRARIES_INSTALL_DIR}/Fast-DDS/install_amd64/setup.bash \
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

ARG BUILD_UNITTEST=TRUE
ARG EXTENSION_PLATFORM=ROS2
COPY . .

ENV CFLAGS="-O2"
ENV CXXFLAGS="-O2"
ENV LDFLAGS="-O1"
ENV MAKEFLAGS="$(nproc)"
ENV BUILD_MODE=Release

# AMD64
RUN mkdir -p /workspace/.build
RUN /bin/bash -c "source $LIBRARIES_INSTALL_DIR/Fast-DDS/install_amd64/setup.bash \
    && cd /workspace/.build \
    && BUILD_MODE=Debug cmake ../ \
        -DBUILD_UNITTEST=${BUILD_UNITTEST} \
        -DEXTENSION_PLATFORM=${EXTENSION_PLATFORM} \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
        -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
        -DCMAKE_C_COMPILER_LAUNCHER=ccache \
    && make -j${MAKEFLAGS} \
    && make install \
    && cd - \
"

# ARM64
RUN /bin/bash -c "\
    source $LIBRARIES_INSTALL_DIR/Fast-DDS/install_aarch64/setup.bash \
    && mkdir -p /workspace/.build_aarch64 \
    && cd /workspace/.build_aarch64 \
    && cmake ../ \
        -DBUILD_UNITTEST=FALSE \
        -DEXTENSION_PLATFORM=${EXTENSION_PLATFORM} \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
        -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
        -DCMAKE_C_COMPILER_LAUNCHER=ccache \
    && make -j${MAKEFLAGS} \
    && make install \
    && cd - \
"

###################################################################################
# Runtime
###################################################################################
FROM ubuntu:22.04 AS runtime

LABEL maintainer="Brian.doh <brian.doh@obigo.com>"
LABEL description="Runtime Environment"
LABEL version="0.0.1"

# Set base environment variables
ENV DEBIAN_FRONTEND=noninteractive
ENV LANG=C.UTF-8
ENV LC_ALL=C.UTF-8

RUN apt-get update && apt-get upgrade -y

RUN apt-get install -y --no-install-recommends \
    libasio-dev \
    libtinyxml2-dev \
    libssl-dev \
    libp11-dev \
    softhsm2 \
    libengine-pkcs11-openssl \
    python3 \
    python3-pip \
    openjdk-11-jre \
    libyaml-cpp-dev \
    && rm -rf /var/lib/apt/lists/*

RUN pip3 install --no-cache-dir \
    colcon-common-extensions

RUN groupadd -r ossuser \
    && useradd -r -g ossuser -d /oss -s /bin/bash ossuser \
    && mkdir -p /oss \
    && chown -R ossuser:ossuser /oss

WORKDIR /oss

#COPY --from=build-stage --chown=ossuser:ossuser /workspace/output/
###################################################################################
# Debug
###################################################################################
FROM builder AS debug

RUN apt update && apt-get install -y --no-install-recommends \
    valgrind \
    gdbserver \
    netcat \
    telnet \
    vim \
    htop \
    tree \
    jq \
    && rm -rf /var/lib/apt/lists/*

ARG BUILD_UNITTEST=FALSE
ARG EXTENSION_PLATFORM=ROS2
COPY . .

ENV CFLAGS="-O2"
ENV CXXFLAGS="-O2"
ENV LDFLAGS="-O1"
ENV MAKEFLAGS="$(nproc)"
ENV BUILD_MODE=Debug

RUN mkdir -p /workspace/.build
RUN /bin/bash -c "source $LIBRARIES_INSTALL_DIR/Fast-DDS/install/setup.bash \
    && cd /workspace/.build \
    && cmake ../ \
        -DBUILD_UNITTEST=${BUILD_UNITTEST} \
        -DEXTENSION_PLATFORM=${EXTENSION_PLATFORM} \
        -DCMAKE_BUILD_PARALLEL_LEVEL=${MAKEFLAGS} \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
        -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
        -DCMAKE_C_COMPILER_LAUNCHER=ccache \
    && make -j${MAKEFLAGS} \
    && make install \
    && cd -"

###################################################################################
# Static Analysis stage
###################################################################################
FROM build-stage AS static-analysis

RUN apt-get install -y --no-install-recommends \
    # Static Analysis
    cppcheck \
    sloccount \
    clang \
    clang-tools \
    clang-tidy \
    clang-format \
    coverage \
    iwyu \
    lcov \
    gcovr \
    perf-tools-unstable \
    && rm -rf /var/lib/apt/lists/*

RUN pip3 install --no-cache-dir \
    lcov_cobertura \
    cpplint \
    cmake-format \
    cmakelang \
    pre-commit \
    black \
    isort

RUN mkdir -p /workspace/reports

#RUN mkdir -p /workspace/reports \
#    && find /workspace/service/ \
#        -not -path "*/unittest/*" \
#        -name "*.cpp" \
#        > /workspace/reports/files_to_cppcheck.txt || true
#
## Run CppCheck
#RUN cppcheck --version \
#    && cppcheck --enable=all \
#        --xml \
#        --xml-version=2 \
#        --output-file=/workspace/reports/cppcheck-result.xml \
#        --suppress=missingIncludeSystem \
#        --suppress=unmatchedSuppression \
#        --suppress=unusedFunction \
#        --std=c++20 \
#        --file-list=/workspace/files_to_cppcheck.txt || true

# Run SlocCount
#RUN mkdir -p /tmp/sloccount-clean \
#    && find /workspace/service/ \
#        -not -path "*/unittest/*" \
#        \( -name "*.cpp" -o -name "*.hpp" -o -name "*.h" \) \
#        -exec cp --parents {} /tmp/sloccount-clean/ \\; 2>/dev/null || true
#
#RUN cd /tmp/sloccount-clean \
#    && sloccount --duplicates --wide --details . \
#        > /workspace/reports/sloccount-detailed.xml || true \
#    && sloccount . > /workspace/reports/sloccount-summary.txt || true \
#    && cd /workspace

# CppLint
#RUN find /workspace/service/ \
#        -not -path "*/unittest/*" \
#        \( -name "*.cpp" -o -name "*.hpp" -o -name "*.h" \) \
#        > /workspace/reports/files_to_lintcheck.txt || true
#
#RUN if [ -f /workspace/files_to_lintcheck.txt ]; then \
#        cat /workspace/files_to_lintcheck.txt | xargs cpplint \
#            --filter=-whitespace/tab,-legal/copyright,-build/include_order \
#            --linelength=120 \
#            --extensions=cpp,cc,h,hpp \
#            --output=vs7 \
#            --counting=detailed \
#            > /workspace/reports/cpplint-result.txt 2>&1 || true \
#    else \
#        echo "No files to check" > /workspace/reports/cpplint-result.txt \
#    fi