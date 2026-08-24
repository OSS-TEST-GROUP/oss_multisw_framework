#!/usr/bin/env bash
#
# run_c1_with_mmf.sh -- C1 구성 + MMF 를 한 도메인에서 재현한다.
#
#   SS_v2(isEgo 없음) ─▶ ①ss2_ego_bridge ─▶ sils/ego/state ─┬─▶ ③ ─▶ /vehicle/status/*  ┐
#                                                            └─▶ ④ ─▶ /localization/*    │
#                                                                                        ▼
#                        vECU 5기 ◀── autoware/control_target ◀── ② ◀── control_cmd ◀ Autoware
#                                                                                        │
#                                        MMF(viss-server) ◀── ADAPI 20 토픽 ◀────────────┘
#                                             └─▶ VSS 트리 (Vehicle.ADS.*)
#
# ★ 무엇을 증명하나
#   - 좌석 소유: /vehicle/status/* · /localization/* 의 발행자가 우리 브리지 하나
#   - 초기화 자동화: /initialpose3d 를 브리지가 낸다. 수동 단계 0
#   - vECU 가 살아 있는 Autoware 판단을 받아 아비트레이션까지
#   - ★ MMF 가 같은 도메인에서 공존하며 ADAPI 를 읽어 VSS 로 서빙
#
# ★ 무엇을 증명하지 않나
#   - 폐루프. isEgo 가 없으면 명령을 소비할 차량 모델이 아예 안 만들어진다
#     (simple_sensor_simulator.cpp:201). 차는 시나리오 각본이 움직인다 — vx 상수가 그 신호다.
#     docs/sils/sils-configuration-map.md C1 주의
#
# ★ 선행 (이 스크립트가 하지 않는 것)
#   1) Autoware 파티션 3종이 떠 있어야 한다. ★ 트랩 없는 쪽을 쓸 것 —
#      상류 run_partitions.sh 는 EXIT 트랩으로 파티션을 stop 하고, 여러 개 띄우면
#      그중 하나만 정리돼도 전멸한다(2026-08-12 사고).
#        tools/sils/ss2/run_partitions_detached.sh --domain-id 42 \
#           --map-path $HOME/autoware_map/kashiwanoha
#   2) control 파티션은 sps 를 끈 우리 것이어야 한다
#        tools/sils/ss2/run_control_partition_sils.sh --domain-id 42 \
#           --map-path $HOME/autoware_map/kashiwanoha
#   3) MMF 는 **패치된 이미지**를 권한다 (oss-mmf-core:local).
#      패치 없는 이미지는 파티션 로그에 invalid type object 1260줄을 쏟는다(죽지는 않는다) —
#      docs/sils/evidence/mmf-typeobject-2026-08-12.md
#
# ⚠ 시나리오는 ~90초짜리다. 자기 스토리를 마치면 global_timeout 과 무관하게 끝난다.
#   그래서 이 스크립트는 pose 가 나오자마자 목표를 발행한다. 늦으면 자차가 이미 종점에
#   서 있고, 목표가 뒤에 있어 경로가 빈다.
#
# Usage:
#   tools/sils/ss2/run_c1_with_mmf.sh
#   tools/sils/ss2/run_c1_with_mmf.sh --samples 40
#   tools/sils/ss2/run_c1_with_mmf.sh --no-mmf       # MMF 빼고 (대조군)

set -uo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${here}/../../.." && pwd)"
cd "${REPO_ROOT}"

DOMAIN="${SILS_DOMAIN:-42}"
CONTROL="adsw-control-${DOMAIN}"
ADAPTER_IMG="${SILS_SS2_ADAPTER_IMAGE:-sils-ss2-adapter:humble}"
VECU_IMG="${SILS_VECU_IMAGE:-oss-mmf-sils-vecu:latest}"
SAMPLES=25
WITH_MMF=1
# evidence/autoware-return-path-2026-08-11.md §11 에서 뽑아둔 kashiwanoha 목표.
# 자차 앞 20 m. 그 이상은 `Goal's footprint exceeds lane!` 로 거부된다 — 좁은 캠퍼스
# 차선 + sample_vehicle 차폭 + check_footprint_inside_lanes(생성 시 캐시라 param set 무효).
GOAL_X="${SILS_GOAL_X:-3731.5301}"; GOAL_Y="${SILS_GOAL_Y:-73776.8370}"; GOAL_Z="${SILS_GOAL_Z:--2.89}"

while [ "${1:-}" != "" ]; do
    case "$1" in
        --samples) SAMPLES="$2"; shift ;;
        --no-mmf)  WITH_MMF=0 ;;
        --help|-h) sed -n '2,45p' "${BASH_SOURCE[0]}"; exit 0 ;;
        *) echo "run_c1_with_mmf: unknown option: $1" >&2; exit 2 ;;
    esac
    shift
done

docker ps --format '{{.Names}}' | grep -qx "$CONTROL" || {
    echo "run_c1_with_mmf: ${CONTROL} 이 없다. 위 '선행' 을 먼저 하라." >&2; exit 2; }

