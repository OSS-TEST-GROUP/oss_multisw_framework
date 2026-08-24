# SiLS 루프 B — `scenario_simulator_v2` 브링업 (Linux)

> **한 줄**: 루프 B 를 붙이기 위한 **1계단(배선)** 절차. 시뮬레이터를 띄우고, 우리 버스와 만날 수
> 있는지 **실측**해서 [BR-C-2a](sils-real-bridge-requirements.md) 를 채우는 것이 목적이다.
> 폐루프도, 결정론도 아직 아니다 — 그건 2계단(B-det)이다.
>
> 배경·구조는 [simulator-loops](sils-simulator-loops.md) 를 먼저 볼 것
> (그림: [simulator-loops.html](sils-simulator-loops.html)).
>
> 상태 (2026-08-06): **1차 실행 완료** — Ubuntu x86_64 에서 빌드·기동·실측까지 성공했다.
> 결과와 거기서 나온 세 가지(어댑터 입력 확정 · `/clock` BEST_EFFORT · 도메인 오염)는 **§6**.

---

## 0. 왜 1계단을 따로 두나

루프 B 의 최종 목표는 **B-det(L3)** 이지만, 거기까지 가려면 두 개의 미결이 먼저 풀려야 한다:

- **B1-2** — concealer 자리에 우리 VCU 를 어떻게 끼우나
- **W3 / B2-2** — 시뮬레이터 frame 구동부를 barrier 에 물리는 patch

**1계단은 이 둘 다 필요 없다.** 제어를 되먹이지 않는 **열린 루프**이기 때문이다. 대신 그 대가로
얻는 것이 크다 — RMW·type name·QoS·도메인·discovery 가 실제로 맞는지를 **여기서 전부 실증**한다.
이 숫자 없이 어댑터를 쓰면 조용한 미매칭으로 시간을 버린다.

| 계단 | 무엇 | 루프 | 선행 | fidelity |
|---|---|---|---|---|
| **1 · 배선** | SS_v2 → 어댑터 → `EgoState` → vECU → trace → gate | **열림** | 이 문서 | L0 |
| 2 · B-det | 제어 되먹임 + barrier | 닫힘 | + B1-2, W3 | **L3** |
| 3 · B-full | Autoware 투입 | 닫힘 | + 실물 Autoware | L0~L1 |

> 1계단은 [§2 의 rosbag 논의](sils-simulator-loops.md) 기준으로 보면 "열린 루프라 재생과 다를 게
> 없는" 상태다. **판정용으로는 그 말이 맞다.** 하지만 여기서 얻으려는 것은 판정이 아니라
> **배선 사실**이므로 별개다. 1계단 결과로 fidelity 를 주장하지 말 것.

---

## 1. ★ 소스로 확인된 것 — Autoware 실물은 필요 없다

브링업 전에 가장 컸던 미결이 이것이었다: *"21.7GB CUDA Autoware 이미지를 받아야 하나?"*

**아니다.** `tier4/scenario_simulator_v2` master 소스 확인 결과:

| 확인 | 결과 | 근거 |
|---|---|---|
| `openscenario_interpreter` 가 `concealer` 에 의존하나 | **예** (회피 불가) | `openscenario_interpreter/package.xml` |
| `concealer` 가 무엇을 요구하나 | **Autoware 메시지 패키지만** | `concealer/package.xml`: `autoware_control_msgs`, `autoware_vehicle_msgs`, `autoware_perception_msgs`, `autoware_system_msgs`, `autoware_adapi_v1_msgs`, `tier4_*_msgs` |
| 그 메시지들은 어디서 오나 | **소스 빌드** — 전부 메시지·유틸 저장소 | `dependency_humble.repos`: `autoware_msgs`, `autoware_adapi_msgs`, `autoware_internal_msgs`, `autoware_cmake`, `autoware_utils`, `autoware_lanelet2_extension`, `color_names`, `tier4_msgs`, `agnocast` |
| 베이스 이미지 | `docker.io/library/ros:humble` — **CUDA 없음** | 상류 `Dockerfile` |

