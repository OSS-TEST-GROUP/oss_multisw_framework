# SiLS 확장 모델 (OSS) — libsils_vecu SDK + 개발자 ECU + Plant(FMU|코드)

> **설계 결정(ADR) + 단계별 계획.** SiLS 를 OSS 로 공개했을 때 외부 개발자가 "자기 ECU"를
> core 를 건드리지 않고 추가하는 확장 경계를 정의한다. 현재-상태 설명은
> [sils-understanding.md](sils-understanding.md), 이 문서는 **앞으로 이렇게 만든다**는 방향.

관련: 확장 옵션 비교·장단점은 sils-understanding 5장(§5.7) 논의 참조.

---

## 1. 결정 요약

세 부분으로 **세분화**한다:

1. **`libsils_vecu` (공통 라이브러리)** — 포트 인터페이스 + tick 루프 + DDS 배선 + trace 방출을
   링크 가능한 라이브러리로 뽑는다. core 가 제공·버전관리.
2. **개발자 ECU (각자 레포)** — 개발자는 `libsils_vecu` 를 링크하고 **자기 IBehavior/IPlant** 를
   구현해 **자기 실행파일·이미지**를 만든다. core 무수정 → 머지 충돌·비대화 없음.
3. **Plant 세분화 — FMU vs 코드** — 물리 모델(IPlant)은 개발자 선택: `.fmu` 파일(FMU 경로) 또는
   직접 C++ IPlant 구현(코드 경로). 둘 중 무엇이든 **나머지는 동일**.

## 2. 진짜 공개 경계 = 계약 + trace 스키마 (버전)

C++ ABI 가 아니라 **① DDS Contract Catalog(topic·type·QoS, `semantic_version` 보유) + ② trace
이벤트 스키마**가 버전관리 대상이다. `libsils_vecu` 는 그 경계를 쉽게 지키게 해주는 **SDK**이지
유일한 길이 아니다(임의 언어 컨테이너도 계약+스키마만 지키면 유효 — 미래 옵션).

> 소스/헤더 배포 + 제공 builder 이미지에서 정적 링크 → **API 안정성만** 필요, ABI 무관.

## 3. 세 부분의 구체 형태

### A. `libsils_vecu` (공통)
```
include/sils/
  clock_source.hpp  behavior.hpp  plant.hpp  scenario.hpp   # 포트 인터페이스 + 기본 구현
  run_loop.hpp      # RunProbe(cfg, behavior, plant, scenario) — tick 루프
  transport.hpp     # DDS participant/topic/writer/QoS (계약 구동)
  trace.hpp         # trace 이벤트 방출 (공개 스키마)
  config.hpp        # ConfigFromEnv() — SILS_* 파싱
```
기본 구현(WallClock/Lockstep, viss-request/accumulate/p-controller, Analytic/Replay/Fmu,
Nominal/Scheduled)도 함께 제공 → 개발자가 재사용하거나 자기 것으로 대체.

### B. 개발자 ECU (각자 레포)
```
my-brake-ecu/
  src/brake_behavior.hpp     # 자기 IBehavior (제어 로직)
  src/brake_plant.hpp        # 자기 IPlant (코드 경로)  ─┐ 택1
  models/brake.fmu           # 또는 .fmu (FMU 경로)      ─┘
  src/main.cc                # 포트 조립 + sils::RunProbe(...)
  descriptors/brake_probe.yaml
  idl/BrakeCommand.idl       # (필요시) 커스텀 신호 타입
  CMakeLists.txt             # target_link_libraries(my_brake_ecu sils_vecu)
  Dockerfile                 # COPY 바이너리 + entrypoint exec
```
```cpp
// main.cc — 개발자가 쓰는 전부
#include <sils/run_loop.hpp>
#include "brake_behavior.hpp"
#include "brake_plant.hpp"     // 코드 경로
int main() {
    auto cfg      = sils::ConfigFromEnv();
    auto behavior = std::make_unique<BrakeBehavior>(cfg.kp, cfg.setpoint, 100.0);
    auto plant    = std::make_unique<BrakePlant>(cfg.gain, cfg.drag);   // ── 코드 경로
    // auto plant = sils::MakeFmuPlant(cfg);                            // ── FMU 경로 (이 줄만 교체)
    return sils::RunProbe(cfg, std::move(behavior), std::move(plant));
}
```

### C. Plant 세분화 — FMU vs 코드 (개발자 선택)

| | 코드 경로 (C++ IPlant) | FMU 경로 (.fmu) |
|---|---|---|
| 물리 모델 | 직접 C++ 클래스 (`BrakePlant`) | Simulink/Dymola 등 export `.fmu` |
| main | `make_unique<BrakePlant>(...)` | `sils::MakeFmuPlant(cfg)` |
| descriptor | `plant: {type: brake}` | `plant: {type: fmu, fmu:{file,input_vr,output_vr,allowlist_id}}` |
| 추가물 | 없음 | `.fmu` 파일(컨테이너 FS) · fmu_map · fmu_gate(§5.6) · `-DSILS_ENABLE_FMU` · arch별 바이너리 |
| **나머지(behavior·lib·IDL·trace·컨테이너)** | **동일** | **동일** |

