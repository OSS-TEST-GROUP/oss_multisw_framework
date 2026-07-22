# SiLS 가상 ECU(vECU) 아키텍처 설계 (v3)

> ARM64 컨테이너 기반 SiLS(Software-in-the-Loop Simulation) 인터페이스 모듈 가상 ECU
>
> 상태: **Final (확정, 2026-07-13)** · 대상 프레임워크: `oss_multisw_framework` (MMF) · 구현 베이스라인
>
> v3 변경 요지: v2 재검토를 반영해 **SiLS Run Coordinator**를 추가하고, 비참여 SUT 입력의 tick 귀속과 결정론 경계, MMF/VISS별 DDS domain 주입, Endpoint Contract Catalog, 배포 backend 선택을 구체화했다. 추가 리뷰에서 확인된 DDS interposition 한계를 반영해 **vECU 내부 TickInbox**와 **선택적 Relay Gateway**를 분리하고 domain 예산, Coordinator 장애 복구, replay 경계를 보강했다. v2 원본은 별도 파일로 보존한다.

---

## 1. 목적과 범위

### 1.1 목적

실차 하드웨어 없이 MMF 소프트웨어 스택(SUT: System Under Test)을 대상 fidelity 수준 내에서 실제 ECU와 동일한 인터페이스로 자극·관측하는 가상 ECU 패키지를 정의한다. 테스트 시나리오별 vECU 조합을 컨테이너 플릿으로 구성하고 MMF의 기존 컨테이너 제어 경로로 기동·종료한다.

### 1.2 설계 요구사항

- **ARM64 컨테이너**: 양산 타깃과 동일 ISA에서 실행해 target-like 환경에 근접한다.
- **명시적 fidelity**: 실제 ECU와의 동등성은 L0~L4 수준별로 선언하고 시험으로 검증한다.
- **확장 가능한 런타임**: 인터페이스 스텁 → 행동 모델 → 물리 플랜트(FMI)를 동일 뼈대에서 확장한다.
- **다중 인스턴스**: 같은 vECU 타입도 instance parameter와 key로 N개 구성한다.
- **시간 모드 전환**: wall-clock과 lockstep을 설정으로 선택한다.
- **결정론 경계 명시**: lockstep 참여자뿐 아니라 입력 스트림의 tick 귀속이 고정된 범위에서만 재현성을 보장한다.
- **병렬 run 격리**: 하나의 run 내부는 동일 domain을 사용하고 서로 다른 run은 고유 domain으로 격리한다.

### 1.3 범위 경계

- 단순 `/clock` 발행은 결정론을 보장하지 않는다. grant/ack 및 tick-indexed input이 결정론의 근거다.
- ARM64 동일성은 ISA 수준의 근접성이다. RTOS, kernel scheduling, driver, accelerator 동일성을 의미하지 않는다.
- 초기에는 SUT 전체를 lockstep에 포함하지 않는다. wall-clock SUT와 실시간 양방향 통신하는 시험은 완전 결정론을 보장하지 않는다.
- RTOS 수준의 결정론적 스케줄링과 FMU 모델 자체 저작은 초기 범위 밖이다.

---

## 2. 설계 원칙

| # | 원칙 | 효과 |
|---|---|---|
| P1 | **IDL + Contract Catalog + conformance test** | 구조, endpoint binding, QoS, semantic의 드리프트를 시험으로 차단 |
| P2 | **포트-어댑터** | 시간·버스·행동·플랜트 구현을 교체 가능하게 유지 |
| P3 | **런타임 1개, 구성으로 다형** | 새 vECU를 Descriptor와 plugin으로 추가 |
| P4 | **레지스트리 기반 plugin** | 기존 커널 수정 없이 adapter 확장 |
| P5 | **run 내부 동일 domain, run 간 격리** | SUT-vECU 통신 보장과 CI cross-talk 차단 |
| P6 | **생성물은 generated artifact** | Topology와 배포 명세의 이중 수정 방지 |
| P7 | **compile-time과 run-time 제어 분리** | Topology Compiler는 정적 변환, Run Coordinator는 실행 상태 관리 |
| P8 | **결정론은 입력까지 포함** | 동일 tick-indexed 입력이라는 전제에서만 반복 trace를 보장 |

---

## 3. 기존 MMF와의 정합성

### 3.1 실제 컨테이너 기동 경로

```text
saved-service-list.yaml
  → adsw_manager가 컨테이너/서비스 등록
  → UpdateServiceListRequest로 lifecycle_mgr에 등록 정보 전달
  → lifecycle_mgr가 등록된 container name으로 StartContainerRequest 전송
  → adsw_manager ContainerController가 docker/compose/kubectl 실행
  → NotifyContainerState로 running/stopped 상태 보고
```

- lifecycle_mgr는 Topology, `replicas`, instance parameter를 해석하지 않는다.
- saved-service-list 등록이 곧 무조건적인 자동 기동을 뜻하지 않는다. `enabled`, recovery policy, 현재 상태가 영향을 준다.
- `NotifyContainerState`는 컨테이너 상태이며 vECU 내부 readiness를 의미하지 않는다.

### 3.2 재사용 자산

- `UpdateServiceListRequest`, `StartContainerRequest`, `NotifyContainerState` 경로
- `idl/dds/**`의 기존 구조 계약
- diagnostics 및 ROS2 DDS bridge의 DynamicData/런타임 IDL 사용 사례
- `docker/build_multiarch.sh`, Dockerfile, compose/K3s 자산

### 3.3 전송 추상화 주의

`vehicle_info_mgr::ITransport`는 `start()`와 `stop()`만 제공한다. vECU `IBus`의 직접 참조 구현이 아니라 포트-어댑터 구조의 참고 사례로만 사용한다. publish, subscribe, type load, QoS 계약은 vECU에서 신규 정의한다.

### 3.4 현재 DDS domain 설정 차이

현재 MMF 컴포넌트는 하나의 환경변수만 사용하지 않는다.

| 대상 | 현재 domain 설정 |
|---|---|
| lifecycle_mgr, diagnostics, ota_mgr 등 raw MMF DDS | `MMF_DDS_DOMAIN_ID` |
| vehicle_info_mgr DDS transport와 DDSProvider | `VISS_DDS_DOMAIN_ID` |
| ROS2/rmw 참여자 | `ROS_DOMAIN_ID` |

따라서 run domain 할당은 구성요소별 binding을 함께 생성해야 한다. `MMF_DDS_DOMAIN_ID`만 주입하면 VISS가 기본 domain 0에 남을 수 있다.

---

## 4. 전체 아키텍처

