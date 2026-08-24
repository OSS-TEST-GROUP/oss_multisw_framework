#!/usr/bin/env bash
#
# run_partitions_detached.sh -- Autoware 파티션 3종을 **트랩 없이** 띄운다.
#
# ★ 왜 이게 따로 있나 — 2026-08-12 에 하루를 날린 사고
#
# 상류 러너 `oss_adsw/partition/run_partitions.sh` 는 이렇게 끝난다:
#
#     trap cleanup INT TERM EXIT     # cleanup() { docker stop <파티션 3종>; }
#     tail -n +1 -F <로그 3개>        # 영원히 안 끝난다
#
# 대화형으로 Ctrl-C 를 눌러 내리려는 설계다. 그런데 파티션은 `--rm` 이라 stop 되는
# 순간 **`docker ps -a` 에서도 사라진다.** 그래서 증상이 이렇게 보인다:
#
#     파티션 3종이 동시에, 흔적도 없이 없어졌다
#
# 여기에 두 가지가 겹치면 진단이 완전히 빗나간다:
#
#   1) `kill <pid>` 로는 트랩이 안 돈다. bash 는 포그라운드 명령(`tail -F`)이 끝나야
#      트랩을 처리하는데 그게 영원히 안 끝나므로 시그널이 pending 으로 남는다.
#      **프로세스 그룹째** 죽어야(터미널 종료·에이전트의 프로세스 그룹 정리 등)
#      `tail` 이 죽고 그제서야 cleanup 이 돈다. → 산발적으로 보인다.
#
#   2) 죽은 파티션 로그의 **마지막 줄**은 십중팔구 Cyclone 의
#      `ddsi_xt_type_init_impl with invalid type object` 다. 그 줄이 decision 로그의
#      **29%** 를 차지하기 때문이지, 그게 원인이라서가 아니다. 실측:
#      docs/sils/evidence/mmf-typeobject-2026-08-12.md
#
# 그리고 러너를 여러 번 띄우면 **전부 살아남아** 각자 트랩을 들고 있게 된다
# (실제로 8개가 동시에 떠 있었다). 그중 아무 하나의 그룹만 정리돼도 파티션이 날아간다.
#
# 이 스크립트는 그 구조를 없앤다 — `docker run -d` 만 하고 **트랩도 tail 도 없다.**
# 내릴 때는 명시적으로 --stop 한다.
#
# ⚠ 상류 러너를 쓰지 말라는 뜻은 아니다. 사람이 터미널에 앉아 Ctrl-C 로 내릴 거면
#   그쪽이 맞다. **에이전트/스크립트가 띄울 때**는 이쪽을 쓴다.
#
# ★ 중복 방지
#
# 기동 전에 상류 러너가 살아 있으면 거부한다. 그게 남아 있으면 이 스크립트가 띄운
# 컨테이너도 그쪽 트랩에 쓸려 나간다.
#
# Usage:
#   tools/sils/ss2/run_partitions_detached.sh                       # 도메인 42, kashiwanoha
#   tools/sils/ss2/run_partitions_detached.sh --domain-id 42 --map-path ~/autoware_map/kashiwanoha
#   tools/sils/ss2/run_partitions_detached.sh --stop
#
# Env: ADSW_REPO · ADSW_WORKSPACE · HOST_SOURCE_DIR
# 이어서: tools/sils/ss2/run_control_partition_sils.sh (control 을 sps OFF 로 교체)

set -uo pipefail

REPO="${ADSW_REPO:-ghcr.io/oss-test-group/autoware-partition}"
DOMAIN="${ROS_DOMAIN_ID:-42}"
MAP_PATH="${ADSW_MAP_PATH:-$HOME/autoware_map/kashiwanoha}"
WORKSPACE_ROOT="${ADSW_WORKSPACE:-$HOME/oss/oss_adsw}"
HOST_SOURCE_DIR="${HOST_SOURCE_DIR:-$HOME/source}"
DELAY=10
STOP=0

while [ "${1:-}" != "" ]; do
    case "$1" in
        --domain-id) DOMAIN="$2"; shift ;;
        --map-path)  MAP_PATH="$2"; shift ;;
        --delay)     DELAY="$2"; shift ;;
        --stop)      STOP=1 ;;
        --help|-h)   sed -n '2,48p' "${BASH_SOURCE[0]}"; exit 0 ;;
        *) echo "run_partitions_detached: unknown option: $1" >&2; exit 2 ;;
    esac
    shift