→ plant 자리 하나만 다르고 전부 동일. ECU 마다 섞어도 됨(brake=fmu, temp=코드, gateway=none).

## 4. 안 변하는 것 (경계의 안정성)

behavior(제어 로직) 작성 방식 · 공통 IDL·계약·QoS · **trace 스키마** · main 조립 관례 ·
컨테이너 entrypoint(probe PID1) · Coordinator/MMF 기동 경로(§4.9) — plant 선택과 무관하게 동일.

## 5. 현재 → 목표 갭 & 단계별 계획

현재: 헤더온리 + 단일 실행파일(`sils_vecu_viss_probe`), tick 루프가 `viss_probe_main.cc` 인라인,
팩토리 하드코딩, wire 타입 `VissRequestTopic` 고정.

| 단계 | 할 일 | 산출 |
|---|---|---|
| **P1. 라이브러리 추출** | tick 루프 → `RunProbe()`/run_loop.hpp API, `add_library(sils_vecu)` + install/export, `viss_probe_main.cc` 를 라이브러리 소비자로 축소 | `libsils_vecu` |
| **P2. 개발자 템플릿** | `service/sils/examples/brake-ecu/`(behavior·plant·main·CMake·Dockerfile·descriptor·README) + `examples/README.md` "내 ECU 시작하기" 가이드 | 외부 개발 시작점 |
| **P3. Plant 세분화 정리** | 코드 경로/FMU 경로 문서·예제 각각, `MakeFmuPlant(cfg)` 공개 API | plant 선택 가이드 |
| **P4. wire 타입 일반화** | (P4a) `IWireCodec` 포트로 transport 추상화 — 인코딩 일반화; (P4b) 계약 `registered_type` 구동 커스텀 IDL codec | 네이티브 신규 타입 |
| **P5. 스키마 명세 + conformance kit** | trace 이벤트를 버전 있는 공개 명세로, evaluate/parity 를 "외부 컨테이너 검사기"로 패키징 | 계약+스키마 경계 확정 |

P1 이 토대(나머지가 그 위에 얹힘). P4·P5 는 계약-레벨 경계로 갈 때.

## 6. 상태

