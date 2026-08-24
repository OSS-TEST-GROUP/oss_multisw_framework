#!/usr/bin/env bash
#
# run_c2_with_mmf.sh -- C2 구성 + MMF. **Autoware 가 차를 모는 유일한 구성**에 MMF 를 얹는다.
#
#   SS_v2 (isEgo 있는 시나리오)
#     EgoEntity ◀── concealer ◀── control_cmd ◀────────────────┐
#        │             │                                       │
#        │             └─▶ /vehicle/status/* · /localization/* ─┼─▶ Autoware
#        │                                                      │      │
#        └─ /entity/status ─▶ ① ─▶ sils/ego/state ─▶ vECU 5기 ◀─┴─ ② ◀─┘
#                                                                      │
#                                        MMF viss-server ◀── ADAPI ◀───┘
#                                             └─▶ VSS (Vehicle.ADS.*)
#
# ★ C1 과 무엇이 다른가 — 좌석 주인이 바뀐다
#
#   C1: 우리 브리지(③④)가 좌석. 차는 시나리오 각본이 움직인다 → vx 상수
#   C2: concealer 가 좌석. **차는 Autoware 제어로 움직인다** → vx 곡선
#
#   그래서 ③④⑤ 를 **꺼야 한다**. 켜면 concealer 와 같은 토픽에 라이터가 둘이 되고
#   아무도 오류를 내지 않는다. docs/sils/sils-configuration-map.md §3 매트릭스
#
#   SS_v2 는 둘을 동시에 못 한다(isEgo 분기, simple_sensor_simulator.cpp:201).
#   MORAI 가 오면 그때 합쳐진다.
#
# ★ 이 구성에만 필요한 둘
#   - localization shim: concealer 가 /api/localization/initialize 를 부르는데 이 파티션에
#     서버가 없다. 없으면 30회 실패하고 죽는다.
#   - deprecated_api: concealer 가 /api/autoware/set/velocity_limit 을 부른다.
#
# ★ 선행
#   1) 파티션 3종은 **이 스크립트가 매번 재기동한다**(위 ★★). 처음 한 번만 있으면 된다.
#      ⚠ 상류 run_partitions.sh 를 여러 개 띄우지 말 것 — EXIT 트랩이 파티션을 쓸어간다.
#   3) MMF 는 패치 이미지 권장(oss-mmf-core:local). 미패치면 파티션 로그가 오염된다.
#      docs/sils/evidence/mmf-typeobject-2026-08-12.md
#
# Usage:
#   tools/sils/ss2/run_c2_with_mmf.sh
#   tools/sils/ss2/run_c2_with_mmf.sh --samples 30
#   tools/sils/ss2/run_c2_with_mmf.sh --no-mmf     # 대조군
#   tools/sils/ss2/run_c2_with_mmf.sh --no-restart # 파티션 재기동 생략(권장 안 함)

set -uo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${here}/../../.." && pwd)"
cd "${REPO_ROOT}"

DOMAIN="${SILS_DOMAIN:-42}"
CONTROL="adsw-control-${DOMAIN}"
ADAPTER_IMG="${SILS_SS2_ADAPTER_IMAGE:-sils-ss2-adapter:humble}"
VECU_IMG="${SILS_VECU_IMAGE:-oss-mmf-sils-vecu:latest}"
SCENARIO="${SILS_SS2_SCENARIO:-/home/ubuntu/Desktop/scenario_simulator_ws/src/test_runner/scenario_test_runner/scenario/RoutingAction.AcquirePositionAction.yaml}"
SAMPLES=25
WITH_MMF=1
RESTART_CONTROL=1
MAP_PATH="${ADSW_MAP_PATH:-$HOME/autoware_map/kashiwanoha}"