**결론: 메시지 패키지만 소스로 빌드하면 되고, Autoware 런타임·CUDA·GPU 는 1·2계단에 불필요하다.**
GPU 는 3계단(B-full, 실물 Autoware)에서만 필요하다.

### ⚠ 그리고 하나 더 — 상류 기본 RMW 는 **Cyclone DDS** 다

상류 `Dockerfile` 의 runtime 스테이지에 `ENV RMW_IMPLEMENTATION=rmw_cyclonedds_cpp` 가 박혀 있다.

이것은 [BR-C-2a](sils-real-bridge-requirements.md) 가 "미문서화 갭" 으로 적어둔 Fast DDS ↔ Cyclone DDS
문제가 **가정이 아니라 기본 구성**이라는 뜻이다. 우리 정규 버스는 Fast DDS 다.

대응은 두 가지이고, **둘 다 측정해야 한다**:

| | 방식 | 장점 | 위험 |
|---|---|---|---|
| **(A)** | SS_v2 를 **Fast DDS 로 재정의** (`RMW_IMPLEMENTATION=rmw_fastrtps_cpp`) | 한 벤더 — 첫 브링업에서 변수 하나 제거 | 상류가 Cyclone 을 고른 이유가 있을 수 있다(성능·agnocast). 실측 필요 |
| **(B)** | Cyclone 유지, **어댑터가 번역** | BR-C-2 의 원래 설계 — rclcpp 로 받고 Fast DDS 네이티브로 재발행 | 한 프로세스에 두 스택. 가능한지 실측 필요 |

[`Dockerfile.ss2`](../../docker/sils/Dockerfile.ss2) 는 **(A)를 기본값**으로 두되 런타임에 뒤집을 수
있게 했다 — **같은 이미지로 두 열을 다 측정하라는 뜻**이지, (B)가 틀렸다는 판단이 아니다.

---

## 2. 준비물

| | 요구 |
|---|---|
| OS | **Ubuntu 22.04 x86_64** (또는 동등) |
| arch | **amd64 필수** — [빌드 스크립트가 arm64 를 거부한다](../../docker/sils/build-ss2.sh). ARM64 는 [target-parity](sils-target-parity.md) 의 별도 축이라 여기서 섞으면 원인 분리가 안 된다 |
| docker | BuildKit 필요 (상류 Dockerfile 이 `RUN --mount=type=cache` 를 쓴다) |
| 디스크 | ~30GB |
| GPU | **불필요** (3계단에서만) |
| 네트워크 | GitHub 접근 (상류 + 메시지 저장소 clone) |

> **macOS 에서 하지 말 것.** Docker Desktop 의 `network_mode: host` 는 macOS 호스트가 아니라
> 리눅스 VM 의 네트워크다. 우리 스택 전체가 `network_mode: host` + `ipc: host` 전제
> ([docker-compose-integration.yaml](../../docker/compose/docker-compose-integration.yaml))이라
> **실측 환경이 배포 환경과 달라지고, 그러면 B1-3 숫자 자체가 무의미해진다.**

---

## 3. 절차

### 3-1. 이미지 빌드

```bash
docker/sils/build-ss2.sh
```

두 단계로 돈다:

1. 상류를 `SS2_REF`로 clone → 상류 `Dockerfile` 의 `runtime` 스테이지를 **그대로** 빌드
2. 그 위에 우리 얇은 레이어([`Dockerfile.ss2`](../../docker/sils/Dockerfile.ss2)) — Fast DDS 프로파일 + 프로브

**상류 빌드 레시피를 우리가 복사하지 않는 이유**: `vcs import` + `rosdep` + `colcon` 조합은 상류가
계속 고친다. 베껴두면 우리 것만 뒤처진다. 그래서 상류 트리에서 그대로 빌드하고, 우리는 얹기만 한다.

#### ✅ SHA 고정됨 (2026-08-07)

```
commit=3370c817efc5458fbe69cc3c02a9bc8956cb8a81
```

[`docker/sils/ss2-upstream.pin`](../../docker/sils/ss2-upstream.pin) 에 커밋했고,
`build-ss2.sh` 의 `SS2_REF` **기본값도 이 SHA** 다 (`master` 아님).

