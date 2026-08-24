# 3계단 — KATECH Autoware Partition 연동

> 작성 2026-08-10 · **갱신 2026-08-10 (1·2·3 단계 실측 반영)** · 대상 장비: **리눅스(Ubuntu 22.04)**
> 이 문서 하나로 3계단 작업을 처음부터 이어갈 수 있게 쓴 인수인계 문서다.
> 1~3 단계는 끝났고 실측 기록이 각 절에 들어 있다 — **다음은 4단계다.**

## 이 문서를 새 세션에서 여는 법

리눅스 장비에서:

```bash
cd <oss_multisw_framework 경로>
git checkout feature/sils-b1-1-ss2-clock-survey
git pull
claude
```

> **2026-08-11 최신**: Autoware 가 우리 브리지 데이터로 **자율주행 모드에 진입해 가속
> 명령을 냈고, 그 명령이 vECU 까지 되돌아왔다.** 도메인 42 단일 구성.
> → [`evidence/autoware-return-path-2026-08-11.md`](evidence/autoware-return-path-2026-08-11.md)
> 남은 것은 플랜트 하나다 — `sils/ego/cmd` 를 소비할 SS_v2.

## ★ 다음에 할 일 (2026-08-11 세션 종료 시점)

> **다음 세션 트리거 문장** — 이 한 줄을 그대로 붙여넣으면 이어서 시작한다:
>
> ```
> docs/sils/evidence/ss2-clock-survey-2026-08-11.md §4 부터 이어서 해줘.
> openscenario_preprocessor_node 로그부터 확보해서 ss2-scenario 즉시 종료 원인을 잡고,
> 그다음 /clock 드리프트 실측 → tick 브리지 → SS_v2 폐루프 순서로 진행.
> ```

**목표는 "Autoware 가 SS_v2 시뮬 데이터로 주행한다".** 부품은 전부 있고 막힌 건 둘이다.

| 상태 | 내용 |
|---|---|
| ✅ 끝남 | 시뮬레이터를 끄고 `aw_vehicle_status_bridge` + `aw_localization_bridge` 만으로 Autoware 구동. 53개 구독 중 52개 매칭, 유실 0, **경로 수립(routing SET)** 까지. [evidence](evidence/vehicle-seat-no-simulator-2026-08-11.md) |
| 🔴 블로커 1 | **`ss2-scenario` 가 6초 만에 종료.** 가설 5개 배제 완료, 전처리기가 "파생 0" 응답하는 지점까지 좁혔다. [evidence §4](evidence/ss2-clock-survey-2026-08-11.md) |
| 🔴 블로커 2 | **시계.** SS_v2 는 `/clock`(epoch)과 `EntityStatus.time`(0 기준 sim 초)을 둘 다 내는데 `ss2_ego_bridge` 가 후자를 쓴다 → Autoware 가 56년 묵은 메시지로 보고 자율주행을 막는다. [evidence §1](evidence/ss2-clock-survey-2026-08-11.md) |

블로커 1 이 먼저다 — SS_v2 가 살아 있어야 `/clock` 드리프트도 재고 폐루프도 돈다.
그 드리프트 값이 **tick 브리지를 만들지, 단순 오프셋으로 끝낼지**를 가른다
([evidence §3](evidence/ss2-clock-survey-2026-08-11.md)).

이 장비에는 저장소·맵·이미지가 이미 다 있다. 컨테이너는 전부 내려둔 상태다.

### 되살리는 법

```bash
# 1) Autoware 파티션 (전용 터미널 또는 백그라운드 — trap 때문에 죽으면 컨테이너도 내려간다)
cd "$HOME/oss/oss_adsw"
./partition/run_partitions.sh --repo ghcr.io/oss-test-group/autoware-partition \
  --map-path "$HOME/autoware_map/sample-map-planning" --domain-id 43 --headless --delay 15

# 2) tier4 external API (3단계에서 찾은 것 — 컨테이너 재시작하면 매번 다시 붙여야 한다)
docker exec -d adsw-control-43 bash -lc \
  'source /opt/ros/humble/setup.bash && source /opt/autoware/setup.bash &&
   exec ros2 launch tier4_autoware_api_launch deprecated_api.launch.xml \
     > /workspace/log/deprecated_api.txt 2>&1'

# 3) 주행시키기 — 좌표는 /workspace/log/{pose.json,pose_cmds.sh,reset.sh} 에 저장돼 있다
#    없으면 evidence 문서의 "유효 좌표 뽑기" 절차를 다시 돌린다
docker exec adsw-control-43 bash -lc 'source /opt/ros/humble/setup.bash &&
   source /opt/autoware/setup.bash &&
   bash -c "$(cat /workspace/log/reset.sh)"; sleep 6
   bash -c "$(sed -n 2p /workspace/log/pose_cmds.sh)"; sleep 8
   ros2 service call /api/operation_mode/change_to_autonomous \
     autoware_adapi_v1_msgs/srv/ChangeOperationMode "{}"'
```

### ✅ 끝남 — `simple_planning_simulator` 를 끄고 우리 브리지만으로 (2026-08-11)

전체 기록: [`evidence/vehicle-seat-no-simulator-2026-08-11.md`](evidence/vehicle-seat-no-simulator-2026-08-11.md)

**계획이 틀려 있었다.** 역브리지 하나로는 안 된다. 그 시뮬레이터는 `/vehicle/status/*`
여섯 개가 아니라 **열 개**를 발행하고, 큰 쪽은 `/localization/*` 였다 —
`kinematic_state` 하나가 구독 28개, `/localization/*` 합계 38개로
`/vehicle/status/*` 전체(14개)의 2.7배다. 그리고
`planning_simulator.launch.xml` 이 `launch_localization=false` 를 못박아서
**끄는 순간 자차 위치를 말해줄 것이 아무것도 남지 않는다.**

보강해서 다시 돌린 결과:

```bash
# 시뮬레이터 끈 제어 파티션으로 교체 (상류 파일 수정 없음)
tools/sils/ss2/run_control_partition_sils.sh --domain-id 43

# 두 브리지 + 열린 루프 픽스처
ROS_DOMAIN_ID=43 VISS_DDS_DOMAIN_ID=42 \
  tools/sils/ss2/run_vehicle_seat_bridges.sh --with-ego-source --duration 300
```

| 확인 | 결과 |
|---|---|
| 8개 토픽 퍼블리셔 | 전부 우리 브리지, QoS 시뮬레이터와 **정확히 일치** |
| Autoware 수신율 | `/localization/kinematic_state` **48.7 Hz** |
| 유실 | `4116/4116` · `4125/4125` — **0** |
| `waiting for current odometry` / `current_twist` / `[AEB] ego velocity` | ✅ **전부 멈춤** |
| IMU 미발행이 제어를 막는가 | **아니오** — AEB 가 요구하지 않고 넘어감 |
| `actuation_status` | **구독자 0** — 발행 불필요 확인 |

⚠ **주행은 증명 안 됨.** 픽스처가 맵 원점에서 출발해 경로가 안 선다
(`Control is skipped ...` 지속). 브리지 결함이 아니라 픽스처 한계이고,
주행 증명에는 맵 좌표를 아는 진짜 플랜트가 필요하다.

### 다음 작업

| 할 일 | 왜 |
|---|---|
| 맵 좌표를 아는 플랜트 확보 (MORAI 어댑터 / SS_v2) | 주행 증명의 유일한 길 |
| `bin/sils_ego_source` 에 초기 pose 인자 추가 | 픽스처만으로 경로·궤적까지 밀어볼 수 있다 |
| vECU 를 `vehicle_interface` 좌석에 넣고 폐루프 | `aw_control_target_bridge` 와 맞물리는 반대편 |

배선 전체 그림과 어떤 브리지가 실제 구성에 남는지:
**[sils-integration-topology.md](sils-integration-topology.md)**

---

## 0. 지금 어디까지 왔나

작업은 **3계단 사다리**로 되어 있다 ([sils-simulator-loops.md §7.3](sils-simulator-loops.md)).

| 계단 | 내용 | 상태 |
|---|---|---|
| 1 · 배선 | 열린 루프. 시뮬레이터 → vECU | ✅ 완료 |
| 2 · B-det | 닫힌 루프. vECU 가 차를 몬다 | ✅ **완료** — ego 가 **10.00 m/s 도달·유지**, vECU 의 `SILS_CTRL_SETPOINT` 와 일치 |
| 3 · B-full | **진짜 Autoware** 투입. 결정론은 포기 | 🟡 **진행 중** — 1·2·3 단계 ✅ (2026-08-10), **4단계 시작** |

### 2계단은 Autoware 없이 닫혔다

SS_v2(`scenario_simulator_v2`) 안에 `concealer` 라는 부품이 있다. Autoware 와 대화하는 담당인데,
**상대가 진짜인지 확인할 방법이 없다** — 정해진 토픽 3개와 서비스 10개에 규격대로 답하면 믿는다.

그래서 파이썬 대역을 만들어 그 자리에 넣었다 → [`adapters/autoware-stub/`](../../adapters/autoware-stub/README.md).
진짜 `concealer` 가 이 stub 을 `initializing → waiting_for_route → waiting_for_engage → driving`
까지 **0.1 초만에** 통과시켰다. 2계단에 GPU 가 필요 없었던 이유다.

**3계단은 이 stub 을 진짜 Autoware 로 바꾸는 일이다.**

### 지금 돌아가는 2계단 구성 (참고)

```
[SS_v2 시뮬레이터] ──상태──▶ [ss2-adapter] ──▶ [vECU] ──▶ [ss2-control] ──제어──┐
        ▲                                                                       │
        └──────────────────── 차량 모델이 가속도를 적용 ◀──────────────────────┘

           옆에서:  concealer ──▶ [autoware-stub]   ← 진짜 Autoware 자리를 막아둔 것
```

