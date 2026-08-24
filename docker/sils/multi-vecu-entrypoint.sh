#!/usr/bin/env bash
#
# multi-vecu-entrypoint.sh -- 컨테이너 하나에서 vECU 여럿을 띄운다.
#
# 왜 한 컨테이너인가: 배포 형태가 그렇다. MMF 노드 하나가 여러 ECU 서비스를 호스팅하고,
# 그 노드가 정준 버스에 참가한다. 프로세스를 호스트에 흩뿌리면 도메인/durability 를
# 각각 맞춰야 하고, 하나 빠뜨려도 증상이 "조용히 아무 일도 안 일어남" 이라 안 보인다.
# 여기서는 env 를 컨테이너가 한 번만 정하고 전원이 그것을 상속한다.
#
# 각 ECU 는 여전히 별개 프로세스다 -- 별개 DDS 참가자이고 따로 죽는다. 한 프로세스에
# 합치면 어느 ECU 가 멈췄는지 카운터로 못 가른다.
#
# 채널맵은 ECU 마다 다르고, OUT 토픽이 서로 겹치면 안 된다. 겹치면 한 토픽에 라이터가
# 여럿이 되고 승자는 발행 순서가 정한다 -- 오류는 안 난다.
# config/sils/channelmap/multi/ 의 사본들이 그것을 갈라놓은 것이다.
#
# env:
#   VISS_DDS_DOMAIN_ID  정준 버스 도메인 (필수 -- 기본값을 두지 않는다. 어긋나면 조용하다)
#   SILS_RUN_ID         트레이스 상관키
#   SILS_DURABILITY     transient-local | volatile  (버스의 라이터와 같아야 한다)
#   MULTI_VECU_LIST     "이름:실행물:채널맵" 을 세미콜론으로. 비우면 아래 기본 5종.
#   MULTI_VECU_OUT      jsonl 출력 디렉터리 (기본 /out)

set -u

BIN="${MULTI_VECU_BIN:-/work/output/x86_64/Debug/extension/ros2/bin}"
OUT="${MULTI_VECU_OUT:-/out}"
LIST="${MULTI_VECU_LIST:-}"

if [ -z "${LIST}" ]; then
    LIST="steer:steer_ecu:/work/service/sils/examples/steer-ecu/channelmap/steer-min.yaml"
    LIST="${LIST};adaptive:adaptive_ecu:/work/config/sils/channelmap/multi/adaptive-multi.yaml"
    LIST="${LIST};classic:classic_vecu:/work/config/sils/channelmap/multi/classic-multi.yaml"
    LIST="${LIST};plainc:plainc_ecu:/work/config/sils/channelmap/multi/plainc-multi.yaml"
    LIST="${LIST};vcu:vehicle_interface:/work/config/sils/channelmap/multi/vcu-arbitration.yaml"
fi

mkdir -p "${OUT}"
echo "[multi-vecu] 도메인 ${VISS_DDS_DOMAIN_ID:-<미설정>} · run_id ${SILS_RUN_ID:-<미설정>} · durability ${SILS_DURABILITY:-<미설정>}"

pids=""
IFS=';'
for entry in ${LIST}; do
    name="$(echo "${entry}" | cut -d: -f1)"
    exe="$(echo "${entry}"  | cut -d: -f2)"
    cmap="$(echo "${entry}" | cut -d: -f3)"
    if [ ! -x "${BIN}/${exe}" ]; then echo "[multi-vecu] 건너뜀 ${name}: ${BIN}/${exe} 없음"; continue; fi
    if [ ! -f "${cmap}" ];      then echo "[multi-vecu] 건너뜀 ${name}: 채널맵 없음 ${cmap}"; continue; fi
    # classic_vecu 만 추가 env 를 요구한다 -- 제어 math 가 FMU 안에 있고, VR(밸류 레퍼런스)
    # 매핑은 그 FMU 의 계약이라 코드에도 채널맵에도 안 들어간다.
    if [ "${name}" = "classic" ]; then
        export SILS_FMU_FILE="${MULTI_VECU_FMU:-/work/service/sils/examples/classic-vecu/models/vecu-fmu/vecu.fmu}"
        export SILS_FMU_VR_VX="${SILS_FMU_VR_VX:-0}"
        export SILS_FMU_VR_ACCEL_OUT="${SILS_FMU_VR_ACCEL_OUT:-1}"
    fi
    SILS_CHANNELMAP="${cmap}" "${BIN}/${exe}" > "${OUT}/${name}.jsonl" 2>&1 &
    pids="${pids} $!"
    echo "[multi-vecu] 기동 ${name} (${exe}) pid $!"
done
unset IFS

# 하나가 죽어도 나머지는 계속 둔다 -- 죽은 것만 카운터가 멈추는 것이 관측 대상이다.
# 컨테이너는 docker stop 까지 산다.
trap 'kill ${pids} 2>/dev/null' TERM INT
wait