**왜 moving branch 를 벗어나야 하나.** [1계단 판정](evidence/ss2-stage1-verdict-2026-08-07.md) —
V1 이 차량 프레임이라는 것, V3 의 50 ms 스텝 — 은 **이 커밋에 대한 사실**이다. `master` 를 따라가면
다음 빌드에서 twist 프레임이나 스텝 크기가 바뀌어도 **아무도 눈치채지 못한 채** 그 결론만 남는다.

`run_vecu_wire.sh` 는 이제 pin 을 읽어 배너 · `env.txt` · `wire-verdict.json` 에 각인하고
pin 파일 자체를 산출물에 복사한다. pin 이 없으면 "재현 불가" 를 경고한다.

```bash
# 상류 최신을 따라가 보려면 — 반드시 재검증할 것
SS2_REF=master docker/sils/build-ss2.sh
tools/sils/ss2/run_vecu_wire.sh --bin-dir <경로>   # V1/V3 가 그대로인지 확인
# 그대로면 build-ss2.sh 의 SS2_REF 와 pin 파일을 함께 갱신
```

### 3-2. Autoware 없이 시뮬레이터 띄우기

```bash
docker compose -f docker/compose/docker-compose-sils-ss2.yaml up ss2-mock
```

`cpp_mock_scenarios` 는 `traffic_simulator` + `simple_sensor_simulator` 를 C++ 에서 직접 몰아
**concealer/Autoware 없이** 돈다. 1계단에 가장 싸고, 나중에 B-det 의 자연스러운 출발점이다.

> `package not found` 로 실패하면 상류 빌드에 `-DBUILD_CPP_MOCK_SCENARIOS=ON` 이 들어갔는지
> 확인할 것 (상류 Dockerfile 은 켜두었다). **결과를 §6 에 적을 것.**

### 3-3. B1-3 실측 — 이게 1계단의 본체다

시뮬레이터를 **띄운 채로** 다른 터미널에서:

```bash
docker compose -f docker/compose/docker-compose-sils-ss2.yaml run --rm ss2-probe
```

`runs/ss2-probe/` 에 두 파일이 나온다:

- `interop-probe.json` — 기계 판독용
- `interop-probe.md` — **BR-C-2a 에 그대로 붙여넣는 표**

수집 항목과 [BR-C-2a](sils-real-bridge-requirements.md) 대응:

| 수집 | BR-C-2a 항목 |
|---|---|
| `RMW_IMPLEMENTATION` (실제 로드된 것) | RMW 교차 조합 |
| `ROS_DOMAIN_ID` | 도메인 정합 |
| 토픽 + ROS 타입 | 공통 IDL |
| **엔드포인트별 QoS** (reliability/durability/history/deadline/liveliness) | QoS offered/requested 매트릭스 |
| **파생 DDS 이름** (`rt/<topic>`, `pkg::msg::dds_::Msg_`) | DDS type name 문자열 |

**두 번 돌릴 것** — 두 열을 다 채워야 한다:

```bash
SILS_RMW=rmw_fastrtps_cpp  docker compose -f docker/compose/docker-compose-sils-ss2.yaml run --rm ss2-probe
SILS_RMW=rmw_cyclonedds_cpp docker compose -f docker/compose/docker-compose-sils-ss2.yaml run --rm ss2-probe
```

> ⚠ **시뮬레이터와 프로브의 RMW 를 같이 바꿔야 한다.** Cyclone 과 Fast DDS 는 서로를 discovery
> 하지 못하므로, 한쪽만 바꾸면 "토픽 0개"가 나오고 그건 측정이 아니라 미스매치다.
> 프로브는 이 경우 **exit 3 과 함께 점검 순서를 출력**한다.

### 3-4. 측정되지 않는 것 — XCDR

`ros2` CLI 에는 협상된 `data_representation` 을 보여주는 verb 가 **없다**. 프로브도 이것을
추측하지 않고 `not_measured` 로 명시한다. RTPS 캡처로 따로 확인할 것:

```bash
sudo tcpdump -i any -w /tmp/rtps.pcap 'udp portrange 7400-7500'
```

Wireshark 의 RTPS dissector 에서 DATA submessage 의 encapsulation id 를 본다 —
`0x0000`/`0x0001` = CDR(XCDR1), `0x0006`/`0x0007` = XCDR2.