관련 문서: [sils-ss2-bringup.md](sils-ss2-bringup.md) · [adapters/ss2/README.md](../../adapters/ss2/README.md)

---

## 1. 무엇을 붙이는가

**KATECH Autoware Partition** — GitHub 공개 저장소.

| | |
|---|---|
| 저장소 | `OSS-TEST-GROUP/katech-autoware` (+ `katech-autoware-universe`), `main` |
| 기반 | **Autoware Universe 0.41.2** · ROS 2 Humble · Ubuntu 22.04 |
| 구조 | **Docker 파티션 3개** — Perception / Decision / Control |
| 사전 빌드 이미지 | `ghcr.io/oss-test-group/autoware-partition:adsw-{perception,decision,control}` |
| 아키텍처 | linux/amd64 **및** linux/arm64 |
| GPU | **불필요** — 빌드·실행 모두 **No-CUDA** 기준 |

> GPU 가 3계단의 하드 블로커라고 알고 있었다면 **그건 틀렸다.** 이 파티션은 No-CUDA 로 검증돼 있다.

### Control 파티션이 우리에게 중요한 이유

구성이 **Control + Vehicle Interface + Simulator + AD API** 다.
`partition/partition_config/sample-adsw-control.json` 에서:

| 패키지 | 우리와의 관계 |
|---|---|
| `autoware_default_adapi`, `autoware_adapi_adaptors` | **우리 stub 이 흉내내던 바로 그것** |
| `autoware_simple_planning_simulator` | 그들의 플랜트 — **SS_v2 와 경쟁** |
| `autoware_raw_vehicle_cmd_converter` | vehicle interface — **우리 vECU 의 목표 자리** |
| `obigo_launch` | 우리 이름의 launch 패키지가 **이미 파티션 안에 있다** |

---

## 2. 이미 뽑아둔 사실 (다시 조사하지 말 것)

1. **파티션은 Cyclone DDS 다.** `partition/run_partitions.sh` 가
   `RMW_IMPLEMENTATION=rmw_cyclonedds_cpp` 를 세운다. 우리 SiLS/SS_v2 는 `rmw_fastrtps_cpp` 다.
   1단계에는 무관하지만(파티션끼리만 대화) **3단계부터 바로 걸린다** — 우리가 파티션의
   ROS 그래프를 들여다보는 순간부터다. 그리고 4·5단계에서는 정면 문제가 된다:
   `concealer` ↔ AD API 는 **전부 서비스**이고, Humble 의 벤더 간 서비스 상호운용은 불안정하다.
   마침 이것이 [B1-3](sils-simulator-loops.md) 의 미측정 칸(Cyclone 열)이다 —
   **B1-3 는 3단계의 선행이지 나중 일이 아니다.**

   실무적으로: 우리 도구는 전부 `SILS_RMW` 기본값이 `rmw_fastrtps_cpp` 다. 파티션을 상대할 때는
   **매번 `SILS_RMW=rmw_cyclonedds_cpp` 를 붙인다.** 안 붙이면 결과가 "규격 불일치"가 아니라
   "그래프가 비었다"로 나온다 — 두 개는 전혀 다른 진단이다.
   (`sils-ss2:humble` 안에 `librmw_cyclonedds_cpp.so` 가 들어 있다. 이미지 재빌드 불필요.)

2. **컨테이너 이름에 도메인이 붙는다**: `adsw-perception-42`, `adsw-decision-42`, `adsw-control-42`.
   `--domain-id` 를 바꾸면 이름도 갈라져서 우리 `sils-*` 컨테이너와 **충돌하지 않는다.**

3. **기본 `ROS_DOMAIN_ID` 가 42 — 우리와 같다.** 실제 충돌이다. Control 파티션이 AD API 를 42 에
   서빙하고, 우리 stub 도 42 다. [`run_ego_scenario.sh`](../../tools/sils/ss2/run_ego_scenario.sh) 는
   42 에 `/api/*` 가 이미 있으면 거부하도록 만들어져 있고, 여기서 그 가드가 **정상적으로** 걸린다.

4. 컨테이너는 `--net=host --pid=host --ipc=host --rm` 으로 뜬다.
   호스트 쪽 로그는 `<katech 저장소>/log/{perception,decision,control}_log.txt`.

5. 기동은 순차: Perception → 10초 → Decision → 10초 → Control (`--delay` 로 조절).

6. ★ **GPU 모순의 정체 — 같은 이미지, 다른 런처다.** 이 파티션을 띄우는 길이 두 개 있고
   둘의 차이가 정확히 GPU 예약이다.

   | 경로 | GPU |
   |---|---|
   | [`docker-compose-integration.yaml`](../../docker/compose/docker-compose-integration.yaml) 의 `oss-adsw-{perception,decision,control}` | `deploy.resources.reservations.devices: nvidia` **있음** → nvidia 런타임 없으면 기동 거부 |
   | `partition/run_partitions.sh` | GPU 인자 **없음**. 실행 시 `CUDA: disabled` 를 출력한다 |

   그래서 [sils-simulator-loops.md §4](sils-simulator-loops.md) 의 08-07 관측
   (`could not select device driver "nvidia"`)은 **틀린 게 아니라 compose 경로에서 맞는 말**이었다.
   3계단은 `run_partitions.sh` 경로로 간다 → **GPU 불필요.**
   compose 로 띄우고 nvidia 에러를 만났다면 문서가 틀린 게 아니라 런처를 잘못 고른 것이다.

7. 컨테이너가 `$HOME/source/{fastdds,ros2}` 와 저장소 루트를 마운트한다. 저장소 루트가
   컨테이너의 `/workspace` 이고, 로그가 거기 떨어지기 때문에 호스트에서 바로 읽힌다(4번).

8. ★★ **파티션의 기본 기동에는 tier4 external API 절반이 빠져 있다 — 런치 인자 하나 때문이다.**
   `tier4_autoware_api_launch/launch/autoware_api.launch.xml` 의
   **`launch_deprecated_api` 기본값이 `false`** 이고, 바로 그 `deprecated_api.launch.xml` 이
   `autoware_iv_external_api_adaptor` 를 띄운다. 패키지는 **이미 이미지 안에 설치돼 있다**
   (`/opt/autoware/lib/libautoware_iv_external_api_adaptor.so`) — 안 띄웠을 뿐이다.
   결과와 대응은 아래 `3단계` 절 참조.

---

## 3. 3계단 목표 그림

```
 [Perception] ──▶ [Decision] ──▶ [Control 파티션: Control + AD API]
                                      │  ▲                    ▲
                        control_cmd   │  │ /vehicle/status/*  │  ADAPI
                                      ▼  │                    │  토픽 3 + 서비스 10
                           우리 vECU (VCU)                     │  ← stub 이 앉아 있던 자리
                        ← vehicle_interface 자리               │
                                      │  ▲                    │
                      VehicleControl  │  │ EgoState           │
                                      ▼  │                    │
                           SS_v2 (플랜트) ── concealer ────────┘
                                            (SS_v2 안에 있다)
```

`autoware_simple_planning_simulator` 는 끄고 SS_v2 를 플랜트로 쓴다.
`vehicle_interface` 자리는 우리 vECU — **이미 구현돼 있다**
([vehicle-interface.yaml](../../service/sils/examples/vehicle-interface/channelmap/vehicle-interface.yaml),
`ControlTarget` in / `EgoState` in / `VehicleControl` out).

**신규 작업은 브리지 하나뿐이다** — `control_cmd` ↔ `ControlTarget` 번역기.
[`ss2_control_bridge.cpp`](../../adapters/ss2/src/ss2_control_bridge.cpp) 와 방향만 반대인 같은 구조다.

### 5단계 계획

| # | 할 것 | 답이 나오는 질문 | 상태 |
|---|---|---|---|
| 1 | 이미지 3개 + 맵 받아서 Lane Driving 을 문서 그대로, headless 로 실행 | 우리 장비에서 뜨긴 뜨는가 | ✅ 2026-08-10 |
| 2 | 파티션을 도메인 42 밖(43)으로 옮김 | 우리 SiLS 와 공존 가능한가 | ✅ 2026-08-10 |
| 3 | [`check_autoware_adapi.sh`](../../tools/sils/ss2/check_autoware_adapi.sh) 를 **진짜** Control 파티션에 겨눔 | ★ 우리 stub 이 규격에 맞았는가 | ✅ **20/20** |
| **4** | **← 여기서 시작.** stub 철거, SS_v2 `concealer` 를 진짜 AD API 에 연결 | 3계단 본체 | ⬜ |
| 5 | 브리지 추가 + vECU 를 vehicle_interface 자리에 | 진짜 Autoware 와 폐루프 | ⬜ |

**3번이 가장 값싸고 가장 많이 알려준다** — 어긋난 지점 목록이 그대로 4번의 작업 목록이 된다.
다만 **3번은 RMW 를 먼저 건너야 한다**(§2-1). 명령은 이것이고, `SILS_RMW` 를 빼면 안 된다:

```bash
SILS_RMW=rmw_cyclonedds_cpp tools/sils/ss2/check_autoware_adapi.sh --domain <파티션 도메인>
```

> ⚠ 함정: `check_autoware_adapi.sh` 의 `gather()` 는 **호스트에 `ros2` 가 있으면 그 경로로 빠지고,
> 그 분기는 `SILS_RMW` 를 무시한다.** 지금 이 장비엔 호스트 ROS 가 없어 안 걸리지만,
> 누가 ROS 를 깔면 조용히 Fast DDS 로 조회한다. 그때는 스크립트를 고쳐야 한다.

---

