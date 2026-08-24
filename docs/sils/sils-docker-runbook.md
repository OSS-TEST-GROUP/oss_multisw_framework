# 도커 런북 — 컨테이너별 구성과 전체 실행 명령 ★

> 작성 2026-08-12 · **명령은 전부 실제로 돈 것**이다 — 살아 있는 컨테이너의
> `docker inspect` 에서 뽑았고, 손으로 지어낸 줄이 없다.
> 짝 문서: [`sils-configuration-map.md`](sils-configuration-map.md) (무엇을 켜고 끄나 / 왜) —
> **이 문서는 "그래서 명령이 뭐냐" 만 다룬다.**

---

## 0. 전제 — 세 가지만 지키면 된다

| | 값 | 왜 |
|---|---|---|
| **도메인** | **42 하나** (`ROS_DOMAIN_ID` = `VISS_DDS_DOMAIN_ID` = 42) | **도메인을 가르지 않는다.** 어긋나면 증상이 "조용히 아무 일도 안 일어남" 이다 |
| **네트워크** | 전부 `--net=host --ipc=host` | DDS SIMPLE discovery + 멀티캐스트가 컨테이너 밖 vECU 까지 닿아야 한다 |
| **러너 중복 금지** | `run_partitions.sh` 는 **한 번에 하나만** | ★ 아래. 여러 개면 **파티션이 흔적 없이 사라진다** |
| **durability** | `SILS_DURABILITY` 를 **버스 전원 동일** | TRANSIENT_LOCAL 리더 ↔ VOLATILE 라이터는 **매칭 자체가 안 된다** |

> RMW 는 **경계마다 다르다.** Autoware 쪽에 붙는 프로세스는 Cyclone
> (`RMW_IMPLEMENTATION=rmw_cyclonedds_cpp`), 정준 SiLS 버스는 네이티브 Fast DDS.
> 어댑터는 **한 프로세스 안에서 둘 다** 쓴다 — 그게 어댑터가 하는 일이다.

### ★★ 파티션이 흔적 없이 사라지면 — `run_partitions.sh` 의 EXIT 트랩부터 의심하라

상류 러너가 이렇게 끝난다:

```bash
trap cleanup INT TERM EXIT     # cleanup() { docker stop <파티션 3종>; }
tail -n +1 -F <로그 3개>        # 영원히 안 끝난다
```

파티션은 `--rm` 이라 **stop 되는 순간 `docker ps -a` 에서도 사라진다.**
그리고 러너를 여러 번 띄우면 **전부 살아남아** 각자 트랩을 들고 같은 세 컨테이너를
겨눈다(2026-08-12 에 8개가 동시에 떠 있었다). 그중 **아무 하나의 프로세스 그룹만
정리돼도 전멸**한다.

```bash
pgrep -cf '[r]un_partitions.sh'      # 1을 넘으면 그게 문제다. [r] 브래킷 필수
```

> ⚠ `kill <pid>` 로는 트랩이 **안 돈다.** bash 는 포그라운드 명령(`tail -F`)이 끝나야
> 트랩을 처리하는데 그게 영원히 안 끝난다. **프로세스 그룹째** 죽어야 돈다 —
> 그래서 증상이 산발적으로 보인다.

**에이전트·스크립트가 띄울 때는 트랩 없는 쪽을 쓴다:**

```bash
tools/sils/ss2/run_partitions_detached.sh --domain-id 42 --map-path "$AUTOWARE_MAP_DIR"
tools/sils/ss2/run_partitions_detached.sh --stop
```

사람이 터미널에 앉아 Ctrl-C 로 내릴 거면 상류 러너가 맞다.

### MMF 의 TypeObject 전파 — 로그는 더럽히지만 죽이지는 않는다

미패치 MMF 가 도메인에 있으면 파티션이 이 줄을 쏟는다:

```
dq.builtin: type [INVALID 0000…]: ddsi_xt_type_init_impl with invalid type object
```

| 참가자 | 규칙 |
|---|---|
| vECU | ✅ `sils_participant_qos.hpp` |
| 어댑터 | ✅ `aw_type_propagation.hpp` |
| MMF | ✅ **고쳐짐** (`DDSProvider.cpp` · `dds_transport.cpp` · `DDSProtocol.cpp`) — 단 **이미지 재빌드 필요** |