```text
 Topology + ECU Descriptors + Contract Catalog + QoS Profiles
                              │
                              ▼
                    ┌──────────────────┐
                    │ Topology Compiler │  정적 변환·검증
                    └────────┬─────────┘
             ┌───────────────┼────────────────┐
             ▼               ▼                ▼
    deployment artifacts  resolved-run.yaml  validation report
    (backend별 생성물)      (hash 포함)
             │               │
             └───────┬───────┘
                     ▼
             ┌──────────────────────┐
             │ SiLS Run Coordinator │  실행 상태·판정·정리
             └──────────┬───────────┘
                        │
          ┌─────────────┼────────────────────┐
          ▼             ▼                    ▼
 lifecycle_mgr/     Clock Master       Scenario/Trace
 adsw_manager       + optional Relay   services
          │             │                    │
          └─────────────┼────────────────────┘
                        ▼
        ┌──────────────────────────────────┐
        │ SiLS Run: SUT + vECU Fleet       │
        │ run 전용 DDS domain              │
        └──────────────────────────────────┘
```

### 핵심 책임 분리

- **Topology Compiler**: 파일을 읽고 확장·검증·생성한다. 실행 상태를 갖지 않는다.
- **Run Coordinator**: run을 기동하고 readiness를 집계하며 clock/scenario 시작, 종료 판정, teardown을 수행한다.
- **기존 lifecycle_mgr/adsw_manager**: 생성된 평면 배포 명세에 따라 컨테이너를 기동·정지·감시한다.
- **Clock Master**: lockstep participant에게 tick을 승인하고 완료를 기다린다.
- **TickInbox**: vECU 내부에서 이미 수신된 DDS sample의 소비 시점과 target tick 귀속을 관리한다. DDS 전달 자체를 가로채지 않는다.
- **Relay Gateway(선택)**: source topic을 구독해 별도 target topic으로 재발행하는 명시적 relay다. 기존 peer 사이에 투명하게 개입하지 않는다.
- **vECU Runtime**: Descriptor에 따라 ECU 동작, plant, fault, bus endpoint를 실행한다.

### 4.1 외부 Simulator와 Protocol Bridge

MORAI 같은 외부 차량 Simulator는 vECU 내부 adapter나 Relay Gateway로 모델링하지 않는다. Simulator와 UDP–ROS2 Bridge를 SiLS Run에 참여하는 **external component**로 등록한다.

```text
┌──────────────────────────────── SiLS control boundary ────────────────────────────────┐
│ Run Coordinator                                                                       │
│   ├─ topology/IP/port/domain 검증                                                      │
│   ├─ managed component start/stop 또는 external component attach/health              │
│   └─ readiness, scenario, pass/fail, artifact, cleanup                                │
│                                                                                        │
│ K-Autoware/MMF ◀── ROS2/DDS ──▶ [선택 Contract Adapter] ◀──▶ UDP–ROS2 Bridge          │
└───────────────────────────────────────────────────────────────┬────────────────────────┘
                                                                │ UDP
                                                                ▼
                                                        MORAI Simulator
                                                        External Plant/Sensor
```

역할 원칙:

- MORAI는 Ego 차량 동역학과 sensor/environment 생성을 소유한다.
- 검증된 외부 Bridge는 MORAI UDP packet codec과 ROS2 pub/sub를 소유한다.
- SiLS는 Bridge 기능을 중복 구현하지 않고 구성·계약·readiness·장애 감시·시험 판정을 소유한다.
- Bridge가 `morai_msgs`까지만 제공하는 endpoint는 선택적 Contract Adapter가 K-Autoware canonical type으로 변환한다.
- 이 Bridge는 DDS peer 사이 topic을 relay하는 §7의 Relay Gateway가 아니라 서로 다른 protocol 경계를 변환하는 **external protocol bridge**다.
- 초기 연동은 `WallClock + live`이며 외부 Simulator/Bridge를 lockstep required participant에 넣지 않는다.

---

## 5. SiLS Run Coordinator

### 5.1 책임

Run Coordinator는 다음 종단 흐름의 단일 소유자다.

```text
Topology compile
 → deployment artifact 설치
 → SUT/vECU와 managed external component 기동 또는 external component attach
 → container running 대기
 → VecuReady, Bridge/Simulator readiness 및 필수 peer match 대기
 → scenario와 clock 시작
 → pass/fail/timeout 판정
 → clock 중지 및 trace flush
 → 컨테이너 정지·정리
```

### 5.2 Run 상태기계

```text
CREATED
  → COMPILED
  → STARTING
  → WAITING_READY
  → RUNNING
  → COMPLETING
  → PASSED | FAILED | ABORTED
  → CLEANED
```

모든 전이는 `run_id`, 원인, wall time, sim time을 기록한다. 실패한 전이는 idempotent cleanup을 호출한다.

Coordinator는 run당 단일 소유자이지만 고가용성 cluster를 Phase 1 범위에 넣지 않는다. 대신 run state와 ownership lease를 외부에서 복구 가능한 형태로 남기고 모든 정리 동작을 idempotent하게 만든다.

```text
run directory
  ├─ resolved-run.yaml
  ├─ run-state.json
  ├─ ownership-lease.json
  └─ trace/result artifacts
```

모든 컨테이너와 생성물에는 `run_id` label을 부여하며 외부 CI finalizer가 `cleanup --run-id`를 재실행할 수 있어야 한다.

### 5.3 Container state와 readiness 분리

```text
NotifyContainerState: 프로세스/컨테이너가 running인지 보고
VecuReport.READY:      IDL load, endpoint 생성, 필수 peer match, behavior/plant init 완료
```

Run Coordinator는 필수 vECU가 모두 READY인 후에만 scenario와 Clock Master를 시작한다. partial startup은 기본적으로 전체 rollback하며, `allow_degraded: true`가 명시된 topology에서만 degraded run을 허용한다.

### 5.4 제어 IDL 후보

- `RunControl`: prepare/start/stop/abort
- `VecuReport`: state, reason, matched_endpoints, sequence
- `ScenarioControl`: load/start/stop
- `TickGrant`, `TickComplete`

기존 lifecycle_mgr가 이 상태를 직접 이해하도록 수정하지 않는다. 신규 SiLS control plane에서 소비한다.

---

## 6. vECU 런타임 내부 구조

### 6.1 논리 계층과 SiLS 플랫폼 경계

제어 알고리즘을 실행하는 고충실도 vECU는 다음 네 계층으로 구성한다. 단, 네 계층이 항상 양산 AUTOSAR 수준으로 모두 구현된다는 뜻은 아니다. 요구 fidelity에 따라 실제 ASW/BSW를 사용하거나 behavior model과 최소 runtime으로 축약할 수 있다.

