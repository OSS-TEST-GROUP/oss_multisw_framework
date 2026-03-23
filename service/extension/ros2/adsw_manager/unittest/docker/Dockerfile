# adsw_manager 컨테이너 통합 테스트용 샘플 Docker 이미지
# 실행: docker build -t adsw-test-app:latest -f Dockerfile .
FROM alpine:3.18

# 테스트용 간단한 서비스 스크립트 추가
RUN echo '#!/bin/sh' > /usr/local/bin/adsw_sample_service.sh && \
    echo 'echo "[adsw-sample] started, pid=$$"' >> /usr/local/bin/adsw_sample_service.sh && \
    echo 'trap "echo [adsw-sample] shutting down; exit 0" TERM INT' >> /usr/local/bin/adsw_sample_service.sh && \
    echo 'while true; do sleep 1; done' >> /usr/local/bin/adsw_sample_service.sh && \
    chmod +x /usr/local/bin/adsw_sample_service.sh

ENTRYPOINT ["/usr/local/bin/adsw_sample_service.sh"]