| 도메인 42 상태 | `invalid type object` | 파티션 |
|---|---|---|
| MMF 없음 | 0 | 생존 |
| 미패치 MMF | **1260** | **생존** |
| 패치 MMF (`oss-mmf-core:local`) | **0** | 생존 |

> ⚠ **이건 가용성 문제가 아니다.** 초판 런북은 "4초 만에 죽는다" 고 적었는데 **틀렸다** —
> 사망 원인은 위의 트랩이었다. 경위: [`evidence/mmf-typeobject-2026-08-12.md` §4](evidence/mmf-typeobject-2026-08-12.md)
>
> ⚠ **로그 마지막 줄이 `invalid type object` 인 것은 원인의 근거가 못 된다.**
> 그 줄이 decision 로그의 **29%** 를 차지한다 — 아무 이유로 끊겨도 마지막 줄이 그것이기 쉽다.
>
> 판정: `grep -c 'invalid type object' ~/oss/oss_adsw/log/*_log.txt`
> (`tools/sils/ss2/check_mmf_typeobject.sh`)

---

## 1. 이미지 — 한 번만 만들면 된다

| 이미지 | 크기 | 무엇 | 만드는 법 |
|---|---|---|---|
| `ghcr.io/oss-test-group/autoware-partition:adsw-{control,decision,perception}` | 4.5 / 4.1 / 4.1 GB | **벤더 Autoware 파티션 3종** | `docker pull` (빌드 안 한다) |
| `sils-ss2:humble` | 4.6 GB | **SS_v2 런타임** (시나리오 러너 + `simple_sensor_simulator` + kashiwanoha 맵) | `docker/sils/build-ss2.sh` |
| `sils-ss2-dev:humble` | 11 GB | 위의 dev 단계 — 어댑터 빌드에 필요 (`traffic_simulator_msgs` 헤더) | 〃 (같이 태깅됨) |
| `sils-ss2-adapter:humble` | 11.3 GB | **어댑터 6종 전부** (`ros2 run ss2_ego_bridge <실행물>`) | `docker/sils/build-ss2-adapter.sh` |
| `oss-mmf-sils-vecu:latest` | 306 MB | **vECU 런타임** (예제 ECU 5종) | `./build.sh --build-sils` + 이미지 빌드 |

```bash
# 최초 1회 — 순서가 있다 (어댑터는 SS_v2 dev 이미지를 필요로 한다)
docker/sils/build-ss2.sh                    # ~30 GB 디스크, 오래 걸린다
docker/sils/build-ss2-adapter.sh

# 맵 꺼내기 (Autoware 파티션이 SS_v2 와 같은 맵을 봐야 한다) — 1회
mkdir -p "$HOME/autoware_map"
cid=$(docker create sils-ss2:humble)
docker cp "$cid:/home/ubuntu/Desktop/scenario_simulator_ws/install/kashiwanoha_map/share/kashiwanoha_map/map" \
          "$HOME/autoware_map/kashiwanoha"
docker rm "$cid"
```

> ⚠ `oss-mmf-sils-vecu:latest` 에 **`unzip` 이 없다** → `classic_vecu` 가
> `Failed to unzip FMU: status = 32512` 로 죽는다. 이미지 수정 전까지는 그 ECU 만
> 호스트에서 돌리거나 목록에서 뺀다.

---

## 2. 컨테이너 카탈로그 ★

```
   ┌─────────────── Autoware (ROS 2 · Cyclone · 도메인 42) ────────────────┐
   │  adsw-perception-42     adsw-decision-42     adsw-control-42          │
   └───────────────────────────────┬───────────────────────────────────────┘
                                   │ ROS 토픽
   ┌───────────────────────────────┼───────────────────────────────────────┐
   │            어댑터 컨테이너 (두 얼굴: Cyclone ↔ Fast DDS)                │
   │  sils-ss2-adapter   sils-aw-target   sils-aw-status   sils-aw-localization │
   │  sils-ss2-control                                                      │
   └───────────────────────────────┬───────────────────────────────────────┘
                                   │ 정준 DDS (Fast DDS · 도메인 42)
   ┌───────────────────────────────┼───────────────────────────────────────┐
   │  sils-multi-vecu   (컨테이너 하나에 ECU 프로세스 5개)                    │
   └───────────────────────────────────────────────────────────────────────┘

   ┌─── 플랜트 ───┐
   │ sils-ss2-scenario  또는  sils-ss2-mock   (ROS 2 · 도메인 42)
   └──────────────┘
```

