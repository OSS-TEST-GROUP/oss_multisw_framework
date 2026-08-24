#!/bin/bash
#
# 저장소 루트. 아래 경로들은 전부 루트 기준이다 (${WORK_DIR}/output/...,
# ${WORK_DIR}/docker/scripts/...).
#
# 이 스크립트는 두 가지 배치로 실행된다. 어느 쪽도 고정 상대경로로는 못 맞춘다.
#   (1) CI  : Jenkinsfile.unittest 가 워크스페이스 **루트에** 단독 마운트하고
#             `./run_unittest.sh` 로 부른다  → 스크립트 디렉터리 = 루트
#   (2) 로컬: `bash docker/scripts/run_unittest.sh`  → 루트는 두 단계 위
# 그래서 디렉터리를 계산하지 않고, 루트에만 있는 표식을 찾아 확정한다.
_src_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
WORK_DIR=
for _cand in "${_src_dir}" "${_src_dir}/../.." "${PWD}"; do
    if [ -f "${_cand}/docker/scripts/integration-tests.regex" ]; then
        WORK_DIR="$( cd "${_cand}" && pwd )"
        break
    fi
done
if [ -z "${WORK_DIR}" ]; then
    echo "run_unittest.sh: 저장소 루트를 찾지 못했다 (docker/scripts/integration-tests.regex 없음)" >&2
    exit 1
fi

OUTPUT_DIR=${WORK_DIR}/output/x86_64/Debug
RESULT_DIR=${WORK_DIR}/output/unittest-results

# 덮어쓰지 말고 앞에 붙인다. 빌더 컨테이너는 setup.bash 로 FastDDS 경로를
# LD_LIBRARY_PATH 에 넣어 두는데, 대입해 버리면 그게 날아간다. WORK_DIR 이
# 어긋났을 때 실제로 이렇게 터졌다 — 없는 경로가 대입되면서 libfastcdr.so.2
# 를 못 찾아 general 레인 180건 중 127건이 로드 실패했고, 실행이 안 됐으니
# .gcda 도 안 남아 커버리지까지 붕괴했다.
export LD_LIBRARY_PATH=${WORK_DIR}/output/x86_64/lib${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}
export MMF_CONFIG_PATH=${OUTPUT_DIR}/core/bin/config
export DIAG_CONFIG_PATH=${OUTPUT_DIR}/core/bin/unittest/diagnostics/config
export DIAG_IDL_PATH=${OUTPUT_DIR}/core/bin/unittest/diagnostics/idl
export VEHICLE_CONFIG_PATH=${OUTPUT_DIR}/core/bin/config
export SAVED_REGISTRY_BACKUP=${OUTPUT_DIR}/extension/ros2/bin/unittest/adsw_manager/config

mkdir -p ${RESULT_DIR}

# Integration / runtime-dependent tests live in a separate lane (run by
# run_integration.sh). Exclude them here so the unit lane stays deterministic.
# Single source of truth: integration-tests.regex (strip comments/blanks).
INTEGRATION_RE=$(grep -vE '^\s*#|^\s*$' "${WORK_DIR}/docker/scripts/integration-tests.regex" 2>/dev/null \
                 || grep -vE '^\s*#|^\s*$' "$( dirname "${BASH_SOURCE[0]}" )/integration-tests.regex")

for testdir in $(find .build/ -not -path "*/_deps/*" -name CTestTestfile.cmake -exec dirname {} \;)
do
	# 레이블은 결과 파일 경로가 된다. basename 만 쓰면
	# service/extension/{ros2,general,agl}/adsw_manager 세 레인이 모두
	# "adsw_manager" 가 되어 같은 junit_results.xml 에 덮어써지고, 마지막
	# 레인 것만 남는다 (ota_client 도 ros2/agl 두 벌이라 같은 문제).
	# extension 레인은 플랫폼 세그먼트를 앞에 붙여 구분한다. core 레인의
	# 레이블은 기존과 동일하게 유지된다.
	label=
	moddir=${testdir}
	[[ ${moddir} == */unittest ]] && moddir=$(dirname ${moddir})
	case ${moddir} in
		*/service/extension/*)
			rel=${moddir#*/service/extension/}   # <plat>/<mod>
			label=${rel//\//_}
			;;
		*)
			label=$(basename ${moddir})
			;;
	esac
	mkdir -p ${RESULT_DIR}/${label}
	ctest \
		--test-dir ${testdir} \
		--label-add ${label} \
		${INTEGRATION_RE:+--exclude-regex "${INTEGRATION_RE}"} \
		--timeout ${CTEST_TIMEOUT:-60} \
		--output-junit ${RESULT_DIR}/${label}/junit_results.xml \
		--output-on-failure \
		--no-compress-output || true
done

