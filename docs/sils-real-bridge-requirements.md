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
- **[확인]** MORAI가 외부 스텝/동기 제어(네트워크 sync·`step` API)를 지원하는가?
- **지원 O** → MORAI를 우리 ClockMaster barrier 참가자로 → 실 루프도 **L3 bit-재현 결정론 유지**.
- **지원 X** → MORAI wall-free-run 허용 → 실 루프는 **"동작 검증"(루프 닫힘·안전)까지, 결정론은 포기**(evaluate는 wall=L0, analyze_vehicle 폐루프는 유효). 이 경우 결정론 판정은 fixture 경로에만.
- 이 한 항목이 #1의 성격(검증완료 수준)을 가른다. **가장 먼저 답할 것.**

### BR-C-2 · ROS2 ↔ 우리 DDS interop
- 브리지 = **rclcpp 노드**가 ROS2 측을 받고 **FastDDS 정규 타입**으로 재발행(ROS2 type-hash 회피 — 우리가 택한 방향).
- **도메인 정합**: 정규 DDS는 `VISS_DDS_DOMAIN_ID`(기본 0). ROS2 rmw 도메인과 분리·매핑.
- **QoS**: 상태/명령은 RELIABLE 권장(제어 평면). 계약과 정합.

### BR-C-3 · 실행 오케스트레이션 (MMF로 수렴)
- **SiLS 측 서비스(vECU·브리지·ClockMaster)는 MMF가 기동·관리**(adsw_manager/lifecycle_mgr, speclist) — 경로 B가 이미 vECU로 실증. 브리지를 MMF 서비스로 추가하면 오케스트레이션이 speclist 하나로 수렴. **MORAI·Autoware는 외부 기동**(각자 런처).
- 기동 순서(MORAI → Autoware → 브리지 → vECU → ClockMaster), `run_id` 전파.
- **시나리오 로딩**: `.xosc`를 MORAI 시나리오 러너 / Autoware `scenario_simulator_v2`에 로드. → [(b) importer](sils-functional-verification-interop.md)가 같은 `.xosc`에서 요구를 뽑으므로 **자극과 판정이 한 소스**.
- 트레이스 병합: 브리지/vECU/ClockMaster stdout → merged JSONL → `gate.py`.

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

1. **[BR-C-1] MORAI 스텝 제어 지원 여부** — 결정론 L3의 생사. (최우선)
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
