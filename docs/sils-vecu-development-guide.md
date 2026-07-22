# SiLS vECU 개발 가이드 (libsils_vecu SDK)

이 문서는 **`libsils_vecu` SDK로 나만의 가상 ECU(vECU)를 개발·패키징·배포**하는 전체
흐름을 단계별로 설명한다. core 를 전혀 건드리지 않고 ECU 하나를 만드는 것이 목표다.

- **설계/경계 근거**: [sils-extension-model.md](sils-extension-model.md) (왜 이런 경계인가)
- **아키텍처 전체**: [sils-virtual-ecu-architecture-v3.md](sils-virtual-ecu-architecture-v3.md)
- **간단 퀵스타트**: [service/sils/examples/README.md](../service/sils/examples/README.md)
- **참조 구현**: [brake-ecu](../service/sils/examples/brake-ecu/) (코드/FMU 플랜트),
  [brake-ecu-native](../service/sils/examples/brake-ecu-native/) (커스텀 DDS 타입)

이 가이드는 위 문서들을 대체하지 않고, **"처음부터 끝까지 실제로 만드는 절차"**로 엮는다.

---

## 1. 큰 그림 — SDK가 주는 것 vs 내가 쓰는 것

vECU는 **작은 단일 프로세스 실행파일**이다. 자기 포트(behavior/plant/wire)를 만들어
`sils::RunProbe`에 넘기면, 나머지(DDS·틱 루프·trace·종료)는 SDK가 소유한다.

| SDK(`sils_vecu`)가 소유 — 재사용 | 개발자가 작성 — ECU마다 |
|---|---|
| DDS participant/topic/writer + QoS | `IBehavior` (제어 로직) |
| clock → behavior → publish → plant 틱 루프 | `IPlant` *(선택; 폐루프용)* |
| JSONL trace 계약, 시그널 처리·teardown | `IWireCodec` *(기본 VISS 재사용 가능)* |
| ConfigFromEnv (SILS_* 환경 파싱) | `main.cc` (~25줄), descriptor, Dockerfile |

### 헥사고날 포트 5개 (architecture v3 §11)
| 포트 | 헤더 | 소유 | 커스텀 빈도 |
|---|---|---|---|
| `IClockSource` | `clock_source.hpp` | RunProbe 내부(SILS_CLOCK_MODE) | 거의 안 함 |
| **`IBehavior`** | `behavior.hpp` | **주입** | 항상 |
| **`IPlant`** | `plant.hpp` | **주입** | 폐루프면 |
| `IScenario` | `scenario.hpp` | RunProbe 내부(SILS_SCENARIO) | 드묾 |
| **`IWireCodec`** | `wire.hpp` | **주입** | 커스텀 타입일 때만 |

`RunProbe` 시그니처:
```cpp
int RunProbe(const ProbeConfig& cfg,
             std::unique_ptr<IBehavior> behavior,
             std::unique_ptr<IPlant>    plant,   // nullptr = open loop
             std::unique_ptr<IWireCodec> wire);
```

> **결합 모드 2가지.** 위는 **VISS 요청 모델**(요청 발행/응답, `RunProbe`). 시뮬레이터
> (MORAI/AWSIM)와 폐루프로 붙는 vECU는 **플랜트 결합 모드**를 쓴다 — 버스에서 STATE를
> 구독하고 ACTUATION을 발행(§7). **아래 §2~§6은 VISS 모델 기준**이고, 플랜트 결합은
> §7을 참조. 둘 다 같은 SDK·같은 결정론 원칙을 공유한다.

---

## 2. 개발 절차

### Step 0 — 프로젝트 뼈대
가장 빠른 시작은 `service/sils/examples/brake-ecu/`를 복사하는 것이다.

- **In-tree**(이 레포 안): 디렉터리를 [examples/CMakeLists.txt](../service/sils/examples/CMakeLists.txt)에
  `add_subdirectory(my-ecu)`로 추가. `sils_vecu` 타겟에 직접 링크되어 SDK 경계를 컴파일-검증.