| 컨테이너 | 이미지 | 안에서 도는 것 | 띄우는 법 | RMW |
|---|---|---|---|---|
| `adsw-perception-42` | `:adsw-perception` | `sample_adsw_perception_run.launch.xml` | **`run_partitions_detached.sh`** ★ | Cyclone |
| `adsw-decision-42` | `:adsw-decision` | `/autoware/start_script/adsw-decision.sh` | 〃 | Cyclone |
| `adsw-control-42` | `:adsw-control` | **우리 launch 파일** (sps OFF) | `run_control_partition_sils.sh` ★ | Cyclone |
| `sils-ss2-scenario` | `sils-ss2:humble` | `scenario_test_runner` (+ concealer, `isEgo` 면) | compose `ss2-scenario` | 설정 가능 |
| `sils-ss2-mock` | `sils-ss2:humble` | `cpp_mock_scenarios` (concealer 없음) | compose `ss2-mock` | 설정 가능 |
| `sils-ss2-adapter` | `sils-ss2-adapter:humble` | ① `ss2_ego_bridge` | compose `ss2-adapter` | Cyclone |
| `sils-ss2-control` | 〃 | ⑤ `ss2_control_bridge` | compose `ss2-control` | Cyclone |
| `sils-aw-target` | 〃 | ② `aw_control_target_bridge` | **수동 `docker run`** (§5) | Cyclone |
| `sils-aw-status` | 〃 | ③ `aw_vehicle_status_bridge` | `run_vehicle_seat_bridges.sh` | Cyclone |
| `sils-aw-localization` | 〃 | ④ `aw_localization_bridge` | 〃 | Cyclone |
| `sils-multi-vecu` | `oss-mmf-sils-vecu:latest` | ECU 5기 (steer/adaptive/classic/plainc/VCU) | `multi-vecu-entrypoint.sh` | Fast DDS |

어댑터 번호(①~⑥)는 [`sils-configuration-map.md` §1](sils-configuration-map.md) 의 전수표와 같다.

### ★ 이름 함정 — compose 이름과 실제 이름이 다르다

| 어디 | 이름 |
|---|---|
| `docker-compose-integration.yaml` | `oss-adsw-control-container` |
| **실제로 도는 것** | **`adsw-control-42`** ← `run_partitions.sh` / `run_control_partition_sils.sh` 가 만든다 |

**둘은 같은 이미지의 다른 컨테이너다.** compose 로 띄우고 스크립트로 또 띄우면 파티션이
둘이 되고 한 ROS 그래프에 같은 노드가 두 벌 뜬다. **compose 경로와 스크립트 경로를
섞지 말 것.** 아래 명령들은 전부 **스크립트 경로**다.

---

## 3. 구성별 전체 실행 명령 ★ 복붙용

전부 `cd /path/to/oss_multisw_framework` 기준. 공통 환경:

```bash
export ROS_DOMAIN_ID=42
export VISS_DDS_DOMAIN_ID=42
export SILS_DURABILITY=transient-local
export AUTOWARE_MAP_DIR="$HOME/autoware_map/kashiwanoha"
```

---

### C0 · 픽스처 — 플랜트 없이 좌석 배선만