```text
┌────────────────────── vECU package ────────────────────────┐
│ 1. ASW / Behavior                                          │
│    production control algorithm 또는 대체 behavior model   │
├────────────────────────────────────────────────────────────┤
│ 2. Virtual BSW                                             │
│    virtual OS scheduler · RTE · communication · diagnostics│
├────────────────────────────────────────────────────────────┤
│ 3. V-HAL                                                   │
│    sensor · actuator · timer · bus · plant abstraction     │
├────────────────────────────────────────────────────────────┤
│ 4. SiLS Host Adapter                                       │
│    clock client · lifecycle · I/O exchange · trace hook    │
└────────────────────────────────────────────────────────────┘
                         │
                         ▼
┌────────────────────── SiLS platform ────────────────────────┐
│ Run Coordinator · Clock Master · Topology Compiler         │
│ Recorder/Diagnostics · deployment backend · MORAI Bridge   │
└────────────────────────────────────────────────────────────┘
```

| 논리 계층 | vECU 포함 | v3 구성요소 매핑 | 책임 |
|---|---:|---|---|
| ASW / Behavior | 포함 | `IBehavior`, behavior plugin | 제어 알고리즘, runnable 또는 상태 머신 실행 |
| Virtual BSW | 포함 | `ExecutionKernel`, scheduler, bus/RTE adapter | task 순서, RTE 호출, 통신, 선택적 Diagnostics/NvM 제공 |
| V-HAL | 포함 | `IBus`, `IPlant`, port/contract adapter | 실제 센서·actuator·timer·bus 접근을 simulation I/O로 치환 |
| SiLS Host Adapter | 포함 | `IClockSource`, lifecycle client, trace hook | vECU를 외부 시간·실행 제어·관측 경로에 연결 |
| SiLS control plane | **제외** | Run Coordinator, Clock Master, Topology Compiler | 여러 vECU와 외부 Simulator를 run 단위로 통제 |
| External integration | **제외** | MORAI, UDP–ROS2 Bridge, Bridge Supervisor | 외부 Simulator 실행 및 protocol 연동 |

Host Adapter는 vECU 내부의 **접속점**까지만 소유한다. 전체 participant에 tick을 승인하는 Clock Master와 start/readiness/teardown을 지휘하는 Run Coordinator는 vECU 밖에 둔다. 따라서 각 vECU에는 Clock Master의 client인 `IClockSource`가 들어가지만 master 역할의 `IClockCoordinator` 구현은 SiLS control plane에 배치한다.

`DLL 컴파일`은 runtime 계층의 책임이 아니라 build/package 선택이다. Windows host는 `.dll`, 현재 Linux/ARM64 container는 `.so` 또는 executable, 표준 model exchange가 필요하면 FMU를 사용할 수 있다. Host Adapter의 플랫폼 독립 책임은 **시간 동기화, lifecycle, 입출력 교환, trace와 host API 연동**이다.

Fidelity별 기본 구성은 다음과 같다.

| Fidelity | ASW/Behavior | Virtual BSW | V-HAL/Host |
|---|---|---|---|
| L0 Interface Stub | 고정 응답 또는 echo | 최소 runtime | endpoint와 lifecycle 중심 |
| L1 Behavior Model | 상태 머신/규칙 모델 | scheduler·통신 최소본 | simulated I/O와 clock 연결 |
| L2 Production ASW | 실제 제어 코드 | 필요한 RTE/BSW 가상화 | ECU hardware API 치환 |
| L3 Plant Coupled | L2 또는 고급 behavior | L2 범위 | plant 결합: FMU/코드(in-process `IPlant`) 또는 MORAI/AWSIM(버스 결합) 추가 |
| L4 Target-like | 실제 ASW | 실제 BSW/OS 동작에 최대한 근접 | target timing/interface 검증 강화 |

### 6.2 포트-어댑터 런타임 구현

```text
┌─────────────────────────────────────────────────────────────┐
│ Virtual ECU Runtime                                         │
│ Descriptor ─▶ Assembler/DI ─▶ ExecutionKernel               │
│                              │                              │
│  IClockSource  IBus  IBehavior  IPlant  IScenario           │
│                 │                                           │
│             TickInbox                                       │
│          FaultInterceptor                                   │
│          TraceInterceptor                                   │
│          Dds/Viss/vCAN/SomeIP Adapter                       │
└─────────────────────────────────────────────────────────────┘
```

| 포트 | 책임 | 개념 시그니처 |
|---|---|---|
| `IClockSource` | 시각 조회와 tick 대기 | `now()`, `wait_for_tick()`, `sequence()` |
| `IClockCoordinator` | master 측 tick 승인 | `grant_tick()`, `wait_for_completion()`, `abort()` |
| `IBus` | endpoint 기반 pub/sub | `publish(ep,sample)`, `subscribe(ep,cb)`, `load_type(contract)` |
| `IBehavior` | ECU 동작 로직 | `on_init(ctx)`, `on_tick(t)`, `on_message(ep,sample)` |
| `IPlant` | plant/FMU step 소유 | `set_inputs()`, `step(dt)`, `outputs()` |
| `IScenario` | fault와 sequence 평가 | `evaluate(t,ctx)` |

FMU step ownership은 `IPlant`에 둔다. FMI 자체가 ECU behavior인 특수 사례만 별도 behavior adapter로 허용한다.

> **plant 결합의 두 형태 (중요).** in-process `IPlant`(FMU/코드)는 vECU tick 루프가 직접 step 하는 폐루프다. 반면 **MORAI/AWSIM은 `IPlant`가 아니라 버스 결합**(authority=sim, 공유 세계)이다 — vECU가 STATE를 구독하고 ACTUATION을 발행하며, 폐루프는 시뮬레이터가 버스 너머에서 닫는다(`IBus`/채널맵 경로, 정규 `EgoState`/`VehicleControl` 계약). 시뮬레이터를 in-process IPlant로 step 하지 않는다. 위 L3 행의 "MORAI 연결"과 §5(External Component)의 MORAI는 이 버스 결합을 뜻한다. 결정·근거·3안 비교: [sils-morai-plant-adapter-design.md](sils-morai-plant-adapter-design.md)(권장 Option 2).

위 포트 구조는 6.1의 계층을 대체하는 별도 구조가 아니라 구현 방식이다. 예를 들어 `IBehavior`는 ASW/Behavior, `ExecutionKernel`은 Virtual BSW, `IBus`와 `IPlant`는 V-HAL, `IClockSource`와 trace/lifecycle adapter는 Host Adapter에 대응한다.

---

## 7. Tick 처리와 입력 귀속

### 7.0 DDS interposition 경계