- **Out-of-tree**(별도 repo): `find_package(sils_vecu)` 후 동일 타겟 링크(T1-B install/export).

### Step 1 — `IBehavior` 구현 (필수)
매 틱 무엇을 내보낼지 계산한다. **결정성이 핵심**(같은 입력 → 같은 출력, golden 재현).

```cpp
#include "behavior.hpp"
class MyBehavior : public sils::IBehavior {
public:
    sils::BehaviorOutput OnTick(const sils::BehaviorContext& ctx) override {
        // ctx.tick_seq, ctx.sim_time_ns, ctx.staged_input_count, ctx.plant_feedback
        double cmd = /* 제어 로직 (예: P 제어) */;
        std::string req = /* 내부 VISS request json (wire가 envelope로 감쌈) */;
        return { req, cmd };   // {request_json, value(trace/golden 비교값)}
    }
    const char* name() const override { return "my-behavior"; }
};
```
- `ctx.plant_feedback`: 지난 틱의 플랜트 출력(폐루프 §9 L3).
- `value`: trace `behavior_output`으로 기록되고 golden 비교 대상(§11 L2).

### Step 2 — `IPlant` (선택, 폐루프면)
세 가지 경로 중 택1:

| 경로 | 방법 | 언제 |
|---|---|---|
| **open loop** | `plant = nullptr` | 물리 모델 불필요 |
| **code** | `IPlant` 직접 구현 (`Reset/SetInputs/Step/output`) | 간단한 C++ 모델 |
| **FMU** | `sils::MakeFmuPlant(cfg.fmu, err)` | 검증된 물리 모델(.fmu) |

```cpp
class MyPlant : public sils::IPlant {
    void  Reset(double initial) override { x_ = initial; }
    void  SetInputs(double command) override { u_ = command; } // 이번 틱 액추에이션
    double Step(int64_t dt_ns) override { x_ += (u_ - drag_*x_)*Sec(dt_ns); return x_; }
    double output() const override { return x_; }
    const char* name() const override { return "my-plant"; }
    const char* mode() const override { return "live"; }   // "live"|"replay"
};
```
FMU 경로는 [sils-fmu-plant.md](sils-fmu-plant.md) 참고. FMU 지원은 `-DSILS_ENABLE_FMU`로 빌드.

### Step 3 — `IWireCodec` 선택 (대개 재사용)
- **내장 VISS/raw** (권장 기본): `sils::MakeWireCodec(cfg.wire_kind, cfg.response_topic_prefix)`.
  DDS 타입·토픽 계약이 기본 probe와 **동일** → 상대편(responder/SUT) 무변경.
- **커스텀 네이티브 타입**: 자체 IDL + `IWireCodec` 구현
  ([brake-ecu-native](../service/sils/examples/brake-ecu-native/)).
  ⚠️ **이 경우만 DDS 타입/토픽 계약이 바뀌므로 상대편도 그 타입을 알아야 한다.**

### Step 4 — `main.cc` (~25줄, 전체 프로그램)
```cpp
#include "config.hpp"      // sils::ConfigFromEnv / GetEnvDoubleOr
#include "run_loop.hpp"    // sils::RunProbe
#include "viss_wire.hpp"   // sils::MakeWireCodec (내장 wire 쓸 때)
#include "my_behavior.hpp"
#include "my_plant.hpp"

int main() {
    const sils::ProbeConfig cfg = sils::ConfigFromEnv();

    // ECU 고유 knob은 SDK 헬퍼로 직접 읽는다 (SDK는 COMMON 계약만 소유)
    const double gain = sils::GetEnvDoubleOr("MY_GAIN", 0.02);

    auto behavior = std::make_unique<MyBehavior>(cfg.ctrl_kp, cfg.ctrl_setpoint);
    auto plant    = std::make_unique<MyPlant>(gain, cfg.plant_drag);   // 또는 nullptr / MakeFmuPlant
    auto wire     = sils::MakeWireCodec(cfg.wire_kind, cfg.response_topic_prefix);

    return sils::RunProbe(cfg, std::move(behavior), std::move(plant), std::move(wire));
}
```