# 1단계 — 있는 그대로 띄우기 ✅ (2026-08-10 완료)

목표는 딱 하나였다: **우리 장비에서 뜨는가.** → **뜬다.**
변수를 최소로 두려고 문서와 똑같이 **도메인 42 · headless · 사전 빌드 이미지 · 소스 빌드 없음**으로 갔다.

> **이 절은 이제 재현 절차다.** 이 장비에는 이미 다 되어 있다 —
> 저장소 `~/oss/oss_adsw`(`main`), 맵 `~/autoware_map/sample-map-planning`, 이미지 3개(≈12.6 GB).
> 아래 명령은 **전부 다시 돌려도 안전하도록** 가드를 달아 두었다.

## ① 도메인 42 비우기 + 전제 패키지

**우리 SiLS 가 도메인 42 를 쓰고 있다.** 안 내리면 노드가 섞여서 실패했을 때 누구 탓인지 못 가린다.

```bash
docker ps -a --filter 'name=sils-' --format '{{.Names}}' | xargs -r docker rm -f
pgrep -af sils_vecu | awk '{print $1}' | xargs -r kill

sudo apt update
sudo apt install -y git git-lfs python3-pip jq unzip
git lfs install
python3 -m pip install --user gdown

echo "--- 아래 둘 다 비어야 한다"
docker ps --format '{{.Names}}' | grep sils- || echo "  ok: sils 컨테이너 없음"
pgrep -af sils_vecu || echo "  ok: vECU 없음"
df -h /var/lib/docker
```

## ② 소스와 맵

```bash
mkdir -p "$HOME/oss"
[ -d "$HOME/oss/oss_adsw/.git" ] \
  || git clone https://github.com/OSS-TEST-GROUP/katech-autoware.git "$HOME/oss/oss_adsw"
git -C "$HOME/oss/oss_adsw" pull --ff-only

MAP="$HOME/autoware_map/sample-map-planning"
if [ ! -f "$MAP/lanelet2_map.osm" ]; then
  mkdir -p "$HOME/autoware_map"
  python3 -m gdown -O "$HOME/autoware_map/sample-map-planning.zip" \
    'https://docs.google.com/uc?export=download&id=1499_nsbUbIeturZaDj7jhUownh5fvXHd'
  unzip -o "$HOME/autoware_map/sample-map-planning.zip" -d "$HOME/autoware_map"
fi

echo "--- 맵 파일 4개가 다 나와야 한다 (한 줄이라도 없으면 exit != 0)"
ls -1 "$MAP"/{lanelet2_map.osm,pointcloud_map.pcd,map_projector_info.yaml,map_config.yaml} \
  && echo "  ok: 맵 4/4" || echo "  ✗ 맵 불완전 — 위 목록에서 빠진 파일을 볼 것"
```

> `gdown` 이 HTML 이나 "Permission denied" 를 받으면 구글 드라이브 쿼터다. 브라우저로 받아 옮기면 된다.
> `git lfs install` 을 ① 에서 이미 했으므로 clone 시점에 LFS 가 따라온다 — `git lfs pull` 은 불필요.

## ③ 이미지 받기 (빌드 안 함)

```bash
REPO=ghcr.io/oss-test-group/autoware-partition
for t in adsw-perception adsw-decision adsw-control; do docker pull "$REPO:$t"; done

docker image inspect "$REPO:adsw-control" --format '{{.Os}}/{{.Architecture}}'
docker images "$REPO"
```

## ④ 실행 — 이 터미널은 로그가 계속 흐른다 (Ctrl-C 로 셋 다 종료)

```bash
cd "$HOME/oss/oss_adsw"
REPO=ghcr.io/oss-test-group/autoware-partition
MAP_PATH="$HOME/autoware_map/sample-map-planning"
./partition/run_partitions.sh --repo "$REPO" --map-path "$MAP_PATH" --domain-id 42 --headless
```

보드가 느리면 `--delay 20`.

## ⑤ 확인 — **새 터미널**에서

```bash
echo "=== 1. 컨테이너"
docker ps --filter 'name=adsw-' --format '{{.Names}}\t{{.Status}}'

echo; echo "=== 2. 에러"
grep -aE '\[ERROR\]|Traceback|died|No such file|command not found' \
  ~/oss/oss_adsw/log/{perception,decision,control}_log.txt | head -30 || echo "  (없음)"

echo; echo "=== 3. ★ AD API — 우리 stub 이 앉아 있던 자리"
docker exec adsw-control-42 bash -lc \
  'source /opt/ros/humble/setup.bash && source /opt/autoware/setup.bash && ros2 service list | grep "^/api/" | sort'

echo; echo "=== 4. 노드 수"
docker exec adsw-control-42 bash -lc \
  'source /opt/ros/humble/setup.bash && source /opt/autoware/setup.bash && ros2 node list | wc -l'
```

## 판정 기준

| 결과 | 다음 |
|---|---|
| 컨테이너 3개 up **그리고** 3번이 서비스 목록을 출력 | ✅ 2단계(도메인 분리)로 |
| 컨테이너는 뜨는데 `/api/` 가 비어 있음 | Control 파티션이 안 올라온 것 — `log/control_log.txt` 확인 |
| 컨테이너가 죽음 | 해당 로그 확인 |

## ★ 실측 결과 (2026-08-10 13:23–13:33, 도메인 42 · headless)

| 파티션 | 결과 |
|---|---|
| perception | ✅ `Succeeded to load lanelet2_map. Map is published.` · `[ERROR]` **0** |
| decision | ✅ 기동. `[ERROR]` 120 건 — **전부** `map`→`base_link` lookupTransform 실패 |
| control | ✅ 기동. `[ERROR]` **0** · `simple_planning_simulator: waiting initialization...` |

**decision 의 ERROR 120 건은 실패가 아니다.** 초기 위치(2D Pose Estimate)를 안 넣었으니
`base_link` TF 가 없고, `costmap_generator` 가 그걸 매 주기 보고하는 것뿐이다. headless 로
띄운 이상 정상 동작이다. 같은 이유로 control 의 `waiting initialization` 도 정상이다.

> ⚠ 이것이 "조용한 실패" 규칙이 적용되는 자리다. **로그가 흘렀다는 것을 증거로 받지 말 것.**
> 이 단계가 실제로 증명한 것은 딱 하나 — *세 이미지가 No-CUDA 로 기동하고 맵을 읽는다.*
> 규격이 맞는지는 3단계가, 차가 움직이는지는 5단계가 답한다.

## 이번 단계에서 안 하는 것

RViz 와 실제 Lane Driving 주행(2D Pose Estimate → Goal → Auto)은 **아직**이다.
X11·DISPLAY 가 끼면 "안 뜨는" 원인이 두 배가 된다. **헤드리스로 뜬다는 것만 먼저 확정한다.**

## 2계단으로 돌아가려면

①이 `sils-*` 컨테이너와 vECU 를 전부 내렸다. 파티션을 끄고(터미널 Ctrl-C, 또는
`docker rm -f adsw-{perception,decision,control}-42`) 2계단을 다시 세우는 길:

```bash
cd <oss_multisw_framework>
LD_LIBRARY_PATH="$PWD/output/x86_64/lib:$LD_LIBRARY_PATH" \
  tools/sils/ss2/run_closed_loop.sh --domain 42 --bin-dir "$PWD/bin"
tools/sils/ss2/check_closed_loop.sh        # 4개 컨테이너를 한 화면으로
```

> ⚠ `LD_LIBRARY_PATH` 를 빼면 vECU 가 **1초 만에 죽는다** —
> `libfastdds.so.3.2: cannot open shared object file`. `bin/` 의 바이너리는 호스트에서 직접
> 실행되고 Fast DDS 를 `output/x86_64/lib` 에서 찾는다. `run_closed_loop.sh` 는 이걸 세워주지
> 않는다. 다행히 이 실패는 조용하지 않다 — 스크립트가 "EMPTY trace … startup problem" 으로
> 정확히 짚어준다.

**파티션과 2계단을 같은 도메인에서 동시에 띄우지 말 것** — 그 충돌을 피하는 것이 다음 절이다.

---

# 2단계 — 도메인 분리 ✅ (2026-08-10)

질문: **우리 SiLS 와 공존 가능한가.** → **가능하다.** 파티션 쪽을 42 밖으로 옮기면 된다.

## 왜 분리해야 하나 — 이유는 네 가지고, 세 개는 조용히 고장난다

"편의상 갈라놨다" 가 아니다. 같은 도메인에 두면 **각각 다른 방식으로** 망가진다.

### ① 이름이 겹친다 — 그리고 DDS 는 겹침을 오류로 안 본다

파티션과 우리 스택은 **같은 이름**을 쓴다. `/vehicle/status/*`,
`/localization/kinematic_state`, `/control/command/control_cmd`, `/api/*`, `/tf`, `/clock`.
한 도메인에 두면 "두 시스템이 공존" 이 아니라 **이름마다 writer 가 둘인 그래프 하나**다.

| | 무슨 일이 일어나나 |
|---|---|
| 토픽 | 마지막에 도착한 샘플이 이긴다. 구독자는 두 값을 번갈아 본다 |
| 서비스 | 클라이언트가 **먼저 발견한** 서버에 붙는다. 비결정적이다 |

둘 다 **아무도 오류를 안 낸다.** [`aw_localization_bridge.cpp`](../../adapters/ss2/src/aw_localization_bridge.cpp)
헤더가 같은 얘기를 이미 한다 — `launch_localization=true` 면 우리 브리지가
`/localization/kinematic_state` 의 **두 번째** writer 가 되고, "발행 순서가 어느 쪽을
믿을지 정하며, 충돌은 어디에도 보고되지 않는다."