DDS는 peer-to-peer이므로 제3 컴포넌트가 기존 publisher와 subscriber 사이의 sample을 투명하게 붙잡을 수 없다. 본 설계는 다음 두 메커니즘을 구분한다.

```text
1) Consumer-local staging (기본)

SUT ── 기존 topic ──▶ vECU DDS Reader ──▶ TickInbox ──▶ Behavior

2) Explicit relay (선택)

SUT ── source topic ──▶ Relay Gateway ── target topic ──▶ vECU
```

- **TickInbox**는 기존 topic 배선을 유지하지만 DDS sample의 도착 시점을 통제하지 않는다. 수신 완료된 sample 중 어느 것을 현재 tick에서 소비할지만 결정한다.
- **Relay Gateway**는 source/target topic을 분리하고 Contract Catalog와 Topology가 양쪽 배선을 명시해야 한다. 투명한 interposition이 아니다.
- 기존 SUT가 target topic을 직접 publish하지 않도록 single-owner 검증을 수행한다.
- Phase 0B에서 각 input mode가 어떤 메커니즘과 topic wiring을 요구하는지 계약으로 확정한다.

### 7.1 단일 tick 파이프라인

```text
TickGrant 수신 또는 wall-clock 주기 만료
 → target tick용 inbound sample 확정
 → inbound trace 기록
 → inbound fault 적용
 → behavior 계산
 → plant input 설정
 → plant step
 → plant output 반영
 → outbound fault 적용
 → publish_intent trace 기록
 → DDS write
 → publish_result 기록
 → TickComplete 전송
```

`publish_intent`는 실제 payload를, `publish_result`는 write 결과와 timestamp를 기록한다. Recorder 위치를 “publish 전”과 “publish 후”로 명확히 구분한다.

### 7.2 비참여 SUT 입력 문제

wall-clock SUT가 보낸 DDS sample은 network/thread scheduling에 따라 서로 다른 tick에 도착할 수 있다. 따라서 vECU만 lockstep이라고 해서 양방향 SiLS 전체가 자동으로 결정론적이지 않다.

결정론 보장은 다음 전제를 갖는다.

> 동일한 tick-indexed 입력 스트림이 주어질 때 lockstep 참여 vECU의 실행 순서와 model step 결과를 재현한다.

실시간 SUT 입력을 사용하는 시험은 다음 모드 중 하나를 명시한다.

| 입력 모드 | 구현 메커니즘 | 동작 | 결정론 |
|---|---|---|---|
| `live` | 직접 DDS reader | 도착한 sample을 즉시 또는 다음 주기에 소비 | 보장 안 함 |
| `local-staged` | vECU 내부 TickInbox | tick 경계에서 이미 도착한 입력 집합을 freeze | 도착 시점은 비결정적, 소비 순서만 고정 |
| `relay-staged` | 명시적 Relay Gateway | source topic을 staging 후 target topic으로 재발행 | relay 입력 경계 내 제한적 보장 |
| `replay` | Relay Gateway 또는 replay bus source | 사전 기록한 tick-indexed 입력 재생 | open-loop 참여 범위 내 보장 |
| `full-lockstep` | SUT 포함 barrier | 모든 feedback participant가 tick 완료에 참여 | 전 시스템 후보, 별도 과제 |

### 7.3 TickInbox 및 Relay Gateway 귀속 규칙

```text
DDS sample 수신(TickInbox) 또는 source topic 수신(Relay)
 → source timestamp / reception sequence 기록
 → staging queue 저장
 → TickGrant(N) 경계에서 N 입력 집합 freeze
 → freeze 이후 수신 sample은 N+1 이후로 이월
```

필수 규칙:

- source timestamp가 없는 IDL의 ordering은 gateway reception sequence 사용
- 동일 endpoint와 tick 내 sample 정렬 기준 명시
- KEEP_LAST에서 중간 sample 유실을 허용하는 contract인지 검증
- late sample drop/rollover/fail 정책을 endpoint contract에 선언
- 기존 payload를 변경하지 않고 sidecar trace metadata에 `run_id`, tick, receive sequence 기록

기존 MMF IDL에 `run_id`를 무조건 추가하지 않는다. run 격리는 DDS domain으로 수행하고 trace correlation은 sidecar metadata로 수행한다.

Relay Gateway를 사용하는 endpoint contract에는 다음 항목을 추가한다.

```yaml
relay:
  source_topic: mmf/live/BrakeCommand
  target_topic: sils/staged/BrakeCommand
  source_role: sut_publish
  target_role: vecu_subscribe
  preserve_key: true
  preserve_source_timestamp: true
```

### 7.4 Replay 경계

Replay가 결정론을 제공하려면 replay 대상 채널이 시스템의 모든 외생 입력을 포함하고 live component 출력이 replay 경계 안으로 feedback되지 않아야 한다.

| 시험 형태 | 판정 |
|---|---|
| 모든 외생 입력 replay, live 출력은 관측만 함 | open-loop 재현 가능 |
| replay 입력 + live SUT 출력이 다시 feedback | hybrid, 완전 재현 불가 |
| 양방향 trace replay, SUT 출력은 비교만 함 | open-loop conformance |
| SUT와 vECU 모두 lockstep 참여 | closed-loop 결정론 후보 |

Topology는 replay boundary를 명시한다.

```yaml
replay:
  controlled_inputs: [brake-command, wheel-speed]
  observed_outputs: [brake-status]
  live_feedback: forbidden
```

`live_feedback: allowed`인 replay run은 결과에 `hybrid/non-deterministic`을 기록하고 golden trace 완전 일치 시험에 사용하지 않는다.

---

## 8. 시간 아키텍처

### 8.1 모드

```text
WallClock
  - 통합 smoke와 target-like 실시간 시험
  - 결정론 보장 없음

LockstepClock
  - TickGrant/TickComplete grant/ack
  - tick-indexed 입력이 동일한 참여자 범위에서 재현성 보장
```

### 8.2 Lockstep 프로토콜

```text
ClockMaster
  → TickGrant(run_id, sequence, sim_time, dt)
      → participant가 입력/behavior/plant/output 처리
  ← TickComplete(run_id, sequence, participant_id, status)
  → 필수 participant 완료 후 다음 TickGrant
```

정의할 계약:

- participant 등록과 필수/선택 구분
- duplicate/stale TickComplete 처리
- tick timeout과 retry/abort 정책
- participant 재시작 후 재가입 정책
- 동일 timestamp 이벤트 정렬
- random seed, floating-point mode, image digest 기록

Clock Master는 독립 프로세스를 기본으로 한다. 특정 vECU 겸임은 PoC 편의 옵션으로만 허용하며 production test topology에서는 권장하지 않는다.

