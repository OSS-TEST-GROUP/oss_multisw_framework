# 통합 토폴로지 — 시뮬레이터 · Autoware · MMF · SiLS 가 어떻게 붙나

> 작성 2026-08-10 · 근거: [evidence/adapi-real-partition-2026-08-10.md](evidence/adapi-real-partition-2026-08-10.md)
> 이 문서는 **배선도**다. 3계단 진행 순서는 [sils-stage3-autoware-partition.md](sils-stage3-autoware-partition.md),
> 사다리·백로그는 [sils-simulator-loops.md](sils-simulator-loops.md).

실제 배포는 네 덩어리다. 어떤 브리지가 **두 구성 모두에 필요**하고, 어떤 것이 **지금뿐인
비계(scaffold)** 이며, **무엇이 아직 없는지**를 한자리에 적어둔다. 이걸 안 적어두면 지금
있는 것들이 왜 있는지 다음 사람이 알 수 없다.

**먼저 읽을 곳** — 질문별로:

| 궁금한 것 | 절 |
|---|---|
| **어떤 구성에서 어떤 어댑터를 켜고 끄나 (매트릭스)** | [**별 문서: 구성 지도**](sils-configuration-map.md) ★★ |
| **컨테이너를 실제로 띄우는 명령 (복붙)** | [**별 문서: 도커 런북**](sils-docker-runbook.md) ★★ |
| 우리 브리지는 왜 둘인가 / Autoware 는 차량 상태를 어디서 받나 | **§3** `vehicle_interface` 자리 ★ |
| 무엇이 구현됐고 무엇이 없나 | **§4** 브리지 인벤토리 ★ |
| **어떤 토픽·타입이 버스에 있나** | **§5** 채널 목록 ★ |
| **fixture · SS_v2 · MORAI · Web.Auto 는 서로 무엇인가** | **§1.1** 용어 ★ |
| **TIER IV 자신은 어떻게 구성하나 / 우리는 어디서 갈리나** | [별 문서](sils-tieriv-evaluator-review.md) ★ |
| MORAI 와 Web.Auto 는 뭐가 다른가 | **§6** |
| VISS 는 이 그림에서 어디인가 | **§2** |
| 왜 서비스를 안 쓰는 배선인가 | **§8-(1)(2)** |

---

## 1. 네 구성요소, 두 구성

| | 역할 | 실체 |
|---|---|---|
| **시뮬레이터(플랜트)** | 차를 물리적으로 굴린다 | **MORAI** 또는 **SS_v2** (+ 시험용 스텁 `sils_ego_source`) — §1.1 |
| **Autoware** | 자율주행 스택 | KATECH 파티션 3종 (Cyclone DDS) |
| **MMF** | 차량 플랫폼 — VISS/VSS 로 차량 데이터 서빙 | `service/core/*` (`viss-server` 등) |
| **SiLS** | 검증 하네스 — vECU + 어댑터 + trace/gate | `service/sils/*`, `adapters/*` |

구성은 플랜트만 갈린다:

```
(A)  MORAI      + Autoware(partition) + MMF + SiLS
(B)  Web.Auto   + Autoware(partition) + MMF + SiLS
```

**그래서 플랜트에 의존하는 부품과 아닌 부품을 가르는 것이 이 문서의 핵심이다.**

---

## 1.1 용어 — 플랜트 자리에 들어가는 것은 셋, 그리고 **Web.Auto 는 그 셋이 아니다**

이 문서와 형제 문서들이 `Web.Auto (SS_v2)` 처럼 붙여 쓰는 곳이 많아서, 넷이 같은 층의
부품처럼 읽힌다. 아니다. **실제로 교체되는 것은 셋이고, 넷째는 층이 다르다.**

```
                        Autoware 파티션 3종
                              ▲   ▼
        ╔═════════════════════════════════════════════╗
        ║  aw_vehicle_status_bridge                   ║   ← 여기까지 플랜트와 무관하게 동일
        ║  aw_control_target_bridge                   ║
        ║  vECU (검증 대상)                             ║
        ╚═════════════════════════════════════════════╝
              ▲ EgoState          ▼ VehicleControl        ← 정준 계약
        ┌──────────────── 플랜트 자리 ────────────────┐
        │   ① sils_ego_source   ② SS_v2   ③ MORAI    │   ← 이 셋이 서로 교체된다
        └────────────────────────────────────────────┘
```

| | 정체 | 물리·지도 | 명령을 받나 | 쓰는 이유 |
|---|---|---|---|---|
| **① `sils_ego_source`** (fixture) | **시뮬레이터가 아니다.** 정준 `EgoState` 를 뿜는 시험용 스텁 | ❌ 없음, 직진만 | ❌ **무시한다 (개루프)** | 플랜트 변수를 0으로 두고 위쪽 배선만 검증할 때 |
| **② SS_v2** (`scenario_simulator_v2`) | TIER IV 오픈소스 **시나리오 러너**. 차량 물리는 `simple_sensor_simulator` 의 축약 모델 | ✅ (축약) | ⚠ EgoEntity 일 때만 (→ concealer) | MORAI 대기 중의 대역 |
| **③ MORAI** | 상용 시뮬레이터 | ✅ | ✅ 어댑터가 적용 | **본선**(구성 A). 벤더 대기 |
| (④ AWSIM) | **TIER IV 자신의 구성에서 플랜트인 것.** Unity 바이너리 | ✅ | ✅ | 우리는 아직 안 쓴다 — [Evaluator 검토](sils-tieriv-evaluator-review.md) |

> ★ **② 는 TIER IV 자신의 구성에서 플랜트가 아니다.** 상류는 **AWSIM + SS_v2** 조합을
> 전제하고, AWSIM 이 차량 물리·센서를 맡고 SS_v2 는 시나리오를 돌리며 **Autoware 를 띄운다.**
> 우리가 SS_v2 를 플랜트로 쓰는 것은 축약 경로이고, 2026-08-11 에 만난 벽들(차량 모델이
> EgoEntity 전용 · concealer 가 AD API 초기화를 요구)이 그 결과다.
> → [`sils-tieriv-evaluator-review.md`](sils-tieriv-evaluator-review.md)

### ★★ 기성 시뮬레이터는 전부 자기 `vehicle_interface` 를 갖고 온다

우리 구조의 핵심은 **그 자리에 vECU 를 앉히는 것**(§3)이다. 그런데:

| 플랜트 | 그 자리에 이미 있는 것 | vECU 가 들어가려면 |
|---|---|---|
| SS_v2 (`isEgo` **있음**) | `concealer` | 우회 필요 — **08-12 실측으로 확인**. 우리 브리지 셋을 꺼야 돈다 |
| SS_v2 (`isEgo` **없음**) | **없다** | **자리가 비어 있다** — 08-11 에 AUTONOMOUS 까지 도달 |
| AWSIM | AWSIM 자체 (미확인) | 같은 문제 반복 예상 |
| **MORAI** | **없다 — 어댑터를 우리가 쓴다** | **자리가 비어 있다** |

