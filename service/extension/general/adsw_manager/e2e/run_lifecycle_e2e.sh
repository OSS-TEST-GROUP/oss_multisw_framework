#!/bin/bash
# ============================================================================
# 경로 B end-to-end 스모크: 실 lifecycle_mgr + 실 adsw_manager_general + 실 docker
#
# 인젝터/ctrl 명령 없이, recovery: ALWAYS speclist 로 자동 기동을 유발한다.
#
#   adsw_manager_general 기동
#     → UpdateServiceList(container+service) 발행
#   lifecycle_mgr
#     → registry 등록 → ProcessServiceHandling
#     → PolicyAlways: NOT_RUNNING → START
#     → StartServiceRequest → adsw → docker run <sils-vecu 이미지>
#   [이 스크립트] docker ps 폴링으로 컨테이너 기동 확인 → PASS
#
# 전제:
#   - core 빌드(lifecycle_mgr)와 general 확장 빌드(adsw_manager_general)가 모두
#     되어 있어야 한다. (core: BUILD_EXTENSION=FALSE, general: EXTENSION_PLATFORM=GENERAL)
#   - docker 데몬이 이 호스트에서 접근 가능해야 한다.
#   - FastDDS 런타임(.so)이 로드 가능해야 한다(FASTDDS_SETUP 로 setup.bash 지정 가능).
#
# 환경변수(선택):
#   OUTPUT_ROOT        빌드 산출물 루트 (기본: <repo>/output/x86_64/Debug)
#   MMF_DDS_DOMAIN_ID  두 프로세스 공통 DDS 도메인 (기본: 0)
#   SILS_VECU_IMAGE    기동할 이미지 (기본: speclist 의 registry 이미지)
#   FASTDDS_SETUP      source 할 FastDDS setup.bash 경로 (있으면 source)
#   TIMEOUT_SEC        컨테이너 기동 대기 상한 (기본: 60)
#
# 사용:
#   service/extension/general/adsw_manager/e2e/run_lifecycle_e2e.sh
# ============================================================================
set -u

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
# repo root = script_dir/../../../../..  (e2e → adsw_manager → general → extension → service → repo)
REPO_ROOT="$( cd "${SCRIPT_DIR}/../../../../.." && pwd )"

OUTPUT_ROOT="${OUTPUT_ROOT:-${REPO_ROOT}/output/x86_64/Debug}"
export MMF_DDS_DOMAIN_ID="${MMF_DDS_DOMAIN_ID:-0}"
CONTAINER_NAME="sils-vecu-viss-probe"
TIMEOUT_SEC="${TIMEOUT_SEC:-60}"

LCM_BIN="${OUTPUT_ROOT}/core/bin/lifecycle_mgr"
ADSW_BIN="${OUTPUT_ROOT}/extension/general/bin/adsw_manager_general"
ADSW_CONFIG_DIR="${SCRIPT_DIR}/config"

LOG_DIR="$(mktemp -d 2>/dev/null || echo /tmp/sils-e2e-$$)"
mkdir -p "${LOG_DIR}"
LCM_LOG="${LOG_DIR}/lifecycle_mgr.log"
ADSW_LOG="${LOG_DIR}/adsw_manager_general.log"

LCM_PID=""
ADSW_PID=""

log()  { echo "[e2e] $*"; }
fail() { echo "[e2e][FAIL] $*" >&2; }

cleanup() {
    log "정리 중..."
    [ -n "${ADSW_PID}" ] && kill "${ADSW_PID}" 2>/dev/null
    [ -n "${LCM_PID}" ]  && kill "${LCM_PID}"  2>/dev/null
    wait 2>/dev/null
    docker rm -f "${CONTAINER_NAME}" >/dev/null 2>&1
    log "로그: ${LCM_LOG}"
    log "      ${ADSW_LOG}"
}
trap cleanup EXIT INT TERM

# ── 사전 점검 ────────────────────────────────────────────────────────────
[ -n "${FASTDDS_SETUP:-}" ] && [ -f "${FASTDDS_SETUP}" ] && source "${FASTDDS_SETUP}"

if ! docker info >/dev/null 2>&1; then
    fail "docker 데몬을 사용할 수 없습니다."
    exit 2
fi
if [ ! -x "${LCM_BIN}" ]; then
    fail "lifecycle_mgr 바이너리 없음: ${LCM_BIN} (core 빌드 필요: BUILD_EXTENSION=FALSE)"
    exit 2