---

## 9. Endpoint Contract Catalog와 구성 스키마

### 9.1 Contract Catalog

IDL은 type을 정의하지만 runtime topic name과 semantic을 기계 판독 가능한 방식으로 모두 제공하지 않는다. 따라서 검증된 endpoint binding을 Catalog로 관리한다.

```yaml
# contracts/mmf-endpoints.yaml
contracts:
  - id: viss-request
    transport: dds
    idl: idl/dds/vehicle_info_mgr/VissRequest.idl
    struct: VissRequestTopic
    registered_type: VissRequestTopic
    topic: /VIN/Vehicle
    qos_profile: request-reliable
    ownership: single
    late_sample_policy: next_tick
    roles:
      sut: subscribe
      vecu: publish
```

Catalog는 IDL path/type 존재, type registration compatibility, topic, QoS, ownership, semantic version을 함께 관리한다. Descriptor는 임의 조합보다 `contract_id` 참조를 우선한다.

### 9.2 ECU Descriptor

```yaml
ecu: brake_ecu
instance_id: brake-1
fidelity: L1

clock: {mode: lockstep}

buses:
  - name: mmf-dds
    type: dds
    endpoints:
      - contract_id: brake-command
        direction: subscribe
      - contract_id: brake-status
        direction: publish
        key: {ecu_id: "${instance_id}"}

behavior:
  type: state_machine
  spec: behaviors/brake_ecu.yaml
plant: {type: none}
scenario: {faults: scenarios/brake_faults.yaml}
```

Catalog에 없는 실험 endpoint가 필요하면 inline contract를 허용할 수 있지만 validation report에 비표준 계약으로 표시한다.

### 9.3 QoS Catalog

```yaml
request-reliable:
  reliability: RELIABLE
  durability: VOLATILE
  history: {kind: KEEP_LAST, depth: 10}

state-latest:
  reliability: RELIABLE
  durability: TRANSIENT_LOCAL
  history: {kind: KEEP_LAST, depth: 1}
```

### 9.4 Topology

Topology는 **한 번의 SiLS test run에 어떤 컴포넌트가 참여하고, 서로 어떤 통신 경로로 연결되는지를 정의한 실행 구성도**다. 네트워크 이론보다는 테스트를 실행하기 위한 **배선도와 참여자 명세**로 이해하는 것이 적절하다.

Topology에는 최소한 다음 항목이 들어간다.

- 참여자: SUT, vECU, Simulator, Bridge, Recorder 등
- 연결 관계: 누가 어떤 endpoint를 publish/subscribe하는지
- 인터페이스와 통신 자원: topic, protocol, DDS domain ID, IP, UDP port
- 실행 위치와 소유 방식: 동일 host, container, 외부 장비 및 `managed`/`external`
- 시간 참여 여부: wall-clock 또는 lockstep participant
- 필수 여부: 장애 시 run을 중단해야 하는 component인지

Brake vECU만 시험하는 가장 단순한 Topology는 다음과 같다.

```text
[Test/SUT]
  brake-command publish
          │ DDS
          ▼
    [Brake vECU]
  command 처리·상태 계산
          │ DDS
          ▼
  brake-status publish
          │
          ▼
    [Test/SUT 검증]
```

MORAI 폐루프 시험은 Simulator와 protocol bridge가 추가된 별도 Topology다.

```text
[MORAI Simulator]
  sensor·vehicle status
          │ UDP
          ▼
[UDP–ROS2 Bridge]
  protocol/message 변환
          │ ROS2/DDS
          ▼
    [K-Autoware]
  perception·planning·control
          │ ROS2/DDS
          ▼
[UDP–ROS2 Bridge]
          │ UDP
          ▼
[MORAI Simulator]
  vehicle motion 반영
```

이 예에서 Bridge는 MORAI UDP와 ROS2/DDS를 연결하는 protocol bridge이며, DDS sample을 투명하게 가로채는 Relay Gateway가 아니다. 현재 MORAI 연동은 wall-clock 기반이므로 MORAI와 Bridge를 lockstep participant로 선언하지 않는다.

각 구성 개념의 경계는 다음과 같다.

| 구성 요소 | 답하는 질문 | 예 |
|---|---|---|
| Contract Catalog | 어떤 데이터를 어떤 형식과 QoS로 주고받는가? | `brake-command`, `sensor_msgs/msg/PointCloud2` |
| Topology | 그 contract를 실제로 누가 누구와 어떤 경로로 주고받는가? | `MORAI → UDP Bridge → K-Autoware` |
| Topology Compiler | 선언이 실행 가능한지 검증하고 무엇을 생성하는가? | IP/port/domain 충돌 검사, deployment artifact 생성 |
| Run Coordinator | 구성요소를 어떤 순서로 시작·감시·종료하는가? | start → readiness → run → verdict → teardown |

다음 YAML은 위 개념을 실제 실행 선언으로 표현한 예다.

```yaml
topology: adas_brake_test

run:
  domain_strategy: inject       # inject | allocate
  domain_id: "${SILS_DOMAIN_ID}"
  input_mode: local-staged      # live | local-staged | relay-staged | replay | full-lockstep
  allow_degraded: false

deployment:
  backend: docker               # docker | compose | k3s

domain_bindings:
  MMF_DDS_DOMAIN_ID: "${run.domain_id}"
  VISS_DDS_DOMAIN_ID: "${run.domain_id}"
  ROS_DOMAIN_ID: "${run.domain_id}"  # ROS2 participant가 run에 참여할 때만

external_components:
  - name: morai-simulator
    kind: simulator
    deployment: external              # external | managed
    vehicle_state_authority: morai
    clock: {mode: wall, target_rate_hz: 50}
    readiness: {kind: udp-connected, timeout_ms: 30000}
  - name: hanyang-morai-bridge
    kind: protocol-bridge
    protocol: {source: morai-udp, target: ros2-dds}
    deployment: external
    host: "${MORAI_BRIDGE_HOST}"
    udp_port_profile: morai-ego-0
    ros_domain_id: "${run.domain_id}"
    readiness:
      required_endpoints: [vehicle-control, vehicle-status]
      require_fresh_sample: true

vecus:
  - {name: vecu-brake, descriptor: brake_ecu.yaml}
  - name: wheelspeed
    descriptor: wheelspeed_ecu.yaml
    replicas: 4
    params: [{pos: FL}, {pos: FR}, {pos: RL}, {pos: RR}]

clock:
  mode: lockstep
  master: sils-clock-master
  required_participants: [vecu-brake, wheelspeed-FL, wheelspeed-FR, wheelspeed-RL, wheelspeed-RR]
  tick_timeout_ms: 1000
```