while [ "${1:-}" != "" ]; do
    case "$1" in
        --samples) SAMPLES="$2"; shift ;;
        --no-mmf)  WITH_MMF=0 ;;
        --no-restart) RESTART_CONTROL=0 ;;
        --map-path) MAP_PATH="$2"; shift ;;
        --help|-h) sed -n '2,40p' "${BASH_SOURCE[0]}"; exit 0 ;;
        *) echo "run_c2_with_mmf: unknown option: $1" >&2; exit 2 ;;
    esac
    shift
done

docker ps --format '{{.Names}}' | grep -qx "$CONTROL" || {
    echo "run_c2_with_mmf: ${CONTROL} 이 없다. 위 '선행' 을 먼저 하라." >&2; exit 2; }

# ★★ C2 는 런마다 **파티션 3종 전부** 재기동해야 한다.
#
# Autoware 상태가 런 사이에 안 씻긴다. 앞 런이 끝나면 /autoware/state 가 3 에 머물고,
# 다음 concealer 는 WAITING_FOR_ENGAGE 를 기다리다 죽는다:
#   AutowareError: Simulator waited for the Autoware state to transition to
#                  WAITING_FOR_ENGAGE, but time is up.
#
# ⚠ **control 만 재기동해선 부족하다** — 실측으로 확인했다. decision/perception 도
#   상태를 든다. 5번 실패하고 3종 전부 재기동한 뒤에야 재현됐다.
#     control 만 재기동 → 실패 (15:43 · 15:48)
#     3종 전부 재기동   → 성공 (15:51, 완전한 속도 곡선)
#
# 재기동은 컨테이너 안에서 exec 한 shim·deprecated_api 도 같이 치운다 — 그것들도
# 매 실행마다 쌓여서 같은 증상을 만든다(3번 돌리자 shim 2개·api 5개까지 갔다).
if [ "$RESTART_CONTROL" = "1" ]; then
    echo "==> -1. 파티션 3종 재기동 (Autoware 상태 초기화)"
    "${here}/run_partitions_detached.sh" --domain-id "$DOMAIN" --map-path "$MAP_PATH" >/dev/null 2>&1
    "${here}/run_control_partition_sils.sh" --domain-id "$DOMAIN" --map-path "$MAP_PATH" >/dev/null 2>&1
    until docker exec "$CONTROL" bash -lc \
        'source /opt/ros/humble/setup.bash && timeout 20 ros2 node list --no-daemon 2>/dev/null | grep -q vehicle_cmd_gate'; do
        sleep 5
    done
    echo "    준비 완료 (shim $(docker exec "$CONTROL" bash -lc "pgrep -cf '[l]ocalization_initialize_shim'" 2>/dev/null) · api $(docker exec "$CONTROL" bash -lc "pgrep -cf '[d]eprecated_api'" 2>/dev/null))"
fi

echo "==> 0. 전제"
# --no-daemon: ros2cli 데몬이 죽으면 모든 조회가 빈 출력이 되고 rig 고장과 구분이 안 된다.
nodes=$(docker exec "$CONTROL" bash -lc 'source /opt/ros/humble/setup.bash && timeout 25 ros2 node list --no-daemon 2>/dev/null' | wc -l)
[ "$nodes" -ge 10 ] || { echo "    ⛔ ros2 조회 불가 (노드 ${nodes}개)" >&2; exit 2; }
echo "    ros2 CLI OK (노드 ${nodes})"
r=$(pgrep -cf '[r]un_partitions.sh')
[ "$r" -le 1 ] || { echo "    ⛔ run_partitions.sh 가 ${r}개 — EXIT 트랩이 파티션을 쓸어간다" >&2; exit 2; }
echo "    상류 러너 ${r}개"