### Step 5 — `CMakeLists.txt`
```cmake
add_executable(my_ecu src/main.cc)
target_include_directories(my_ecu PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(my_ecu PRIVATE sils_vecu sils_viss_idl)  # 내장 VISS wire 쓰면 sils_viss_idl
install(TARGETS my_ecu RUNTIME DESTINATION bin)
```

### Step 6 — descriptor (env 계약)
런타임 동작은 **전부 env 구동**이다. 아래는 `ProbeConfig` ↔ `SILS_*` 매핑(핵심 발췌,
전체는 [config.hpp](../service/sils/vecu/src/config.hpp)):

| env | 필드 | 기본 | 의미 |
|---|---|---|---|
| `VISS_DDS_DOMAIN_ID` | domain_id | 0 | **SUT와 동일해야 함** |
| `SILS_RUN_ID` | run_id | local | 런 식별자(trace) |
| `SILS_VECU_INSTANCE_ID` | instance_id | viss-probe-1 | vECU 고유 id(participant명+trace) |
| `SILS_TOPIC` | topic | /VIN/Vehicle | 계약 토픽 |
| `SILS_VISS_PROBE_PERIOD_MS` | period_ms | 500 | 틱 주기 |
| `SILS_CLOCK_MODE` | clock_mode | wall | wall\|lockstep |
| `SILS_WIRE` | wire_kind | viss | viss\|raw |
| `SILS_BEHAVIOR` | behavior_name | — | (기본 probe 팩토리 선택자) |
| `SILS_CTRL_KP`/`SILS_CTRL_SETPOINT` | ctrl_* | 0.8/10.0 | 컨트롤러 |
| `SILS_PLANT`/`SILS_PLANT_DRAG`/`SILS_PLANT_INIT` | plant_* | — | 플랜트 |
| `SILS_SCENARIO`/`SILS_SCENARIO_FAULTS` | scenario_* | — | 결함 주입 |
| (내 ECU 고유) | — | — | 예: `MY_GAIN` — SDK 헬퍼로 직접 읽음 |

`descriptors/my_ecu.yaml`에 이 run 정의를 적어둔다(참고: [brake_probe.yaml](../service/sils/examples/brake-ecu/descriptors/brake_probe.yaml)).

### Step 7 — 패키징 (단일 프로세스 컨테이너)
`entrypoint.sh` — 바이너리를 **PID 1로 exec**해 `docker stop`의 SIGTERM이 직접 닿게:
```bash
#!/bin/bash
set -u
export LD_LIBRARY_PATH="${LD_LIBRARY_PATH:-/oss/lib}"
[ -x /oss/bin/my_ecu ] || { echo "FATAL: my_ecu missing" >&2; exit 1; }
exec /oss/bin/my_ecu
```
Dockerfile은 `docker/Dockerfile.rel`의 `runtime-mmf-sils-vecu` 스테이지가 정석 예다
(ubuntu:22.04 + FastDDS 런타임 + 바이너리 + entrypoint, **ROS2 없음**). 자체 예는
[brake-ecu/Dockerfile](../service/sils/examples/brake-ecu/Dockerfile).

---

## 3. 실행·배포

### A. 로컬 단독 실행 (가장 빠른 확인)
```bash
VISS_DDS_DOMAIN_ID=0 SILS_RUN_ID=dev SILS_VECU_INSTANCE_ID=my-1 \
MY_GAIN=0.02 ./output/x86_64/Debug/core/bin/my_ecu
```

### B. MMF lifecycle_mgr 연동 (경로 B — 실 오케스트레이션)
`general/adsw_manager`가 컨테이너를 관리하고 `lifecycle_mgr`가 정책으로 기동한다.
**ROS2 불필요.** 절차:

1. 이미지 빌드 → `docker/build_local.sh`에 타겟 추가하거나 자체 Dockerfile로 `my-ecu:local`
2. speclist에 등록 — [e2e/config/saved-service-list.yaml](../service/extension/general/adsw_manager/e2e/config/saved-service-list.yaml)
   형식으로 `containers[].image`/`extra_args`(env)/`services[].recovery` 지정