CI에서는 domain 충돌을 피하기 위해 외부 job이 `SILS_DOMAIN_ID`를 주입하는 방식을 우선 권장한다. 로컬 실행의 `allocate`는 lock file 또는 중앙 allocator로 동시 실행 충돌을 방지해야 한다.

### 9.5 정적 검증

Topology Compiler는 다음을 배포 전에 실패 처리한다.

- Descriptor/Topology/Contract Catalog schema 오류
- 존재하지 않는 IDL, struct, registered type, QoS profile
- Catalog에 등록되지 않은 `contract_id`
- single-owner endpoint의 producer 중복
- container, participant, instance, key 중복
- 필수 participant와 생성된 replica 불일치
- MMF/VISS/ROS domain binding 불일치
- external component의 host/port profile 누락 또는 run 간 UDP port 충돌
- Simulator와 FMU/vECU가 동일 vehicle state를 동시에 소유
- Bridge ROS2 endpoint type/QoS/rate/frame/unit 계약 불일치
- 실 provider와 virtual provider 동시 활성화
- 선택한 backend에 필요한 필드 또는 manifest 누락

“topic 존재 여부”는 IDL 주석을 파싱해 추정하지 않고 Contract Catalog의 등록 여부로 검증한다.

---

## 10. 배포 backend와 생성물

Topology는 run마다 하나의 backend를 선택한다. 동일 vECU를 여러 backend가 동시에 소유하지 않는다.

| Backend | 생성물 | adsw_manager 동작 |
|---|---|---|
| `docker` | image, extra_args, command_args를 포함한 saved-service-list | `docker run/start/stop` |
| `compose` | compose 파일 + deploy file을 참조하는 saved-service-list | `docker compose up/down` |
| `k3s` | K3s manifest + deploy file을 참조하는 saved-service-list | `kubectl apply/delete` |

Topology Compiler의 공통 출력:

- `resolved-run.yaml`: parameter, replica, domain, contract가 모두 resolve된 canonical 입력
- backend별 deployment artifact
- validation report
- topology/descriptor/contract/QoS hash manifest

생성물은 사람이 직접 수정하지 않으며 원본 Topology와 Catalog를 수정한 뒤 다시 생성한다.

---

## 11. Fidelity와 Conformance

| 수준 | 명칭 | 보장 범위 | 대표 검증 |
|---|---|---|---|
| L0 | Interface Stub | type/topic match와 기본 sample | endpoint matching |
| L1 | Protocol Fidelity | QoS, request/response, 오류 처리 | protocol conformance |
| L2 | Behavioral Fidelity | 상태기계, timing, failure semantic | golden scenario |
| L3 | Plant-coupled Fidelity | FMU 기반 폐루프 | model correlation |
| L4 | Target-like Fidelity | ARM64와 실제 gateway/protocol | target parity |

- 각 Descriptor와 CI 결과에 fidelity 수준을 기록한다.
- ARM64는 L4 구성 요소 중 하나이며 L4 전체를 자동으로 충족하지 않는다.
- conformance 결과에는 Contract Catalog version과 image digest를 기록한다.

---

## 12. Run 격리, 관측성, 재현성

### 12.1 격리

- 한 run의 SUT, vECU, control plane은 같은 domain ID를 사용한다.
- 다른 run은 다른 domain ID를 사용한다.
- 초기 구현은 domain 격리를 기준으로 한다. DDS partition은 SUT 지원 여부가 확인된 후 선택 기능으로 추가한다.
- container, participant, instance 이름에는 `run_id` 기반 prefix를 적용할 수 있다.

### 12.2 Domain ID 예산과 동시성 상한

Domain-only 격리는 병렬 run마다 domain과 transport port 자원을 소비한다. 무제한 fan-out을 가정하지 않는다.

```yaml
domain_pool:
  range: "${CI_ASSIGNED_DOMAIN_RANGE}"
  max_concurrent_runs: "${MEASURED_RUN_LIMIT}"
  lease_ttl_sec: 3600
  allocation: external
```

- 정확한 동시성 상한은 0A Contract Spike에서 Fast DDS transport 설정, run당 participant 수, runner의 port 범위를 기준으로 측정한다.
- CI scheduler는 domain lease를 획득하지 못하면 run을 queueing한다.
- lease는 TTL과 owner heartbeat를 가지며 Coordinator 비정상 종료 후에도 회수 가능해야 한다.
- runner별 domain pool을 분리해 서로 다른 runner의 할당 충돌을 피한다.
- 동시성 요구가 측정 상한을 넘으면 DDS partition, network namespace 또는 runner/VM 격리를 평가한다.

외부 Simulator run은 DDS domain 외에 UDP port와 Simulator/GPU/license도 예산으로 관리한다.

- topology는 run별 UDP port block을 할당하고 종료 시 회수한다.
- 화면이나 예제의 `127.0.0.1`과 고정 port를 운영 기본값으로 사용하지 않는다.
- external host에 배치할 경우 routing, firewall, MTU와 container host networking을 manifest에 기록한다.
- 병렬도 상한은 `domain × UDP port block × Simulator instance/license × GPU capacity` 중 최소값이다.
- partition은 동일 domain의 discovery 부하까지 제거하지 않으며 모든 SUT participant의 일관된 partition 설정이 확인된 뒤 도입한다.

### 12.3 Trace metadata

- `run_id`
- topology, Descriptor, Contract Catalog, QoS hash
- IDL hash와 image digest
- wall time, sim time, tick sequence
- source timestamp, reception sequence
- publish intent/result
- random seed와 floating-point 실행 정보

기존 MMF payload에 trace metadata를 삽입하지 않는다. Recorder sidecar 형식으로 저장한다.

### 12.4 회귀 판정

- floating-point tolerance
- unordered event 비교 규칙
- 허용 latency window
- missing/extra sample 정책
- golden trace와 SUT/vECU version 연결

`live`와 `local-staged` 입력 모드는 golden trace 완전 일치 시험 대상으로 사용하지 않는다. 결정론적 회귀는 모든 외생 입력이 통제된 `replay`, 명시적 입력 경계를 가진 `relay-staged`, 또는 `full-lockstep` 모드에서 수행한다.

---

## 13. 다중 인스턴스와 Topology Compiler

```text
Topology Compiler
  ├─ Descriptor, contract, QoS resolve
  ├─ replicas 및 instance parameter 확장
  ├─ container/participant/key 이름 생성
  ├─ domain binding과 backend 검증
  ├─ resolved-run.yaml 생성
  ├─ backend별 평면 배포 명세 생성
  └─ validation/hash report 생성
```