# ★ sps 검사 — C2 에서도 필요하다. 처음엔 "concealer 가 좌석을 갖는 구성이니 sps 는
# 상관없다" 고 생각해 빼먹었는데, 그러면 좌석에 **셋**이 앉는다(sps + concealer).
# 실측: Publisher count 2, Autoware 가 두 상태를 번갈아 보고 차가 1.6→1.0→0 으로 서고
# 시나리오가 failures=1 로 떨어졌다. 오류는 아무 데서도 안 난다.
n=$(docker exec "$CONTROL" bash -lc 'source /opt/ros/humble/setup.bash && timeout 25 ros2 node list --no-daemon 2>/dev/null' \
      | grep -c simple_planning_simulator)
[ "$n" -eq 0 ] || { echo "    ⛔ simple_planning_simulator 가 살아 있다 (좌석에 sps + concealer 둘)." >&2
                    echo "       tools/sils/ss2/run_control_partition_sils.sh --domain-id ${DOMAIN} --map-path <맵> 로 교체하라." >&2
                    exit 2; }
echo "    sps OFF"

echo "==> 1. ③④⑤ 를 내린다 (concealer 가 그 자리를 갖는다)"
docker rm -f sils-aw-status sils-aw-localization sils-ss2-control >/dev/null 2>&1
echo "    OK"

if [ "$WITH_MMF" = "1" ]; then
    echo "==> 2. MMF"
    if docker ps --format '{{.Names}}' | grep -qx oss-mmf-core; then
        echo "    이미 떠 있다: $(docker inspect oss-mmf-core --format '{{.Config.Image}}')"
    else
        CORE_IMG="${CORE_IMG:-oss-mmf-core:local}" ROS_DOMAIN_ID="$DOMAIN" docker compose \
            -f docker/compose/docker-compose-integration.yaml \
            -f docker/compose/docker-compose-nogpu.override.yaml up -d mmf-core >/dev/null 2>&1
        echo "    기동 ($(docker inspect oss-mmf-core --format '{{.Config.Image}}'))"
    fi
fi

echo "==> 3. localization shim + deprecated_api (C2 에만 필요)"
# ★ 멱등하게. 이 스크립트는 여러 번 돌린다 — 매번 새로 띄우면 한 벌씩 쌓인다.
# 실측: 3번 돌리자 shim 2개 · deprecated_api 5개가 되었고, 그 상태에서 Autoware 가
# WAITING_FOR_ENGAGE 에 못 가서 시나리오가 errors=1 로 죽었다:
#   AutowareError: Simulator waited for the Autoware state to transition to
#                  WAITING_FOR_ENGAGE, but time is up.
# 같은 서비스에 서버가 여럿이면 누가 응답할지 발행 순서가 정하고, 아무도 오류를 안 낸다.
#
# ⚠ pkill 이 권한으로 실패할 수 있다(컨테이너 안 uid 가 다름). 그래서 "죽인다" 가 아니라
#   **개수를 1로 맞춘다** 가 목표다: 0이면 띄우고, 1이면 그대로 쓰고, 2 이상이면 중단.
cnt() { docker exec "$CONTROL" bash -lc "pgrep -cf '$1'" 2>/dev/null | tr -d '\r'; }

sh_n=$(cnt '[l]ocalization_initialize_shim'); ap_n=$(cnt '[d]eprecated_api')
if [ "${sh_n:-0}" -gt 1 ] || [ "${ap_n:-0}" -gt 1 ]; then
    docker exec "$CONTROL" bash -lc 'pkill -f "[l]ocalization_initialize_shim"; pkill -f "[d]eprecated_api"; exit 0' >/dev/null 2>&1
    sleep 4
    sh_n=$(cnt '[l]ocalization_initialize_shim'); ap_n=$(cnt '[d]eprecated_api')
fi
[ "${sh_n:-0}" -le 1 ] && [ "${ap_n:-0}" -le 1 ] || {
    echo "    ⛔ 중복이 남아 있다 (shim ${sh_n} · api ${ap_n}). 파티션을 재기동하라." >&2; exit 2; }

if [ "${sh_n:-0}" -eq 0 ]; then
    "${here}/run_localization_shim.sh" --domain-id "$DOMAIN" >/dev/null 2>&1 && echo "    shim 기동"