**이것이 MORAI 가 본선인 진짜 이유다** — 편의가 아니라 구조적 이유다. 어댑터를 우리가 쓰는
유일한 플랜트라서 seat 를 우리가 소유한다.

### ★ Web.Auto 는 플랜트가 아니다 — 층이 다르다

**Web.Auto** 는 TIER IV 의 **클라우드 플랫폼**이고, 그 안의 **Scenario Editor** 로 시나리오
파일을 **저작**한다. 저작한 파일을 **실행하는 것**이 ② SS_v2 다.

```
Web.Auto ──(시나리오 저작)──▶ .xosc / .yaml ──(실행)──▶ SS_v2
 플랫폼                          파일                   시뮬레이터
 (런타임 부품 아님)                                      (플랜트)
```

[`scenarios/ss2/README.md`](../../scenarios/ss2/README.md) 가 명시한다 — **Web.Auto 계정은
브링업에 필요 없다.** 계정이 필요한 것은 저작뿐이고, 실행·판정은 계정 없이 된다. 지금까지의
모든 실측은 **상류에 동봉된 시나리오**로 했고 Web.Auto 를 쓴 적이 없다.

> 그래서 "구성 B = Web.Auto 구성" 은 **"TIER IV 시나리오 기반 구성"** 이라는 뜻이고,
> 그 구성의 **플랜트는 SS_v2** 다. 문장에서 `Web.Auto` 를 보면 **저작 출처**를 가리키는
> 것인지 **플랜트**를 가리키는 것인지 구분해서 읽을 것 — 후자라면 SS_v2 를 뜻한다.

### 셋을 섞어 읽으면 결론이 틀어진다 (실제로 겪었다)

2026-08-11 의 두 실측은 **플랜트가 달라서 결론이 다르다.** 한 런으로 착각하면
"Autoware 가 SS_v2 로 자율주행했다" 는 잘못된 요약이 나온다:

| | 플랜트 | 좌석 주인 | Autoware | vECU | 근거 |
|---|---|---|---|---|---|
| A 런 (08-11) | **① fixture** | 우리 브리지 | ✅ 경로 SET → AUTONOMOUS | ✅ 출력 검증됨 | [evidence §1](evidence/autoware-return-path-2026-08-11.md) |
| B 런 (08-11) | **② SS_v2** `isEgo` | `concealer` | ❌ 경로 미설정 (concealer 에 막힘) | (safe-stop 값만) | [evidence §6](evidence/autoware-return-path-2026-08-11.md) |
| C 런 (08-11) | **② SS_v2** `isEgo` 없음 | **우리 브리지** | ✅ AUTONOMOUS (목표 좌표만 미해결) | ✅ | [evidence §11](evidence/autoware-return-path-2026-08-11.md) |
| **D 런 (08-12)** | **② SS_v2** `isEgo` | `concealer` | ✅ **주행 + 시나리오 Passed** | 관측자 | [evidence](evidence/ss2-autoware-driving-2026-08-12.md) |

**A·B 를 한 런으로 착각하면 "Autoware 가 SS_v2 로 자율주행했다" 는 잘못된 요약이 나온다.**
그것이 실제로 성립한 것은 **D 런(08-12)** 이고, 대가로 **좌석을 concealer 에게 내줬다** —
vECU 가 제어 경로 밖으로 나갔다. 좌석을 우리가 쥔 채 주행하려면 C 런 계열이고,
③ MORAI 에서는 concealer 가 없으므로 이 선택 자체가 없다.
구성별 차이는 [`sils-configuration-map.md`](sils-configuration-map.md) 에 표로 있다.

---

## 2. 데이터 평면은 둘이고, 섞으면 안 된다

| 평면 | 무엇이 흐르나 | 매체 | 소비자 |
|---|---|---|---|
| **관측** | 차량 상태·인지·진단 | **VSS 경로 + VISS** (JSON 문자열, 구독 기반) | 앱·HMI·진단·텔레매틱스·판정 |
| **제어** | 목표·명령·상태 되먹임 | **캐노니컬 DDS** (`EgoState`/`ControlTarget`/`VehicleControl`, SI 단위, 바이너리) | vECU 제어 루프 |

이 분리는 코드에 이미 반영돼 있다. MMF `DDSProvider` 가 구독하는 Autoware 토픽 20개에
**`/control/command/control_cmd` 는 없다** — 관측만 읽는다. 반대로 SiLS 캐노니컬 계약은
`vehicle_info_mgr` 어디에서도 참조되지 않는다.

두 세계가 만나는 접점은 **딱 하나**: `/VIN/Vehicle` 토픽의 `VissRequestTopic` 타입.
MMF `dds_transport.cpp` 와 SiLS `viss_wire.hpp` 가 이것만 공유하고, 도메인 환경변수도
`VISS_DDS_DOMAIN_ID` 로 같다. 그래서 vECU 의 `SILS_WIRE` 기본값이 `viss` 다.

> ⚠ **단위가 다르다.** 같은 소스에서 나온 같은 필드가 평면마다 다른 단위로 표현된다 —
> VSS 는 `Vehicle.Speed` **km/h**, `AngularVelocity.*` **deg/s**, `SteeringAngle` **degrees**.
> 캐노니컬은 전부 SI(m/s, rad/s, rad). **한쪽 변환 코드를 다른 쪽에 복사하면 안 된다.**

### ★ VISS 는 Autoware 의 하류다 — 섞으면 안 되는 게 아니라 섞을 수가 없다

관측 평면은 **완전한 일방통행**이다. Autoware 에 VISS 입력 경로는 존재하지 않는다.

```
   Autoware ──ADAPI 20 토픽(rt/...)──▶ MMF DDSProvider ──▶ VSS 트리 ──▶ VISS 클라이언트
      ▲                                                                  (앱·HMI·진단)
      └──────────────────────── ✗ 경로 없음 ───────────────────────────────
```

근거 두 가지:

| | |
|---|---|
| MMF → Autoware 쓰기 없음 | `DDSProvider::setValue` 는 일반 `VehicleDataTopic{path,value}` 하나만 쓴다. 코드 주석이 `"only for subscribing … no-op for now"` 라고 못박고 있다 |
| Autoware 쪽 수신자 없음 | `VehicleDataTopic` 은 우리 IDL 이다. Autoware 노드 중 이를 아는 것이 없다 |

**그래서 역브리지가 ROS 토픽을 발행한다.** VSS 경로를 아무리 채워도 Autoware 는 읽지
않는다 — 읽을 줄 모른다. VISS 로 Autoware 에 상태를 넣는 선택지는 애초에 없다.

---

## 3. `vehicle_interface` 자리 — 우리가 들어가는 곳 ★

