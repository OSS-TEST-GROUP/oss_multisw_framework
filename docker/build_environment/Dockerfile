FROM ubuntu:22.04
LABEL org.opencontainers.image.authors="Tori.Obigo <tori.park@obigo.com>"

ARG SDK_DIR=/opt

USER root

# Set base environment variables
ENV DEBIAN_FRONTEND=noninteractive
ENV LANG=C.UTF-8
ENV LC_ALL=C.UTF-8
ENV PYTHONUNBUFFERED=1

ENV TZ=Asia/Seoul
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone
RUN rm -rf /var/lib/apt/lists/*

# Install system dependencies
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
	apt update && \
	apt install -y --no-install-recommends \
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
	libcurl4-openssl-dev \
	nlohmann-json3-dev \
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
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
	pip3 install --no-cache-dir \
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

RUN /bin/bash -c "\
	mkdir -p /tmp/fastddsgen \
	&& cd /tmp/fastddsgen \
	&& git clone https://github.com/eProsima/Fast-DDS-Gen.git \
	&& cd /tmp/fastddsgen/Fast-DDS-Gen \
	&& chmod +x gradlew \
	&& ./gradlew clean \
	&& ./gradlew build -x test \
	&& ./gradlew install \
"

RUN groupadd -r -g 1000 ossuser \
    && useradd -r -u 1000 -g ossuser -d /home/ossuser -s /bin/bash ossuser \
	&& mkdir -p /home/ossuser/workspace \
	&& chown -R ossuser:ossuser /home/ossuser \
	&& mkdir -p /opt \
	&& chown -R ossuser:ossuser /opt

USER ossuser
WORKDIR /home/ossuser/workspace

CMD [ "/bin/bash"]