```bash
# 1) Autoware 파티션 3종 — 트랩 없는 쪽 (§0 ★★ 참고)
tools/sils/ss2/run_partitions_detached.sh --domain-id 42 --map-path "$AUTOWARE_MAP_DIR"

tools/sils/ss2/run_control_partition_sils.sh \
    --domain-id 42 --map-path "$AUTOWARE_MAP_DIR"
# ↑ sps(simple_planning_simulator) 를 끈다. 이게 좌석을 비우는 유일한 방법이다.

# 2) 좌석 브리지 ③④ + 개루프 픽스처
ROS_DOMAIN_ID=42 VISS_DDS_DOMAIN_ID=42 \
tools/sils/ss2/run_vehicle_seat_bridges.sh --with-ego-source --duration 60

# 3) 명령 하행 ② (vECU 에 Autoware 판단을 준다)
docker run -d --name sils-aw-target --rm --net=host --ipc=host \
    -e ROS_DOMAIN_ID=42 -e RMW_IMPLEMENTATION=rmw_cyclonedds_cpp -e ROS_LOCALHOST_ONLY=0 \
    -e VISS_DDS_DOMAIN_ID=42 -e SILS_DURABILITY=transient-local \
    sils-ss2-adapter:humble \
    ros2 run ss2_ego_bridge aw_control_target_bridge
```

**확인** — sps 가 정말 꺼졌는지가 이 구성의 전부다:

```bash
docker exec adsw-control-42 bash -lc \
  'source /opt/ros/humble/setup.bash && ros2 node list | grep simple_planning_simulator'
# 빈 출력이어야 한다. 나오면 좌석에 둘이 앉아 있다.
```

---

### C1 · SS_v2 를 플랜트로 (`isEgo` 없음) — 좌석은 우리 것

C0 의 1)·2)·3) 을 그대로 하되 **픽스처 대신 SS_v2** 를 쓰고, ①과 ⑤를 추가한다.

```bash
# 1) 파티션 (C0 과 동일)
tools/sils/ss2/run_partitions_detached.sh --domain-id 42 --map-path "$AUTOWARE_MAP_DIR"
tools/sils/ss2/run_control_partition_sils.sh --domain-id 42 --map-path "$AUTOWARE_MAP_DIR"

# 2) 좌석 브리지 ③④  (--with-ego-source 없이! 상태는 SS_v2 가 준다)
ROS_DOMAIN_ID=42 VISS_DDS_DOMAIN_ID=42 tools/sils/ss2/run_vehicle_seat_bridges.sh

# 3) ② 명령 하행  (위 C0 3) 과 동일한 docker run)

# 4) vECU
docker run -d --name sils-multi-vecu --net=host --ipc=host \
    -v "$PWD:/work:ro" -v "$PWD/runs/multi-vecu:/out" \
    -e VISS_DDS_DOMAIN_ID=42 -e SILS_RUN_ID=c1 -e SILS_DURABILITY=transient-local \
    -e LD_LIBRARY_PATH=/work/output/x86_64/lib \
    --entrypoint /work/docker/sils/multi-vecu-entrypoint.sh \
    oss-mmf-sils-vecu:latest

# 5) ①⑤ 어댑터 + 시나리오 — 시나리오는 반드시 마지막
SILS_DOMAIN=42 SILS_RMW=rmw_cyclonedds_cpp \
  docker compose -f docker/compose/docker-compose-sils-ss2.yaml up -d ss2-adapter ss2-control

SILS_DOMAIN=42 SILS_RMW=rmw_cyclonedds_cpp \
SILS_SS2_SCENARIO=/scenarios/<isEgo 없는 시나리오>.yaml \
SILS_SS2_LAUNCH_AUTOWARE=false \
SILS_SS2_VEHICLE_MODEL=sample_vehicle \
SILS_SS2_STATE_TOPIC=/simulation/entity/status \
  docker compose -f docker/compose/docker-compose-sils-ss2.yaml up -d ss2-scenario
```

> ★ **시나리오가 마지막인 이유**: 다섯 중 넷은 idle-safe 다 (붙일 게 없는 브리지는 조용히
> 기다린다). **시나리오만 시계를 갖는다** — `global_timeout` 초를 돌고 끝난다. 먼저 띄우면
> 나머지를 올리는 동안 측정하려던 창을 태워 먹는다. 실제로 그렇게 한 번 실패했다.
>
> ⚠ `ss2_topic` 은 **런처마다 다르다**: `ss2-mock` → `/entity/status`,
> `ss2-scenario` → `/simulation/entity/status`. 틀리면 **구독이 그냥 매칭 안 된다 — 조용히.**