브리지가 왜 **둘**인지, 그리고 왜 **그 둘뿐**인지는 이 자리를 이해해야 풀린다.

### Autoware 가 차량과 주고받는 것은 딱 두 갈래다

```
              ┌──────────────── Autoware ────────────────┐
              │   Perception ─▶ Planning ─▶ Control      │
              └───┬──────────────────────────────▲───────┘
                  │                              │
    /control/command/control_cmd          /vehicle/status/*
    (가속도 m/s², 조향각 rad, 기어)        (속도·조향·기어·control_mode …)
                  │                              │
                  ▼                              │
        ╔═════════════════════════════════════════════════╗
        ║           vehicle_interface 자리 (VCU)           ║
        ║   상류가 붙인 이름:  <!-- Vcu emulation -->       ║
        ╚═════════════════════════════════════════════════╝
```

**양방향이다.** 상태를 올려주는 것만이 아니라 명령을 받아 가는 것도 이 자리의 일이다.
그래서 우리 브리지가 두 개이고, 그 둘이 **한 자리의 양쪽 반쪽**이다.

### 그 자리에 앉을 수 있는 것은 셋

| 구성 | 차량 물리 | `/vehicle/status/*` 생산 | `control_cmd` 소비 |
|---|---|---|---|
| **실차** | 진짜 차 | CAN 드라이버 (`vehicle_interface`) | 같은 드라이버 |
| **플래닝 시뮬레이터** (지금 파티션) | `simple_planning_simulator` | **같은 노드** | 같은 노드 |
| **우리 목표** | MORAI / Web.Auto | `aw_vehicle_status_bridge` | `aw_control_target_bridge` → **vECU** |

> ⚠ 이 이미지의 `sample_vehicle_launch/vehicle_interface.launch.xml` 은 **텅 비어 있다**
> (`<arg name="vehicle_id"/>` 한 줄이 전부). 샘플 차량이라 CAN 드라이버가 없다.
> 즉 `launch_vehicle_interface:=true` 로 켜도 이 이미지에서는 **아무것도 안 뜬다.**

### 자리를 비우는 스위치 — 이미 상류에 있다

`obigo_launch/launch/planning_simulator.launch.xml`:

```xml
<!-- Vcu emulation -->
<arg name="vehicle_simulation" default="true" description="use vehicle simulation"/>
...
<let name="launch_vehicle_interface" value="false" if="$(var vehicle_simulation)"/>
<let name="launch_vehicle_interface" value="true"  unless="$(var vehicle_simulation)"/>

<let name="launch_dummy_vehicle" value="false" if="$(var scenario_simulation)"/>
<let name="launch_dummy_vehicle" value="$(var launch_simulator_dummy_vehicle)"
                                 unless="$(var scenario_simulation)"/>
<let name="launch_scenario_simulator_v2_adapter" value="$(var scenario_simulation)"/>
```

읽는 법:

| 인자 | 켜면 | 우리가 원하는 값 |
|---|---|---|
| `launch_simulator_dummy_vehicle` | `simple_planning_simulator` 를 띄운다 | **`false`** — 자리를 비운다 |
| `vehicle_simulation` | `launch_vehicle_interface` 를 **끈다** | **`true`**(기본) — Autoware 소유 인터페이스도 안 띄운다 |
| `scenario_simulation` | dummy vehicle 대신 **SS_v2 어댑터**를 띄우고 `use_sim_time`·전 모듈 auto mode 를 켠다 | Web.Auto 구성이라면 검토 대상 |

`vehicle_simulation:=false` 로 가면 Autoware 가 **자기 인터페이스**를 띄우려 한다 — 우리가
원하는 게 아니다(그리고 이 이미지에서는 빈 껍데기다). 원하는 조합은

```
launch_simulator_dummy_vehicle:=false    # 더미 차량 끔 → 자리가 빈다
vehicle_simulation:=true                 # Autoware 소유 인터페이스도 안 띄움
```

⚠ 그런데 `sample_adsw_control_run.launch.xml` 이 이 값들을 **하드코딩**한다
(`is_launch_simulator=true`, `launch_simulator_dummy_vehicle=true`, `initial_engage_state=true`).
인자로 넘기려면 그 파일을 고치거나 `planning_simulator.launch.xml` 을 직접 부르는 수밖에
없고, `obigo_launch` 는 **상류(`katech-autoware`) 저장소**다 — §7 "공유 파일" 과 같은 성격의
문제다.

### 파티션이 시뮬레이터 책임을 나눠 갖고 있다

| 파티션 | `launch_simulator_dummy_perception` | `launch_simulator_dummy_vehicle` |
|---|---|---|
| perception | **true** | false |
| control | false | **true** |

**지금 3계단 구성 전체가 센서 없이 돈다.** 인지는 `dummy_perception` 이 객체를 만들어 주고,
차량은 `simple_planning_simulator` 가 굴린다. 실제 lidar/camera 파이프라인은 안 탄다.
이것이 MORAI 구성에서 갈리는 지점이다 → §6.

### 절단면을 어디에 둘 것인가 — 선택했다

`autoware_raw_vehicle_cmd_converter` 가 control 파티션에 있다. 실차에서는 이것이 Autoware 의
SI 명령을 **raw 액추에이션**(페달 %, 조향휠 각)으로 바꿔 CAN 드라이버에 넘긴다. 즉 자를 수
있는 지점이 둘이다:

| | 절단면 | 우리가 다루는 값 | 대가 |
|---|---|---|---|
| **(a) 채택** | `control_cmd` 레벨 | 가속도 m/s², 노면차륜각 rad — **SI** | 없음. MORAI `CtrlCmd` 가 그대로 받는다 |
| (b) | raw 액추에이션 레벨 | 페달 %, 조향휠 각 | **차량별 페달 맵**을 우리가 떠안는다 |

(a) 를 골랐다. MORAI 는 `longl_cmd_type=acceleration` + `front_steer`(rad) 를 받으므로 변환
없이 연결되고, (b) 로 가면 차량 파라미터가 우리 책임이 된다.

---

## 4. 브리지 인벤토리 ★ 이 문서의 본론

```
        ┌──────────── Autoware 파티션 (Cyclone) ────────────┐
        │   Perception ─▶ Decision ─▶ Control + AD API      │
        └──┬──────────────────────────────────────▲─────────┘
  control_cmd│                                    │ /vehicle/status/*
             ▼                                    │
   [aw_control_target_bridge] ✅      [aw_vehicle_status_bridge] ✅
             │ ControlTarget                      │
             ▼                                    │
        ┌────────────────── vECU (VCU) ───────────┴────┐
        │  ControlTarget in                            │
        │  EgoState in  ◀── 플랜트 어댑터 ──────────────│
        │  VehicleControl out ─────────────▶ 플랜트     │
        └──────────────────────────────────────────────┘
                            ▲
              ┌─────────────┴─────────────┐
        (A) MORAI 어댑터 ❌         (B) ss2_ego_bridge ✅
```