done

NAMES=("adsw-perception-${DOMAIN}" "adsw-decision-${DOMAIN}" "adsw-control-${DOMAIN}")

if [ "$STOP" = "1" ]; then
    docker rm -f "${NAMES[@]}" >/dev/null 2>&1
    echo "run_partitions_detached: 내렸다 (${NAMES[*]})"
    exit 0
fi

# ★ 상류 러너가 살아 있으면 거부한다 — 그쪽 트랩이 우리 컨테이너를 쓸어간다.
# [r] 브래킷은 pgrep 이 자기 자신을 매치하지 않게 한다(이것도 밟았다).
if pgrep -f '[r]un_partitions.sh' >/dev/null 2>&1; then
    echo "run_partitions_detached: ⛔ 상류 run_partitions.sh 가 $(pgrep -cf '[r]un_partitions.sh')개 살아 있다." >&2
    echo "  그 트랩(docker stop)이 여기서 띄운 컨테이너도 같이 내린다. 먼저 정리하라:" >&2
    echo "    for p in \$(pgrep -f '[r]un_partitions.sh'); do kill -TERM -\$(ps -o pgid= -p \$p | tr -d ' '); done" >&2
    exit 2
fi

[ -d "$MAP_PATH" ]       || { echo "run_partitions_detached: 맵 없음: $MAP_PATH" >&2; exit 2; }
[ -d "$WORKSPACE_ROOT" ] || { echo "run_partitions_detached: 워크스페이스 없음: $WORKSPACE_ROOT" >&2; exit 2; }
mkdir -p "${WORKSPACE_ROOT}/log"

echo "==> 파티션 3종 (도메인 ${DOMAIN}, 맵 $(basename "$MAP_PATH"), 트랩 없음)"

start_one() {  # <이름> <태그> <명령> <로그파일>
    docker rm -f "$1" >/dev/null 2>&1
    docker run -d --name "$1" \
        --rm --net=host --pid=host --ipc=host --privileged \
        -e "LOCAL_UID=$(id -u)" -e "LOCAL_GID=$(id -g)" \
        -e "LOCAL_USER=$(id -un)" -e "LOCAL_GROUP=$(id -gn)" \
        -e "FASTDDS_BUILTIN_TRANSPORTS=UDPv4" \
        -e "ROS_DOMAIN_ID=${DOMAIN}" \
        -e "RMW_IMPLEMENTATION=rmw_cyclonedds_cpp" \
        -e "ROS_LOCALHOST_ONLY=0" \
        -v /etc/localtime:/etc/localtime:ro \
        -v "${WORKSPACE_ROOT}:/workspace" \
        -v "${MAP_PATH}:/autoware_map:ro" \
        -v "${HOST_SOURCE_DIR}/fastdds:/fastdds:rw" \
        -v "${WORKSPACE_ROOT}:/exec" \
        -v "${HOST_SOURCE_DIR}/ros2:/ros2" \
        "${REPO}:$2" \
        bash -lc "mkdir -p /workspace/log && $3 > /workspace/log/$4 2>&1" >/dev/null \
      && echo "    $1"
}

# headless: perception 은 rviz 를 끈 launch 를 직접 부른다 (상류의 --headless 와 동일).
start_one "${NAMES[0]}" adsw-perception \
  "ros2 launch obigo_launch sample_adsw_perception_run.launch.xml map_path:=/autoware_map vehicle_model:=sample_vehicle sensor_model:=sample_sensor_kit rviz:=false" \
  perception_log.txt
sleep "$DELAY"
start_one "${NAMES[1]}" adsw-decision "/autoware/start_script/adsw-decision.sh" decision_log.txt
sleep "$DELAY"
start_one "${NAMES[2]}" adsw-control  "/autoware/start_script/adsw-control.sh"  control_log.txt

echo
echo "로그:  tail -f ${WORKSPACE_ROOT}/log/{perception,decision,control}_log.txt"
echo "내리기: tools/sils/ss2/run_partitions_detached.sh --stop --domain-id ${DOMAIN}"
echo
echo "다음: control 을 sps OFF 로 교체"
echo "  tools/sils/ss2/run_control_partition_sils.sh --domain-id ${DOMAIN} --map-path ${MAP_PATH}"