### 컨테이너 입도

| 방식 | 장점 | 단점 | 권장 |
|---|---|---|---|
| 1 container = 1 vECU | ECU 경계, 개별 kill, 독립 restart | 다수 인스턴스 오버헤드 | 기본 |
| 1 container = N actors | 기동·자원 효율 | 장애 격리 약함 | 대량 센서 그룹 |

Run Coordinator는 actor별 readiness와 container별 lifecycle을 별도 identity로 추적한다.

---

## 14. Lifecycle과 장애 정책

### 14.1 Readiness

- container running 후 vECU가 IDL/type load, endpoint 생성, required peer match를 완료하면 READY 보고
- Run Coordinator가 required participant 전체 READY를 확인
- readiness timeout 시 기본적으로 run 실패와 rollback

### 14.2 Shutdown

```text
새 TickGrant 중지
 → 진행 tick 완료 또는 timeout
 → scenario stop
 → trace flush 확인
 → DDS participant 해제
 → lifecycle stop
 → artifact/result 확정
```

### 14.3 장애 처리

- Clock Master 장애: run abort
- required participant timeout: 기본 abort, topology에서만 예외 허용
- FMU NaN/step 실패: 해당 tick FAILED
- Descriptor/IDL/contract load 실패: fail-fast
- endpoint mismatch: readiness 실패
- participant 재시작: 기본적으로 같은 run 재가입 금지, 명시적 recovery mode에서만 허용

### 14.4 Run Coordinator 장애와 고아 run 회수

Coordinator heartbeat가 lease TTL 안에 갱신되지 않으면 외부 supervisor 또는 CI finalizer가 다음 순서로 회수한다.

```text
Coordinator lease 만료 감지
 → Clock Master abort 요청 또는 timeout 확인
 → run_id label로 container/resource 조회
 → trace flush 가능 여부 확인
 → lifecycle/docker backend cleanup 재실행
 → domain lease 회수
 → ABORTED_ORPHANED 상태와 원인 기록
```

- `cleanup --run-id`는 여러 번 호출해도 같은 최종 상태가 되어야 한다.
- Clock Master와 required participant는 Coordinator heartbeat 상실 시 무한 대기하지 않고 안전 정지한다.
- Phase 1은 Coordinator HA가 아니라 **장애 후 외부 회수 가능성**을 통과 기준으로 삼는다.

---

## 15. 확장 로드맵

| 포트/영역 | 최소 | 중간 | 최대 |
|---|---|---|---|
| Clock | WallClock | vECU lockstep | SUT 포함 full-lockstep |
| Input | live/TickInbox | explicit relay/replay gateway | 전 시스템 barrier |
| IBus | DDS DynamicData | VISS/vCAN | SOME-IP/CAN-FD gateway |
| IBehavior | Stub | StateMachine/Script | 고충실도 ECU model |
| IPlant | None | Replay | FMI 2.0/3.0 co-sim |
| IScenario | 없음 | fault injection | UDS 부정응답/복합 sequence |
| Control | local CLI | Run Coordinator | CI fleet scheduler |

---

## 16. 제안 리포 레이아웃

```text
tools/sils/
  topology_compiler/
  run_coordinator/

service/sils/
  vecu/
    kernel/
    tick_inbox/
    ports/
    adapters/
    descriptors/
    behaviors/
    scenarios/
  clock_master/
  relay_gateway/                 # 선택 기능, 투명 interposition 아님
  external_integration/
    bridge_supervisor/           # external/managed bridge readiness와 health
    contract_adapter/            # 선택: 누락된 canonical 변환만 구현

config/sils/
  contracts/
  qos/
  topologies/

idl/dds/sils/
  Tick.idl
  RunControl.idl
  VecuReport.idl
  ScenarioControl.idl

docker/compose/
  docker-compose-sils.yaml       # generated

docs/
  sils-virtual-ecu-architecture-v3.md
```

---

## 17. 단계별 구현 계획

| Phase | 목표 | 주요 산출물 | 통과 기준 |
|---|---|---|---|
| 0A Contract Spike | 위험 검증 | DynamicData↔codegen, QoS, ARM64, 실제 topic 실험 | endpoint match와 sample 일치 |
| 0B Contract Design | 계약 확정 | Catalog/schema, domain bindings, backend schema, readiness IDL | schema 및 설계 리뷰 |
| 1 Vertical PoC | 종단 경로 | vECU 1개, Run Coordinator 최소본, lifecycle start/stop, trace | 기동·READY·수신·정리 성공 |
| 1B External Simulator PoC | MORAI 폐루프 | MORAI 1개, 검증된 UDP–ROS2 Bridge 1개, K-Autoware control/status, Bridge Supervisor | UDP 연결·ROS2 match·직진/조향/정지·장애 정리 성공 |
| 2 Fleet Runtime | 다중 인스턴스 | Topology Compiler, replica, run 격리, local-staged input | 병렬 run cross-talk 0 |
| 3 Deterministic Regression | 반복 재현 | lockstep, replay, comparator | 반복 trace 허용 범위 내 일치 |
| 4 High Fidelity | plant/protocol | FMI, vCAN/SOME-IP/CAN-FD, conformance suite | 선언 fidelity 통과 |

Phase 1은 WallClock과 실제 MMF endpoint 하나를 사용한다. lockstep은 계약은 0B에서 정의하되 구현은 Phase 3에서 완료한다.

Phase 1B는 Phase 1의 VISS vertical slice를 대체하지 않는다. Phase 1이 증명한 Coordinator/Contract/readiness 기반 위에서 external component 모델을 추가한다. Camera/LiDAR/Radar와 lockstep은 Phase 1B 범위에서 제외한다.

### Phase 1 Scope Gate

Phase 1의 목적은 범용 SiLS platform 완성이 아니라 하나의 실제 endpoint에 대한 vertical slice 증명이다.

**Phase 1 필수 범위**

- Docker backend 하나
- CI 또는 사용자가 주입한 domain 하나
- 실제 Contract Catalog entry 하나와 최소 schema
- vECU container 하나, WallClock 하나
- 단일 run CLI 형태의 Coordinator 최소본
- `Start → container running → VecuReady → sample 확인 → Stop/Cleanup`
- 최소 JSONL trace와 idempotent `cleanup --run-id`

**Phase 1 명시적 제외 범위**

- 독립 Relay Gateway와 topic remapping
- lockstep/replay/fault/FMI
- replica와 다중 backend
- 범용 Catalog 관리 서비스
- Coordinator scheduler, Web UI, HA cluster
- partition 기반 격리