---

## 4. 파생 DDS 이름이 왜 중요한가

ROS 2 는 이름을 **망글링**한다:

```
  ROS 토픽   /simulation/entity/status
  DDS 토픽   rt/simulation/entity/status          ← 앞에 rt/

  ROS 타입   traffic_simulator_msgs/msg/EntityStatus
  DDS 타입   traffic_simulator_msgs::msg::dds_::EntityStatus_    ← ::dds_:: 삽입, 끝에 _
```

우리 vECU 는 **순수 DDS 참가자**라 망글링된 이름을 본다. 같은 도메인·같은 RMW 라 해도
`sils/ego/state` 와 `rt/simulation/...` 은 만나지 않는다.

**그래서 어댑터가 존재한다.** 직접 subscribe 로 해결되는 문제가 아니다 — 이것이
[BR-C-2](sils-real-bridge-requirements.md) 가 "브리지 = rclcpp 노드" 로 못박은 이유다.

> 프로브가 내는 DDS 이름은 **규약에서 유도한 값이지 관측값이 아니다.** BR-C-2a 에 사실로
> 적기 전에 RTPS 캡처로 확인할 것. 프로브 출력에도 그렇게 표시된다.

---

## 5. `.xosc` 로 돌리기

```bash
mkdir -p scenarios/ss2
# 상류 동봉 샘플을 먼저 쓸 것 — Web.Auto 계정 없이 시작할 수 있다
docker compose -f docker/compose/docker-compose-sils-ss2.yaml up ss2-scenario
```

**Web.Auto 계정은 1계단에 필요 없다.** 계정이 필요한 것은 Scenario Editor 저작뿐이고,
[BR-S](sils-real-bridge-requirements.md) 에 적었듯 **실행·판정은 계정 없이 된다.**

같은 `.xosc` 를 판정 쪽으로도 흘릴 수 있다 — 이건 **Linux 없이 지금도 된다**:

```bash
python3 tools/sils/conformance/import_openscenario.py \
    --xosc scenarios/ss2/sample.xosc --out /tmp/requirements.yaml
```

---

## 6. 1차 실행 결과 (2026-08-06) ★

Ubuntu x86_64, kernel 6.8.0-124-generic. 원측정값:
[`evidence/ss2-interop-probe-2026-08-06.md`](evidence/ss2-interop-probe-2026-08-06.md).

| 항목 | 결과 | 비고 |
|---|---|---|
| 상류 `SS2_REF` / commit | ⚠ **미기록** | `ss2-upstream.pin` 을 커밋하지 못했다. **다음 측정부터 반드시** |
| 빌드 소요 시간 | ⚠ **오래 걸림** — 정확한 값 미기록 | 상류 워크스페이스 전체 colcon 소스 빌드. ccache 가 BuildKit 캐시에 남으므로 **재빌드는 훨씬 빠르다** |
| `ros:humble` 로 빌드되는가 | ✅ | 상류가 Humble 유지 확인 |
| **`cpp_mock_scenarios` 가 Autoware 없이 도는가** | ✅ | 노드 `scenario_node` · `simple_sensor_simulator` · `visualizer` 뿐. **concealer/Autoware 부재** — §1 결론 실증 |
| **Fast DDS 재정의(A) 성립하는가** | ✅ | `rmw_fastrtps_cpp` 로 정상 발행. 상류의 Cyclone 기본값은 강제가 아니다 |
| **bare DDS ↔ ROS 2 discovery** | ✅ **실증** | 같은 도메인의 우리 스택 참가자와 상호 가시. BR-C-2a 핵심 질문 첫 긍정 근거 |
| BR-C-2a 표를 채웠는가 | 🟡 **Fast DDS 열만** | Cyclone 열 미측정 |
| Cyclone(B) 열 | ⬜ | `SILS_RMW=rmw_cyclonedds_cpp` 로 재실행 |
| XCDR 버전 (RTPS 캡처) | ⬜ | §3-4 |
| QoS history | ❌ **측정 불가** | `rmw_fastrtps` 보고 갭 — 전 엔드포인트 `UNKNOWN`. QoS 값이 아니다 |

