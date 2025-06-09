FROM ubuntu:22.04
LABEL org.opencontainers.image.authors="Tori.Obigo <tori.park@obigo.com>"

ARG LIBRARIES_INSTALL_DIR=/opt

USER root

# Install system dependencies
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    apt update && \
    apt install -y wget git cmake libasio-dev libtinyxml2-dev libssl-dev libp11-dev softhsm2 libengine-pkcs11-openssl python3 python3-pip openjdk-11-jre \
                   libyaml-cpp-dev && \
    pip3 install -U colcon-common-extensions vcstool

# Build libraries
RUN /bin/bash -c "mkdir -p ${LIBRARIES_INSTALL_DIR}/Fast-DDS && cd ${LIBRARIES_INSTALL_DIR}/Fast-DDS && \
    wget https://raw.githubusercontent.com/eProsima/Fast-DDS/master/fastdds.repos && mkdir src && \
    vcs import src < fastdds.repos && colcon build --packages-up-to fastdds && source ${LIBRARIES_INSTALL_DIR}/Fast-DDS/install/setup.bash && \
    cd ${LIBRARIES_INSTALL_DIR} && git clone https://github.com/eProsima/Fast-DDS-Gen.git && cd Fast-DDS-Gen && \
    chmod +x gradlew && ./gradlew clean && ./gradlew build ; ./gradlew install"


CMD /bin/bash