| 부품 | 구성 A (MORAI) | 구성 B (Web.Auto) | 상태 |
|---|---|---|---|
| [`aw_control_target_bridge`](../../adapters/ss2/src/aw_control_target_bridge.cpp) | ✅ 필요 | ✅ 필요 | **구현·실측 완료 (08-10)** |
| [`ss2_ego_bridge`](../../adapters/ss2/src/ss2_ego_bridge.cpp) | — | ✅ EgoState 담당 | 구현됨 |
| [`ss2_control_bridge`](../../adapters/ss2/src/ss2_control_bridge.cpp) | — | ✅ 플랜트로 명령 | 구현됨 |
| **MORAI 어댑터** | ✅ EgoState + 명령 담당 | — | ❌ **미구현** (설계 문서만) |
| [**`aw_vehicle_status_bridge`**](../../adapters/ss2/src/aw_vehicle_status_bridge.cpp) | ✅ 필요 | ✅ 필요 | **구현·실측 완료 (08-10)** |
| [**`aw_localization_bridge`**](../../adapters/ss2/src/aw_localization_bridge.cpp) | ✅ 필요 | ✅ 필요 | **구현·실측 완료 (08-11)** — 단 `planning_simulation` 모드 전용 |
| [`aw_ego_state_bridge`](../../adapters/ss2/src/aw_ego_state_bridge.cpp) | ❌ **쓰면 안 됨** | ❌ **쓰면 안 됨** | 🟡 **비계 전용** |

### ★★ 위 표는 "구성 A/B" 기준이다 — 실제 런은 넷이고, 켜고 끄는 게 다르다

**여섯이 한 번도 같이 돈 적이 없다.** 좌석(`vehicle_interface`) 주인이 구성마다 바뀌기
때문이다. 특히 **SS_v2 를 `isEgo` 로 띄우면 `concealer` 가 좌석을 가져가므로
`aw_vehicle_status_bridge` · `aw_localization_bridge` · `ss2_control_bridge` 셋을 꺼야
한다** — 켜면 같은 토픽에 라이터가 둘이 되고 **아무도 오류를 안 낸다**
(실측: [`evidence/ss2-autoware-driving-2026-08-12.md`](evidence/ss2-autoware-driving-2026-08-12.md)).

구성별 ON/OFF 매트릭스·플로우 그림·실행 명령은 **별 문서로 분리했다**:
→ [**`sils-configuration-map.md`**](sils-configuration-map.md) ★

> 그 셋이 "불필요해진" 것이 아니다. **그 구성에서 자리가 없어진 것**이고, MORAI 구성
> (concealer 가 없다)에서는 다시 필수다. 이것이 §1.1 의 "기성 시뮬레이터는 전부 자기
> `vehicle_interface` 를 갖고 온다" 가 예고한 바로 그 상황이다.

### ⚠ `aw_ego_state_bridge` 는 비계다 — 실제 구성에 넣지 말 것

Autoware ADAPI 에서 `EgoState` 를 만드는 브리지다. **두 실제 구성 어디에도 들어가지 않는다.**

- 실제 구성에서 ego 상태는 **플랜트가** 낸다. Autoware 는 차량 상태의 *소비자*이지
  진리의 생산자가 아니다.
- 이 브리지가 읽는 값은 Autoware 자신의 `simple_planning_simulator` 가 **자기 자신을
  보고하는 것**이다. 여기서 먹인 ECU 는 자기가 읽는 값에 영향을 줄 수 없다 →
  **무슨 짓을 해도 열린 루프**다.
- 실제 구성에 섞여 들어가면 **`sils/ego/state` 에 퍼블리셔가 둘**이 된다. DDS 는 막지
  않고, vECU 는 서로 다른 차량의 상태를 표시 없이 번갈아 읽는다. **에러도 경고도 없다.**

존재 이유는 하나 — 플랜트가 아직 없는 지금(MORAI 벤더 대기, SS_v2 는 concealer 블로커)
개발자 ECU 를 **진짜 Autoware 에 물려 시험**하기 위해서다. 그 목적이 끝나면 지운다.

### 시뮬레이터를 끄면 무엇이 사라지나 — 여섯이 아니라 **열** ★

실제 구성에서는 Autoware 의 `simple_planning_simulator` 를 **끄고** MORAI/SS_v2 를
플랜트로 쓴다. 오래 "그러면 `/vehicle/status/*` 여섯 개가 빈다"고 적어뒀는데,
**틀렸다.** 그 시뮬레이터는 열 개를 발행하고, 큰 쪽은 차량 상태가 아니었다.

`simple_planning_simulator.launch.py` 의 리매핑. 이 파티션은 `localization_sim_mode=api`
라서 **`motion_publish_mode=full_motion`** 경로를 탄다:

```
     시뮬레이터 출력          실제 토픽                              구독자(실측 08-11)
  ┌────────────────────┬──────────────────────────────────────┬──────────────────┐
  │ output/twist       │ /vehicle/status/velocity_status      │   2              │
  │ output/steering    │ /vehicle/status/steering_status      │   5              │
  │ output/gear_report │ /vehicle/status/gear_status          │   2   aw_vehicle │
  │ output/control_mode│ /vehicle/status/control_mode         │   3   _status_   │
  │ output/turn_ind.   │ /vehicle/status/turn_indicators_status│   1   bridge     │
  │ output/hazard      │ /vehicle/status/hazard_lights_status │   1              │
  │                    │                              소계 ───┤  14              │
  ├────────────────────┼──────────────────────────────────────┼──────────────────┤
  │ output/odometry    │ /localization/kinematic_state        │  28   aw_        │
  │ output/acceleration│ /localization/acceleration           │  10   localization│
  │                    │                              소계 ───┤  38   _bridge ★  │
  ├────────────────────┼──────────────────────────────────────┼──────────────────┤
  │ output/actuation   │ /vehicle/status/actuation_status     │   0   구독자 없음 │
  │ output/imu         │ /sensing/imu/imu_data                │   1   AEB, 안 막힘│
  └────────────────────┴──────────────────────────────────────┴──────────────────┘
                                                        합계   53
```

**`/localization/*` 쪽이 2.7배 크다.** 그리고 결정타 —
`obigo_launch/launch/planning_simulator.launch.xml` 이 못박고 있다:

```xml
<arg name="launch_localization" value="false"/>
```

**Autoware 자체 측위가 꺼져 있으므로, 시뮬레이터를 끄면 자차 위치를 말해줄 것이 하나도
남지 않는다.** 그래서 좌석의 귀환 경로는 브리지 **둘**이다:

| 브리지 | 채우는 것 | 구독 |
|---|---|---|
| [`aw_vehicle_status_bridge`](../../adapters/ss2/src/aw_vehicle_status_bridge.cpp) | `/vehicle/status/*` 6개 | 14 |
| [`aw_localization_bridge`](../../adapters/ss2/src/aw_localization_bridge.cpp) | `/localization/*` 2개 + **`/initialpose3d`** | 38 |

**이 둘이 `aw_control_target_bridge` 의 짝이다.**

> `/initialpose3d` 는 포즈 입력처럼 보이지만 이 파티션에서는 **불리언 래치**다 — 유일한
> 소비자인 벤더 shim(`planning_sim_localization_state.py`)이 내용을 버리고
> `/localization/initialization_state` 만 INITIALIZED 로 바꾼다. 그게 autonomous 게이트라
> 이것 없이는 다른 진단이 다 통과해도 모드가 안 열린다. `aw_localization_bridge` 가
> 1 Hz 로 발행한다(`publish_initialpose`, 기본 참). 근거·주기 이유:
> [`evidence/autoware-return-path-2026-08-11.md` §10](evidence/autoware-return-path-2026-08-11.md).

> ⚠ `aw_localization_bridge` 는 **`planning_simulation` 모드 전용**이다. 실센서 측위
> (`launch_localization=true`)를 켜면 Autoware 가 자기 pose 를 추정하므로 이 노드는
> **꺼야 한다** — 안 끄면 `/localization/kinematic_state` 에 라이터가 둘이 되고,
> 어느 쪽을 믿을지는 발행 순서가 정한다. **아무도 충돌을 보고하지 않는다.**

### ✅ 구현·검증 완료 (2026-08-10)

[`aw_vehicle_status_bridge`](../../adapters/ss2/src/aw_vehicle_status_bridge.cpp) —
캐노니컬 `EgoState` → 위 6개 토픽.

| 확인 | 결과 |
|---|---|
| 브리지 카운터 | `received=355 published=355 bad_gear=0` |
| Autoware 가 본 퍼블리셔 수 | 6개 토픽 전부 **1 → 2** |
| **실제 도달** | `frame_id` 를 `SILS_BRIDGE` 로 바꿔 발행 → Autoware 수신 스트림 30개 중 **3개가 `SILS_BRIDGE`** |

마지막 줄이 판정이다. 퍼블리셔 수가 늘어난 것은 discovery 까지만 증명하고,
**남의 프로세스가 우리 값을 실제로 읽었다**는 것은 구분 가능한 표식으로만 증명된다.
3:27 비율은 발행율 차이다(시뮬레이터 ~30 Hz vs 우리 10 Hz — ADAPI kinematics 속도).

⚠ 이 시험은 `simple_planning_simulator` 를 **켠 채로** 했으므로 한 토픽에 퍼블리셔가
둘이었다. 그 진짜 시험은 아래에서 했다.

### ✅ 시뮬레이터를 끄고 두 브리지만으로 (2026-08-11)

전체 기록: [`evidence/vehicle-seat-no-simulator-2026-08-11.md`](evidence/vehicle-seat-no-simulator-2026-08-11.md)

시뮬레이터를 끈 제어 파티션에 `aw_vehicle_status_bridge` + `aw_localization_bridge` 만
투입. EgoState 공급은 열린 루프 픽스처 `bin/sils_ego_source`(50 Hz).

| 확인 | 결과 |
|---|---|
| 여덟 토픽의 퍼블리셔 | 전부 `pubs=1`, **발행자가 우리 브리지** |
| QoS | 여덟 개 전부 시뮬레이터의 것과 **정확히 일치** (RELIABLE·KEEP_LAST(1)·VOLATILE) |
| Autoware 수신율 | `/localization/kinematic_state` **48.7 Hz** |
| 브리지 유실 | `4116/4116`, `4125/4125` — **0** |
| `waiting for current odometry` | ✅ **멈춤** |
| `waiting for current_twist msg` | ✅ **멈춤** |
| `[AEB] waiting for ego velocity` | ✅ **멈춤** |

**두 항목이 이 시험으로 해소됐다:**

- **IMU 는 제어를 막지 않는다.** AEB 는 ego velocity 를 받자 IMU 를 요구하지 않고
  `waiting for object pointcloud message` 로 넘어갔다 — 인지 쪽이고, 벤더 구성에서도
  `launch_simulator_dummy_perception=false` 라 원래 비어 있다.
- **`actuation_status` 는 구독자 0.** `raw_vehicle_cmd_converter` 만 읽는데
  `sample_vehicle` 이 `DELAY_STEER_ACC_GEARED` 라 그 노드가 안 뜬다. 실측 확인.

⚠ **증명하지 않은 것 — Autoware 가 주행하지는 않았다.** 픽스처가 맵 원점(0,0,0)에서
출발하는데 그 좌표는 lanelet 맵 위가 아니라, 경로가 안 서고 궤적이 없어
`Control is skipped since input data is not ready` 가 계속된다. **브리지 결함이 아니라
픽스처의 한계**이고, 주행 증명에는 맵 좌표를 아는 진짜 플랜트가 필요하다.

재현: `tools/sils/ss2/run_control_partition_sils.sh` → `run_vehicle_seat_bridges.sh --with-ego-source`

---

## 5. 채널 목록 — 버스 위에 실제로 무엇이 있나 ★

절마다 흩어진 토픽을 한자리에 모은다. **QoS 는 측정값이고, 안 잰 것은 그렇게 적었다.**

### SiLS 캐노니컬 (도메인 = `VISS_DDS_DOMAIN_ID`, 실험에서 42)

네이티브 Fast DDS. ROS 맹글링(`rt/…`)이 없으므로 ROS 노드와 직접 만나지 않는다.

| 토픽 | 타입 (IDL) | 발행 | 구독 |
|---|---|---|---|
| `sils/ego/state` | `EgoState` | **플랜트 어댑터** — `ss2_ego_bridge` ✅ / MORAI 어댑터 ❌ / `aw_ego_state_bridge` 🟡비계 / `bin/sils_ego_source` 🟡픽스처 | vECU · `aw_vehicle_status_bridge` · `aw_localization_bridge` |
| `autoware/control_target` | `ControlTarget` | `aw_control_target_bridge` | vECU (VCU) |
| `sils/ego/cmd` | `VehicleControl` | vECU — 단일 ECU 구성에서는 **모든 채널맵**(`ego-min` · `plainc-min` · `adaptive-min` · `classic-min` · `vehicle-interface`). **다중 ECU 구성에서는 VCU 만** — 아래 ★ | 플랜트 어댑터 · `ss2_control_bridge` |
| `sils/ego/steer_cmd` | `VehicleControl` | vECU (**`steer-min`** 채널맵만) | 플랜트 어댑터 |
| `sils/ego/cmd_{adaptive,classic,plainc}` | `VehicleControl` | 각 vECU (`config/sils/channelmap/multi/*.yaml`) | **VCU**(아비트레이션 구현 후) · 채널별 verify |
| `/VIN/Vehicle` | `VissRequestTopic` | vECU(VISS wire) ↔ MMF `dds_transport` | 서로 |