| 항목 | 상태 |
|---|---|
| 확장 방향 결정(본 문서) | ✅ |
| 포트 인터페이스(IBehavior/IPlant/…) | ✅ 존재(헤더온리) |
| FMU 경로(FmuPlant·fmu_map·fmu_gate) | ✅ 존재 |
| **P1a `sils_vecu` 라이브러리 타깃** | ✅ CMake INTERFACE 라이브러리(포트 헤더 + fastdds/fastcdr + FMU 플래그 handle), 5개 실행파일이 링크로 dogfood. **빌드 통과 확인** |
| **P1b RunProbe/run_loop.hpp 추출**(tick 루프 → 라이브러리) | ✅ `config.hpp`(ConfigFromEnv→ProbeConfig) + `run_loop.hpp`(RunProbe: DDS·tick 루프·trace·teardown) 추출, `viss_probe_main.cc` 626→~40줄 소비자로 축소. behavior/plant 주입, clock/scenario 는 cfg 내부 생성. **빌드 통과 확인** |
| **P2 개발자 템플릿** | ✅ `service/sils/examples/brake-ecu/` 워크드 예제(BrakeBehavior/BrakePlant 코드경로 + main + CMake + descriptor + Dockerfile/entrypoint + README) + `examples/README.md` 가이드. `sils_vecu` 를 링크하는 첫 외부 소비자, `SILS_BUILD_EXAMPLES=ON` 로 SiLS 빌드에 포함(dogfood). **빌드 통과 확인** |
| **P3 Plant 세분화(FMU경로)** | ✅ `MakeFmuPlant(FmuConfig&)`+`MakeFmuPlant(ProbeConfig&)` 공개 API(plant.hpp/config.hpp, MakePlant 가 위임) + brake-ecu FMU 대칭 예제(`src/main_fmu.cc`, 코드경로와 plant 한 줄만 상이) + `brake_ecu_fmu` 타깃(SILS_ENABLE_FMU) + `models/brake-fmu/`(손수 짠 FMI 2.0 co-sim 소스 `brake_vehicle.c`·modelDescription.xml·build.sh, **바이너리 미포함/재현빌드**) + 코드경로 vs FMU경로 선택 가이드. **리눅스에서 빌드+FMU 로드+폐루프 실행 확인**. 검증 중 3개 픽스: ①FmuPlant 플랫폼 preflight(wrong-arch→크래시 대신 에러) ②modelDescription 루트 앞 주석 제거(ezxml 파싱 실패→fmi4c SIGSEGV) ③build.sh Darwin ARCHFLAGS |
| **P4a IWireCodec 포트** | ✅ `wire.hpp` — `IWireCodec`(RegisterType/type_name/Publish) 포트 + `VissWireCodec`(현재 VISS 봉투, 동작 보존) + `RawValueWireCodec`(같은 타입·봉투 없는 스칼라). RunProbe 가 wire 를 주입받거나 `cfg.wire_kind`(SILS_WIRE)로 생성, 타입 등록·payload 인코딩·corrupt/duplicate 를 codec 에 위임. RequestId/봉투 헬퍼 wire.hpp 로 이동. **빌드+실행 확인**(VISS trace 불변, `SILS_WIRE=raw` 로 `wire_start` name=raw-value 전환) |
| **P4b 커스텀 IDL codec (네이티브 신규 타입)** | ✅ `service/sils/examples/brake-ecu-native/` — `idl/BrakeCmd.idl`(네이티브 타입, 예제 CMake 가 `fastddsgen` 자체 호출 = core idl/dds 무수정·비-SILS 빌드 미생성) + `BrakeCmdWireCodec`(BrakeCmd 등록·타입필드 발행) + main(behavior/plant 는 ../brake-ecu 재사용, wire codec 한 줄만 주입). **빌드+실행 확인**(wire_start name=brake-cmd-native/type=BrakeCmd, plant_step 동일) |
| **T1-A 읽기 경로 codec 화 (P4b 완성)** | ✅ 포트에 읽기 대칭 `WireInbound`+`Take()` 추가(`wire.hpp`) → 두 리스너(reqresp 응답·입력 스테이징)가 concrete 타입 대신 codec 위임. VISS 구현을 `viss_wire.hpp` 로 분리 → **`wire.hpp`/`run_loop.hpp` 가 DDS 타입 무명명**. `wire` = RunProbe 필수 인자(기본 폴백 제거, 호출부가 주입). `BrakeCmdWireCodec::Take` 추가 + brake-ecu-native CMake 의 VISS 재-glob 제거 → **커스텀 타입 ECU 가 VISS 생성소스 미컴파일**(발행·응답·입력 100% 네이티브). **빌드+실행 확인**(PR #203: wire_start type=BrakeCmd, VISS trace 불변, 스키마 PASS) |
| **T1-B SDK install/export (재-glob 제거)** | 🟡 생성 IDL 을 `sils_tick_idl`/`sils_viss_idl` **STATIC 라이브러리로 한 번만 컴파일** → 5개 실행파일·2개 예제의 `file(GLOB ...GENERATED_SRCS)` 재컴파일 제거(link 만). `sils_vecu`(INTERFACE)가 tick 을 전파, VISS wire 소비자만 `sils_viss_idl` 추가 link(브레이크-네이티브는 미링크). `find_package(sils_vecu)` 지원: `install(TARGETS ... EXPORT)` + 헤더(src/*.hpp + 생성 IDL) 설치 + `sils_vecuConfig.cmake`(BUILD/INSTALL interface, `sils::sils_vecu` alias). FMU 빌드는 fmi4c 의존성 때문에 export 스킵(런타임 바이너리는 설치). **빌드+실행 확인(PR #205)** |
| **P5 스키마 명세·conformance kit** | ✅ `config/sils/trace-schema/v1.json`(머신 리더블 이벤트 레지스트리, 실물 trace 로 required 필드 근거) + `docs/sils-trace-schema.md`(사람용 스펙·버전정책) + `tools/sils/conformance/validate_trace.py`(스키마 검사기, fail-closed, 기본 lenient/`--strict`) + `tools/sils/conformance/README.md`(외부 검사기 패키징). 샌드박스 검증: 실물 trace 8종 PASS·bad input 포착 |
| **T1-C conformance kit 완결 (게이트+스키마 정합)** | ✅ 단일 `gate.py`(schema→fidelity→optional parity, fail-closed, `--junit`) — 검사기들을 라이브러리로 오케스트레이션. 게이트 작성 중 발견: 스키마 v1 이 **coordinator producer 전체 누락**(evaluate 가 의존하는 `run_pass`/`domain_leased`… 이 unknown-event) + `run_start` 이중-producer(coordinator/vecu) 필드 불일치. 스키마에 coordinator 21개 이벤트 + `verify_fail` 추가, `run_start` required 를 교집합(`instance_id`/`wall_time_ms`/`domain_id`)으로 완화(backward-compatible, v1 유지). 커밋 샘플 `samples/nominal.trace.jsonl`(완결 merged trace, validate --strict+gate PASS) + `test_gate.py` + `.github/workflows/sils-conformance.yml`(pytest+gate). **샌드박스 검증: 130 테스트 통과·실물 trace 8종 무회귀·CI 스텝 로컬 재현** |