한 방에 하려면 (2계단 폐루프 확인용, Autoware 없이도 돈다):

```bash
tools/sils/ss2/run_closed_loop.sh --watch 20 --domain 42
```

---

### C2 · SS_v2 EgoEntity 주행 (08-12 실측) — 좌석을 concealer 에게 내준다

**차이는 셋을 끄고 셋을 더하는 것이다.**

```bash
# 1) 파티션 (동일)
tools/sils/ss2/run_partitions_detached.sh --domain-id 42 --map-path "$AUTOWARE_MAP_DIR"
tools/sils/ss2/run_control_partition_sils.sh --domain-id 42 --map-path "$AUTOWARE_MAP_DIR"

# 2) ⛔ 좌석 브리지 ③④⑤ 를 반드시 내린다 — concealer 와 같은 토픽에 쓴다
docker rm -f sils-aw-status sils-aw-localization sils-ss2-control 2>/dev/null

# 3) ★ 초기화 서비스 공백 메우기 — 이게 없으면 concealer 가 30회 실패하고 죽는다
tools/sils/ss2/run_localization_shim.sh --domain-id 42

# 4) ★ tier4 external API — concealer 가 /api/autoware/set/velocity_limit 을 부른다
docker exec -d adsw-control-42 bash -lc \
  'source /opt/ros/humble/setup.bash && source /opt/autoware/setup.bash &&
   exec ros2 launch tier4_autoware_api_launch deprecated_api.launch.xml \
        > /workspace/log/deprecated_api.txt 2>&1'

# 5) ② 명령 하행 + ① 상태 상행 (vECU 관측용) + vECU  — C1 의 3)4)5) 중 ss2-control 만 뺀다
docker run -d --name sils-aw-target --rm --net=host --ipc=host \
    -e ROS_DOMAIN_ID=42 -e RMW_IMPLEMENTATION=rmw_cyclonedds_cpp -e ROS_LOCALHOST_ONLY=0 \
    -e VISS_DDS_DOMAIN_ID=42 -e SILS_DURABILITY=transient-local \
    sils-ss2-adapter:humble ros2 run ss2_ego_bridge aw_control_target_bridge

docker run -d --name sils-multi-vecu --net=host --ipc=host \
    -v "$PWD:/work:ro" -v "$PWD/runs/multi-vecu:/out" \
    -e VISS_DDS_DOMAIN_ID=42 -e SILS_RUN_ID=multi-vecu -e SILS_DURABILITY=transient-local \
    -e LD_LIBRARY_PATH=/work/output/x86_64/lib \
    --entrypoint /work/docker/sils/multi-vecu-entrypoint.sh \
    oss-mmf-sils-vecu:latest

SILS_DOMAIN=42 SILS_RMW=rmw_cyclonedds_cpp SS2_EGO_NAME=ego \
SILS_SS2_STATE_TOPIC=/simulation/entity/status \
  docker compose -f docker/compose/docker-compose-sils-ss2.yaml up -d ss2-adapter

# 6) 시나리오 — isEgo 가 있는 것. 이미지 내장 경로를 그대로 쓴다
SILS_DOMAIN=42 SILS_RMW=rmw_cyclonedds_cpp \
SILS_SS2_SCENARIO=/home/ubuntu/Desktop/scenario_simulator_ws/src/test_runner/scenario_test_runner/scenario/RoutingAction.AcquirePositionAction.yaml \
SILS_SS2_LAUNCH_AUTOWARE=false \
SILS_SS2_VEHICLE_MODEL=sample_vehicle \
SILS_SS2_FRAME_RATE=20 \
SILS_SS2_GLOBAL_TIMEOUT=600 \
  docker compose -f docker/compose/docker-compose-sils-ss2.yaml up -d ss2-scenario
```

**주행 관측:**

```bash
# 속도가 곡선인지 (상수면 명령이 안 먹은 것이다)
docker exec adsw-control-42 bash -lc 'source /opt/ros/humble/setup.bash &&
  timeout 15 ros2 topic echo /vehicle/status/velocity_status --field longitudinal_velocity'

# 자율주행 모드 (2 = AUTONOMOUS)
docker exec adsw-control-42 bash -lc 'source /opt/ros/humble/setup.bash &&
  ros2 topic echo /api/operation_mode/state --once'

# 시나리오 판정
cat runs/ss2-scenario/*/result.junit.xml
```