Phase 1 통과 전에는 제외 기능의 풀 구현을 시작하지 않는다. 0B에서는 향후 호환을 위한 계약만 정의하고 구현은 해당 Phase로 미룬다.

### 최소 Vertical PoC

1. Contract Catalog에 현재 저장소의 실제 IDL/topic 하나 등록
2. DynamicData vECU와 codegen SUT 간 type/QoS match
3. ARM64 vECU container 생성
4. Topology Compiler가 docker backend saved-service-list 생성
5. Run Coordinator가 lifecycle start 요청
6. container running과 VecuReady 구분 확인
7. sample N개 송수신 및 필드 검증
8. stop/restart 후 재연결과 idempotent cleanup 확인
9. Coordinator 강제 종료 후 `cleanup --run-id`로 고아 resource 회수
10. `MMF_DDS_DOMAIN_ID`와 `VISS_DDS_DOMAIN_ID` 주입 검증

---

## 18. 기존 MMF 수정 영향도

### 변경 없음

- lifecycle_mgr와 adsw_manager의 기존 상태기계 및 컨테이너 제어 경로
- ota_mgr, diagnostics, security_mgr 비즈니스 로직
- 기존 IDL 의미와 wire format

### 배포 설정 변경

- run별 `MMF_DDS_DOMAIN_ID` 주입
- VISS 참여 시 동일 값의 `VISS_DDS_DOMAIN_ID` 주입
- ROS2 참여 시 동일 값의 `ROS_DOMAIN_ID` 주입
- 외부 Bridge 참여 시 host IP, UDP port profile, ROS2 endpoint contract와 `ROS_DOMAIN_ID` 주입
- Topology Compiler가 backend별 saved-service-list와 manifest 생성

따라서 **SUT 코드 변경 0**은 초기 목표로 유지할 수 있지만 **SUT 배포 설정 변경 0은 아니다**.

### 신규 코드와 IDL

- `tools/sils/topology_compiler`
- `tools/sils/run_coordinator`
- `service/sils/vecu`, clock master, input gateway
- external Simulator/Bridge용 Supervisor와 선택적 Contract Adapter
- `idl/dds/sils/` control 및 tick IDL
- vECU Docker target과 build option

### 향후 변경 가능성

- SUT 포함 full-lockstep은 SUT clock/timer 인지가 필요해 코어 변경 가능
- DDS partition 사용은 기존 participant QoS 지원 추가 가능
- VISS provider를 vECU source로 사용할 실제 경로는 Contract Spike에서 검증

---

## 19. 열린 결정 사항

1. Run Coordinator 구현 언어와 lifecycle control client 방식
2. `VecuReport`의 transport 및 durability QoS
3. TickInbox의 freeze 시점과 late sample 기본 정책
4. Relay Gateway source/target topic naming 및 correlation 보존 규칙
5. CI domain ID 예산, 측정된 동시성 상한, partition/network namespace 전환 기준
6. Coordinator lease 저장 위치와 외부 finalizer 소유자
7. DynamicData type coercion 허용 수준
8. script behavior의 sandbox와 timeout
9. Clock Master 장애 시 checkpoint/restart 지원 여부
10. FMU 결합 시점과 co-simulation master 역할
11. external component를 직접 기동하는 managed 범위와 attach-only 범위
12. MORAI/Bridge UDP port allocator와 운영 네트워크 소유자
13. Bridge가 보장하는 K-Autoware canonical 변환 범위 및 인수 기준

---

## 20. 변경 이력

### v3 vECU 계층 경계 보강 (2026-07-15)

- vECU 내부를 ASW/Behavior, Virtual BSW, V-HAL, SiLS Host Adapter의 4개 논리 계층으로 명시
- 기존 `ExecutionKernel`, `IBehavior`, `IBus`, `IPlant`, `IClockSource`의 계층별 매핑 추가
- Host Adapter는 vECU에 포함하되 Run Coordinator, Clock Master, Topology Compiler와 MORAI 연동은 외부 SiLS 플랫폼으로 경계 설정
- `DLL 컴파일`을 runtime 책임에서 제외하고 `.dll`/`.so`/executable/FMU를 build/package 선택으로 구분
- L0~L4 fidelity별 ASW, Virtual BSW, V-HAL 구성 수준 정의

### v3 external simulator 보강 (2026-07-14)

- MORAI Simulator와 UDP–ROS2 Bridge를 vECU/Relay Gateway가 아닌 external component로 모델링
- managed/external lifecycle, Bridge readiness, ROS2 endpoint match와 fresh sample 조건 추가
- DDS domain 예산에 UDP port, Simulator instance/license와 GPU capacity를 추가
- 기존 Phase 1을 유지하고 후속 External Simulator Vertical PoC를 별도 정의
- 검증된 Bridge 기능은 재구현하지 않고 Supervisor/Contract Validator와 선택적 Adapter만 소유하도록 역할 분리

### v3 (2026-07-13)

- **SiLS Run Coordinator** 추가: compile, start, readiness, clock/scenario, 판정, teardown의 단일 소유자 정의
- 결정론 보장 조건을 “동일 tick-indexed 입력”으로 강화하고 `live/local-staged/relay-staged/replay/full-lockstep` 입력 모드 정의
- 비참여 SUT 입력을 소비 시점에 귀속하는 **TickInbox**, 명시적 topic relay를 수행하는 **Relay Gateway**, freeze/late sample 규칙 추가
- 실제 코드에 맞춰 `MMF_DDS_DOMAIN_ID`, `VISS_DDS_DOMAIN_ID`, 선택적 `ROS_DOMAIN_ID` binding 추가
- IDL만으로 topic을 검증할 수 없는 문제를 해결하기 위해 **Endpoint Contract Catalog** 추가
- docker/compose/K3s backend를 상호 배타적으로 선택하고 backend별 생성물 정의
- payload를 변경하지 않는 sidecar trace metadata와 publish intent/result 구분
- SUT 코드 변경과 배포 설정 변경을 구분
- DDS peer-to-peer 특성을 반영해 consumer-local **TickInbox**와 명시적 topic relay인 **Relay Gateway**를 분리
- domain pool/lease/동시성 예산과 partition 전환 판단 조건 추가
- Coordinator 장애 시 heartbeat/lease와 idempotent orphan cleanup 추가
- replay를 open-loop/hybrid/closed-loop 경계로 구분하고 live feedback 제한 명시
- Phase 1 Scope Gate를 추가해 Relay, lockstep, FMI, 다중 backend의 선행 풀 구현을 금지

### v2

- lockstep grant/ack, 확장 Descriptor, Topology Compiler, fidelity, run 격리, tick pipeline 도입

### v1

- 초기 개념 아키텍처

---

_문서 끝._