### ② RMW 가 다르다 — 어댑터가 segfault 한 적이 있다 (단, 원인은 아직 좁혀야 한다)

파티션은 **Cyclone**, 우리 SiLS 는 **Fast DDS** 다(2026-08-11 확정된 방침 — 아래
"어느 RMW 로 갈 것인가" 절). 아래 `⛔ Cyclone 전환` 절에서 SiLS 를 Cyclone 으로 옮겨봤을 때:

```
dq.builtin: type [COMPLETE 750d3e2c...]: ddsi_xt_type_add_typeobj with invalid type object
[ros2run]: Segmentation fault
```

그리고 **어댑터를 Cyclone 에 단독으로 띄우면 40초간 멀쩡했다** — `invalid type object` 도
segfault 도 0 줄. 상대가 있어야 터진다.

> ⚠ **"한 도메인에 두 RMW 를 두면 죽는다" 로 읽지 말 것.** 그렇게 쓴 적이 있는데
> **관측과 안 맞는다** — 운영 구성에서 둘은 **이미 도메인 42 에 공존하고 있고 안 죽는다**:
>
> | | 도메인 | RMW |
> |---|---|---|
> | 파티션 (adsw_manager 경로) | 42 | **Cyclone** (명시) |
> | MMF (`mmf-core`, hmi …) | 42 | **Fast DDS**. `MMF_DDS_DOMAIN_ID` 로 raw Fast DDS 참가자도 직접 띄운다 |
>
> ~~그러니 트리거는 더 좁다. 유력한 가설은 **우리 정준 타입**의 TypeObject 를 Cyclone 의
> 타입 디스커버리가 못 삼키는 것이다 — MMF 의 raw Fast DDS 타입은 같은 도메인에서 문제를
> 안 일으키므로.~~
>
> ⚠ **취소 (2026-08-13).** 이 좁힘의 근거였던 "MMF 는 문제를 안 일으킨다" 가 틀렸다.
> 미패치 MMF 는 같은 도메인의 파티션 3종에 **1260건**을 찍고(08-12), 우리 참가자가
> **하나도 없을 때도 224건**이 난다(08-13). 오염은 우리 타입 특유가 아니다.
> → [`evidence/our-typeobject-2026-08-13.md` §3](evidence/our-typeobject-2026-08-13.md)
>
> 도메인 분리는 **확인된 회피로**이지 **증명된 필요조건이 아니다.** ①③④ 는 그와 무관하게
> 유효하다.

### ③ 3계단의 주장이 **증명 불가능**해진다

3계단이 증명하려는 문장은 이거다:

> `simple_planning_simulator` 를 끄고 **우리 브리지만으로** Autoware 를 구동했다 —
> 53개 구독 중 52개 매칭, 유실 0.

같은 도메인에 다른 발행자가 있으면 이 문장은 **반증할 수 없다.** 매칭된 52개가 우리
브리지 덕인지 옆 스택 덕인지 구별할 방법이 없기 때문이다. `지켜야 할 것 두 가지` 가
요구하는 "위조할 수 없는 관측" 이 성립하지 않는다.

거꾸로, 도메인을 갈라두면 2단계 실측이 실제로 무언가를 증명한다 — 파티션 3개가 43 에서
도는 채로 42 의 폐루프가 **그대로 재현됐다**(아래 표). 공존의 증거는 "안 겹치더라" 가
아니라 이것이다.

### ④ 고장 났을 때 누구 탓인지 못 가린다

①의 실무적 귀결. `①` 절이 우리 컨테이너를 먼저 내리라고 하는 이유이기도 하다 —
도메인 42 에는 이미 MMF 스택 전체(`/mmf/lifecyclemgr`, `/mmf/otamanager`, …)와 rosbridge 가
있고, 여기에 125노드짜리 Autoware 를 얹으면 디스커버리 트래픽과 로그가 서로를 묻는다.
[`docker-compose-sils-ss2.yaml`](../../docker/compose/docker-compose-sils-ss2.yaml) 의
`record:=false` 주석이 그 사례다 — rosbag2 가 도메인 42 의 남의 토픽마다 수천 자짜리
WARN 을 뱉어 시나리오 자체 출력을 완전히 덮어버렸다.

> **요약**: ③④ 는 "측정이 무의미해진다", ①② 는 "**틀린 채로 돌아간다**". 특히 ② 는
> 프로세스가 죽고, ① 은 안 죽는다 — 둘 중 무서운 쪽은 ① 이다.

## 파티션을 띄우는 경로는 **셋**이고, 셋이 서로 다르다 (2026-08-11 조사)

"파티션을 어떻게 띄우나" 에 답이 하나가 아니다. 도메인과 RMW 가 **경로마다 다르다.**

| | 어디 | 도메인 | RMW |
|---|---|---|---|
| **A. adsw_manager** (운영) | [`saved-service-list.yaml`](../../docker/compose/saved-service-list.yaml) | **`ROS_DOMAIN_ID=42` 리터럴** | **Cyclone 명시** |
| **B. compose** | [`docker-compose-integration.yaml`](../../docker/compose/docker-compose-integration.yaml) `oss-adsw-*` | `${ROS_DOMAIN_ID:-42}` | **미지정 → Fast DDS** |
| **C. run_partitions.sh** | 외부 repo `OSS-TEST-GROUP/katech-autoware` (`~/oss/oss_adsw`) | `--domain-id`, 기본 42 | **Cyclone 명시** |

A 는 `adsw_manager` 가 그 yaml 을 읽어 컨테이너마다 `docker run` 을 직접 실행한다
(`exec_type: container`, `/var/run/docker.sock` 마운트가 그 때문). C 만 이 저장소 밖이고,
컨테이너 이름에 도메인이 붙어(`adsw-control-43`) 병행 실행이 쉽다 — 위 2단계 실측이 C 다.

### ★ B 는 Fast DDS 로 뜬다 — 이미지 기본값이 Cyclone 이 **아니다**

이미지 안에 `RMW_IMPLEMENTATION=rmw_cyclonedds_cpp` 가 있긴 한데 **`/root/.bashrc`** 에
있다. ENTRYPOINT(`/ros_entrypoint.sh`)는 `setup.bash` 둘만 source 하고
`exec /usr/sbin/gosu "$USER_NAME" "$@"` 로 넘긴다 — **`.bashrc` 를 읽지 않고, root 도
아니다.** 실측:

```
compose 경로 (LOCAL_UID 설정)      RMW_IMPLEMENTATION=<unset>  ->  actual rmw: rmw_fastrtps_cpp
saved-service-list 경로 (-e 명시)  RMW_IMPLEMENTATION=rmw_cyclonedds_cpp
```

같은 이미지, 런처에 따라 RMW 가 갈린다. 그리고 통합 compose 전체에서
`RMW_IMPLEMENTATION` 을 설정하는 서비스는 **`oss-adsw-rviz` 하나뿐**이다(417행) — 즉 B 로
띄우면 그 원격 rviz(Cyclone)만 파티션 셋(Fast DDS)과 어긋난다. **버그로 보인다.**

> `FASTDDS_BUILTIN_TRANSPORTS=UDPv4` 는 세 경로 모두에 있지만 Cyclone 일 때는 무시되는
> 변수다. "Fast DDS 를 쓴다" 의 근거로 읽지 말 것.

### rviz 가 지금까지 잘 보였던 이유 — 컨테이너 **안**에서 돈다

SILS 도입 전 구성에서 rviz 가 멀쩡했던 것은 교차 벤더 상호운용이 아니라, rviz 가
perception 컨테이너 **안**에서 뜨기 때문이다:

```bash
# 이미지 /autoware/start_script/adsw-perception.sh
args_perception+=("rviz:=true")
```

`saved-service-list.yaml` 의 `DISPLAY=:1`, `LIBGL_ALWAYS_SOFTWARE=1` 이 그것 때문이다.
같은 프로세스 환경 = 같은 RMW 이므로 RMW 가 무엇이든 보인다. 별도 `oss-adsw-rviz`
서비스를 쓸 때만 위 불일치가 드러난다.

### 도메인 주석은 낡았다 — 분리가 아니라 **합류**다

`saved-service-list.yaml` 은 "domain 42 로 격리하여 MMF(domain 0)와 분리" 라고 써 있는데,
커밋 `3c61230 feat(mmf): integration compose 도 도메인 파일/기본 42 를 따르도록 정렬` 이후
MMF 기본값도 42 다. 통합 compose 헤더는 거꾸로 "기본 42 인 이유는 파티션이 42 라서" 라고
한다. **서로를 근거로 같은 도메인을 쓰고 있다.**

## ★ 도메인 42 통일을 실제로 해봤다 — 재현됐다 (2026-08-11)

방침(파티션 Cyclone / SiLS Fast DDS)을 정한 뒤, **도메인까지 42로 통일**해보고 부딪혀보기로
했다. 결과: **죽는다.** 그리고 **무엇이 트리거인지가 잡혔다.**

### 관측

파티션 3종을 도메인 42(Cyclone, `run_partitions.sh --domain-id 42`)로 띄우고,
제어 파티션을 시뮬레이터 끈 것으로 교체한 뒤, 브리지 2개를
`ROS_DOMAIN_ID=42`(Cyclone) + `VISS_DDS_DOMAIN_ID=42`(Fast DDS)로 올렸다.

| 시각 | 사건 | 결과 |
|---|---|---|
| A. `sils_ego_source` **먼저**, 브리지 나중 | Cyclone 노드가 기존 writer 를 발견 | **산다.** 1484 샘플, 40초+ 정상 |
| B. 브리지 **먼저**, `sils_ego_source` 나중 | 새 Fast DDS writer 가 기존 Cyclone 에 합류 | **7초 뒤 segfault** |