3. `adsw_manager_general` + `lifecycle_mgr`를 같은 `MMF_DDS_DOMAIN_ID`에 기동

핵심 동작(검증된 흐름):
```
adsw UpdateServiceList(container 등록)
  → lifecycle_mgr 등록 → PolicyAlways(NOT_RUNNING→START, recovery:ALWAYS일 때)
  → StartServiceRequest → adsw ContainerController → docker run my-ecu:local
```
- `recovery: ALWAYS` → 등록만 하면 **자동 기동**(트리거 불필요).
- `recovery: MANUAL` → 운영자 ctrl 명령(CtrlStartContainer)이 있어야 기동.
- 참조 e2e: [e2e/run_lifecycle_e2e.sh](../service/extension/general/adsw_manager/e2e/run_lifecycle_e2e.sh)
  (probe), [e2e/run_brake_e2e.sh](../service/extension/general/adsw_manager/e2e/run_brake_e2e.sh) (brake).
  두 변형은 speclist·이미지·env만 다르고 lifecycle_mgr·adsw·DDS 계약은 무변경.

---

## 4. 검증 (충실도 레벨별)

| 레벨 | 무엇 | 어떻게 |
|---|---|---|
| 빌드/타입 | 컴파일·링크·DDS 타입 | `./build.sh --build-sils` |
| L0/L1 | trace 스키마 적합 | [conformance gate](../tools/sils/conformance/gate.py) — schema→fidelity→parity, fail-closed |
| L2 | behavior 결정성 | golden scenario 재현(같은 틱 입력 → 같은 `value` 열) |
| L3 | 폐루프 물리 | plant 연동 trace 비교 |
| 타깃 | ARM64 동등성 | [sils-target-parity.md](sils-target-parity.md) |

trace 스키마: [sils-trace-schema.md](sils-trace-schema.md) / `config/sils/trace-schema/v1.json`.

---

## 5. 개발 체크리스트

- [ ] `IBehavior::OnTick`이 **결정적**인가 (같은 ctx → 같은 출력)
- [ ] 폐루프면 `IPlant` 3경로(code/FMU/none) 중 선택했는가
- [ ] wire: 내장 VISS 재사용? 커스텀이면 **상대편 타입 계약도** 맞췄는가
- [ ] `main.cc`가 `ConfigFromEnv` → 포트 조립 → `RunProbe` 형태인가
- [ ] ECU 고유 knob은 `GetEnv*Or`로 직접 읽는가(SDK는 COMMON만)
- [ ] descriptor에 env 계약 문서화 + `VISS_DDS_DOMAIN_ID`가 SUT와 일치하는가
- [ ] entrypoint가 바이너리를 **PID 1로 exec**하는가(graceful stop)
- [ ] conformance gate 통과하는가
- [ ] (배포 시) speclist 등록 + recovery 정책(ALWAYS/MANUAL) 결정했는가

---

## 6. 자주 하는 결정

- **plant 없이 시작** → 배관부터 확인 후 폐루프 추가.
- **wire는 내장 VISS 유지** → 상대편 계약 안 건드림. 커스텀 타입은 정말 필요할 때만.
- **recovery: 스모크는 ALWAYS**(자동 기동), **정밀 start/stop 제어는 MANUAL**.
- **core를 바꾸고 싶어지면** 멈추고 [sils-extension-model.md](sils-extension-model.md)를
  다시 볼 것 — 대부분 포트 주입으로 해결된다(core 변경 불필요가 이 SDK의 목적).

---

## 7. 플랜트 결합 모드 — MORAI/AWSIM 연동 (Option 2)

§2~§6의 **VISS 요청 모델**과 달리, 시뮬레이터(MORAI/AWSIM)와 폐루프로 붙는 vECU는
**버스 위 공유 세계**(authority=sim)에서 **STATE를 구독하고 ACTUATION을 발행**한다.
플랜트가 in-process `IPlant`가 아니라 **DDS 버스 너머**에 있고, 폐루프는 시뮬레이터가
버스에서 닫는다. 설계 근거·3안 비교: [sils-morai-plant-adapter-design.md](sils-morai-plant-adapter-design.md),
토폴로지: [sils-morai-autoware-topology.html](sils-morai-autoware-topology.html) ①/④.

