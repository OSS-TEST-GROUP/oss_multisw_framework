#!/bin/bash
#
# Integration test lane — counterpart to run_unittest.sh.
#
# Runs the CI-runnable, self-contained integration subset listed in
# integration-tests-runnable.regex — ctest cases that stand up their own
# in-process DDS pub/sub (or endpoint) and pass reliably in ISOLATION, needing
# NO external gRPC server / ROS graph / live DDS peer / docker daemon.
#
# The broader integration-tests.regex is the UNIT-lane exclusion superset
# (run_unittest.sh excludes exactly that set); the remaining runtime-dependent
# tests there are deferred to a later phase that provisions those services.
#
# Isolation: every run joins a UNIQUE, non-zero DDS domain (via a generated
# Fast DDS profile + ROS_DOMAIN_ID) so the ctest participants never cross-talk
# with a live MMF stack (e.g. the docker-compose stack the smoke stage brings
# up, which sits on domain 0). Override the domain with MMF_INTEGRATION_DOMAIN.
#
WORK_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )/../.." && pwd )"
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

OUTPUT_DIR=${WORK_DIR}/output/x86_64/Debug
RESULT_DIR=${WORK_DIR}/output/integration-results

# Runtime libs for the ctest binaries: core + ros2 extension + Fast DDS + ROS.
export LD_LIBRARY_PATH=${OUTPUT_DIR}/core/lib:${OUTPUT_DIR}/extension/ros2/lib:${WORK_DIR}/output/x86_64/lib${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}
export MMF_CONFIG_PATH=${OUTPUT_DIR}/core/bin/config
export DIAG_CONFIG_PATH=${OUTPUT_DIR}/core/bin/unittest/diagnostics/config
export DIAG_IDL_PATH=${OUTPUT_DIR}/core/bin/unittest/diagnostics/idl
export VEHICLE_CONFIG_PATH=${OUTPUT_DIR}/core/bin/config
export SAVED_REGISTRY_BACKUP=${OUTPUT_DIR}/extension/ros2/bin/unittest/adsw_manager/config

# ── DDS domain isolation ────────────────────────────────────────────────────
# Unique domain per run (never 0). Inject <domainId> into a copy of the
# canonical Fast DDS profile so create_participant_with_default_profile() joins
# it; ROS_DOMAIN_ID isolates any ROS2/rmw participants the tests may create.
DOMAIN_ID=${MMF_INTEGRATION_DOMAIN:-$(( ($$ % 200) + 11 ))}
BASE_PROFILE="${WORK_DIR}/docker/build_environment/fastdds-profile.xml"
ISO_PROFILE="/tmp/mmf_integration_fastdds_${DOMAIN_ID}.xml"
if [[ -f "${BASE_PROFILE}" ]]; then
	# Insert <domainId> as the first child of the default <participant> block.
	sed -E "0,/(is_default_profile=\"true\">)/s//\1\n            <domainId>${DOMAIN_ID}<\/domainId>/" \
		"${BASE_PROFILE}" > "${ISO_PROFILE}"
	export FASTDDS_DEFAULT_PROFILES_FILE="${ISO_PROFILE}"
fi
export ROS_DOMAIN_ID=${DOMAIN_ID}
echo "[run_integration] DDS domain ${DOMAIN_ID} (profile: ${FASTDDS_DEFAULT_PROFILES_FILE:-<none>})"

# ROS runtime for the ROS-forwarding / discovery paths, when present.
[[ -f /opt/ros/humble/setup.bash ]] && source /opt/ros/humble/setup.bash

# Runnable subset — strip comments/blanks. Look next to the script first, then
# fall back to the repo path (CI may mount the script standalone).
RUNNABLE_RE=$(grep -vE '^\s*#|^\s*$' "${SCRIPT_DIR}/integration-tests-runnable.regex" 2>/dev/null \
              || grep -vE '^\s*#|^\s*$' "${WORK_DIR}/docker/scripts/integration-tests-runnable.regex" 2>/dev/null)
if [[ -z "${RUNNABLE_RE}" ]]; then
	echo "[run_integration] integration-tests-runnable.regex empty/missing — nothing to run" >&2
	exit 0
fi

mkdir -p ${RESULT_DIR}

for testdir in $(find .build/ -not -path "*/_deps/*" -name CTestTestfile.cmake -exec dirname {} \;)
do
	# 레이블은 결과 파일 경로가 된다. basename 만 쓰면
	# service/extension/{ros2,general,agl}/adsw_manager 세 레인이 모두
	# "adsw_manager" 가 되어 같은 junit_results.xml 에 덮어써진다 (ota_client 도
	# ros2/agl 두 벌이라 같은 문제). 실측: ros2/adsw 가 14건을 통과했는데도
	# 뒤이어 돌은 agl/adsw 가 "테스트 없음" 으로 같은 파일을 덮어써서 그 14건이
	# 산출물에서 사라졌다 — CI 가 이걸 읽으면 조용히 절반을 못 본다.
	# extension 레인은 플랫폼 세그먼트를 앞에 붙여 구분한다. core 레인의
	# 레이블은 기존과 동일하게 유지된다. (run_unittest.sh 와 같은 규칙)
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
	# --tests-regex selects only the runnable subset; a dir with no match is a no-op.
	ctest \
		--test-dir ${testdir} \
		--label-add ${label} \
		--tests-regex "${RUNNABLE_RE}" \
		--timeout ${CTEST_TIMEOUT:-60} \
		--output-junit ${RESULT_DIR}/${label}/junit_results.xml \
		--output-on-failure \
		--no-compress-output || true
done