```
[42] dq.builtin: type [COMPLETE b9f5646283cf...]: ddsi_xt_type_add_typeobj with invalid type object
[42] dq.builtin: type [INVALID 000000...]: ddsi_xt_type_init_impl with invalid type object
[ros2run]: Segmentation fault          (컨테이너 Exited 245)
```

**순서가 갈랐다.** 이것이 "어떤 날은 되고 어떤 날은 안 되던" 이유이고, 08-10 에 원인을
못 좁혔던 이유이기도 하다. A 를 보고 "42 에서 된다" 고 결론내면 틀린다.

### ★★ 그리고 죽은 것은 우리 브리지만이 아니다 — **Autoware 파티션이 같이 죽는다**

이게 이 조사에서 제일 중요한 줄이다. 파티션 3종의 로그를 보면 **같은 서명**이 찍혀 있고,
**판단 파티션은 segfault 로 내려갔다**:

```
$ grep -c "invalid type object" ~/oss/oss_adsw/log/*_log.txt
   perception  49     decision  141     control  49

decision_log.txt:
  /autoware/start_script/adsw-decision.sh: line 12: 1276826 Segmentation fault
      ros2 launch obigo_launch sample_adsw_decision_run.launch.xml ...
```

시각이 초 단위로 일치한다:

| | 시각 |
|---|---|
| 브리지 + `sils_ego_source` 기동 | `1786424043` |
| perception 첫 `invalid type object` | `1786424043.325822` |
| decision | `1786424043.325839` |
| control | `1786424043.325769` |

**우리 정준 타입 writer 가 도메인 42 에 나타난 그 순간, 파티션 셋이 전부 오염됐다.**

> ## ⚠⚠ 정정 (2026-08-13) — 위 문단의 "죽인다" 는 **취소한다**
>
> 대조군을 세워 재검증했다 → [`evidence/our-typeobject-2026-08-13.md`](evidence/our-typeobject-2026-08-13.md).
> 위 08-11 관측은 세 명제를 한 문장에 붙여 놓았는데, **하나만 참이다.**
>
> | 명제 | 판정 |
> |---|---|
> | 우리 타입이 Cyclone 로그를 오염시킨다 | ✅ 참 (미패치 +600 ↔ 패치 +0, 스위치 하나 차이) |
> | 그 오염이 **우리 타입 특유**다 | ❌ **거짓** — 우리 참가자가 0개일 때도 +224 (해시 4종) |
> | 우리 타입이 Autoware 스택을 **죽인다** | ⚠ **미지지** — 500초 동안 재현 안 됨 |
>
> 죽을 후보를 둘 다 세웠고 둘 다 살았다: 자리 잡은 파티션 3종(컨테이너 ID 3/3 동일)과,
> **오염된 도메인에 새로 합류한** rclcpp-Cyclone + Fast DDS 겸용 리더
> (`aw_vehicle_status_bridge` — 08-10 에 실제로 segfault 했던 바로 그 프로세스).
>
> 08-10/08-11 의 segfault 로그가 가짜였다는 뜻이 **아니다.** 그 줄은 실제로 찍혔다.
> 지지되지 않는 것은 **원인이 TypeObject 라는 연결**이다. 08-11 런은 중복
> `run_partitions.sh` 러너 8개가 떠 있던 그 rig 였다(그 사고: [`evidence/mmf-typeobject-2026-08-12.md` §4](evidence/mmf-typeobject-2026-08-12.md)).

그러니 이건 "우리 어댑터가 약하다" 도 아니고, **"우리 `EgoState` writer 가 Autoware 스택을
죽인다" 도 아니다.** 확인된 것은 **우리 writer 가 남의 로그를 초당 수백 줄 더럽힌다** 는 것이고,
그것만으로도 조치할 값어치는 있다 — 실제로 그 소음 때문에 이틀을 진단에 잃었다.
다만 등급은 **안전 문제가 아니라 위생 문제**다.

### 트리거 — 좁혔다가 다시 넓혔다

깨는 것은 "한 도메인에 두 RMW" 가 **아니다** — A 가 그걸 반증한다. 08-11 에는 여기서 한 걸음 더
나가 **우리 정준 타입 특정**이라고 좁혔는데, 그 좁힘의 근거였던 "MMF 의 raw Fast DDS 타입은
문제를 안 일으킨다" 가 이후 **두 번 반증됐다**:

| 반증 | 언제 | 내용 |
|---|---|---|
| MMF 도 낸다 | 08-12 | 미패치 MMF 로 파티션 3종에 **1260건** |
| 아무도 없어도 난다 | 08-13 | **우리 참가자 0개**에서 **224건**, 해시 4종 (보고 주체는 Autoware 자기 노드들) |

그래서 현재 가장 정확한 진술은 이렇다:

> `ddsi_xt_type_add_typeobj with invalid type object` 는 이 **Cyclone 0.10.5 + Autoware 조합의
> 풍토병**이고, 우리 정준 타입은 그 위에 얹히는 **여러 유발원 중 하나**다.

우리 몫(신규 지문 3종)은 `type_propagation=disabled` 로 정확히 사라진다. 나머지 4종은 남는다 —
그쪽 뿌리는 미해결이고, `fastddsgen` 인코딩 가설만으로는 설명되지 않는다.

### 회피로 시험 — Cyclone 타입 디스커버리 끄기: **불가**

```
CYCLONEDDS_URI=<CycloneDDS><Domain><TypeDiscovery>...     -> config: unknown element (거부)
CYCLONEDDS_URI=...<Discovery><EnableTopicDiscoveryEndpoints>false  -> 그대로 segfault
```

`ros-humble-cyclonedds 0.10.5` 에서 관련 런타임 노브는 `EnableTopicDiscoveryEndpoints`
하나뿐이고, 그건 DCPSTopic 빌트인 엔드포인트를 끌 뿐 SEDP 로 실려오는 `TypeInformation`
을 막지 못한다. XTypes 타입 디스커버리 자체는 컴파일 타임 옵션이다.

### ✅ 해소 — `fastdds.type_propagation = disabled` (2026-08-11, 같은 날)

선행 조사가 이미 있었다. `feature/para-dds-interop` 브랜치의
[`docs/interop/popcorn-para-dds-interop.md`](../interop/popcorn-para-dds-interop.md) §4.7 이
같은 서브시스템을 **다른 증상**으로 파고들어 놓았다(매칭은 되는데 수신 0, 로그 없음).
거기서 검증된 우회로를 우리 writer 에 적용했다:

```cpp
// service/sils/vecu/src/sils_ego_source.cc
pqos.properties().properties().emplace_back("fastdds.type_propagation", "disabled");
```

**실측 — 크래시를 냈던 바로 그 순서(Cyclone 먼저, Fast DDS writer 나중)로 재시험:**

| | 이전 | 이후 |
|---|---|---|
| `invalid type object` (perception/decision/control) | 49 / 141 / 49 | **0 / 0 / 0** |
| 판단 파티션 | **segfault** | Up |
| 브리지 2개 | Exited 245 | Up |
| `aw_vehicle_status_bridge` | — | received=2663 published=2638 |
| Autoware `/vehicle/status/velocity_status` | — | 49.1 Hz ⚠ 아래 정정 |
| Autoware `/localization/kinematic_state` | — | 49.3 Hz ⚠ 아래 정정 |

**도메인 42 통일이 성립한다.** 분리 없이 파티션(Cyclone)과 SiLS(Fast DDS)가 한 도메인에서
돌고, 데이터가 Autoware 까지 들어간다.

#### 무엇을 잃었나

TypeObject 를 안 보내므로 topic/type **이름**으로만 매칭하고 각자 로컬 타입으로
역직렬화한다. 원격 타입 자동 발견과 런타임 타입 호환성 검사가 사라진다.

외부 벤더 상대라면 큰 대가지만 여기서는 **모든 참가자가 우리 것이고 `idl/dds/sils/` 한
곳에서 나온다.** 다만 vECU 는 Fast DDS 3.2, `adapters/ss2` 는 2.6 으로 **따로 빌드**되므로,
둘 사이에 IDL 이 어긋나면 이제 매칭 실패 대신 **조용한 오역직렬화**가 된다. IDL 을 단일
출처로 유지하고 양쪽을 같이 재빌드할 것.

#### ✅ 전 참가자로 확대 (같은 날, 2번 작업)

15개 참가자 생성 지점 전부에 적용했다. 같은 두 줄을 15군데 흩뿌리면 16번째를 반드시
빠뜨리고, 이 규칙은 빠뜨리면 **남의 프로세스가 죽는다** — 그래서 트리마다 헬퍼 하나로 뒀다.

| 트리 | Fast DDS | 헬퍼 | 지점 |
|---|---|---|---|
| `service/sils/vecu` | 3.2 | [`sils_participant_qos.hpp`](../../service/sils/vecu/src/sils_participant_qos.hpp) `ApplySilsParticipantQos()` | 9 |
| `adapters/ss2` | 2.6 | [`aw_type_propagation.hpp`](../../adapters/ss2/include/aw_type_propagation.hpp) `DisableTypePropagation()` | 6 |

2.6 에는 property 가 없고 **조용히 무시된다** — 그래서 이름 붙인 헬퍼를 쓴다.
`auto_fill_type_object` 와 `auto_fill_type_information` **둘 다** 꺼야 한다. 전자는
TypeObject 를, 후자는 SEDP 에 실리는 TypeIdentifier 를 막는데, 크래시가 도착한 경로가
SEDP 다. 그리고 `register_type` **전에** 불러야 한다 — 후에 부르면 아무 일도 안 일어나고
정상으로 보인다.

**실측 (도메인 42, 파티션 3종 Cyclone 이 먼저 떠 있는 상태):**

