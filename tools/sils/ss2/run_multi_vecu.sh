#!/usr/bin/env bash
#
# run_multi_vecu.sh -- 정준 버스 하나에 vECU 다섯을 동시에 올린다.
#
#     sils/ego/state (EgoState)        autoware/control_target (ControlTarget)
#            │                                    │
#     ┌──────┼──────┬──────────┬─────────┬────────┴──────┐
#     ▼      ▼      ▼          ▼         ▼               ▼
#  steer  adaptive classic  plainc            vehicle-interface (VCU)
#     │      │        │        │                         │
#  steer_cmd cmd_adaptive cmd_classic cmd_plainc     sils/ego/cmd  ──▶ 플랜트 어댑터
#
# ★ 왜 채널맵을 따로 두는가
#
# 예제 ECU 넷(adaptive/classic/plainc/vehicle-interface)은 전부 sils/ego/cmd 에 쓰도록
# 돼 있다. 그것들은 "동시에 도는 구성"이 아니라 서로의 대안이기 때문이다. 그대로 같이
# 올리면 한 토픽에 라이터가 넷이 되고, 플랜트가 누구 말을 듣는지는 발행 순서가 정한다 --
# 그리고 아무도 오류를 내지 않는다. 그래서 config/sils/channelmap/multi/ 의 사본을 쓴다.
# 코드는 안 바뀐다. 토픽 바인딩이 config 인 것이 이 SDK 의 설계다.
#
# ★ 이 스크립트가 증명하는 것과 아닌 것
#
# 증명: 여러 vECU 가 한 정준 버스에서 각자 자기 입력을 구독하고 각자 명령을 낸다.
# 아님: 통합(아비트레이션). VCU 는 아직 다른 ECU 의 출력을 입력으로 받지 않는다 --
#       Autoware target + EgoState 만 본다. 그건 vehicle_interface_behavior 의 일이고
#       채널맵으로는 못 한다. 다음 단계.
#
# 선행: 정준 버스에 EgoState 가 흐르고 있어야 한다 (플랜트 또는 픽스처).
#       autoware/control_target 은 aw_control_target_bridge 가 낸다 -- 없으면 VCU 는
#       safe stop 을 내고, 그것도 관측 대상이다.
#
# Usage:
#   tools/sils/ss2/run_multi_vecu.sh                 # 띄우고 30초 뒤 요약
#   tools/sils/ss2/run_multi_vecu.sh --duration 60
#   tools/sils/ss2/run_multi_vecu.sh --stop          # 전부 내린다
# Env: VISS_DDS_DOMAIN_ID (기본 42) · SILS_RUN_ID (기본 multi-vecu)

set -uo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${here}/../../.." && pwd)"
cd "${REPO_ROOT}"

DOMAIN="${VISS_DDS_DOMAIN_ID:-42}"
RUN_ID="${SILS_RUN_ID:-multi-vecu}"
DURABILITY="${SILS_DURABILITY:-transient-local}"
BIN="${SILS_BIN_DIR:-output/x86_64/Debug/extension/ros2/bin}"
LIB="${SILS_LIB_DIR:-output/x86_64/lib}"
OUT="${REPO_ROOT}/runs/multi-vecu"
DURATION=30
STOP=0

while [ "${1:-}" != "" ]; do
    case "$1" in
        --duration) DURATION="$2"; shift ;;
        --stop)     STOP=1 ;;
        --help|-h)  sed -n '2,40p' "${BASH_SOURCE[0]}"; exit 0 ;;
        *) echo "run_multi_vecu: unknown option: $1" >&2; exit 2 ;;
    esac
    shift
done

# 이름:실행물:채널맵  -- 순서는 표시 순서일 뿐, 기동 순서는 의미 없다(전부 idle-safe).
ECUS=(
  "steer:steer_ecu:service/sils/examples/steer-ecu/channelmap/steer-min.yaml"
  "adaptive:adaptive_ecu:config/sils/channelmap/multi/adaptive-multi.yaml"
  "classic:classic_vecu:config/sils/channelmap/multi/classic-multi.yaml"
  "plainc:plainc_ecu:config/sils/channelmap/multi/plainc-multi.yaml"
  "vcu:vehicle_interface:config/sils/channelmap/multi/vcu-arbitration.yaml"
)

stop_all() {
    for e in "${ECUS[@]}"; do
        exe="$(echo "$e" | cut -d: -f2)"
        # 자기 명령줄을 안 잡도록 [] 트릭 -- pkill -f 는 자기 자신도 매치한다
        pkill -f "$(echo "$exe" | sed 's/^\(.\)/[\1]/')" 2>/dev/null
    done
}

if [ "$STOP" = "1" ]; then
    stop_all; echo "run_multi_vecu: 전부 내렸다"; exit 0
fi

[ -d "$BIN" ] || { echo "run_multi_vecu: 실행물 디렉터리 없음: $BIN" >&2; exit 2; }
mkdir -p "$OUT"
stop_all   # 이전 런 잔여물 -- 남아 있으면 같은 토픽에 라이터가 둘이 된다

echo "==> multi-vECU  도메인 ${DOMAIN} · run_id ${RUN_ID} · durability ${DURABILITY}"
for e in "${ECUS[@]}"; do
    name="$(echo "$e" | cut -d: -f1)"
    exe="$(echo "$e"  | cut -d: -f2)"
    cmap="$(echo "$e" | cut -d: -f3)"
    [ -x "${BIN}/${exe}" ] || { echo "    건너뜀 ${name}: ${BIN}/${exe} 없음"; continue; }
    [ -f "${cmap}" ]       || { echo "    건너뜀 ${name}: 채널맵 없음 ${cmap}"; continue; }
    VISS_DDS_DOMAIN_ID="${DOMAIN}" SILS_RUN_ID="${RUN_ID}" SILS_DURABILITY="${DURABILITY}" \
    SILS_CHANNELMAP="${cmap}" LD_LIBRARY_PATH="${REPO_ROOT}/${LIB}" \
        nohup "${BIN}/${exe}" > "${OUT}/${name}.jsonl" 2>&1 &
    echo "    기동 ${name}  (${exe})  out=$(grep -A2 'dir: out' "${cmap}" | grep topic: | awk '{print $2}')"
done

echo
echo "==> ${DURATION}초 관측"
sleep "${DURATION}"

echo
printf '%-10s %-22s %s\n' "ECU" "이벤트" "건수"
for e in "${ECUS[@]}"; do
    name="$(echo "$e" | cut -d: -f1)"
    f="${OUT}/${name}.jsonl"
    [ -f "$f" ] || continue
    # state_received 가 0이면 버스에 EgoState 가 없거나 도메인/durability 가 어긋난 것이다.
    # 그 둘은 증상이 같다 -- 조용히 아무 일도 안 일어난다.
    for ev in state_received target_received control_published; do
        n=$(grep -c "\"event\":\"${ev}\"" "$f" 2>/dev/null)
        [ "${n:-0}" = "0" ] && continue
        printf '%-10s %-22s %s\n' "$name" "$ev" "$n"
    done
done
echo
echo "로그: ${OUT}/*.jsonl    내리기: tools/sils/ss2/run_multi_vecu.sh --stop"