else
    echo "    shim 재사용 (1개)"
fi
if [ "${ap_n:-0}" -eq 0 ]; then
    docker exec -d "$CONTROL" bash -lc \
      'source /opt/ros/humble/setup.bash && source /opt/autoware/setup.bash &&
       exec ros2 launch tier4_autoware_api_launch deprecated_api.launch.xml > /workspace/log/deprecated_api.txt 2>&1' \
      && echo "    deprecated_api 기동"
else
    echo "    deprecated_api 재사용 (1개)"
fi
sleep 8
echo "    최종: shim $(cnt '[l]ocalization_initialize_shim') · api $(cnt '[d]eprecated_api')"

echo "==> 4. ①② + vECU"
docker rm -f sils-ss2-adapter sils-aw-target sils-multi-vecu >/dev/null 2>&1
docker run -d --name sils-aw-target --rm --net=host --ipc=host \
    -e ROS_DOMAIN_ID="$DOMAIN" -e RMW_IMPLEMENTATION=rmw_cyclonedds_cpp -e ROS_LOCALHOST_ONLY=0 \
    -e VISS_DDS_DOMAIN_ID="$DOMAIN" -e SILS_DURABILITY=transient-local -e SILS_RUN_ID=c2-mmf \
    "$ADAPTER_IMG" ros2 run ss2_ego_bridge aw_control_target_bridge >/dev/null && echo "    ② sils-aw-target"
docker run -d --name sils-ss2-adapter --rm --net=host --ipc=host \
    -e ROS_DOMAIN_ID="$DOMAIN" -e RMW_IMPLEMENTATION=rmw_cyclonedds_cpp -e ROS_LOCALHOST_ONLY=0 \
    -e VISS_DDS_DOMAIN_ID="$DOMAIN" -e SILS_DURABILITY=transient-local -e SILS_RUN_ID=c2-mmf \
    -e SS2_EGO_NAME=ego "$ADAPTER_IMG" \
    ros2 run ss2_ego_bridge ss2_ego_bridge --ros-args -p ss2_topic:=/simulation/entity/status >/dev/null \
    && echo "    ① sils-ss2-adapter"
mkdir -p runs/multi-vecu
docker run -d --name sils-multi-vecu --net=host --ipc=host \
    -v "${REPO_ROOT}:/work:ro" -v "${REPO_ROOT}/runs/multi-vecu:/out" \
    -e VISS_DDS_DOMAIN_ID="$DOMAIN" -e SILS_RUN_ID=c2-mmf -e SILS_DURABILITY=transient-local \
    -e LD_LIBRARY_PATH=/work/output/x86_64/lib \
    --entrypoint /work/docker/sils/multi-vecu-entrypoint.sh "$VECU_IMG" >/dev/null && echo "    vECU 5기"

echo "==> 5. 시나리오 (isEgo 있음 — concealer 가 뜬다)"
docker rm -f sils-ss2-scenario >/dev/null 2>&1
SILS_DOMAIN="$DOMAIN" SILS_RMW=rmw_cyclonedds_cpp \
SILS_SS2_SCENARIO="$SCENARIO" \
SILS_SS2_LAUNCH_AUTOWARE=false SILS_SS2_VEHICLE_MODEL=sample_vehicle \
SILS_SS2_FRAME_RATE=20 SILS_SS2_GLOBAL_TIMEOUT=600 \
    docker compose -f docker/compose/docker-compose-sils-ss2.yaml up -d ss2-scenario >/dev/null 2>&1
echo "    기동 $(date +%T)  ($(basename "$SCENARIO"))"

# ★ 목표도 engage 도 우리가 안 한다 — concealer 가 AD API 로 직접 한다. 그게 C2 다.
docker exec -e N_SAMPLES="$SAMPLES" "$CONTROL" bash -lc '
source /opt/ros/humble/setup.bash; source /opt/autoware/setup.bash
vel() { timeout 6 ros2 topic echo --no-daemon /vehicle/status/velocity_status --once 2>/dev/null \
          | grep longitudinal_velocity | awk "{print \$2}"; }