| | 결과 |
|---|---|
| 양쪽 전파 차단 후 매칭·수신 | received +750 / **published +750**, 앵커 편차 0.4 ms |
| Autoware 두 토픽 | 발행자 각 1개, 우리 브리지 |
| **vECU 합류** (`run_vecu_wire.sh --domain 42`) | 파티션 3종 생존, `invalid type object` **0** |
| vECU 트레이스 | 4690줄, `state_received` 2341건 |

마지막 줄이 2번의 원래 동기였다 — vECU 는 새 Fast DDS writer 라 파티션을 다시 죽일 수
있었고, 이제 안 죽는다.

#### ⚠ 소스만 고쳐서는 끝나지 않는다 — 이미지도 다시 구워야 한다

vECU 는 호스트 바이너리로도 돌고(`run_vecu_wire.sh`) **컨테이너로도 돈다.**
`docker/Dockerfile.rel` 의 `runtime-mmf-sils-vecu-x86_64` 스테이지가
`output/.../core/bin/sils_vecu_viss_{probe,verify}` 를 이미지에 굽는다.

소스 수정 직후 시점에 `oss-mmf-sils-vecu:local` 은 **3주 전** 이미지였다 — 그걸 띄웠으면
고친 게 하나도 없는 바이너리가 도메인에 들어가 파티션을 그대로 죽였을 것이고, 소스에는
분명히 수정이 들어 있으니 원인을 찾는 데 한참 걸렸을 것이다.

```bash
BUILD_TYPE=Debug ./docker/build_local.sh --sils-vecu-only
BUILD_TYPE=Debug ./docker/build_local.sh --brake-ecu-only   # 같은 트리, 같은 함정
```

확인은 산출물에서 직접 한다 (`|| true` 빌드가 성공을 가장하는 이 저장소에서는 필수):

```bash
cid=$(docker create oss-mmf-sils-vecu:local)
docker cp "$cid:/oss/bin/sils_vecu_viss_probe" /tmp/x && docker rm "$cid"
strings /tmp/x | grep -c fastdds.type_propagation      # 0 이면 옛 바이너리다
```

측정 시점 결과: `sils_vecu_viss_probe` 5건, `sils_vecu_viss_verify` 3건, `brake_ecu` 5건.
- **근본 원인 미규명.** Cyclone 이 우리 TypeObject 를 왜 "invalid" 로 보는지는 여전히
  모른다. 전파를 껐을 뿐이다. 랩의 본해법(`ForceFinal`)은 **런타임 파싱된 DynamicType**
  용이라 우리 정적 생성 타입에는 그대로 안 맞는다. 우리 쪽 대응물은 IDL 에 `@final` 을
  넣는 것이고 **미시험**이다 — 랩 표에서 `final`/`default` 가 같았던 것은 동적 파서가
  어노테이션을 무시해서이지, `fastddsgen` 도 그렇다는 뜻이 아니다.
  (참고: 우리 `EgoState.idl` 에는 어노테이션이 하나도 없다 → 기본 APPENDABLE.)

### 정정 — 첫 측정의 49 Hz 는 귀속이 안 된 값이었다

처음 이 표를 채울 때 **벤더 `simple_planning_simulator` 가 켜진 채였다.** 크래시 후
파티션을 다시 띄우면서 `run_partitions.sh` 만 돌리고
`run_control_partition_sils.sh` 를 다시 안 돌린 탓이다. 그래서 두 토픽 모두 발행자가
**2개**였고("우리 브리지 + 시뮬레이터"), 49 Hz 가 누구 것인지 구분되지 않았다.

`run_vehicle_seat_bridges.sh` 는 정확히 이 경우를 **거부하도록** 만들어져 있는데,
브리지를 `docker run` 으로 직접 띄우면서 그 가드를 우회했다. **가드를 우회하면 가드가
막던 것을 그대로 밟는다.**

시뮬레이터를 끄고 재측정한 값이 아래다 — 이쪽이 유효하다:

| 토픽 | 발행자 | 발행 노드 | 실측 |
|---|---|---|---|
| `/vehicle/status/velocity_status` | **1** | `aw_vehicle_status_bridge` | 48.99 Hz |
| `/localization/kinematic_state` | **1** | `aw_localization_bridge` | 48.99 Hz |

크래시 해소(`invalid type object` 0, 파티션 생존)는 이 정정과 무관하게 유효하다 —
시뮬레이터 유무가 TypeObject 전파에 관여하지 않기 때문이다.

### 결론

**도메인 42 통일이 가능하다** — 위 해소 이후. 아래 표는 해소 전의 선택지로 남긴다:

| | 비용 | 상태 |
|---|---|---|
| 도메인 분리 (파티션 43 / SiLS 42) | 운영과 도메인이 달라짐 | **작동 확인됨** (08-11 52/53 매칭·유실 0) |
| 우리 IDL/TypeObject 수정 | fastddsgen 버전·인코딩 조사 | 미착수. **42 에 올리려면 필수** |

~~폐루프는 분리로 진행한다.~~ **전파 차단으로 42 통일이 성립했으므로 분리는 불필요하다.**
남은 것은 위 "아직 안 한 것" 두 줄이다.

첫 조사 지점: 우리 `fastddsgen` 은 `dev-ss2-adapter.sh` 의 `FASTDDSGEN_REF=v2.5.3`
(Fast DDS 2.x 계열)이다. 같은 도메인의 MMF raw Fast DDS 타입은 파티션을 안 죽이므로,
"Fast DDS 면 다 죽는다" 가 아니라 **우리 생성물** 쪽을 먼저 본다.

---

## 어느 RMW 로 갈 것인가 — 결정 (2026-08-11)

> **파티션은 현상태 그대로 Cyclone, MMF/SiLS 는 Fast DDS.**

즉 A 경로가 정본이고, SiLS 를 Cyclone 으로 옮기려던 `⛔ Cyclone 전환` 시도는 **폐기**한다
(그 절은 왜 안 되는지의 기록으로 남긴다). 대신 경계가 **우리 브리지 프로세스 안**으로
들어온다 — rclcpp 쪽은 Cyclone 으로 Autoware 를 보고, 네이티브 쪽은 Fast DDS 로 vECU 를
본다. 이건 이미 어댑터가 지원하는 모양이다(`ROS_DOMAIN_ID` 와 `VISS_DDS_DOMAIN_ID` 를
따로 받는다).

**그 프로세스가 도메인 42 에서 살아남는가는 위 절에서 답이 났다 — 아니다.** 네이티브
Fast DDS 쪽을 다른 도메인으로 빼야 하고, 코드 수정은 필요 없다(`VISS_DDS_DOMAIN_ID`).

```bash
cd "$HOME/oss/oss_adsw"
./partition/run_partitions.sh \
  --repo ghcr.io/oss-test-group/autoware-partition \
  --map-path "$HOME/autoware_map/sample-map-planning" \
  --domain-id 43 --headless --delay 15
```

> ⚠ 이 스크립트는 `trap cleanup INT TERM EXIT` 이 걸려 있고 마지막이 `tail -F` 다.
> **프로세스가 죽으면 컨테이너 3개도 같이 내려간다**(`--rm` 이므로 삭제까지). 백그라운드로
> 돌리든 전용 터미널을 주든, **살아 있게 둬야 한다.**

## 실측 결과

| 확인 | 결과 |
|---|---|
| 컨테이너 이름 | `adsw-{perception,decision,control}-**43**` — `sils-*` 와 충돌 없음 |
| 도메인 42 에서 본 `/api/` 서비스 | **0 개** ← 격리됐다 |
| 도메인 43 에서 본 `/api/` 서비스 | **47 개** |
| 로그 | `~/oss/oss_adsw/log/*_log.txt` — **도메인과 무관하게 같은 파일에 덮어쓴다** |

마지막 줄이 함정이다. `LOG_DIR` 은 저장소 루트 고정이라 **도메인 42 실행 기록이 43 실행에
지워진다.** 비교가 필요하면 실행 전에 복사해 둘 것.

### ★ 진짜 판정 — 그래프가 안 겹치는 것으로는 부족하다

"도메인 42 에 `/api/` 가 0 개"는 **DDS 가 안 섞인다**는 뜻일 뿐이다. `지켜야 할 것 두 가지` 의 규칙대로,
위조할 수 없는 관측 하나를 고집해야 한다 → **파티션 3개를 43 에서 계속 돌린 채 2계단
폐루프를 42 에서 통째로 다시 돌렸다.**

```
  samples 401   sim-time span 20.00s
  vx first +10.000 · vx last +10.000 · vx max +10.000 m/s
  VERDICT: MOVING -- the ego reached 10.00 m/s.
  ss2-adapter  received=698 published=697 skipped_no_ego=1 write_failed=0
  ss2-control  forwarded=858 published=858 stale_holds=0 bad_gear=0 matched=1
```

**Autoware 파티션이 옆에서 도는 상태에서 2계단이 그대로 재현된다.** 공존 증명 끝.
(속도가 *달렸다*가 아니라 *유지됐다*는 것이 핵심 — 되먹임 양방향이 다 살아 있다는 뜻이다.)

---

# 3단계 — 우리 stub 이 규격에 맞았는가 → 답: **YES (20/20)**

```bash
SILS_RMW=rmw_cyclonedds_cpp tools/sils/ss2/check_autoware_adapi.sh --domain 43
```

## ★ 결과 — 두 번 돌렸고, 그 사이에 답이 바뀌었다

| | 파티션 기본 기동 | `deprecated_api` 추가 후 |
|---|---|---|
| present | 17 / 20 | **20 / 20** |
| core 실패 | 0 | 0 |
| **drive 누락** | **2** | **0** |
| feature 누락 | 1 | 0 |
| state source | `adapi` 3/3 (+ `legacy` 도 존재) | 동일 |