QoS 는 전부 **RELIABLE · KEEP_LAST(256) · TRANSIENT_LOCAL**
(`SILS_DURABILITY=volatile` 이면 VOLATILE — **양쪽이 같이 따라가야 한다**).

> **종축 액추에이션 채널은 `sils/ego/cmd` 하나다.** 플랜트 브리지 요구
> ([`sils-real-bridge-requirements.md`](sils-real-bridge-requirements.md) BR-M-2)가
> 이 이름으로 쓰여 있고, `sils_vehicle_verify` 의 기본 구독도 이것이다.
>
> `steer-min.yaml` 만 `sils/ego/steer_cmd` 로 갈리는데, 이건 드리프트가 아니라 **의도**다 —
> 조향 ECU 와 종축 ECU 가 한 버스에 공존할 때 한 토픽에 라이터가 둘 붙는 걸 막는다.
>
> ★ **다중 vECU 구성에서는 이 규칙이 넷으로 확장된다.** 예제 ECU 넷(`adaptive`·`classic`·
> `plainc`·`vehicle-interface`)은 전부 `sils/ego/cmd` 에 쓰도록 돼 있는데, 그것들은 **서로의
> 대안**이지 동시 구성이 아니다. 같이 올리면 라이터가 넷이 되고 승자는 발행 순서가 정한다 —
> **아무도 오류를 안 낸다.** `config/sils/channelmap/multi/` 의 사본이 out 토픽만 갈라놓은
> 것이고, 코드는 안 바뀐다. 실측: [`evidence/multi-vecu-bus-2026-08-12.md`](evidence/multi-vecu-bus-2026-08-12.md).
> 통합 규칙: [`sils-vecu-arbitration.md`](sils-vecu-arbitration.md).
> 액추에이터가 다르면 채널도 다르다.
>
> ⚠ **2026-08-11 이전 트리에는 `ego-min.yaml` 만 `sils/ego/control` 이었다.** vECU 를
> 예제 중 하나로 바꾸면 **아무 데서도 에러 없이** 루프가 열렸다 — 토픽 이름이 어긋난
> 리더와 라이터는 그냥 매칭이 안 될 뿐이다. 지금은 통일했으나, **어댑터 이미지에는 옛
> 기본값이 구워져 있으므로 다음 2계단 런 전에 `docker/sils/build-ss2-adapter.sh` 로
> 재빌드해야 한다**. 재빌드를 빠뜨리면 `ss2_control_bridge` 의 5초 보고가 `matched=0`
> 으로 나오고 `check_closed_loop.sh` 가 그걸 경고한다.

### Autoware ROS 2 (도메인 = `ROS_DOMAIN_ID`, 실험에서 43 · Cyclone)

| 토픽 | 타입 | 방향 | QoS (측정) |
|---|---|---|---|
| `/control/command/control_cmd` | `autoware_control_msgs/Control` | AW → 우리 | RELIABLE · KEEP_LAST(1) · **TRANSIENT_LOCAL** |
| `/control/command/gear_cmd` | `autoware_vehicle_msgs/GearCommand` | AW → 우리 | RELIABLE · KEEP_LAST(1) · **TRANSIENT_LOCAL** (측정 08-11) |
| `/vehicle/status/velocity_status` | `autoware_vehicle_msgs/VelocityReport` | 우리 → AW | RELIABLE · KEEP_LAST(1) · VOLATILE |
| `/vehicle/status/steering_status` | `SteeringReport` | 우리 → AW | 〃 |
| `/vehicle/status/gear_status` | `GearReport` | 우리 → AW | 〃 |
| `/vehicle/status/control_mode` | `ControlModeReport` | 우리 → AW | 〃 |
| `/vehicle/status/turn_indicators_status` | `TurnIndicatorsReport` | 우리 → AW | 〃 |
| `/vehicle/status/hazard_lights_status` | `HazardLightsReport` | 우리 → AW | 〃 |
| `/localization/kinematic_state` | `nav_msgs/Odometry` | 우리 → AW | RELIABLE · KEEP_LAST(1) · VOLATILE — **구독 28** |
| `/localization/acceleration` | `geometry_msgs/AccelWithCovarianceStamped` | 우리 → AW | 〃 — **구독 10** |
| `/vehicle/status/actuation_status` | `tier4_vehicle_msgs/ActuationStatusStamped` | — | **구독 0** — 발행 안 함 (근거 §3) |
| `/sensing/imu/imu_data` | `sensor_msgs/Imu` | — | 구독 1 (AEB). **막지 않음** — 발행 안 함 (근거 §3) |
| `/api/vehicle/kinematics` | `autoware_adapi_v1_msgs/VehicleKinematics` | AW → 우리·MMF | **BEST_EFFORT** · VOLATILE |
| `/api/vehicle/status` | `autoware_adapi_v1_msgs/VehicleStatus` | AW → 우리·MMF | **BEST_EFFORT** · VOLATILE |

기동/조작에 쓰는 것(브리지가 아니라 사람이 쓴다):

| | 용도 |
|---|---|
| `/initialpose3d` (`PoseWithCovarianceStamped`) | 초기 위치. ⚠ `/initialpose` 가 **아니다**. → 2026-08-11 이후 **사람이 안 찍는다**, `aw_localization_bridge` 가 낸다 (위 §3 주석) |
| `/planning/mission_planning/goal` (`PoseStamped`) | 목표 |
| `/api/simulator/set/pose` (`tier4_external_api_msgs/srv/InitializePose`) | 재배치. `deprecated_api` 를 띄워야 존재 |
| `/api/operation_mode/change_to_autonomous` (서비스) | engage |

### 필드 대응 — 캐노니컬 ↔ Autoware

| 캐노니컬 `EgoState` | Autoware 출처 | 변환 |
|---|---|---|
| `position`, `orientation_rpy` | `VehicleKinematics.pose` | 쿼터니언 → RPY |
| `velocity` (body, m/s) | `.twist.twist.linear` | 없음 |
| `acceleration` (body, m/s²) | `.accel.accel.linear` | 없음 |
| `yaw_rate` (rad/s) | `.twist.twist.angular.z` | 없음 |
| `steering_angle` (rad) | `VehicleStatus.steering_tire_angle` | 없음 |
| `gear` (0P/1R/2N/3D) | `VehicleStatus.gear.status` | enum 매핑 |

| 캐노니컬 `ControlTarget` | Autoware 출처 | 변환 |
|---|---|---|
| `steering_angle` (rad) | `Control.lateral.steering_tire_angle` | 없음 |
| `acceleration` (m/s²) | `Control.longitudinal.acceleration` | 없음 |
| `gear` | `GearCommand.command` | enum 매핑 |
| — | `Control.longitudinal.velocity` | **버려진다** (계약이 가속도 전용) |