### 여기서 나온 세 가지 ★

**1. 어댑터(B2-1) 입력이 확정됐다.**

| 정규 계약 | 원천 ROS 토픽 | ROS 타입 | QoS |
|---|---|---|---|
| `EgoState` | **`/entity/status`** | `traffic_simulator_msgs/msg/EntityStatusWithTrajectoryArray` | RELIABLE · VOLATILE |
| (시간) | `/clock` | `rosgraph_msgs/msg/Clock` | ⚠ **BEST_EFFORT** |
| `VehicleControl` | **없음** | — | B1-2 미결 확인 |

`/entity/status` 는 `/simulation/entity/marker`(rviz 마커)와 **다른 토픽**이다.

**2. ⚠ `/clock` 이 BEST_EFFORT 다.** [simulator-loops §7.2](sils-simulator-loops.md) 의 W2
(`/clock` → tick 브리지)를 **유실 가능한 시계 위에 올린다는 뜻**이다. clock 이 유실되면 tick 이
조용히 건너뛰고 그게 정확히 §7.2 가 경고한 **"가짜 lockstep"** 이 된다. W2 는 유실을 **탐지**해야
한다 — 무시하고 중계하면 안 된다.

**3. ⚠ 도메인 0 이 공유되고 있었다.** 우리 스택의 bare DDS 참가자(`/DoorSwitch`,
`/DoorUnlockLockIndicator`, `sdv_vss/msg/*`)가 같은 도메인에 있었다. interop 실증으로는 좋은
소식이지만 **golden 캡처에는 위험**이다. **루프 B 에는 전용 도메인을 줄 것**:

```bash
SILS_DOMAIN=42 docker compose -f docker/compose/docker-compose-sils-ss2.yaml up ss2-mock
# vECU 쪽도 같은 값:  VISS_DDS_DOMAIN_ID=42
```

### 다음 측정에서 채울 것

- [ ] `ss2-upstream.pin` 커밋 (**최우선** — SHA 없는 실측은 재현 불가)
- [ ] 전용 도메인으로 재측정 (도메인 0 오염 제거)
- [ ] Cyclone 열 (`SILS_RMW=rmw_cyclonedds_cpp`, **시뮬레이터·프로브 동시에**)
- [ ] XCDR (RTPS 캡처, §3-4)
- [ ] 빌드 소요 시간 · 이미지 크기

## 6.1 어댑터 투입 (B2-1) — 1계단을 닫는다

[`adapters/ss2`](../../adapters/ss2/README.md) 가 `/entity/status` 를 정규 `EgoState` 로
번역한다. 구현·단위시험은 끝났고 **컨테이너 빌드와 실행이 미검증**이다.

```bash
docker/sils/build-ss2-adapter.sh          # sils-ss2:humble 이 먼저 있어야 한다
SILS_DOMAIN=42 docker compose -f docker/compose/docker-compose-sils-ss2.yaml up ss2-mock
SILS_DOMAIN=42 docker compose -f docker/compose/docker-compose-sils-ss2.yaml up ss2-adapter
```

**성공 판정**: 어댑터가 `received=N published=N` 을 올리고, 같은 도메인의 vECU 트레이스에
`state_received` 가 찍힌다 → **SS_v2 → 어댑터 → 버스 → vECU → trace → gate 관통**.

| 항목 | 결과 |
|---|---|
| 어댑터 이미지 빌드 (fastddsgen gradle 포함) | ✅ **성공 (2026-08-07)** — 걸렸던 5가지는 [README](../../adapters/ss2/README.md) 에 기록 |
| **어댑터 기동 · 발행** | ✅ **성공** — 20 Hz, `received=400 published=400 skipped=0 write_failed=0` |
| **V2 — Fast DDS 2.x(어댑터) ↔ 3.x(vECU) interop** | ✅ **PASS (2026-08-07)** → §6.2 |
| **V1 — `twist` 가 차량 프레임인가** (곡선 주행에서만 드러남) | ✅ **VEHICLE** → §6.2 |
| V3 — `status.time` 이 시뮬 시각인가 | ✅ **SIM · 50 ms 고정 스텝** → §6.2 |
| vECU 트레이스에 `state_received` | ✅ 513 distinct 샘플 |