기본 기동에서 빠졌던 3개는 **전부 tier4 external API 계열**이었다:

| 누락 | 심각도 | concealer 가 실제로 쓰는가 |
|---|---|---|
| `/api/external/set/engage` | drive | ✅ **쓴다.** `field_operator_application.cpp:157` 에서 클라이언트 생성, `:304` 의 `waiting_for_engage` 상태에서 **무조건 호출** → 없으면 ego 가 영원히 engage 안 된다 |
| `/api/autoware/set/velocity_limit` | drive | ✅ **쓴다.** `:163` 생성, `:604 setVelocityLimit()` ← `EgoEntity::setVelocityLimit` ← 시나리오 액션. 08-10 이전 실측에서 **180 초 침묵 후 throw** 로 이미 한 번 당했다 |
| `/api/external/get/emergency` | feature | 토픽. 대체로 `/api/autoware/set/emergency` 는 있다 |

## 원인과 대응 — **패치가 아니라 런치 인자 하나다**

`autoware_api.launch.xml` 의 `launch_deprecated_api` 기본값이 `false` 인데,
그 `deprecated_api.launch.xml` 이 `autoware_iv_external_api_adaptor` 를 띄운다.
**패키지는 이미 이미지 안에 있다** — 빌드도, 상류 patch 도 필요 없다.

검증 방법(실제로 이렇게 확인했다):

```bash
docker exec -d adsw-control-43 bash -lc \
  'source /opt/ros/humble/setup.bash && source /opt/autoware/setup.bash &&
   exec ros2 launch tier4_autoware_api_launch deprecated_api.launch.xml \
     > /workspace/log/deprecated_api.txt 2>&1'
```

→ 20/20. **이것이 4단계의 작업 목록 전부다.**

정식 반영은 `obigo_launch` 의 control 런치에서 `launch_deprecated_api:=true` 를 넘기는 것이
맞지만, 그건 **상류 저장소(`katech-autoware`)** 다. 우리 쪽만으로 끝내려면 위처럼
**사이드카로 붙이는 것**이 답이고, `지켜야 할 것 두 가지` 의 "공유 파일은 건드리지 않는다" 와도 맞는다.

## 이 결과가 실제로 증명한 것 / 아닌 것

- ✅ **stub 의 서비스 10개가 전부 진짜 Autoware 에 존재한다** — 이름도 타입도.
  [`stub_node.py`](../../adapters/autoware-stub/autoware_stub/stub_node.py) 의 규격이 맞았다.
- ✅ `/control/command/control_cmd` 타입 일치 → 5단계에서 우리 vECU 가 받을 그 토픽이다.
- ✅ 상태 경로가 **둘 다** 살아 있다 — `adapi` 3/3 이고 `/autoware/state` 도 있다.
- ❌ **Cyclone ↔ Fast DDS 상호운용을 증명한 것이 아니다.** 우리 조회 쪽을
  `SILS_RMW=rmw_cyclonedds_cpp` 로 **맞춰서** 본 것이다. 즉 Cyclone↔Cyclone 이다.
  [B1-3](sils-simulator-loops.md) 의 진짜 질문(벤더 교차 서비스)은 **아직 미측정**이고,
  4단계에서 SS_v2 를 어느 RMW 로 띄울지가 그 답을 결정한다. **여기서 안심하지 말 것.**

---

# 4단계 — stub 철거 (다음 작업)

3단계가 남긴 것은 "어긋난 지점 목록"이 아니라 **런치 인자 하나**였다. 그래서 4단계는
예상보다 짧다. 순서대로:

| # | 할 것 | 근거 |
|---|---|---|
| 4-a | 파티션을 `deprecated_api` 까지 포함해 띄우는 **재현 가능한 형태**로 만든다 | 3단계 §원인. 지금은 `docker exec -d` 수동이라 다음 사람이 못 재현한다 |
| 4-b | `run_ego_scenario.sh` / `run_closed_loop.sh` 에서 **stub 을 빼고** 파티션 도메인을 겨눈다 | stub 은 [1/6] 자리에 하드코딩돼 있다 |
| 4-c | SS_v2 를 **어느 RMW 로 띄울지 결정**하고 실측 | ★ 아래 |
| 4-d | `concealer` 가 `initializing → … → driving` 을 통과하는지 확인 | 2계단 판정과 같은 관측 |

## ★ 4-c 는 이제 선택이 아니다 — 측정됐다 (2026-08-10)

3단계는 우리 조회 도구를 Cyclone 으로 **맞춰서** 봤다. 4단계는 다르다 — **SS_v2 본체**가
파티션과 대화해야 하고, `concealer` ↔ AD API 는 **전부 서비스**다. 그래서 재봤다:
`sils-ss2:humble` 을 도메인 43(파티션, Cyclone)에 붙이고 RMW 만 바꿔가며.

| 우리 쪽 RMW | discovery | 토픽 데이터 수신 | **서비스 왕복** |
|---|---|---|---|
| `rmw_fastrtps_cpp` (SiLS 기본) | ✅ 토픽 514 / 서비스 1010 | ✅ | ❌ **실패** |
| `rmw_cyclonedds_cpp` | ✅ 동일 | ✅ | ✅ `InterfaceVersion_Response(major=1, minor=6, patch=0)` |

Fast DDS 쪽 실패는 조용하지 않다. 구체적인 오류가 나온다:

```
[RTPS_READER_HISTORY Error] Change payload size of '28' bytes is larger than
the history payload size of '15' bytes and cannot be resized.
```

**그래서 결론은 이렇다:**

- **discovery 와 pub/sub 은 벤더를 넘는다.** Fast DDS 로도 파티션 토픽이 다 보이고
  실제 메시지도 도착한다. 이건 좋은 소식이다.
- **서비스는 못 넘는다.** 그리고 `concealer` ↔ AD API 는 **전부 서비스**다.
  → **SiLS 기본 설정(Fast DDS)으로는 4단계가 성립하지 않는다.**
- 대응 후보: **SS_v2 를 `SILS_RMW=rmw_cyclonedds_cpp` 로 띄운다.**
  → **해봤다. 지금 그대로는 안 된다.** 아래 참조.

## ⛔ Cyclone 전환을 실제로 해봤다 — 2계단이 깨진다 (2026-08-10)

위 권고대로 2계단을 Cyclone 으로 재현해봤고, **실패했다.** 두 개의 서로 다른 문제가 나왔다.

### (1) 어댑터 이미지에 Cyclone 이 없었다 — 고침

```
[rcl] Error getting RMW implementation identifier / RMW implementation not installed
(expected identifier of 'rmw_cyclonedds_cpp') ... failed to load shared library
'librmw_cyclonedds_cpp.so'
```

`sils-ss2:humble` 에는 있는데 **`sils-ss2-adapter:humble` 에는 없었다.**
[`Dockerfile.ss2-adapter`](../../docker/sils/Dockerfile.ss2-adapter) 의 adapter 스테이지에
`ros-humble-rmw-cyclonedds-cpp` 설치를 추가하고 재빌드해 해결했다
(toolchain 스테이지가 아니라 adapter 스테이지에 넣었다 — 앞쪽은 fastddsgen gradle 빌드라
건드리면 매번 몇 분이 날아간다).

### (2) ★ 남은 진짜 블로커 — Cyclone XTypes 에서 어댑터가 죽는다

이미지를 고친 뒤 다시 돌리니 **어댑터가 segfault** 했다:

```
dq.builtin: type [COMPLETE 750d3e2cba7c...]: ddsi_xt_type_add_typeobj with invalid type object
   ... (수백 줄) ...
dq.builtin: type [INVALID 0000000000000000000000000000]: ddsi_xt_type_init_impl with invalid type object
[ros2run]: Segmentation fault
```

같은 `invalid type object` 로그가 **stub 로그에도** 찍혔다 (stub 은 순수 rclpy 다).
결과는 `VERDICT: STATIONARY` · `vx 0.000 m/s` · concealer 는 `initializing` 에서 멈춤 ·
어댑터/컨트롤 카운터 **공백**.

**배제한 것** — 어댑터를 단독으로 Cyclone 에 띄워봤다. `ROS_DOMAIN_ID`/`VISS_DDS_DOMAIN_ID`
를 같게(44/44) 두든 갈라(44/45) 두든 **40초간 멀쩡했다. segfault 도 `invalid type object`
로그도 0 줄.** 즉 원인은 "한 프로세스에 rclcpp(Cyclone)+네이티브 Fast DDS 를 함께 띄우는
[BR-C-2 구조](../../adapters/ss2/README.md) 자체"가 **아니다.** 상대가 있어야 터진다.

**다음 진단 순서** (아직 안 함):

1. 도메인 42 에서 Fast DDS 참가자를 하나씩 빼며 이분 탐색 — vECU · 어댑터 네이티브 writer ·
   컨트롤 브리지. `invalid type object` 를 처음 유발하는 참가자가 범인이다.
2. Cyclone 의 타입 디스커버리를 끈다 — `CYCLONEDDS_URI` 로 XTypes/type-discovery 비활성화.
   되면 원인 확정이고 우회로도 생긴다.
3. 안 되면 4단계 배선을 바꾼다: ROS 쪽만 파티션 도메인(Cyclone)에 두고,
   **vECU 쪽 네이티브 Fast DDS 는 다른 도메인**에 둔다. 어댑터가 이미
   `ROS_DOMAIN_ID` 와 `VISS_DDS_DOMAIN_ID` 를 **따로** 받으므로 코드 수정 없이 가능하다.

