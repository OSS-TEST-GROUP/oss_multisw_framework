FROM ros:humble-ros-base

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
    wget https://raw.githubusercontent.com/eProsima/Fast-DDS/3.2.2/fastdds.repos && mkdir src && \
    vcs import src < fastdds.repos && colcon build --packages-up-to fastdds && source ${LIBRARIES_INSTALL_DIR}/Fast-DDS/install/setup.bash && \
    cd ${LIBRARIES_INSTALL_DIR} && git clone https://github.com/eProsima/Fast-DDS-Gen.git && cd Fast-DDS-Gen && \
    chmod +x gradlew && ./gradlew clean && ./gradlew build ; ./gradlew install"

    
# Optionally, install explicitly (usually not needed)
RUN apt-get update && apt-get install -y \
    ros-humble-rmw-fastrtps-cpp \
    ros-humble-rmw-fastrtps-shared-cpp \
    && rm -rf /var/lib/apt/lists/*

COPY ros2_hello /ros2_hello
RUN chmod +x /ros2_hello

ENTRYPOINT ["/bin/bash", "-c", "source /opt/ros/humble/setup.bash && exec /ros2_hello"]