**ADAPI 는 전부 SI 라 단위 변환이 없다.** MORAI 는 다르다(§5), VSS 도 다르다(§2).

---

## 6. MORAI 구성과 Web.Auto(SS_v2) 구성은 무엇이 다른가

> 용어 주의: 이 절의 "Web.Auto 구성" 은 **플랜트가 SS_v2 인 구성**을 말한다. Web.Auto
> 자체는 런타임 부품이 아니다 — §1.1.

### Autoware 쪽은 **완전히 동일하다**

Autoware 는 플랜트가 누구인지 모른다. 아는 것은 `/vehicle/status/*` 와 `control_cmd` 뿐이다.
그 자리를 채우는 게 CAN 드라이버든 더미 차량이든 우리 브리지든 상관하지 않는다.

```
        (A) MORAI 구성                          (B) Web.Auto 구성
   ┌───────────────────────┐              ┌───────────────────────┐
   │   Autoware 파티션 3종   │              │   Autoware 파티션 3종   │
   └──┬─────────────▲──────┘              └──┬─────────────▲──────┘
      │             │                        │             │
  control_cmd  /vehicle/status/*         control_cmd  /vehicle/status/*
      ▼             │                        ▼             │
 ╔════════════════════════════╗         ╔════════════════════════════╗
 ║ aw_control_target_bridge   ║ 동일    ║ aw_control_target_bridge   ║
 ║ aw_vehicle_status_bridge   ║ ◀────▶  ║ aw_vehicle_status_bridge   ║
 ║          vECU (SUT)        ║         ║          vECU (SUT)        ║
 ╚════════════════════════════╝         ╚════════════════════════════╝
      ▼ VehicleControl │ EgoState            ▼ VehicleControl │ EgoState
 ┌────────────────────────────┐         ┌────────────────────────────┐
 │  MORAI 어댑터  ❌ 미구현     │  다름   │  ss2_ego/control_bridge ✅  │
 │  deg→rad · km/h→m/s        │ ◀────▶  │  (이미 SI — 변환 없음)       │
 └────────────────────────────┘         └────────────────────────────┘
      ▼                                       ▼
    MORAI                                  SS_v2
```

**바뀌는 것은 캐노니컬 계약 아래쪽뿐이다.** 그것이 `EgoState`/`VehicleControl` 을 둔 이유다.

### 플랜트 어댑터에서만 다른 것 둘

