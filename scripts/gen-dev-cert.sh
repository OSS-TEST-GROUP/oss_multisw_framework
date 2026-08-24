#!/bin/bash
# =============================================================================
# gen-dev-cert.sh — vehicle_info_mgr 개발용 self-signed 인증서 생성
#
# 왜 git 에 두지 않는가:
#   사설키(key.pem)를 저장소에 커밋하면 공개 저장소로 전달되는 순간 노출된다.
#   이 인증서는 localhost 개발·테스트 전용이므로 각 환경에서 생성하는 것이 맞다.
#
# 사용법:
#   ./scripts/gen-dev-cert.sh            # 기본 위치에 생성
#   CN=myhost DAYS=365 ./scripts/gen-dev-cert.sh
#
# 운영 환경에서는 이 스크립트를 쓰지 말고 정식 CA 발급 인증서를 배치한다.
# =============================================================================
set -e

OUT_DIR="${OUT_DIR:-service/core/vehicle_info_mgr/config/security}"
CN="${CN:-localhost}"
DAYS="${DAYS:-365}"
SUBJ="${SUBJ:-/C=KR/ST=Seoul/L=Seoul/O=Obigo/CN=${CN}}"

command -v openssl >/dev/null || { echo "ERROR: openssl 이 필요합니다." >&2; exit 1; }

mkdir -p "${OUT_DIR}"

if [ -f "${OUT_DIR}/key.pem" ] && [ "${FORCE}" != "1" ]; then
  echo "이미 존재합니다: ${OUT_DIR}/key.pem  (덮어쓰려면 FORCE=1)"
  exit 0
fi

openssl req -x509 -newkey rsa:2048 -nodes \
  -keyout "${OUT_DIR}/key.pem" \
  -out    "${OUT_DIR}/cert.pem" \
  -days   "${DAYS}" \
  -subj   "${SUBJ}" \
  -addext "subjectAltName=DNS:${CN},DNS:localhost,IP:127.0.0.1" 2>/dev/null

chmod 600 "${OUT_DIR}/key.pem"
chmod 644 "${OUT_DIR}/cert.pem"

echo "생성 완료:"
echo "  ${OUT_DIR}/key.pem   (사설키, 권한 600 — git 에 커밋하지 않는다)"
echo "  ${OUT_DIR}/cert.pem  (인증서, ${DAYS}일 유효, CN=${CN})"