> ⚠ `SILS_SS2_LAUNCH_AUTOWARE=false` 를 유지한다. `true` 면 concealer 가
> `ros2 launch autoware_launch` 를 **직접 execvp** 해서 우리가 이미 띄운 파티션과 겹친다.

---

### C3 · MORAI 본선 — 아직 못 돈다

`MORAI 어댑터`가 없다. 배선은 C1 과 동일하고 ①⑤ 자리에 MORAI 어댑터가 들어간다.

---

## 4. 컨테이너 개별 `docker run` — 스크립트 없이 하나씩

전부 같은 골격이다. **어댑터 6종은 이미지가 하나이고 마지막 인자만 다르다.**

```bash
docker run -d --name <이름> --rm --net=host --ipc=host \
    -e ROS_DOMAIN_ID=42 \
    -e RMW_IMPLEMENTATION=rmw_cyclonedds_cpp \
    -e ROS_LOCALHOST_ONLY=0 \
    -e VISS_DDS_DOMAIN_ID=42 \
    -e SILS_DURABILITY=transient-local \
    sils-ss2-adapter:humble \
    ros2 run ss2_ego_bridge <실행물> [--ros-args -p <파라미터>]
```

| # | 이름 | `<실행물>` | 추가 인자 |
|---|---|---|---|
| ① | `sils-ss2-adapter` | `ss2_ego_bridge` | `--ros-args -p ss2_topic:=/simulation/entity/status`<br>+ `-e SS2_EGO_NAME=ego` |
| ② | `sils-aw-target` | `aw_control_target_bridge` | 없음 |
| ③ | `sils-aw-status` | `aw_vehicle_status_bridge` | 없음 |
| ④ | `sils-aw-localization` | `aw_localization_bridge` | (선택) `-p publish_initialpose:=false` |
| ⑤ | `sils-ss2-control` | `ss2_control_bridge` | 없음 |
| ⑥ | (개발용) | `aw_ego_state_bridge` | 🟡 실 구성 금지 |

### vECU 컨테이너 — ECU 목록을 바꾸려면

```bash
docker run -d --name sils-multi-vecu --net=host --ipc=host \
    -v "$PWD:/work:ro" -v "$PWD/runs/multi-vecu:/out" \
    -e VISS_DDS_DOMAIN_ID=42 -e SILS_RUN_ID=my-run -e SILS_DURABILITY=transient-local \
    -e LD_LIBRARY_PATH=/work/output/x86_64/lib \
    -e MULTI_VECU_LIST="vcu:vehicle_interface:/work/config/sils/channelmap/multi/vcu-arbitration.yaml;adaptive:adaptive_ecu:/work/config/sils/channelmap/multi/adaptive-multi.yaml" \
    --entrypoint /work/docker/sils/multi-vecu-entrypoint.sh \
    oss-mmf-sils-vecu:latest
```

형식은 `이름:실행물:채널맵` 을 `;` 로 잇는다. 비우면 기본 5종.
**OUT 토픽이 겹치면 안 된다** — 겹치면 한 토픽에 라이터가 여럿, 승자는 발행 순서.

호스트 프로세스로 돌리려면 (`unzip` 문제 회피 포함):

```bash
tools/sils/ss2/run_multi_vecu.sh --duration 60
tools/sils/ss2/run_multi_vecu.sh --stop
```

---

## 5. 확인 명령 모음 ★ 조용한 실패를 잡는 법