echo "==> 0. 전제 검사"
# ⚠⚠ 모든 ros2 조회에 --no-daemon 을 붙인다.
#
# ros2cli 데몬은 이 rig 에서 반복적으로 죽는데, 죽어도 프로세스는 남아 있고 조회만
# 실패한다:  xmlrpc.client.Fault: <Fault 1: "<class 'RuntimeError'>:!rclpy.ok()">
# stderr 로 나가므로 2>/dev/null 을 걸면 **빈 출력**이 되고, 그건 "sps 가 없다 /
# 발행자가 0 이다 / 아무도 발행 안 한다" 와 구분이 안 된다. 실제로 그렇게 한 번
# 오진했다 — 브리지 카운터는 멀쩡히 돌고 있는데 rig 가 죽은 줄 알았다.
# --no-daemon 은 느리지만 매번 새 참가자로 직접 조회하므로 이 함정이 없다.
nodes=$(docker exec "$CONTROL" bash -lc 'source /opt/ros/humble/setup.bash && timeout 25 ros2 node list --no-daemon 2>/dev/null' | wc -l)
[ "$nodes" -ge 10 ] || { echo "    ⛔ ros2 조회 불가 (노드 ${nodes}개). 이 상태의 측정은 전부 무의미하다." >&2; exit 2; }
echo "    ros2 CLI OK (노드 ${nodes}개, --no-daemon)"

n=$(docker exec "$CONTROL" bash -lc 'source /opt/ros/humble/setup.bash && timeout 25 ros2 node list --no-daemon 2>/dev/null' \
      | grep -c simple_planning_simulator)
[ "$n" -eq 0 ] || { echo "    ⛔ simple_planning_simulator 가 살아 있다. run_control_partition_sils.sh 로 재기동하라." >&2; exit 2; }
echo "    sps OFF — 좌석이 비어 있다"

if [ "$WITH_MMF" = "1" ]; then
    echo "==> 1. MMF"
    if docker ps --format '{{.Names}}' | grep -qx oss-mmf-core; then
        echo "    이미 떠 있다: $(docker inspect oss-mmf-core --format '{{.Config.Image}}')"
    else
        CORE_IMG="${CORE_IMG:-oss-mmf-core:local}" ROS_DOMAIN_ID="$DOMAIN" docker compose \
            -f docker/compose/docker-compose-integration.yaml \
            -f docker/compose/docker-compose-nogpu.override.yaml up -d mmf-core >/dev/null 2>&1
        echo "    기동 ($(docker inspect oss-mmf-core --format '{{.Config.Image}}'))"
    fi
fi

echo "==> 2. 어댑터 4종"
docker rm -f sils-aw-status sils-aw-localization sils-aw-target sils-ss2-adapter >/dev/null 2>&1
run_adapter() {  # <실행물> <이름> [인자] [추가env]
    docker run -d --name "$2" --rm --net=host --ipc=host \
        -e ROS_DOMAIN_ID="$DOMAIN" -e RMW_IMPLEMENTATION=rmw_cyclonedds_cpp -e ROS_LOCALHOST_ONLY=0 \
        -e VISS_DDS_DOMAIN_ID="$DOMAIN" -e SILS_DURABILITY=transient-local -e SILS_RUN_ID=c1-mmf \
        ${4:+-e "$4"} "$ADAPTER_IMG" ros2 run ss2_ego_bridge "$1" ${3:-} >/dev/null && echo "    $2"
}
run_adapter aw_vehicle_status_bridge sils-aw-status
run_adapter aw_localization_bridge   sils-aw-localization
run_adapter aw_control_target_bridge sils-aw-target
run_adapter ss2_ego_bridge sils-ss2-adapter "--ros-args -p ss2_topic:=/simulation/entity/status" "SS2_EGO_NAME=ego"

echo "==> 3. vECU 5기"
docker rm -f sils-multi-vecu >/dev/null 2>&1
mkdir -p runs/multi-vecu
docker run -d --name sils-multi-vecu --net=host --ipc=host \
    -v "${REPO_ROOT}:/work:ro" -v "${REPO_ROOT}/runs/multi-vecu:/out" \
    -e VISS_DDS_DOMAIN_ID="$DOMAIN" -e SILS_RUN_ID=c1-mmf -e SILS_DURABILITY=transient-local \
    -e LD_LIBRARY_PATH=/work/output/x86_64/lib \
    --entrypoint /work/docker/sils/multi-vecu-entrypoint.sh "$VECU_IMG" >/dev/null && echo "    sils-multi-vecu"

sleep 8
echo "==> 4. 좌석 발행자 (1 = 우리 브리지)"
docker exec "$CONTROL" bash -lc 'source /opt/ros/humble/setup.bash
for t in /vehicle/status/velocity_status /localization/kinematic_state /localization/acceleration; do
  printf "    %-36s " "$t"; timeout 25 ros2 topic info --no-daemon "$t" 2>/dev/null | grep "Publisher count"; done'

