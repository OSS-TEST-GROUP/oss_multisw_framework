# adsw_manager 컨테이너 통합 테스트용 샘플 Docker 이미지
# 실행: docker build -t adsw-test-app:latest -f Dockerfile .
FROM alpine:3.18

# bash 설치 (ROSServiceController가 bash -c "..." 형태로 실행)
RUN apk add --no-cache bash

# /opt/ros/humble/setup.bash mock (BaseRos2Command가 source함)
RUN mkdir -p /opt/ros/humble && \
    echo '#!/bin/bash' > /opt/ros/humble/setup.bash && \
    echo '# ROS2 humble mock setup' >> /opt/ros/humble/setup.bash

# ros2 mock 커맨드: ros2 run <pkg> <name> → /usr/local/bin/<name> 실행
RUN echo '#!/bin/bash' > /usr/local/bin/ros2 && \
    echo 'if [ "$1" = "run" ]; then' >> /usr/local/bin/ros2 && \
    echo '  pkg=$2; exe=$3; shift 3' >> /usr/local/bin/ros2 && \
    echo '  exec /usr/local/bin/$exe "$@"' >> /usr/local/bin/ros2 && \
    echo 'elif [ "$1" = "launch" ]; then' >> /usr/local/bin/ros2 && \
    echo '  pkg=$2; launchfile=$3; shift 3' >> /usr/local/bin/ros2 && \
    echo '  exec /usr/local/bin/$pkg "$@"' >> /usr/local/bin/ros2 && \
    echo 'elif [ "$1" = "node" ] && [ "$2" = "list" ]; then' >> /usr/local/bin/ros2 && \
    echo '  echo "/test_service"' >> /usr/local/bin/ros2 && \
    echo 'fi' >> /usr/local/bin/ros2 && \
    chmod +x /usr/local/bin/ros2

# test_service 실행파일 (package=test_pkg, name=test_service)
RUN echo '#!/bin/bash' > /usr/local/bin/test_service && \
    echo 'echo "[test_service] started, pid=$$"' >> /usr/local/bin/test_service && \
    echo 'trap "echo [test_service] shutting down; exit 0" TERM INT' >> /usr/local/bin/test_service && \
    echo 'while true; do sleep 1; done' >> /usr/local/bin/test_service && \
    chmod +x /usr/local/bin/test_service

# 컨테이너 기본 실행: 컨테이너 유지 (서비스는 adsw_manager가 docker exec으로 시작)
ENTRYPOINT ["sh", "-c", "echo '[adsw-test-docker] container started' && tail -f /dev/null"]