### 두 모드 비교

| | VISS 요청 (§2~§6) | 플랜트 결합 (이 절) |
|---|---|---|
| 진입점 | `RunProbe` (`run_loop.hpp`) | `sils_vecu_vehicle_probe` (`vehicle_probe_main.cc`, 독립) |
| 방향 | 요청 발행 → 응답 | **STATE in → CONTROL out** |
| DDS 타입 | `VissRequestTopic` | 정규 `EgoState` / `VehicleControl` (구조체) |
| 코덱 | `IWireCodec` (`viss_wire.hpp`) | `VehicleWireCodec` (`vehicle_wire.hpp`) |
| 토픽 선언 | 단일 토픽(`SILS_TOPIC`) | **채널맵**(`SILS_CHANNELMAP`, N채널 MIMO) |

### 핵심 조각

- **정규 계약** — [EgoState.idl](../idl/dds/sils/EgoState.idl)(STATE in)·[VehicleControl.idl](../idl/dds/sils/VehicleControl.idl)(ACTUATION out).
  **SI 단위 고정**(m·rad·m/s·m/s²). 시뮬레이터가 자기 메시지를 이 정규 타입으로 **번역**한다
  (anti-corruption layer). **MORAI→DDS 어댑터는 외부 컴포넌트**(SiLS가 만들지 않음); 그 어댑터가
  deg→rad·km/h→m/s 변환을 소유한다. → vECU는 시뮬 종류와 무관하게 정규 타입만 본다.
- **채널맵** — [config/sils/channelmap/*.yaml](../config/sils/channelmap/). 채널마다
  `name`(VSS path)·`dir`(in/out)·`topic`·`type`. `SILS_CHANNELMAP`로 지정, 비면 비활성.
  **채널을 추가하면 MIMO 확장**(예: Autoware 제어 입력 채널 추가). 파서: `channel_map.hpp`.
- **코덱** — `vehicle_wire.hpp`의 `VehicleWireCodec`: 생성 DDS 구조체 ↔ SDK POD
  (`EgoStateSample`/`VehicleControlSample`) 변환. behavior/런타임은 POD만 보고 생성 타입 무의존.
- **레퍼런스 실행파일** — `sils_vecu_vehicle_probe`(채널맵 구동 free-run 프로브),
  `sils_ego_source`(테스트용 EgoState 발행 fixture, MORAI 없이 왕복 실증용).

### 로컬 왕복 확인 (MORAI 없이)

```bash
export VISS_DDS_DOMAIN_ID=0 SILS_RUN_ID=rt
# 터미널 A — vECU 프로브 (먼저 띄워 구독 준비)
SILS_CHANNELMAP=config/sils/channelmap/ego-min.yaml <bin>/sils_vecu_vehicle_probe
# 터미널 B — EgoState 소스 (1초 뒤 발행)
<bin>/sils_ego_source --count 20 --rate-ms 100
```
프로브에 `state_received`(vx가 오름) → `control_published`(제어가 반응)가 흐르면 왕복 성립.

### 성숙도 (2026-07 현재)

- **S1 free-run 완료·머지** — 채널맵·정규 계약·코덱·프로브·fixture, 폐루프 왕복 검증.
- **예정**: S2 tick 경계 staging(결정론 — free-run은 최신 샘플만 채택), S3 lockstep(시뮬 tick 동기),
  Autoware 제어 입력(스키마 확인 후 채널 추가).
- 현재 제어 로직은 **stand-in P-controller** — 전용 behavior 포트(`IVehicleBehavior`)는
  두 번째 실소비자가 생기기 전엔 도입하지 않는다(earned-abstraction). 실제 ECU는
  `vehicle_probe_main.cc`의 `ComputeControl` 자리에 제어 로직을 넣는다.