```bash
# ── 뭐가 떠 있나
docker ps --format '{{.Names}}\t{{.Image}}\t{{.Status}}'

# ── ★ 좌석에 둘이 앉았나 (가장 자주 나는 사고)
docker exec adsw-control-42 bash -lc 'source /opt/ros/humble/setup.bash &&
  ros2 topic info -v /vehicle/status/velocity_status | head -20'
#   Publisher count: 1 이어야 한다.  2 면 ③ 와 concealer/sps 가 겹친 것.
#   0 이면 아무도 안 채우고 있다 — Autoware 는 눈이 먼 상태다.

# ── 파티션 3종이 한 그래프에 있나 (이미지에 패키지가 없다고 "부재" 로 결론내면 틀린다)
docker exec adsw-control-42 bash -lc 'source /opt/ros/humble/setup.bash && ros2 node list' | wc -l

# ── 어댑터가 실제로 흘리고 있나 (5초 카운터)
docker logs --tail 5 sils-ss2-adapter     # received=… published=…  matched=0 이면 토픽 이름/QoS
docker logs --tail 5 sils-aw-target

# ── vECU 별 이벤트 집계
for f in runs/multi-vecu/*.jsonl; do
  echo "== $f"
  for ev in state_received target_received arbitrated control_published; do
    printf '   %-20s %s\n' "$ev" "$(grep -c "\"event\":\"$ev\"" "$f")"
  done
done
#   state_received=0 이면 버스에 EgoState 가 없거나 도메인/durability 가 어긋난 것이다.
#   그 둘은 증상이 같다.

# ── 초기화가 걸렸나 (3 = INITIALIZED)
docker exec adsw-control-42 bash -lc 'source /opt/ros/humble/setup.bash &&
  ros2 topic echo /localization/initialization_state --once'

# ── Autoware 로그
tail -f ~/oss/oss_adsw/log/control_log.txt
```

---

## 6. 정지 · 청소

```bash
# 어댑터 + vECU 만 (파티션은 살려둔다 — 재기동이 비싸다)
docker rm -f sils-ss2-adapter sils-ss2-control sils-aw-target \
             sils-aw-status sils-aw-localization sils-multi-vecu 2>/dev/null

# 시나리오
docker compose -f docker/compose/docker-compose-sils-ss2.yaml down

# 파티션까지 전부
docker rm -f adsw-control-42 adsw-decision-42 adsw-perception-42

# localization shim (컨테이너가 아니라 파티션 안의 프로세스다)
docker exec adsw-control-42 pkill -f '[l]ocalization_initialize_shim' || true
```

> ⚠ **ZeroMQ 5555 를 놓아주지 않은 시뮬레이터**가 남으면 다음 인터프리터가 **옛 시뮬레이터**에
> 붙는다. 소켓은 ESTABLISHED 로 멀쩡해 보이고, 두 프로세스가 서로 다른 맵을 말하며
> **아무 에러 없이 멈춰 있는다.** 의심되면 `ss -lntp | grep 5555`.
>
> ⚠ `pkill -f <패턴>` 은 **자기 셸도 매친다.** `[l]ocalization` 처럼 대괄호 트릭을 쓸 것.

---

## 7. 환경변수 한 장

| 변수 | 기본 | 누가 읽나 | 주의 |
|---|---|---|---|
| `ROS_DOMAIN_ID` | 42 | ROS 2 쪽 전부 | `VISS_DDS_DOMAIN_ID` 와 같게 |
| `VISS_DDS_DOMAIN_ID` | 42 | 어댑터의 DDS 면 · vECU | 〃 |
| `SILS_DURABILITY` | `transient-local` | 어댑터 · vECU | **버스 전원 동일**. 어긋나면 매칭 안 됨 |
| `SILS_RUN_ID` | — | vECU · 어댑터 | 트레이스 상관키 |
| `SILS_DOMAIN` | 0 | compose 의 `ss2-*` | ⚠ 기본이 0 이다. **반드시 42 로 준다** |
| `SILS_RMW` | `rmw_fastrtps_cpp` | compose 의 `ss2-*` | Autoware 와 붙일 땐 `rmw_cyclonedds_cpp` |
| `SILS_SS2_SCENARIO` | `/scenarios/sample.xosc` | `ss2-scenario` | 컨테이너 안 경로 |
| `SILS_SS2_LAUNCH_AUTOWARE` | `false` | 〃 | **false 유지** — true 면 concealer 가 Autoware 를 또 띄운다 |
| `SILS_SS2_VEHICLE_MODEL` | **빈 값** | 〃 | ⚠ 비우면 `IDEAL_STEER_VEL` 로 떨어져 **가속도 명령을 안 읽는다**. `sample_vehicle` 을 준다 |
| `SILS_SS2_STATE_TOPIC` | `/entity/status` | `ss2-adapter` | `ss2-scenario` 면 `/simulation/entity/status` |
| `SILS_SS2_FRAME_RATE` | 20 | `ss2-scenario` | 상류 기본 30 과 다르다 — 스텝이 바뀌면 트레이스 비교가 깨진다 |
| `SILS_SS2_RECORD` | `false` | 〃 | 켜면 도메인 42 **전 토픽**을 녹화해 로그를 덮는다 |
| `SILS_SS2_ZMQ_PORT` | 5555 | 〃 | **호스트 포트**다 (`network_mode: host`) |
| `MULTI_VECU_LIST` | 기본 5종 | vECU 컨테이너 | `이름:실행물:채널맵` `;` 구분 |
| `AUTOWARE_MAP_DIR` | — | 파티션 오버레이 | `~` 말고 `$HOME` 을 쓸 것 |
| `ADSW_MAP_PATH` | `~/autoware_map/sample-map-planning` | `run_control_partition_sils.sh` | **SS_v2 와 맞춰야 한다** → kashiwanoha |

