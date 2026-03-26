#!/bin/bash
# ============================================================================
# adsw_manager Docker 컨테이너 통합 테스트 실행 스크립트
#
# 사용법:
#   chmod +x run_docker_integration_test.sh
#   ./run_docker_integration_test.sh [테스트바이너리경로]
#
# 기본 경로: 빌드 설치 디렉토리 기준
# ============================================================================

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
UNITTEST_DIR="$(dirname "$SCRIPT_DIR")"
TEST_BIN="${1:-$UNITTEST_DIR/ADSWContainerDockerIntegrationTest}"
DOCKER_DIR="$UNITTEST_DIR/docker"
IMAGE_NAME="adsw-test-app:latest"
CONTAINER_NAME_DOCKER="adsw-test-docker"
CONTAINER_NAME_COMPOSE="adsw-test-compose"
CONTAINER_NAME_HM="adsw-test-hm"

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

log_info()  { echo -e "${GREEN}[INFO]${NC} $*"; }
log_warn()  { echo -e "${YELLOW}[WARN]${NC} $*"; }
log_error() { echo -e "${RED}[ERROR]${NC} $*"; }

# ─── 1. 사전 확인 ─────────────────────────────────────────────────────────────
log_info "=== adsw_manager Docker 통합 테스트 ==="

if ! command -v docker &> /dev/null; then
    log_error "Docker가 설치되어 있지 않습니다."
    exit 1
fi

if ! docker info &> /dev/null; then
    log_error "Docker 데몬이 실행되고 있지 않습니다."
    exit 1
fi

if [ ! -f "$TEST_BIN" ]; then
    log_error "테스트 바이너리를 찾을 수 없습니다: $TEST_BIN"
    log_error "먼저 빌드하세요: cmake --build . && cmake --install ."
    exit 1
fi

# ─── 2. Docker 이미지 빌드 ────────────────────────────────────────────────────
log_info "테스트 이미지 빌드 중: $IMAGE_NAME"
docker build -t "$IMAGE_NAME" "$DOCKER_DIR" \
    || { log_error "이미지 빌드 실패"; exit 1; }
log_info "이미지 빌드 완료: $IMAGE_NAME"

# ─── 3. 이전 잔재 정리 ────────────────────────────────────────────────────────
log_info "이전 테스트 컨테이너 정리 중..."
docker rm -f "$CONTAINER_NAME_DOCKER" "$CONTAINER_NAME_COMPOSE" \
             "$CONTAINER_NAME_HM" &> /dev/null || true
if [ -f "$DOCKER_DIR/docker-compose.test.yaml" ]; then
    docker compose -f "$DOCKER_DIR/docker-compose.test.yaml" down &> /dev/null || true
fi

# ─── 4. 통합 테스트 실행 ──────────────────────────────────────────────────────
log_info "통합 테스트 실행: $TEST_BIN"
echo "────────────────────────────────────────────────────────────"

# 테스트 바이너리를 unittest 디렉토리에서 실행 (상대 경로 기준)
pushd "$UNITTEST_DIR" > /dev/null
"$TEST_BIN" --gtest_color=yes "$@"
TEST_RESULT=$?
popd > /dev/null

echo "────────────────────────────────────────────────────────────"

# ─── 5. 사후 정리 ─────────────────────────────────────────────────────────────
log_info "사후 정리 중..."
docker rm -f "$CONTAINER_NAME_DOCKER" "$CONTAINER_NAME_COMPOSE" \
             "$CONTAINER_NAME_HM" &> /dev/null || true
if [ -f "$DOCKER_DIR/docker-compose.test.yaml" ]; then
    docker compose -f "$DOCKER_DIR/docker-compose.test.yaml" down &> /dev/null || true
fi

# ─── 6. 결과 출력 ─────────────────────────────────────────────────────────────
if [ $TEST_RESULT -eq 0 ]; then
    log_info "모든 통합 테스트 PASS ✓"
else
    log_error "일부 테스트 FAIL (종료 코드: $TEST_RESULT)"
fi

exit $TEST_RESULT