echo "==> 5. 시나리오 (맨 마지막 — 다섯 중 시계를 가진 유일한 것)"
docker rm -f sils-ss2-scenario >/dev/null 2>&1
SILS_DOMAIN="$DOMAIN" SILS_RMW=rmw_cyclonedds_cpp \
SILS_SS2_SCENARIO=/home/ubuntu/Desktop/scenario_simulator_ws/src/test_runner/scenario_test_runner/scenario/LateralAction.LaneChangeAction.yaml \
SILS_SS2_LAUNCH_AUTOWARE=false SILS_SS2_VEHICLE_MODEL=sample_vehicle \
SILS_SS2_FRAME_RATE=20 SILS_SS2_GLOBAL_TIMEOUT=600 \
    docker compose -f docker/compose/docker-compose-sils-ss2.yaml up -d ss2-scenario >/dev/null 2>&1
echo "    기동 $(date +%T)"

# ⚠ 반드시 **단일 인용**으로 넘긴다. 큰따옴표로 감싸면 호스트 셸이 awk/sed 의 $1·$2 를
# 먼저 먹어버려서, 컨테이너 안에서는 빈 문자열이 되고 **에러 없이 빈 값만 나온다**.
# 값은 -e 로 넘긴다.
docker exec -e G_X="$GOAL_X" -e G_Y="$GOAL_Y" -e G_Z="$GOAL_Z" -e N_SAMPLES="$SAMPLES" \
    "$CONTROL" bash -lc '
source /opt/ros/humble/setup.bash; source /opt/autoware/setup.bash
pose() { timeout 6 ros2 topic echo --no-daemon /localization/kinematic_state --once 2>/dev/null \
           | sed -n "/position:/,+2p" | grep -E "^      [xy]:" | awk "{printf \"%s \", \$2}"; }
vel()  { timeout 6 ros2 topic echo --no-daemon /vehicle/status/velocity_status --once 2>/dev/null \
           | grep longitudinal_velocity | awk "{print \$2}"; }

for i in $(seq 1 40); do p=$(pose); [ -n "$p" ] && break; sleep 1; done
echo "==> 6. 자차 등장: $p"
printf "    init state (3=INITIALIZED, 수동 0): "
timeout 8 ros2 topic echo --no-daemon /localization/initialization_state --once --qos-durability transient_local 2>/dev/null | grep state

ros2 topic pub --once /planning/mission_planning/goal geometry_msgs/msg/PoseStamped \
  "{header: {frame_id: map}, pose: {position: {x: ${G_X}, y: ${G_Y}, z: ${G_Z}},
    orientation: {x: 0.0, y: 0.0, z: 0.2352, w: 0.9719}}}" >/dev/null 2>&1
echo "    goal 발행"
# ⚠ goal 과 engage 사이에 조회를 끼우지 말 것.
# 이 시나리오의 자차는 5 m/s 로 ~93 m 를 20초 만에 주파한다. --no-daemon 조회는 건당
# 6~8초라, 사이에 routing 상태를 한 번 물어보면 그동안 차가 종점에 닿아 **목표가 뒤로
# 가고** 경로가 무효가 되어 engage 가 success=False 로 거절된다. 실제로 그렇게 한 번
# 실패했고, 그건 rig 결함이 아니라 측정이 만든 지연이었다.
sleep 2
printf "    engage: "
ros2 service call /api/operation_mode/change_to_autonomous \
  autoware_adapi_v1_msgs/srv/ChangeOperationMode "{}" 2>&1 | grep -oE "success=[A-Za-z]+" | head -1
printf "    routing (2=SET): "
timeout 8 ros2 topic echo --no-daemon /api/routing/state --once --qos-durability transient_local 2>/dev/null | grep state || echo "(무응답)"

echo "==> 7. 주행 관측 (vx 가 상수면 시나리오 각본, 곡선이면 제어)"
for t in $(seq 1 ${N_SAMPLES}); do printf "    t=%-3s vx=%-10s %s\n" "$t" "$(vel)" "$(pose)"; sleep 1; done
'

echo
echo "==> 8. 판정"
for c in sils-ss2-adapter sils-aw-status sils-aw-localization sils-aw-target; do
    printf '    %-22s %s\n' "$c" "$(docker logs "$c" 2>&1 | grep -oE 'received=[0-9]+ published=[0-9]+.*' | tail -1 | cut -c1-70)"
done
for f in runs/multi-vecu/vcu.jsonl; do
    [ -f "$f" ] && printf '    %-22s ' "$(basename "$f")" && \
      for ev in state_received target_received arbitrated; do
        printf '%s=%s ' "$ev" "$(grep -c "\"event\":\"${ev}\"" "$f")"; done && echo
done
if [ "$WITH_MMF" = "1" ]; then
    echo "    MMF VSS 서빙:"
    docker logs --tail 200 oss-mmf-core 2>&1 | grep -oE 'Path: Vehicle\.[A-Za-z.]+' | sort -u | head -6 | sed 's/^/      /'
fi
echo
"${here}/check_mmf_typeobject.sh" || true