mode() { timeout 6 ros2 topic echo --no-daemon /api/operation_mode/state --once \
           --qos-durability transient_local 2>/dev/null | grep -E "^mode" | awk "{print \$2}"; }

# ★ 대기 루프를 두지 않는다 — 시나리오 주행 구간은 ~25초뿐이다.
# 처음엔 mode 가 2 가 될 때까지 기다린 다음 속도를 쟀는데, mode() 한 번이
# `timeout 6` + `sleep 2` 라 10회면 80초가 지나고 **차는 이미 완주해 있었다.**
# 그래서 mode 2 · errors=0 인데 속도 샘플이 전부 빈값으로 나왔다.
# 지금은 시나리오 기동 직후부터 바로 재고, mode 는 곁들여 본다.
echo "==> 6. 즉시 관측 (곡선이면 Autoware 제어, 상수면 각본)"
printf "    좌석 발행자(1=concealer): "
timeout 12 ros2 topic info --no-daemon /vehicle/status/velocity_status 2>/dev/null | grep -oE "Publisher count: [0-9]+" || echo "?"
seen=0
for t in $(seq 1 ${N_SAMPLES}); do
  v=$(vel)
  if [ $((t % 6)) -eq 1 ]; then m=$(mode); else m=""; fi
  [ -n "$v" ] && [ "$v" != "0.0" ] && seen=1
  printf "    t=%-3s vx=%-22s %s\n" "$t" "${v:-·}" "${m:+mode=$m}"
done
[ "$seen" = "1" ] && echo "    ✅ 0 이 아닌 속도를 봤다" || echo "    ⚠ 움직임을 못 봤다 (창을 놓쳤거나 안 움직였다)"
'


echo
echo "==> 9. 런 종료 후 집계"
for c in sils-ss2-adapter sils-aw-target; do
    printf '    %-20s %s\n' "$c" "$(docker logs "$c" 2>&1 | grep -oE 'received=[0-9]+ published=[0-9]+.*' | tail -1 | cut -c1-64)"
done
[ -f runs/multi-vecu/vcu.jsonl ] && printf '    %-20s ' vcu.jsonl && \
  for ev in state_received target_received arbitrated; do
    printf '%s=%s ' "$ev" "$(grep -c "\"event\":\"${ev}\"" runs/multi-vecu/vcu.jsonl)"; done && echo
# 좌석 발행자는 §6 에서 주행 중에 쟀다. 여기서 다시 재면 시나리오가 이미 끝나
# concealer 가 사라진 뒤라 0 이 나온다 — 그건 결함이 아니라 측정 시점 문제다.
if [ "$WITH_MMF" = "1" ]; then
    echo "    MMF VSS 경로: $(docker logs oss-mmf-core 2>&1 | grep -oE 'Path: Vehicle\.[A-Za-z.]+' | sort -u | wc -l)개"
    docker logs oss-mmf-core 2>&1 | grep -oE 'Path: Vehicle\.ADS\.VehicleKinematics[A-Za-z.]*' | sort -u | head -3 | sed 's/^/      /'
fi
echo -n "    시나리오 판정: "; ls runs/ss2-scenario/*/result.junit.xml 2>/dev/null | tail -1 | xargs grep -oE 'failures="[0-9]+" errors="[0-9]+"' 2>/dev/null || echo "(아직)"
echo -n "    TypeObject 에러: "
n=0; for l in perception decision control; do c=$(grep -c 'invalid type object' "$HOME/oss/oss_adsw/log/${l}_log.txt" 2>/dev/null); case "$c" in ''|*[!0-9]*) c=0;; esac; n=$((n+c)); done; echo "${n}건"