> 이것이 [B1-3](sils-simulator-loops.md) 백로그의 **XCDR/XTypes 칸**이다. 미측정으로 남아
> 있던 그 칸이 이제 "미측정"이 아니라 **"실패, 원인 미상"** 이다.

**4단계는 이 문제부터 풀어야 한다.** 순서가 바뀌었다 — 4-c 가 4-a 앞으로 왔다.

## ★★ 그런데 이 블로커는 4단계만 막는다 — 5단계는 안 막는다

두 평면을 나눠 봐야 한다. 벤더 교차에서 **토픽은 양방향으로 흐르고 서비스만 죽는다**
(`autoware_control_msgs/msg/Control` 로 Fast DDS → Cyclone 도 확인함).

| 평면 | 무엇이 쓰나 | 벤더 교차 | 막히나 |
|---|---|---|---|
| **데이터 (토픽)** | **vECU ↔ Autoware** — `control_cmd` in, `/vehicle/status/*` out | ✅ **된다** | ❌ |
| 서비스 | `concealer` ↔ AD API — initialize / route / engage / velocity_limit | ❌ 안 된다 | ✅ 여기만 |

즉 **서비스가 필요한 것은 `concealer` 뿐이다.** 우리 vECU 가 진짜 Autoware 와 폐루프를
도는 데에는 **서비스가 한 개도 필요 없다.**

**따라서 5단계를 4단계보다 먼저 할 수 있다.** SS_v2 를 루프에서 빼고:

```
[Autoware 파티션 (Cyclone, 도메인 43)]
        │ control_cmd (토픽)      ▲ /vehicle/status/* (토픽)
        ▼                         │
   브리지 + 우리 vECU (Fast DDS)   ← 벤더 교차, 위에서 실증됨
```

이러면 XTypes segfault(우리 스택을 Cyclone 으로 바꿀 때만 나는 문제)도 안 건드린다.
남는 문제는 **누가 Autoware 를 `driving` 상태로 만드나** 인데, concealer 없이도
파티션 컨테이너 안에서 Cyclone 네이티브로 서비스를 부르면 된다:

```bash
docker exec adsw-control-43 bash -lc 'source /opt/ros/humble/setup.bash &&
  source /opt/autoware/setup.bash && ros2 service call /api/localization/initialize ...'
```

**권장 순서 재조정**: 4단계(concealer 서비스 평면)는 어려운 쪽이고, 5단계(vECU 데이터 평면)는
이미 배선이 증명된 쪽이다. **쉬운 쪽을 먼저 해서 "진짜 Autoware 와 폐루프"를 확보하고**,
concealer 통합은 그 뒤에 붙이는 편이 낫다. 시나리오 기반 판정(`.xosc`)을 포기하는 대신
폐루프가 먼저 선다.

### ✅ 실증 완료 (2026-08-10) — 진짜 control_cmd 가 우리 쪽에 온다

말로 그친 게 아니라 실제로 태웠다. **concealer 없이, RMW 통일 없이** 진짜 Autoware 를
주행 상태로 올리고 Fast DDS 쪽에서 받았다:

```
기본 상태            control_cmd 발행 0 (STOP · route UNSET)
  ↓ /initialpose3d   localization INITIALIZED · kinematic_state 발행
  ↓ .../goal         routing SET · trajectory 10.0 Hz · autonomous 가능
  ↓ change_to_autonomous (Cyclone 네이티브 서비스 호출)
                     operation_mode AUTONOMOUS · control_cmd 33.2 Hz
  ↓
우리 Fast DDS 쪽 수신  33.36 Hz  ★ 벤더 교차, 실데이터
```

⚠ 함정 하나 — 초기위치는 **`/initialpose` 가 아니라 `/initialpose3d`** 다.
`/initialpose` 는 pose_initializer(NDT/GNSS) 경로라 플래닝 시뮬레이터에서는 끝나지 않고
`localization state` 가 UNINITIALIZED 에 머문다. 여기서 한 번 헤맸다.

좌표·명령·원측정값 전부: [evidence 문서](evidence/adapi-real-partition-2026-08-10.md).

> ★ **어떤 브리지가 실제 구성(MORAI / Web.Auto)에 남고 어떤 것이 지금뿐인 비계인지**는
> [sils-integration-topology.md §3](sils-integration-topology.md) 의 인벤토리를 볼 것.
> 요약: `aw_control_target_bridge` 는 **두 구성 모두에 필요**, `aw_ego_state_bridge` 는
> **비계(실제 구성에 넣으면 `sils/ego/state` 퍼블리셔가 둘이 된다)**,
> 그리고 **`/vehicle/status/*` 역브리지가 두 구성 공통의 최우선 공백**이다.

### ✅✅ 브리지도 만들었고, vECU 가 진짜 Autoware 의 목표를 받았다 (2026-08-10)

신규: [`aw_control_target_bridge.cpp`](../../adapters/ss2/src/aw_control_target_bridge.cpp) ·
[`aw_target_mapping.hpp`](../../adapters/ss2/include/aw_target_mapping.hpp)

```
[Autoware Control 파티션] Cyclone·도메인43·AUTONOMOUS
   control_cmd 33Hz + gear_cmd 10Hz
        ▼  aw_control_target_bridge  (Fast DDS 양쪽 — 벤더 교차, 서비스 0개)
   autoware/control_target · canonical · 도메인 42
        ▼
   vECU (bin/vehicle_interface)   target_received=396  control_published=398
```

ego 가 실제로 달리는 동안(4.02 → 2.65 m/s) 잡은 값:
**acceleration 고유값 178 개**(−3.40 ‥ −0.26) · **steering 고유값 219 개** ·
**gear 3(DRIVE) → 0(PARK)**. 상수가 아니라 내용이 살아 움직인다.

QoS 는 추측하지 않고 측정해서 맞췄다 — `control_cmd` 퍼블리셔(`vehicle_cmd_gate`)는
**RELIABLE · KEEP_LAST(1) · TRANSIENT_LOCAL** 이고 브리지 구독자를 거기에 맞췄다.
vECU 쪽 writer 는 vECU 리더 규약(RELIABLE·KEEP_LAST(256)·TRANSIENT_LOCAL)을 따른다.

⚠ 함정 — 값이 `accel=-1.5, gear=0` 한 종류로만 나오면 배선이 아니라 **Autoware 가 이미
목표에 도착한 것**(`routing state: 3 = ARRIVED`)이다. `/initialpose3d` 재발행으로는 ego 가
안 움직이고 **`/api/simulator/set/pose` 서비스**라야 시작점으로 리셋된다.

**⚠ 아직 열린 루프다.** vECU 출력이 어떤 플랜트에도 닿지 않는다 — 파티션의
`autoware_simple_planning_simulator` 가 여전히 `control_cmd` 를 직접 먹고 차를 움직이고,
vECU 는 그 옆의 **병렬 관측자**다. 닫으려면 플랜트 결정이 필요하다.

원측정값 전부: [evidence 문서](evidence/adapi-real-partition-2026-08-10.md).

> 범위 주의: 위 실패는 `/api/interface/version` 한 건에서 관측한 것이고, 원인은 Fast DDS
> 리더의 history payload 크기 불일치다. **모든 서비스가 반드시 실패한다**까지 증명한 것은
> 아니다. 하지만 실패한 쪽이 concealer 가 의존하는 바로 그 상호작용 종류이므로,
> 기본 구성으로 밀어붙일 이유가 없다.

이것으로 [B1-3](sils-simulator-loops.md) 의 Cyclone 칸 중 **서비스 상호운용 부분은 답이 나왔다.**

## 지금 살아 있는 것 (이어받을 때 확인)

```bash
docker ps --filter 'name=adsw-' --format '{{.Names}}\t{{.Status}}'   # adsw-*-43 3개
SILS_RMW=rmw_cyclonedds_cpp tools/sils/ss2/check_autoware_adapi.sh --domain 43
```

`deprecated_api` 는 `docker exec -d` 로 붙인 것이라 **컨테이너가 재시작되면 사라진다.**
사라졌으면 3단계 §원인의 명령을 다시 실행하면 된다 (그게 4-a 가 필요한 이유다).

---

# 지켜야 할 것 두 가지 (전 단계 공통)

### 공유 파일은 건드리지 않는다

**`docker/compose/docker-compose-integration.yaml` 은 수정 금지** — 다른 사람도 쓴다.
우리 SiLS 작업은 `docker/compose/docker-compose-sils-ss2.yaml` 오버레이에 넣는다.

### 이 스택의 실패는 전부 조용했다

지금까지 이 단계의 실패 원인 4건이 **하나같이 카운터는 멀쩡하고 로그는 비어 있었다** —
RxO QoS 불일치, 상대/절대 토픽 이름, 포트를 쥐고 있던 잔여 컨테이너,
우리가 0 으로 발행하던 필드를 읽는 차량 모델.

**"카운터가 건강해 보인다"를 증거로 받지 말 것.** 파이프라인이 위조할 수 없는 관측
하나를 고집해야 한다 — 2계단에서 그것은 차량 모델 하류인 **시뮬레이터가 보고하는 ego 의 속도**였다.

---
*관련: **[sils-integration-topology.md](sils-integration-topology.md)
(★ 배선도 — 시뮬레이터·Autoware·MMF·SiLS 가 어떻게 붙나, 브리지 인벤토리)** ·
[sils-simulator-loops.md](sils-simulator-loops.md) (백로그·사다리) ·
[sils-ss2-bringup.md](sils-ss2-bringup.md) (1·2계단 기록) ·
[sils-vehicle-interface-autoware.md](sils-vehicle-interface-autoware.md) (VCU 설계) ·
**[evidence/adapi-real-partition-2026-08-10.md](evidence/adapi-real-partition-2026-08-10.md)
(2·3단계 원측정 — 20행 계약, 원인, JSON)**
