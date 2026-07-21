#!/bin/bash
# ============================================================================
# 경로 B e2e — brake-ecu 예제 변형.
#
# run_lifecycle_e2e.sh 를 그대로 재사용하되, speclist(config-brake) 와 컨테이너명
# (brake-ecu) 만 바꿔 실행한다. 기본 probe e2e 와 완전히 같은 흐름:
# 등록 → PolicyAlways 자동기동 → docker run(oss-mmf-brake-ecu:local) → PASS.
#
# 선행:
#   ./build.sh --build-sils                  # brake_ecu → core/bin (SILS_BUILD_EXAMPLES 기본 ON)
#   ./docker/build_local.sh --brake-ecu-only  # → oss-mmf-brake-ecu:local
# ============================================================================
set -u
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

export E2E_CONFIG_DIR="${SCRIPT_DIR}/config-brake"
export E2E_CONTAINER_NAME="brake-ecu"

exec "${SCRIPT_DIR}/run_lifecycle_e2e.sh" "$@"
