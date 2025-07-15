FROM ubuntu:22.04

ARG LIBRARIES_INSTALL_DIR=/opt

USER root

# 1. 필수 패키지 설치 및 locale 설정
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    locales \
    curl \
    gnupg2 \
    lsb-release \
    software-properties-common \
    python3-pip

RUN add-apt-repository universe

RUN curl -sSL https://raw.githubusercontent.com/ros/rosdistro/master/ros.key -o /usr/share/keyrings/ros-archive-keyring.gpg

RUN echo "deb [arch=amd64 signed-by=/usr/share/keyrings/ros-archive-keyring.gpg] http://packages.ros.org/ros2/ubuntu $(lsb_release -cs) main" > /etc/apt/sources.list.d/ros2.list

RUN apt-get update && apt-get install -y \
    python3-colcon-common-extensions \
    python3-vcstool


RUN apt-get update && apt-get install -y \
    build-essential \
    python3-colcon-common-extensions \
    python3-vcstool \
    git \
    wget \
    software-properties-common \
    sudo \
    iptables \
    iproute2 \
    socat \
    # k3s 실행에 필요한 도구
    && locale-gen en_US en_US.UTF-8

ENV LANG en_US.UTF-8
ENV LC_ALL en_US.UTF-8

# 2. ROS2 저장소 및 키 등록
RUN curl -sSL https://raw.githubusercontent.com/ros/rosdistro/master/ros.key | apt-key add - && \
    sh -c 'echo "deb [arch=amd64 signed-by=/usr/share/keyrings/ros-archive-keyring.gpg] http://packages.ros.org/ros2/ubuntu $(lsb_release -cs) main" > /etc/apt/sources.list.d/ros2.list'

# FAST-DDS Build libraries
# 시스템 시간 무시 설정 (apt update 오류 방지)
RUN echo "Acquire::Check-Valid-Until \"false\";\nAcquire::Check-Date \"false\";" > /etc/apt/apt.conf.d/10no--check-valid-until

# 패키지 설치
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    apt update && \
    apt install -y wget git cmake libasio-dev libtinyxml2-dev libssl-dev libp11-dev softhsm2 libengine-pkcs11-openssl python3 python3-pip openjdk-11-jre

# pip 패키지 설치는 별도 RUN에서 실행
RUN pip3 install --upgrade pip
RUN pip3 install colcon-common-extensions vcstool


RUN /bin/bash -c "mkdir -p ${LIBRARIES_INSTALL_DIR}/Fast-DDS && cd ${LIBRARIES_INSTALL_DIR}/Fast-DDS && \
    wget https://raw.githubusercontent.com/eProsima/Fast-DDS/3.2.2/fastdds.repos && mkdir src && \
    vcs import src < fastdds.repos && colcon build --packages-up-to fastdds && source ${LIBRARIES_INSTALL_DIR}/Fast-DDS/install/setup.bash && \
    cd ${LIBRARIES_INSTALL_DIR} && git clone https://github.com/eProsima/Fast-DDS-Gen.git && cd Fast-DDS-Gen && \
    chmod +x gradlew && ./gradlew clean && ./gradlew build ; ./gradlew install"

# 3. ROS2 설치
RUN apt-get update && apt-get install -y ros-humble-desktop

# 4. rosdep 초기화
RUN apt-get update && apt-get install -y python3-rosdep && \
    rosdep init && rosdep update

# 5. ROS2 워크스페이스 생성 및 샘플 패키지 복사
WORKDIR /ros2_ws
RUN mkdir -p src
# src 복사, rosdep, colcon build 모두 생략


# 6. ROS2 의존성 설치 및 빌드
#RUN . /opt/ros/humble/setup.sh && \
#    rosdep install --from-paths src --ignore-src -r -y && \
#    colcon build

# 7. k3s 설치 (공식 스크립트 사용)
RUN curl -sfL https://get.k3s.io -o install_k3s.sh && \
    chmod +x install_k3s.sh

# 8. 환경 변수 및 bashrc 설정
RUN echo "source /opt/ros/humble/setup.bash" >> /root/.bashrc
#    echo "source /ros2_ws/install/setup.bash" >> /root/.bashrc

# 9. 포트 오픈 (k3s API 서버 등)
EXPOSE 6443

# 10. 기본 실행 명령 (bash로 진입)
CMD ["/bin/bash"]

# 컨테이너 진입 후 k3s 서버 실행 예시
# bash에서: ./install_k3s.sh server --disable-agent --write-kubeconfig-mode 644