fi
if [ ! -x "${ADSW_BIN}" ]; then
    fail "adsw_manager_general 바이너리 없음: ${ADSW_BIN} (EXTENSION_PLATFORM=GENERAL 빌드 필요)"
    exit 2
fi
if [ ! -f "${ADSW_CONFIG_DIR}/saved-service-list.yaml" ]; then
    fail "e2e speclist 없음: ${ADSW_CONFIG_DIR}/saved-service-list.yaml"
    exit 2
fi

# 이미지 override: speclist 이미지를 강제로 태깅 대체하고 싶을 때
if [ -n "${SILS_VECU_IMAGE:-}" ]; then
    log "이미지 확보 시도: ${SILS_VECU_IMAGE}"
    if ! docker image inspect "${SILS_VECU_IMAGE}" >/dev/null 2>&1; then
        docker pull "${SILS_VECU_IMAGE}" >/dev/null 2>&1 || \
            log "pull 실패(계속 진행 — 로컬에 있을 수 있음): ${SILS_VECU_IMAGE}"
    fi
fi

log "설정: DOMAIN=${MMF_DDS_DOMAIN_ID}  OUTPUT_ROOT=${OUTPUT_ROOT}"
docker rm -f "${CONTAINER_NAME}" >/dev/null 2>&1   # 이전 잔재 정리

# ── lifecycle_mgr 기동 ───────────────────────────────────────────────────
log "lifecycle_mgr 기동..."
( cd "$(dirname "${LCM_BIN}")" && exec "${LCM_BIN}" ) >"${LCM_LOG}" 2>&1 &
LCM_PID=$!
sleep 2
if ! kill -0 "${LCM_PID}" 2>/dev/null; then
    fail "lifecycle_mgr 즉시 종료됨. 로그:"; cat "${LCM_LOG}" >&2; exit 1
fi

# ── adsw_manager_general 기동 (e2e speclist 주입) ─────────────────────────
log "adsw_manager_general 기동 (SAVED_REGISTRY_BACKUP=${ADSW_CONFIG_DIR})..."
( cd "$(dirname "${ADSW_BIN}")" && SAVED_REGISTRY_BACKUP="${ADSW_CONFIG_DIR}" exec "${ADSW_BIN}" ) >"${ADSW_LOG}" 2>&1 &
ADSW_PID=$!
sleep 2
if ! kill -0 "${ADSW_PID}" 2>/dev/null; then
    fail "adsw_manager_general 즉시 종료됨. 로그:"; cat "${ADSW_LOG}" >&2; exit 1
fi

# ── 컨테이너 자동 기동 대기 (핸드셰이크 → 정책 → docker run) ──────────────
log "컨테이너 '${CONTAINER_NAME}' 자동 기동 대기 (최대 ${TIMEOUT_SEC}s)..."
deadline=$(( SECONDS + TIMEOUT_SEC ))
started=0
while [ ${SECONDS} -lt ${deadline} ]; do
    if docker ps --filter "name=^${CONTAINER_NAME}$" --filter "status=running" \
                 --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
        started=1
        break
    fi
    # 두 프로세스가 살아있는지 확인 (죽었으면 조기 실패)
    if ! kill -0 "${LCM_PID}" 2>/dev/null; then fail "lifecycle_mgr 종료됨"; cat "${LCM_LOG}" >&2; exit 1; fi
    if ! kill -0 "${ADSW_PID}" 2>/dev/null; then fail "adsw_manager_general 종료됨"; cat "${ADSW_LOG}" >&2; exit 1; fi
    sleep 2
done

if [ ${started} -eq 1 ]; then
    log "PASS ✅ — lifecycle_mgr 연동으로 컨테이너가 자동 기동됨:"
    docker ps --filter "name=^${CONTAINER_NAME}$" \
              --format 'table {{.Names}}\t{{.Status}}\t{{.Image}}'
    exit 0
else
    fail "시간초과 — 컨테이너가 기동되지 않음."
    echo "---- adsw_manager_general 로그(마지막 40줄) ----" >&2; tail -40 "${ADSW_LOG}" >&2
    echo "---- lifecycle_mgr 로그(마지막 40줄) ----" >&2;      tail -40 "${LCM_LOG}" >&2
    exit 1
fi
