# 실 브리지 요구사항 — MORAI · Autoware 연동 (#1)

SiLS vECU를 **실제 MORAI(플랜트) + Autoware(인지·판단)**와 닫힌 루프로 연동하기 위한 요구사항. 다른 개발자가 바로 착수할 수 있도록 **항목별**로 정리한다. 브리지 두 개(MORAI·Autoware)가 몸통이고, 어디에도 안 속하는 **공통층**이 소수 있다.

> **핵심 사실**: 정규 계약(IDL)·vECU 소비/발행부·요구/시나리오 판정((a)(b))은 **이미 완성**되어 있다. 남은 일은 **"알려진 ROS2 메시지 ↔ 알려진 정규 계약" 번역 + 기동 배관**이다. 정규 계약이 authoritative이고, 아래 ROS2 메시지 필드·단위는 실제 `morai_msgs`/`autoware_*_msgs` 정의로 **[확인]** 후 확정한다.

관련: [플랜트결합 설계](sils-morai-plant-adapter-design.md) · [VCU/Autoware 설계](sils-vehicle-interface-autoware.md) · [기능검증·상호운용](sils-functional-verification-interop.md)

---

## 0. 토폴로지 & 데이터 흐름

세 구역: **외부(그들의 세계)** / **브리지(=#1, 구축 대상)** / **우리 SiLS(완료)**. "우리 SiLS"는 vECU만이 아니라 **ClockMaster(결정론) + conformance 판정 + .xosc importer**를 포함한다 — 이게 제품이다.

```
┌──────────────── 외부 (ROS2 · 그들의 세계) ────────────────┐
│   MORAI  ──EgoVehicleStatus──▶  Autoware                  │
│  (시뮬=플랜트/권위) ◀──CtrlCmd──┐  (인지·판단·경로)         │
└────┬──────────────────────────┼──────────┬───────────────┘
     │ EgoStatus                │ CtrlCmd   │ control_cmd / gear_cmd
     ▼                          │           ▼
╔═══ #1 구축 대상 ══════════════╪═══════════════════════════╗
║ ┌─────────────────┐          │        ┌──────────────────┐║
║ │ MORAI↔DDS 브리지 │          │        │ Autoware↔DDS 브리지│║
║ └────┬────────▲───┘          │        └─────────┬────────┘║
╚══════╪════════╪══════════════╪══════════════════╪═════════╝
   EgoState  VehicleControl → CtrlCmd         ControlTarget
════════╪════════╪═══════════════════════════════╪═══ DDS 정규 버스 ═══
╔═══════╪════════╪═══════ 우리 SiLS (완료) ═══════╪═══════════════════╗
║  sils/ego/state │ sils/ego/cmd        autoware/control_target       ║
║       │         │                             │                     ║
║       ▼         │                             ▼                     ║
║   ┌─────────────┴─────────────────────────────────────┐            ║
║   │ vECU (VCU / 개발자 ECU)                             │            ║
║   │   EgoState + ControlTarget ─▶ 통합 VehicleControl   │            ║
║   └───────────────▲───────────────────────┬────────────┘            ║
║   ClockMaster ────┘ tick (lockstep 결정론)  │ trace (JSONL)           ║
║                                            ▼                         ║
║   conformance 판정:  evaluate / gate / requirements                  ║
║                          ▲ import_openscenario(.xosc)               ║
╚══════════════════════════════════════════════════════════════════════╝
```

위는 **데이터 평면**(DDS 제어 루프)이다. 그 아래 **제어 평면**(누가 무엇을 기동·관리하나)에 **MMF**가 있다 — vECU(및 향후 브리지)를 컨테이너 서비스로 띄운다:

```
                          ╎ 기동·수명관리 (제어 평면 — 데이터 아님)
  ╭╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌┴╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╮
  ╎ MMF  ─ adsw_manager + lifecycle_mgr                ╎ ╌╌▶ vECU(및 브리지)
  ╎   speclist → StartServiceRequest → docker run      ╎     컨테이너 기동·수명관리
  ╎   이미지: oss-mmf-sils-vecu                         ╎     (경로 B, 실기동 검증됨)
  ╰╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╯
```

**닫힌 루프 흐름**(데이터 평면): ① MORAI 상태 →(MORAI 브리지)→ `EgoState`(DDS) → vECU. ② Autoware 제어 →(Autoware 브리지)→ `ControlTarget`(DDS) → vECU. ③ vECU가 통합 `VehicleControl` 발행(`sils/ego/cmd`). ④ →(MORAI 브리지)→ `CtrlCmd`(ROS2) → MORAI(루프 닫힘). ⑤ 전 참가자가 남긴 trace를 conformance가 판정(요구는 `.xosc`에서 (b)로 추출).

- **권위(authority) = MORAI**(공유 세계). vECU는 상태·타깃을 소비하고 통합 명령을 발행한다.
- **브리지(#1)만 새로 만든다.** MORAI·Autoware는 외부, 우리 SiLS(vECU·ClockMaster·판정)는 완료.
- 브리지는 **ROS2 ↔ 우리 DDS 정규 계약** 경계(rclcpp 노드). ClockMaster의 tick에 MORAI가 물리는지가 결정론을 좌우(BR-C-1).
- **MMF = 배포·수명 관리 평면**(제어 평면, 데이터 루프 아님): `adsw_manager`+`lifecycle_mgr`가 speclist 기반으로 vECU를 **컨테이너 서비스로 기동·관리**(경로 B, `oss-mmf-sils-vecu` 이미지로 실기동 검증됨). "vECU가 어떻게 뜨고 관리되나"의 답. 브리지(#1)도 완성되면 MMF 서비스로 배포 가능 → **BR-C-3 오케스트레이션이 MMF speclist로 수렴**할 수 있다.

## 1. 이미 확정된 것 — 브리지 개발자가 건드리지 않는 경계

| 자산 | 위치 | 상태 |
|---|---|---|
| 정규 계약 IDL | `idl/dds/sils/{EgoState,VehicleControl,ControlTarget}.idl` | ✅ 확정 (필드·단위·프레임 명시) |
| vECU(VCU) 소비/발행 | `service/sils/vecu`, 예제 `vehicle-interface` | ✅ EgoState+ControlTarget 소비 → VehicleControl 발행 |
| 채널 토픽 | `sils/ego/state`·`autoware/control_target`(in), `sils/ego/cmd`(out) | ✅ 채널맵 확정 |
| 판정 트레이스 | vECU가 `state_received`/`target_received`/`control_published` emit | ✅ **브리지가 버스에 실 데이터만 얹으면 evaluate/gate가 실 런 판정** |
| 요구·시나리오 판정 | `evaluate.py --requirements`, `import_openscenario.py` | ✅ (a)(b) 완료 |

→ **브리지의 유일한 책임은 "실 데이터를 정규 토픽에 얹기"**. 트레이스 캡처·판정은 공짜로 따라온다.

---

## 2. [BR-M] MORAI 연동 브리지 요구사항

**형태**: rclcpp 노드. MORAI ROS2 토픽 ↔ 우리 DDS 정규 토픽 양방향 번역.

### BR-M-1 · 상태 IN: `morai_msgs/EgoVehicleStatus` → `EgoState` @ `sils/ego/state`

| 정규 EgoState 필드 | 단위/프레임 | MORAI 소스 | 변환 |
|---|---|---|---|
| `position` (x,y,z) | map, m | EgoVehicleStatus.position | 프레임 정합 **[확인]** |
| `orientation_rpy` | rad | heading | **deg→rad** |
| `velocity` (x,y,z) | vehicle, m/s | velocity | **km/h→m/s** (단위 [확인]) |
| `acceleration` | vehicle, m/s² | (EgoStatus에 없으면 IMU/미분) | **[확인]** 소스 |
| `yaw_rate` | rad/s | angular velocity | **deg/s→rad/s** |
| `steering_angle` | rad | front wheel angle | **deg→rad** |
| `gear` | 0=P 1=R 2=N 3=D | MORAI gear | enum 매핑 **[확인]** |
| `run_id`/`seq`/`stamp_ns` | — | — | run_id 주입, seq 증가, sim 시각 |

### BR-M-2 · 명령 OUT: `VehicleControl` @ `sils/ego/cmd` → `morai_msgs/CtrlCmd`

| MORAI CtrlCmd 필드 | 정규 VehicleControl 소스 | 변환 |
|---|---|---|
| `longCmdType` | (모드 선택) | **결정 필요**: acceleration 모드 지원 시 직결, 아니면 accel/brake 정규화 |
| steering | `steering_angle` (rad) | 단위 정합 (CtrlCmd 조향 단위 [확인]) |
| accel/brake **or** acceleration | `acceleration` (m/s², signed) | 모드에 따라: 직결(m/s²) 또는 **+→accel(0~1) / −→brake(0~1) 정규화** |
| gear | `gear` (0~3) | MORAI gear enum 역매핑 |

> **BR-M-2 핵심 결정**: MORAI `CtrlCmd`의 종방향 명령 방식(`longCmdType`). MORAI가 **가속도(m/s²) 모드**를 지원하면 정규 `acceleration`을 직결(무손실). 아니면 accel/brake 0~1로 정규화해야 하고 **차량 파라미터(최대 가감속) 필요** → 매핑 손실·튜닝 발생. 이 선택이 OUT 브리지 난이도를 좌우한다.

### BR-M-3 · 시간 (→ 공통 BR-C-1)
MORAI가 우리 tick에 물릴 수 있는지가 결정론(L3)을 좌우 → **BR-C-1**에서 다룸.

> **★ 공개 자료 확인 (2026-08-07): MORAI 에 Sync Mode 가 있다.**
> [`MORAI-ROS_morai_msgs`](https://github.com/MORAI-Autonomous/MORAI-ROS_morai_msgs)(ROS 1)에
> `SyncModeCmd{user_id, start_sync_mode, time_step[ms]}` ·
> `SyncModeCtrlCmd{frame, CtrlCmd, sensor_capture}` ·
> `WaitForTick{user_id, frame}` → `WaitForTickResponse{tick_status, pause_status, frame,
> EgoVehicleStatus vehicle_status, time}` 과 서비스 `MoraiSyncModeCmdSrv`/`MoraiWaitForTickSrv`/
> `MoraiSyncModeCtrlCmdSrv` 가 정의되어 있다.
>
> **형태가 우리 barrier 와 사실상 같다** — 시간이 ms 고정 스텝이고, 제어가 `frame` 에 태그되며,
> `WaitForTick` 응답에 틱 이후 상태가 함께 온다. 역할만 뒤집혀 있다(우리가 호출, MORAI 가 응답).
> **SS_v2 와 달리 시뮬레이터 patch 가 불필요해 보인다.**
>
> ⚠ **단 ROS 2 메시지 집합에는 없다** —
> [`MORAI-ROS2_morai_msgs`](https://github.com/MORAI-Autonomous/MORAI-ROS2_morai_msgs)(main,
> 2026-06 갱신)는 32개로 ROS 1(137개)의 부분집합이고 Sync Mode·Fault Injection·로봇 플랫폼이
> 빠져 있다. 빠진 것이 한 부류로 묶여 **순차 포팅 중**으로 읽힌다. 또 `user_id` 필드는 ROS 서비스에
> 불필요한 개념이라 **본체가 네트워크(UDP) API 이고 morai_msgs 가 래퍼**일 가능성을 시사한다
> (그렇다면 ROS 버전과 무관하게 쓸 수 있다). gRPC 절차 목록에는 `Pause`/`Resume` 만 있다.
>
> **이것은 공개 자료 기준 추정이며 벤더 확인 전이다** →
> [morai-vendor-questions §2 (Q1)](sils-morai-vendor-questions.md) 로 질의.

---

## 3. [BR-A] Autoware 연동 브리지 요구사항

**형태**: rclcpp 노드. Autoware 제어 출력 → 우리 DDS 정규 `ControlTarget`. (단위가 우리와 거의 일치해 가장 쉬움.)

### BR-A-1 · 타깃 IN: Autoware 제어 → `ControlTarget` @ `autoware/control_target`

| 정규 ControlTarget 필드 | Autoware 소스 | 변환 |
|---|---|---|
| `steering_angle` (rad) | `/control/command/control_cmd` = `autoware_control_msgs/Control` · `lateral.steering_tire_angle` (rad) | **1:1** |
| `acceleration` (m/s²) | `Control.longitudinal.acceleration` (m/s²) | **1:1** |
| `gear` (0~3) | `/control/command/gear_cmd` = `GearCommand` (DRIVE=2/REVERSE=20/PARK=22/NEUTRAL=1) | enum 매핑 (DRIVE→3·REVERSE→1·PARK→0·NEUTRAL→2) |
| `run_id`/`seq`/`stamp_ns` | — | run_id 주입, seq 증가, 시각 |

> control_cmd(제어)와 gear_cmd(기어)는 **별도 토픽** → 최신값 병합해 한 `ControlTarget`으로 낸다(staging은 vECU가 §7.3로 처리).

### BR-A-2 · 차량 상태 피드백 (Autoware ← 상태)
Autoware는 계획을 위해 `/vehicle/status/{steering,velocity,gear}_status`(feedback)를 **입력받아야** 한다. 소스 = MORAI 상태.
- **결정 필요**: MORAI→Autoware 직접(ROS2) vs 상태 브리지가 EgoState → `/vehicle/status/*` 역변환. 전자면 BR-A 범위 밖(구성만), 후자면 BR-A-2 브리지 추가.

---

## 4. [BR-C] 공통층 요구사항 (둘 다 아니지만 필수)

### BR-C-1 · 시간 동기 (clock) — **최우선 리스크**

> **이분법을 고칠 것 (2026-08-06).** 아래 "지원 O → L3 / 지원 X → L0" 은 **불완전하다.**
> BR-S-3 조사에서 드러났듯 시뮬레이터를 barrier 에 넣어도 **제어기(Autoware)가 비동기면 결정론은
> 여전히 성립하지 않는다.** 따라서 답은 O/X 둘이 아니라 **구성별로 갈린다**:
> **det 구성**(vECU + 시뮬레이터, 제어 목표는 fixture 공급) → L3 가능 ·
> **full 구성**(Autoware 포함) → 폐루프 동작 검증까지. 상세: [simulator-loops §1·§7.1](sils-simulator-loops.md).
>
> **"지원 X" 를 free-run 으로 처리할 때의 함정도 같은 문서 §7.2 에 있다** — 시뮬레이터 시계를
> 그대로 `TickGrant` 로 중계하면 `LockstepClockSource` 큐가 뒤처진 채 `tick_complete` 를 계속 찍어
> `analyze_lockstep` 이 **건강한 barrier 로 오판**한다. barrier 없는 시간 종속은 `lockstep` 이 아닌
> **별도 mode** 로 분리해야 하며, 이는 MORAI(BR-M-3)에도 그대로 적용된다.

> **SS_v2 측 실측 (2026-08-07) ★ — "스텝이 없다" 가 아니라 "스텝을 우리가 못 민다" 다.**
> vECU 트레이스의 `stamp_ns` 가 **전부 50 ms 의 배수**였다(차분의 GCD, 시작 오프셋 34.05 s 와 무관).
> 즉 SS_v2 는 **고정 스텝 적분기를 실시간 속도로 굴리는** 구조다.
> 근거: [`evidence/ss2-stage1-verdict-2026-08-07.md`](evidence/ss2-stage1-verdict-2026-08-07.md).
>
> 이것이 W1/W2/W3 의 성격을 바꾼다 — 스텝을 **새로 도입**하는 것이 아니라 **이미 있는 스텝의
> 트리거를 외부로 돌리는** 작업이 된다([simulator-loops §7.4](sils-simulator-loops.md)).
> MORAI 질의(BR-M-3)에도 같은 구분이 유효하다: **"스텝이 있는가" 와 "외부가 그 스텝을
> 밀 수 있는가" 는 다른 질문**이고, 후자만이 L3 를 가른다.

- **[확인]** MORAI가 외부 스텝/동기 제어(네트워크 sync·`step` API)를 지원하는가?
- **지원 O** → MORAI를 우리 ClockMaster barrier 참가자로 → 실 루프도 **L3 bit-재현 결정론 유지**.
- **지원 X** → MORAI wall-free-run 허용 → 실 루프는 **"동작 검증"(루프 닫힘·안전)까지, 결정론은 포기**(evaluate는 wall=L0, analyze_vehicle 폐루프는 유효). 이 경우 결정론 판정은 fixture 경로에만.
- 이 한 항목이 #1의 성격(검증완료 수준)을 가른다. **가장 먼저 답할 것.**

### BR-C-2 · ROS2 ↔ 우리 DDS interop
- 브리지 = **rclcpp 노드**가 ROS2 측을 받고 **FastDDS 정규 타입**으로 재발행(ROS2 type-hash 회피 — 우리가 택한 방향).
- **도메인 정합**: 정규 DDS는 `VISS_DDS_DOMAIN_ID`(기본 0). ROS2 rmw 도메인과 분리·매핑.
- **QoS**: 상태/명령은 RELIABLE 권장(제어 평면). 계약과 정합.

#### BR-C-2a · 벤더 상호운용 계약 (Fast DDS ↔ Cyclone DDS) — **미문서화 갭**

현재 브리지 설계는 재발행 측을 **Fast DDS 정규 타입**으로 고정했을 뿐, 상대 RMW/DDS 구현이
Cyclone DDS 인 경우의 **계약이 정의되어 있지 않다.** Autoware 배포는 `rmw_cyclonedds_cpp` 를
쓰는 구성이 흔하므로, 실 브리지 #1을 붙이기 전에 아래를 **명시적 계약 + 시험 매트릭스**로 확정해야
한다.

> **격상 (2026-08-06, 소스 확인).** 이것은 더 이상 가정이 아니라 **기본 구성**이다 —
> `tier4/scenario_simulator_v2` 상류 `Dockerfile` 의 runtime 스테이지에
> `ENV RMW_IMPLEMENTATION=rmw_cyclonedds_cpp` 가 박혀 있다. 즉 SS_v2 를 공식 이미지로 띄우면
> **Cyclone 이 기본**이고 우리 정규 버스는 Fast DDS 다. 대응은 두 갈래이며 **둘 다 측정 대상**이다:
> **(A)** SS_v2 를 Fast DDS 로 재정의(변수 하나 제거) · **(B)** Cyclone 유지 + 어댑터가 번역
> (BR-C-2 의 원래 설계). 측정 절차·스크립트: [ss2-bringup §1·§3-3](sils-ss2-bringup.md).

| 항목 | 확정해야 하는 것 | 왜 |
|---|---|---|
| **지원 범위** | Fast DDS ↔ Cyclone DDS 를 **지원 조합으로 선언**할지, 아니면 Fast DDS 단일로 못 박고 경계에 브리지를 둘지 | 선언하지 않으면 "되는 줄 알았는데 안 되는" 경계가 남는다 |
| **공통 IDL + DDS type name** | 정규 `EgoState`/`VehicleControl`/`ControlTarget` 의 **정확한 DDS type name 문자열**(ROS2 는 `<pkg>::msg::dds_::<Msg>_` 관례) | type name 이 한 글자 달라도 매칭이 안 된다 |
| **XCDR 버전** | XCDR1 vs XCDR2 (`data_representation` QoS) | 두 구현의 기본값이 다를 수 있어 payload 해석이 깨진다 |
| **QoS offered/requested 매트릭스** | reliability · durability · history · deadline · liveliness 별 offered↔requested 호환 조합 표 | RxO 규칙 위반은 조용한 미매칭으로 나타난다 |
| **Discovery/전송** | SIMPLE discovery + UDPv4 요구, 멀티캐스트 가용성, `initialPeers`. 현재 SiLS canonical 프로파일은 **UDP-only + SHM/DataSharing OFF**(§4.7.1) | 한쪽만 SHM/Discovery Server 를 쓰면 안 만난다 |
| **keyed topic 제한** | 정규 타입에 key 를 둘지(현재 keyless), keyed 로 갈 때 instance 처리 | ROS2 측은 keyless 관례라 계약이 갈린다 |
| **RMW 교차 조합** | `rmw_fastrtps_cpp` ↔ `rmw_cyclonedds_cpp` 양방향, + 순수 DDS 참가자(우리 vECU) | 3자 조합에서만 나타나는 실패가 있다 |
| **양방향 interop CI** | 조합별 pub→sub 왕복을 **자동 시험**으로 고정(양방향 각각) | 한 방향만 통과하는 사례가 흔하다 |

**산출물**: 위 표를 채운 `docs/sils/sils-dds-vendor-interop.md` + 조합별 왕복 시험을 `sils-ci.sh`
스테이지로. **BR-C-1(clock) 다음 우선순위**이며, Cyclone 지원을 SiLS 범위에 넣기로 결정하는
시점에 이 항목이 BR-C-2 에서 독립 요구로 분리된다.

#### 1차 실측 (2026-08-06) — Fast DDS 열 ★

원측정값: [`evidence/ss2-interop-probe-2026-08-06.md`](evidence/ss2-interop-probe-2026-08-06.md)
(`.json` 동봉). 구성: Ubuntu x86_64 · `cpp_mock_scenarios`(**Autoware 없음**) ·
`RMW_IMPLEMENTATION=rmw_fastrtps_cpp` · domain 0 · 토픽 16 / 엔드포인트 38.

| 항목 | 실측 결과 |
|---|---|
| **RMW 교차 조합** | ✅ **Fast DDS 재정의(A) 성립** — SS_v2 가 `rmw_fastrtps_cpp` 로 정상 기동·발행. 상류가 Cyclone 을 기본으로 두었지만 강제는 아니다 |
| **순수 DDS ↔ ROS 2** | ✅ **실증됨** — 같은 도메인의 bare DDS 참가자(`_CREATED_BY_BARE_DDS_APP_`, `sdv_vss/msg/*`)와 ROS 2 노드가 **서로 보인다.** BR-C-2a 의 핵심 질문에 대한 첫 긍정 근거 |
| **DDS type name** | 관례대로 `<pkg>::msg::dds_::<Msg>_`. `ros2 topic list` 는 demangle 된 것만 반환하므로, 표에 뜬 것은 이미 `rt/` 접두를 달고 있다는 뜻 |
| **QoS — reliability/durability** | 측정됨 (엔드포인트별, 원측정값 참조) |
| **QoS — history** | ❌ **측정 불가** — `rmw_fastrtps` 가 `ros2 topic info -v` 로 History 를 보고하지 않아 전 엔드포인트 `UNKNOWN`. **QoS 값이 아니라 보고 갭이다.** 발행측 소스/프로파일에서 읽을 것 |
| **XCDR 버전** | ⬜ 미측정 — RTPS 캡처 필요([ss2-bringup §3-4](sils-ss2-bringup.md)) |
| **Cyclone 열** | ⬜ 미측정 — `SILS_RMW=rmw_cyclonedds_cpp` 로 재실행 필요 |

#### 2차 실측 (2026-08-07) — **Fast DDS 2.6 ↔ 3.x 실데이터 관통** ★

1차는 "서로 **보인다**"까지였다. 2차는 **실제 샘플이 건너간다**를 확인했다.
원측정값: [`evidence/ss2-stage1-verdict-2026-08-07.md`](evidence/ss2-stage1-verdict-2026-08-07.md).

| 항목 | 실측 결과 |
|---|---|
| **Fast DDS 메이저 버전 교차** | ✅ **실증** — 어댑터(2.6, ROS 2 Humble 동봉) writer → vECU(3.x, 우리 빌드) reader 로 `EgoState` **distinct 513 샘플** 수신, `src_seq` 역행 0 |
| 타입 이름 · CDR 레이아웃 | ✅ 일치 — 같은 `EgoState.idl` 에서 각 측 생성기로 별도 생성한 것이 그대로 맞물림 |
| QoS 매칭 (RELIABLE · TRANSIENT_LOCAL) | ✅ 양측 일치 시 정상 매칭 |

> **이 결과의 범위**: `EgoState` **한 타입**이다. `VehicleControl`·`ControlTarget` 은 미확인이나
> 같은 IDL 원시타입·같은 생성기이므로 위험은 낮게 본다.
>
> ⚠ **버전 결합의 출처는 SS_v2 가 아니라 ROS 배포판이다.** 어댑터가 rclcpp 와 한 프로세스를
> 쓰므로 Humble 동봉 2.6 에 묶인다. 결합을 끊으려면 어댑터를 두 프로세스로 쪼개면 되는데,
> 이 실측으로 **그럴 필요가 없어졌다.**

**어댑터(B2-1) 입력이 확정됐다:**

| 정규 계약 | 원천 ROS 토픽 | ROS 타입 | QoS |
|---|---|---|---|
| `EgoState` | **`/entity/status`** | `traffic_simulator_msgs/msg/EntityStatusWithTrajectoryArray` | RELIABLE · VOLATILE |
| (시간) | `/clock` | `rosgraph_msgs/msg/Clock` | ⚠ **BEST_EFFORT** · VOLATILE |
| `VehicleControl` | **없음** | — | Autoware/concealer 부재 → B1-2 미결 확인 |

> ⚠ **`/clock` 이 BEST_EFFORT 다.** [simulator-loops §7.2](sils-simulator-loops.md) 의 W2
> (`/clock` → tick 브리지)를 **손실 가능한 시계 위에 올린다는 뜻**이다. clock 샘플이 유실되면
> tick 이 조용히 건너뛰고, 그것이 정확히 §7.2 가 경고한 "가짜 lockstep" 이 된다.
> W2 는 유실을 **탐지**해야 하며(연속 tick_seq 검사), 유실을 무시하고 중계하면 안 된다.
> `/entity/status` 는 `/simulation/entity/marker`(rviz 마커)와 다른 토픽이다 — 혼동 금지.

> ⚠ **도메인 0 은 공유되고 있다.** 측정 당시 우리 스택의 bare DDS 참가자(`/DoorSwitch`,
> `/DoorUnlockLockIndicator`)가 같은 도메인에 있었다. 위에서 이것은 **좋은 소식**(interop 실증)
> 이지만, **golden 캡처에는 위험**이다 — 무관한 엔드포인트가 매칭될 수 있고 이는 §5 플랜트 격리가
> 막으려는 것과 같은 종류의 조용한 오판이다. **루프 B 는 전용 도메인 id 를 줄 것**
> (`SILS_DOMAIN=<n>` + vECU `VISS_DDS_DOMAIN_ID` 동일값).

**부수 확인**: `simple_sensor_simulator` 는 `/parameter_events`·`/rosout` 외에 DDS 로 아무것도
발행하지 않는다 — [simulator-loops §7.1](sils-simulator-loops.md) B1-1 의 "ZeroMQ req/resp" 소스
확인을 **런타임에서 독립 재확인**한 셈이다. 또 `autoware_perception_msgs` 토픽이 Autoware 없이
존재한다 — "메시지 패키지만 필요" 결론의 실증.

### BR-C-3 · 실행 오케스트레이션 (MMF로 수렴)
- **SiLS 측 서비스(vECU·브리지·ClockMaster)를 MMF가 기동·관리**(adsw_manager/lifecycle_mgr, speclist)하는 것이 **목표 상태**다. **현재는 미실증** — Topology Compiler가 `saved-service-list.yaml` 포맷을 방출하는 데까지 왔고(경로 B의 절반), 실제 lifecycle_mgr 기동은 아직 검증되지 않았다. 현행 검증 경로는 `run_phase1.sh` 직접 기동(경로 A)이다([understanding §4.6](sils-understanding.md) 경로 A/B 표). 브리지를 MMF 서비스로 추가하면 오케스트레이션이 speclist 하나로 수렴한다. **MORAI·Autoware는 외부 기동**(각자 런처).
- 기동 순서(MORAI → Autoware → 브리지 → vECU → ClockMaster), `run_id` 전파.
- **시나리오 로딩**: `.xosc`를 MORAI 시나리오 러너 / Autoware `scenario_simulator_v2`에 로드. → [(b) importer](sils-functional-verification-interop.md)가 같은 `.xosc`에서 요구를 뽑으므로 **자극과 판정이 한 소스**.
- 트레이스 병합: 브리지/vECU/ClockMaster stdout → merged JSONL → `gate.py`.

---

## 4-S. [BR-S] scenario_simulator_v2 연동 (루프 B)

> 전체 그림·다이어그램·작업목록: **[sils-simulator-loops.md](sils-simulator-loops.md)**

BR-M(MORAI)과 **동일한 Option 2 버스 결합**을 TIER IV `scenario_simulator_v2` 에 적용한 것.
바뀌는 것은 어댑터가 말을 거는 상대뿐이고 정규 계약·vECU·게이트는 무변경이다. MORAI 와 달리
**오픈소스**라 미결 항목을 벤더 문의 없이 코드로 답할 수 있어, **BR-M 보다 먼저 진행한다.**

### BR-S-1 · 자산 체인 — `.xosc` 가 자극이자 판정 기준
- Web.Auto **Scenario Editor** 산출물은 **TIER IV Scenario Format v2.0(YAML)** 이며
  `.xosc` 로 변환되어 `scenario_test_runner` 에 들어간다. 변환 단계가 우리 파이프라인 **앞**에 붙는다.
- 같은 `.xosc` 를 `import_openscenario.py` 가 소비 → `requirements.yaml` → `gate.py --requirements`.
  BR-C-3 의 "자극과 판정이 한 소스"가 **여기서만** 완전히 성립한다(MORAI 는 A1-3 답에 종속).
- **[확인]** Scenario Editor 접근에 Web.Auto 계정이 필요한가. **실행·판정은 계정 없이 가능**하다
  (`.xosc` 파일만 있으면 됨) — 저작 도구만 계정 이슈다.

### BR-S-2 · vECU 삽입 지점 — concealer 경계 **[결정 필요]**
`scenario_simulator_v2` 는 concealer(Autoware 어댑터) + simple_sensor_simulator 로 **이미 폐루프를
닫는다.** 그 자리가 우리 VCU 자리(실차 `vehicle_interface` 위치)와 겹친다.

| 안 | 방식 | 평가 |
|---|---|---|
| (a) concealer 뒤 | simulator 의 ego 동역학 입력을 VCU 출력으로 치환 | 개입 지점이 시뮬레이터 내부 |
| **(b) concealer 우회** | VCU 가 `/control/command/control_cmd` 직접 소비 → `ControlTarget` → `VehicleControl` | **권장** — BR-A-1 매핑표 그대로 재사용 |

### BR-S-3 · 시간 동기 — **조사 완료 (2026-08-06, 소스 근거)**
> 상세 표·근거 파일: [simulator-loops §7.1](sils-simulator-loops.md)

- **외부 스텝 제어 API 는 없다.** frame 진행이 interpreter 내부 wall 타이머다
  (`create_wall_timer(currentLocalFrameRate(), ...)`, 기본 30 Hz).
- **시간 *값* 은 이미 논리 고정 증분이다** — `seconds_since_the_simulator_started_ +=
  realtime_factor / frame_rate_`. wall 경과와 무관하다.
- **제어기를 기다리지 않는다** — ego 상태 갱신이 `spinSome()` 비블로킹이라 최신 수신 명령만 읽는다.
  프레임당 제어 명령의 수·시점이 실행마다 흔들린다.
- **판정: 그대로는 L3 불가.** 단 개조 비용은 낮다 — 시간 모델이 이미 논리적이고, frame 진행이
  `updateFrame()` 함수 하나로 캡슐화돼 있으며, 이미 동기 req/resp(ZeroMQ) 구조다.
- **필요한 것**: wall 타이머를 대체해 `TickGrant` 수신 → `updateFrame()` → `TickComplete` 로 응답하는
  구동부. (i) 포크/패치 또는 (ii) 상류에 "external clock source" 기여. **우리 IDL·barrier 변경 0**
  (참가자 집합만 확장).
- ⚠ **Autoware 는 별개다.** 시뮬레이터를 barrier 에 넣어도 Autoware 는 비동기로 남는다 →
  목표를 **B-det**(vECU+SS_v2, L3)와 **B-full**(Autoware 포함, 폐루프 동작 검증)로 분리해야 한다.

### BR-S-4 · 단위·매핑 — MORAI 보다 쉽다
Autoware 네이티브가 rad · m/s² 라 정규 계약과 **거의 1:1**. BR-M-1 의 deg→rad · km/h→m/s
변환(누락 시 ~57.3× 오차)이 **여기엔 없다**. gear enum 매핑 정도만 남는다.

### BR-S-5 · 플랜트 격리 — **구현 완료**
두 루프를 병행하면 golden 을 섞을 위험이 생긴다(시뮬레이터가 다르면 동역학이 달라 값이 다름 →
"REGRESSION" 으로 오독). 가드가 들어갔다:
- `capture_golden --plant ID` → `golden_meta.plant` 기록
- `compare_golden --plant ID` → 불일치 시 **`exit 2`(정책 거부)**, REGRESSION 아님
- `sils-ci.sh --plant ID` 로 두 스테이지에 전달. 미선언 시 경고(기존 golden 호환)
- golden 디렉터리도 분리: `goldens/morai/` · `goldens/scenario_simulator_v2/`

### BR-S-6 · importer 커버리지
- **완료**: `ParameterDeclarations` `$name` 치환(체인·순환·미선언·`${expr}` 모두 명시 처리).
  Editor 산출물은 파라미터화돼 있어 이것 없이는 대부분의 조건이 unmapped 로 탈락했다.
- **남음**: `ReachPosition`/`Collision`/`TTC` 매핑 — 선행 조건이 **두 겹**이다:
  1. 트레이스에 position 신호가 없다 → **trace schema v1 변경**([trace-schema](sils-trace-schema.md))
  2. 신호가 생겨도 **닫힌 루프가 아니면 판정에 의미가 없다** — 재생본은 내 제어에 반응하지
     않으므로 결과형 조건은 ECU 가 아니라 녹화본을 판정한다
     ([simulator-loops §2](sils-simulator-loops.md))

  → 그래서 이 항목은 BR-S-2/BR-S-3(실 루프)이 서기 전에는 **혼자 끝낼 수 없다**.

### BR-S-7 · 왜 rosbag 재생으로 대체할 수 없나 (범위 확정용)
"시나리오를 rosbag 으로 돌리면 되지 않나" 는 자주 나오는 질문이고, **자극 재생만 보면 맞다** —
`ReplayPlant`/`input_mode: replay` 로 이미 지원한다. 다만 재생은 **열린 루프**라 `.xosc`
StopTrigger 의 결과형 조건을 판정할 수 없고, `evaluate.py` 가 이를 코드로 강제한다
(`mode=="replay"` → `closed_loop=false` → **L3 승급 불가**). 둘은 대체재가 아니라 층이 다르다 —
L0/L1·회귀는 재생으로 충분하고, **L3 는 시나리오 실행이 필수**이며, **L4 는 다시 실차 rosbag 이
레퍼런스로 필요**하다. 상세: [simulator-loops §2](sils-simulator-loops.md).

---

## 5. 인수 기준 (Acceptance)

| 레벨 | 확인 방법 |
|---|---|
| **연결** | 브리지 기동 시 vECU가 `state_received`·`target_received` emit(실 MORAI/Autoware 데이터) |
| **폐루프** | `control_published`가 실 상태에 반응 → `gate.py` **FIDELITY PASS**(analyze_vehicle closed_loop=true) |
| **요구 판정** | `.xosc`→`import_openscenario.py`→`gate.py --requirements` → 요구별 pass/fail |
| **결정론(L3)** | (BR-C-1 지원 시) 두 실행 `control_published` bit-동일 |
| **parity(L4)** | (후속) MORAI ground-truth 별도 캡처 + `parity.py` |

**최소 성공 = 연결 + 폐루프 + 요구 판정**. 결정론(L3)은 BR-C-1에 종속.

---

## 6. 열린 결정 (팀이 답할 것)

0. **[BR-S] 루프 순서** — `scenario_simulator_v2`(루프 B)를 **먼저** 붙인다. 오픈소스라 미결을
   자체 해소할 수 있고, L3 기준선이 서면 MORAI(루프 A)를 그 대비로 평가할 수 있다.
   근거·작업목록: [sils-simulator-loops.md](sils-simulator-loops.md).
1. **[BR-C-1] MORAI 스텝 제어 지원 여부** — 결정론 L3의 생사. (MORAI 측 최우선)
2. **[BR-M-2] MORAI 종방향 명령 모드** — 가속도 직결 vs accel/brake 정규화(차량 파라미터 필요).
3. **[BR-A-2] Autoware 상태 피드백 경로** — MORAI 직접 vs 상태 브리지.
4. **메시지 필드·단위 [확인]** — `morai_msgs`/`autoware_*_msgs` 실제 정의로 §2·§3 표 확정.
5. **좌표 프레임 정합** — MORAI map 프레임 ↔ 정규 EgoState map 프레임(원점·축).

---

## 7. 범위 요약

| 버킷 | 요구 | 규모 |
|---|---|---|
| **BR-M** MORAI 브리지 | 상태 IN + 명령 OUT 번역 | 중 (OUT 종방향 모드가 변수) |
| **BR-A** Autoware 브리지 | 타깃 IN(거의 1:1) + 상태 피드백 경로 | 소~중 |
| **BR-C** 공통 | clock 확인 + interop + 오케스트레이션 | 소 (단 BR-C-1은 리스크) |
| ~~계약·vECU·판정·요구·시나리오~~ | — | ✅ 완료 |

**결론**: 두 브리지 + clock 확인 1 + interop/오케스트레이션 2 = #1 요구사항 닫힘. 번역 대상(양쪽 메시지·정규 계약)이 다 확정돼 있어 범위가 명확하며, **BR-C-1(MORAI 스텝 제어)만 먼저 답하면** 나머지는 순수 번역·배관이다.