| | MORAI | Web.Auto (SS_v2) |
|---|---|---|
| **단위** | `front_steer` **deg**, `velocity` **km/h**, angular **deg/s** → 어댑터가 57.3×·3.6× 변환을 떠안는다 | 이미 SI. 변환 없음 |
| **종방향 모드** | `CtrlCmd.longl_cmd_type` 이 throttle(1)/velocity(2)/**acceleration(3)** 중 선택 → **acceleration 으로 계약에 고정**해야 한다(우리 `VehicleControl` 이 가속도 전용) | 해당 없음 |

⚠ MORAI 는 입력과 출력의 단위가 **엇갈린다** — `CtrlCmd.front_steer` 입력은 **rad** 인데
`EgoVehicleStatus.front_steer` 출력은 **deg** 다. 빠뜨리면 조향 폐루프가 57.3× 틀어지고
**아무 데서도 에러가 안 난다**([통합 리뷰 §296](sils-morai-simulator-integration-review.md)).

### ★ MORAI 에만 생기는 새 축 — 센서/인지

§3 에서 본 대로 지금 구성은 `dummy_perception` 으로 돌아 **센서 파이프라인이 아예 없다.**
MORAI 는 lidar·camera·radar·GPS·IMU 를 실제로 낼 수 있어서 선택지가 생긴다:

| | 무엇 | 대가 |
|---|---|---|
| **(a) 권장** | `dummy_perception` 유지, MORAI 는 **차량 물리만** | 우리 배선 그대로. **vECU 검증에는 이걸로 충분** |
| (b) | MORAI 센서를 Autoware Perception 에 투입 | 풀스택. **vehicle_interface 자리와 무관한 별개 작업** |

(b) 에는 이미 알려진 지뢰가 있다 — MORAI `RadarDetections` 를 `radar_msgs/RadarTrack` 로
이름만 바꿔 넘기는 것은 틀렸다. `RadarTrack` 은 안정적 UUID·velocity·acceleration·size·
classification·covariance 를 요구하는데 detection 에 그것이 없거나 프레임 간 ID 가 불안정하면
detection→track 변환기나 tracker 가 따로 필요하다(리뷰 §210).

**vECU 는 vehicle_interface 층의 ECU 다.** 인지 충실도는 그 판정에 들어가지 않으므로
(a) 로 간다.

### ⚠ 리뷰 문서의 매핑표는 **다른 토폴로지**를 가정한다

[통합 리뷰 §283-296](sils-morai-simulator-integration-review.md) 이 Autoware 메시지 ↔ MORAI
메시지를 **직접** 매핑해 두었다(`SteeringReport` ↔ `EgoVehicleStatus.front_steer` 등).
그것은 **vECU 없이 MORAI 를 Autoware 에 바로 붙이는** 그림이다.

SiLS 목적은 다르다 — vECU 가 그 사이에 **피험체(SUT)로** 들어가야 한다. 그래서 같은
물리량이 두 번 번역된다:

```
   직접 매핑 (리뷰)            vECU 경유 (SiLS)
   MORAI ──▶ Autoware         MORAI ──▶ 캐노니컬 ──▶ vECU ──▶ Autoware
   변환 1회                    변환 2회
   vECU 는 구경꾼               ★ vECU 가 루프 안에 있다
```

비효율로 보이지만 **그 두 번째 번역이 vECU 를 검증 대상으로 만드는 값**이다. 직접 매핑하면
빠르지만 우리가 검증하려는 물건이 경로에서 빠진다.

---

## 7. 도메인 배치 — 제품과 3계단이 다르다

| | Autoware(ROS) | MMF raw DDS | VISS | SiLS 캐노니컬 |
|---|---|---|---|---|
| **제품 (`docker-compose-integration.yaml` `mmf-core`)** | 42 | 42 | 42 | 42 |
| **3계단 실험 (08-10)** | **43** | — | 43 | **42** |

제품은 `ROS_DOMAIN_ID` · `MMF_DDS_DOMAIN_ID` · `VISS_DDS_DOMAIN_ID` 를 **전부 42** 로 둔다.
그리고 MMF `DDSProvider` 는 Autoware 를 읽을 때도 `VISS_DDS_DOMAIN_ID` 를 쓴다 —
즉 **MMF 는 Autoware 와 VISS 가 같은 도메인에 있다고 전제**한다.

3계단은 SiLS 격리를 위해 파티션을 43 으로 옮겨 두었다(2단계). **MMF 를 붙일 때 이 둘을
조율해야 한다** — 파티션을 42 로 되돌리거나, MMF 의 세 도메인을 나눠 주거나.

---

## 8. 실측으로 확정된 제약 3가지

전부 2026-08-10, 실제 KATECH 파티션 대상. 원측정값:
[evidence/adapi-real-partition-2026-08-10.md](evidence/adapi-real-partition-2026-08-10.md)

### (1) 벤더 교차 — 토픽은 넘고 서비스는 못 넘는다

| 계층 | Fast DDS ↔ Cyclone |
|---|---|
| discovery | ✅ 양방향 |
| **토픽 pub/sub** | ✅ **양방향** (`autoware_control_msgs/msg/Control` 33 Hz 포함) |
| **서비스 왕복** | ❌ **실패** (`RTPS_READER_HISTORY` payload 크기 불일치) |

→ **데이터 평면은 RMW 통일 없이 성립한다.** 막히는 것은 `concealer` ↔ AD API 뿐이고,
그건 전부 서비스다.

### (2) 우리 스택을 Cyclone 으로 바꾸면 2계단이 깨진다

`SILS_RMW=rmw_cyclonedds_cpp` 로 전환하니 어댑터가
`ddsi_xt_type_add_typeobj with invalid type object` 후 **segfault**. 어댑터 단독 실행은
멀쩡했으므로 BR-C-2 구조 자체의 문제는 아니고, 원인은 **미상**이다.

→ 위 (1) 과 합치면: **양쪽에서 막혔다.** Fast DDS 로는 서비스가 안 되고, Cyclone 으로는
우리 스택이 죽는다. 그래서 3계단은 **서비스를 안 쓰는 배선**(= concealer 우회)으로 간다.

### (3) QoS 는 측정해야 한다 — 추측하면 조용히 실패한다

같은 날 세 건이 전부 QoS/매칭 문제였다:

| 어디 | 증상 | 원인 |
|---|---|---|
| MMF `DDSProvider` | `/api/vehicle/status` 데이터 0, 그런데 토픽은 발견됨 | RELIABLE 리더 vs BEST_EFFORT 제공자 |
| 우리 브리지 | (예방됨) | 발행 전 `ros2 topic info -v` 로 측정해 맞춤 |
| 맵 마커 조회 | `--once` 가 아무것도 못 받음 | latched(TRANSIENT_LOCAL) 인데 기본 QoS 로 조회 |

**측정한 값** (실제 Control 파티션):

| 토픽 | Reliability | Durability |
|---|---|---|
| `/control/command/control_cmd` | RELIABLE | TRANSIENT_LOCAL |
| `/api/vehicle/kinematics`, `/api/vehicle/status` | **BEST_EFFORT** | VOLATILE |
| `/api/routing/state`, `/api/operation_mode/state` | RELIABLE | TRANSIENT_LOCAL |

---

## 9. MMF 쪽에서 고친 것 (2026-08-10)

`service/core/vehicle_info_mgr` — 셋 다 실측으로 확인 후 수정하고 재실측으로 검증.

| # | 내용 | 증상 |
|---|---|---|
| 1 | `rt/api/vehicle/status` 의 `best_effort` `false`→`true` | 위 (3). 이 메시지가 통째로 안 와서 **기어·조향각이 둘 다 비어 있었다** |
| 2 | `handleVehicleStatus()` 에 조향각 push 추가 (rad→deg) | `VehicleStatus.steering_tire_angle` 은 원래 오고 있었고 VSS 로 안 내보내던 것. 받을 자리(`Vehicle.Chassis.Axle.Row1.SteeringAngle`)도 이미 있었다 |
| 3 | `idl/autoware/{NavSatFix,GeoPoint,GnssInsOrientation}.idl` 의 `@unit` 주석 제거 | **`viss-server` 가 3주간 소스에서 빌드되지 않고 있었다** |

3번이 가장 무겁다. fastddsgen 4.3.0 이 `@unit (value="m")` 을 TypeObjectSupport 로 내보낼 때
따옴표를 빠뜨려(`unit_altitude = m;`) `idl_types` 타깃이 깨졌고, `build.sh` 의
`make -k … || true` 가 그 실패를 삼켰다. **빌드는 매번 "성공"으로 끝나고 옛 바이너리가
그대로 남아서** 아무도 눈치채지 못했다. 주석은 문서용 메타데이터라 제거해도 와이어 포맷·
필드 순서·타입 이름이 모두 불변이고, 수정 후 `GeoPoint` 계열 데이터가 계속 들어오는 것으로
확인했다.

> 교훈: **`make -k … || true` 는 빌드 실패를 성공으로 바꾼다.** 산출물 타임스탬프를
> 확인하지 않으면 몇 주 전 바이너리를 시험하고 있을 수 있다. 실제로 그랬다.

---

## 10. 다음 작업 (권장 순서)

| # | 할 것 | 어떻게 | 왜 |
|---|---|---|---|
| 1 | `simple_planning_simulator` 를 끄고 역브리지만으로 Autoware 가 도는지 | §3 의 인자 조합. 단 `sample_adsw_control_run.launch.xml` 이 하드코딩하므로 상류 파일을 건드려야 한다 | 역브리지의 **진짜** 시험. 지금까지는 시뮬레이터를 켠 채라 퍼블리셔가 둘이었다 |
| 2 | 플랜트 확보 — MORAI 어댑터 **또는** SS_v2 concealer 우회 | §6 | 폐루프의 전제 |
| 3 | MMF 도메인 조율 | §7 | MMF 를 3계단 배선에 붙이려면 |
| 4 | `aw_ego_state_bridge` 철거 | — | 1·2 가 끝나면 존재 이유가 사라진다 |

**MORAI 는 벤더 대기 중**이다. `EgoVehicleStatus` 자체는 ROS 2 에 실재하고 우리 `EgoState`
계약이 그것을 보고 만들어졌지만([EgoState.idl](../../idl/dds/sils/EgoState.idl) 헤더),
**Sync Mode 가 ROS 2 메시지 집합에 없다** — 결정론 등급이 여기서 갈린다.
미해결 질문: [sils-morai-vendor-questions.md](sils-morai-vendor-questions.md).

---
*관련: [sils-stage3-autoware-partition.md](sils-stage3-autoware-partition.md) (3계단 절차·실측) ·
[sils-simulator-loops.md](sils-simulator-loops.md) (사다리·백로그) ·
[sils-vehicle-interface-autoware.md](sils-vehicle-interface-autoware.md) (VCU 설계) ·
[evidence/adapi-real-partition-2026-08-10.md](evidence/adapi-real-partition-2026-08-10.md) (원측정값)*