---

## 8. 자주 나는 실패 → 원인

| 증상 | 원인 |
|---|---|
| `pull access denied / repository does not exist` | 이미지를 **아직 안 만든 것**이다 (레지스트리에 없다). §1 |
| `could not select device driver "nvidia"` | 파티션이 GPU 를 요구. **GPU 예약을 지워 반쪽만 띄우지 말 것** |
| concealer 가 `Requested the service … 30 times` 로 죽음 | localization shim 미기동. C2 의 3) |
| 차가 완벽히 정지, 그런데 카운터는 전부 건강 | `SILS_SS2_VEHICLE_MODEL` 빈 값 → `IDEAL_STEER_VEL` 이 velocity 를 읽는다 |
| 시나리오가 ~6초 만에 조용히 종료 | `:ro` 마운트 시나리오를 전처리기가 제자리 수정 시도 (EROFS). 래퍼가 `/out/staging` 으로 복사해야 한다 |
| 어댑터 `matched=0` | 토픽 이름(`ss2_topic`) · 도메인 · durability 셋 중 하나. 증상이 전부 같다 |
| `Failed to unzip FMU: status = 32512` | vECU 이미지에 `unzip` 없음 |
| 브리지를 띄웠는데 `Publisher count: 2` | 좌석에 둘. [`configuration-map`](sils-configuration-map.md) §3 매트릭스 |
| **파티션 3종이 흔적 없이 사라짐** (`docker ps -a` 에도 없음) | **`run_partitions.sh` 중복 + EXIT 트랩.** §0 ★★ |
| `ros2` 조회가 전부 빈 출력 | **ros2cli 데몬 사망** (`Fault: RuntimeError:!rclpy.ok()`). `--no-daemon` 을 쓴다 |
| `docker exec` 안에서 `awk $2` 가 빈 값 | 호스트 셸이 먼저 먹었다. **단일 인용** + `-e` 로 값 전달 |
| `Simulator waited for ... WAITING_FOR_ENGAGE, but time is up` | **Autoware 상태가 앞 런에서 안 씻겼다.** 파티션 **3종 전부** 재기동 (control 만으론 부족) · shim/api 중복도 같이 치워진다 |
| `Goal's footprint exceeds lane!` | 목표가 너무 멀다. 10~30 m 앞으로 (`ros2 param set` 은 안 먹는다 — 생성 시 캐시) |

---

*관련: [`sils-configuration-map.md`](sils-configuration-map.md) (무엇을 켜고 끄나) ·
[`sils-integration-topology.md`](sils-integration-topology.md) (배선 원리) ·
[`sils-ss2-bringup.md`](sils-ss2-bringup.md) (SS_v2 브링업 절차) ·
[`sils-stage3-autoware-partition.md`](sils-stage3-autoware-partition.md) (3계단) ·
[`evidence/`](evidence/) (원측정값)*