> 셋 다 [§6.2](#62-vecu-투입--v2v1v3-판정) 의 `run_vecu_wire.sh` 한 번으로 판정된다.

**기동 중 배운 것 2가지** ([어댑터 README](../../adapters/ss2/README.md) 상세):

1. **Autoware 없으면 ego 엔티티 타입이 EGO(0)가 아니라 VEHICLE(1)** — `SS2_EGO_NAME=ego` 필요.
   타입 기반 선택은 B-full 용이다.
2. **compose 가 호스트 환경변수를 받는다** — 이전 세션에 남은 `SILS_RUN_ID` 가 새어 들어와
   run id 가 바뀌었다. golden 캡처 런에서는 `SILS_*` 를 명시적으로 넘길 것.

> V1·V2 상세는 [adapters/ss2 README §5](../../adapters/ss2/README.md).

---

## 6.2 vECU 투입 — V2/V1/V3 판정

어댑터가 **썼다**는 것까지가 6.1 이다. **읽었다**는 것은 vECU 를 붙여야 나오고, 그것이
1계단의 최종 판정이다.

```bash
# 시뮬레이터 + 어댑터가 떠 있는 상태에서
tools/sils/ss2/run_vecu_wire.sh --bin-dir build/bin

# 무엇을 실행할지만 보고 싶으면
tools/sils/ss2/run_vecu_wire.sh --bin-dir build/bin --dry-run
```

산출물은 `runs/ss2-wire/<run-id>/` 에 `vecu.jsonl`(트레이스) · `wire-verdict.json`(판정) ·
`env.txt`(재현용 환경) 로 떨어진다. 종료코드는 **0 = V2 통과 / 1 = V2 실패 / 2 = 설정 오류**.

### 판정 세 가지 — 게이트는 V2 하나뿐이다

| | 무엇을 묻나 | 성격 |
|---|---|---|
| **V2** interop | Fast DDS 2.6(어댑터) 이 쓴 것을 3.x(vECU) 가 읽는가 | **게이트**. 여기서 막히면 위층이 전부 무의미 |
| **V1** frame | `velocity` 가 차량 프레임인가 지도 프레임인가 | 측정. **곡선 시나리오에서만 결론이 난다** |
| **V3** clock | `stamp_ns` 가 시뮬 시각인가 wall clock 인가 | 측정. 2계단 시간동기(W1/W2) 설계 입력 |

V1·V2·V3 를 게이트로 묶지 않은 이유: **직선 주행에서는 두 프레임 가설이 같은 값을 예측한다.**
그때 "VEHICLE 통과" 라고 적으면 아직 열려 있는 질문을 닫아버린다. 그래서
[`analyze_wire.py`](../../tools/sils/ss2/analyze_wire.py) 는 그런 런에 **INCONCLUSIVE** 를 내고,
이것을 통과로도 실패로도 집계하지 않는다. V1 결론을 원하면 **커브가 있는 시나리오**로 돌릴 것.

### 이 스크립트가 실제로 막는 것 — 세 가지 증상이 전부 똑같다

`state_received` 가 하나도 없을 때, 원인은 interop 이 아닐 가능성이 더 높다:

| 원인 | 왜 조용한가 |
|---|---|
| 도메인 불일치 | 다른 도메인은 서로 **보이지도 않는다** |
| `SILS_DURABILITY=volatile` ↔ 어댑터의 TRANSIENT_LOCAL writer | RxO 불만족 → DDS 가 **말없이 매칭 거부** |
| 어댑터가 실은 발행 중이 아님 | vECU 쪽에는 구별할 방법이 없음 |

그래서 스크립트는 상속받은 **`SILS_*`/`VISS_*` 를 전부 지우고** 필요한 것만 다시 세운다
(`--keep-env` 로 해제). 6.1 에서 겪은 `SILS_RUN_ID` 누수가 이 부류다.

### 실행 결과 (2026-08-07) ★ — 1계단 종료

원측정값과 해석은 [`evidence/ss2-stage1-verdict-2026-08-07.md`](evidence/ss2-stage1-verdict-2026-08-07.md).

| | 결과 | 근거 |
|---|---|---|
| **V2** interop | ✅ **PASS** | `traffic_simulation_demo` 30 초에 **distinct 513 샘플**, `src_seq` 역행 0 |
| **V1** frame | ✅ **VEHICLE** | `lanechange_left` 에서 헤딩 0.166 rad 편차 지점의 `\|vy\|/\|v\| = 0.0000`. 지도 프레임이면 0.165 였어야 함 |
| **V3** clock | ✅ **SIM · 50 ms 고정 스텝** | stamp 가 34.05 s 에서 시작(= epoch 아님) + 전 stamp 가 50 ms 배수 |

**V3 가 2계단 설계를 바꾼다.** [B1-1](sils-simulator-loops.md) 의 "wall clock 으로 돈다" 는
절반만 맞다 — SS_v2 는 **50 ms 고정 스텝 적분기**이고 그 스텝을 실시간에 맞춰 굴린다.
W1/W2 는 스텝을 **새로 만드는** 것이 아니라 **이미 있는 스텝을 외부에서 구동**하면 된다.

### ⚠ 짧은 mock 시나리오 — 순서를 뒤집어야 한다

`lanechange_*` 계열은 기동을 마치면 **스스로 종료**한다 (`lanechange_left` 는 sim **1.4 초**).
vECU 를 나중에 띄우면 이미 끝난 뒤라, 어댑터 writer 가 TRANSIENT_LOCAL 로 보관하던
**샘플 하나**만 받는다. ZOH 때문에 트레이스에는 수백 행이 쌓이므로 **정상처럼 보인다.**

```bash
docker compose -f docker/compose/docker-compose-sils-ss2.yaml up -d ss2-adapter   # ① 대기
tools/sils/ss2/run_vecu_wire.sh --bin-dir <경로> --duration 60                     # ② 먼저 띄우고
docker compose -f docker/compose/docker-compose-sils-ss2.yaml up -d ss2-mock       # ③ 곧바로 투입
```

`analyze_wire.py` 는 이 상태를 `distinct_src_seq` 로 잡아내고 원인·해법을 함께 출력한다.

### 트레이스 필드 추가

V1·V3 는 트레이스에서 답이 나와야 하는데 `state_received` 에 `vy`·`stamp_ns` 가 없었다.
[`vehicle_run.hpp`](../../service/sils/vecu/src/vehicle_run.hpp) 에 **추가**했다 —
스키마 v1 은 추가 필드를 forward-compatible 로 허용하므로 `required` 는 그대로다.
필드가 없는 구버전 vECU 트레이스는 실패가 아니라 **INCONCLUSIVE** 로 나온다.

---

## 7. 파일

| 파일 | 역할 |
|---|---|
| [`docker/sils/build-ss2.sh`](../../docker/sils/build-ss2.sh) | 상류 clone/pin + 2단계 빌드 |
| [`docker/sils/Dockerfile.ss2`](../../docker/sils/Dockerfile.ss2) | 얇은 SiLS 레이어 (Fast DDS 프로파일 · 프로브) |
| [`docker/compose/docker-compose-sils-ss2.yaml`](../../docker/compose/docker-compose-sils-ss2.yaml) | `ss2-probe` / `ss2-mock` / `ss2-scenario` / `ss2-shell` |
| [`tools/sils/ss2/probe_interop.sh`](../../tools/sils/ss2/probe_interop.sh) | B1-3 실측 → `interop-probe.{json,md}` |
| [`tools/sils/ss2/run_vecu_wire.sh`](../../tools/sils/ss2/run_vecu_wire.sh) | §6.2 vECU 투입 — 환경 정리 + 실행 + 판정 |
| [`tools/sils/ss2/analyze_wire.py`](../../tools/sils/ss2/analyze_wire.py) | V2/V1/V3 판정 로직 (단위시험 있음) |
| `docker/sils/ss2-upstream.pin` | 빌드 성공 시 기록되는 상류 commit (**커밋할 것**) |

---
*작성 2026-08-06 (미검증). 상류 사실관계는 `tier4/scenario_simulator_v2` master 소스 확인 기준.*
