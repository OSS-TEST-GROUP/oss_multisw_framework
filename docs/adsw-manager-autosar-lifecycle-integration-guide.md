# Adaptive AUTOSAR `adsw_manager` ↔ MMF `lifecycle_manager` 통합 가이드

> 작성일: 2026-07-05
> 대상 독자: Adaptive AUTOSAR 플랫폼에서 `adsw_manager` 를 신규 구현하는 팀
> 범위: MMF `lifecycle_manager`(C++ core service) 와의 **연동 계약(contract)** 정의 및
>       Adaptive Platform(AP) 매핑 가이드
> 기준 소스: `oss_multisw_framework` — `idl/dds/lifecycle/*`, `service/core/lifecycle_mgr/*`,
>            `service/extension/{agl,ros2}/adsw_manager/*`

---

## 0. 요약 (TL;DR)

- `adsw_manager` 는 **특정 OS 도메인(AGL / ROS2 / AUTOSAR)의 애플리케이션(=service)들을 실제로
  기동·종료·감시하는 "액추에이터/에이전트"** 이고, `lifecycle_manager` 는 **모든 OS 도메인을
  가로질러 상태를 집계·정책결정하는 "단일 애그리게이터/오케스트레이터"** 다.
- 둘은 **FastDDS bare-DDS 토픽**(`mmf/lifecyclemgr/*`)으로만 통신한다. 언어·플랫폼 독립.
- **`MwType` enum 에 이미 `AUTOSAR` 가 정의**되어 있다 (`LifecycleCommon.idl`). AP adsw_manager 는
  **기존 토픽을 그대로 재사용**하고, 발행 시 `mw_type = AUTOSAR`, 수신 시 `mw_type == AUTOSAR`
  필터만 하면 프레임워크에 그대로 붙는다. **IDL 변경 불필요.**
- 구현해야 할 것은 새 프로토콜이 아니라 **6개 토픽에 대한 pub/sub + AP 표준 컴포넌트(EM/SM/PHM)
  와의 브릿지** 다.

---

## 1. 두 컴포넌트의 관계 (근본)

```
        ┌──────────────────────── lifecycle_manager (core, 단일 인스턴스) ────────────────────────┐
        │  ServiceRegistry(모든 OS의 service/container) · StateManager(정책·복구) · StateMonitor    │
        │  주기적 LifecycleDiagReport 발행 (집계 결과) → dds_bridge → lfc_monitor(HMI)              │
        └───▲───────────────▲──────────────────────────────────┬──────────────────┬───────────────┘
            │ 등록/상태(adsw→lfc)                                │ 제어(lfc→adsw)    │ 제어
   ┌────────┴─────────┐   ┌─┴──────────────┐          ┌─────────┴────────┐  ┌──────┴──────────┐
   │ adsw_manager     │   │ adsw_manager   │   ...    │ adsw_manager     │  │ (신규)          │
   │  mw_type = ROS2  │   │  mw_type = AGL │          │  mw_type = AGL   │  │ mw_type=AUTOSAR │
   └──────────────────┘   └────────────────┘          └──────────────────┘  └─────────────────┘
        실제 앱 기동·감시         실제 앱 기동·감시                                  ara::exec/sm/phm 로 AP 앱 관리
```

- **lifecycle_manager 는 하나**. 모든 adsw_manager(OS별)가 여기로 등록하고 상태를 보고한다.
- **adsw_manager 는 OS 도메인마다 하나**. AUTOSAR 도메인용 adsw_manager 가 이 가이드의 구현 대상.
- lifecycle_manager 는 **직접 앱을 기동하지 않는다.** 정책(어떤 서비스를 켜라/꺼라/OTA모드로 바꿔라)을
  결정하고, 해당 OS 의 adsw_manager 에게 **위임**한다. 실제 `systemctl`/`ara::exec`/`docker` 조작은
  adsw_manager 몫이다. (관측·정책 = lifecycle / 실행·감시 = adsw, **책임 분리**)

---

## 2. 통합 계약 (SSOT) — 6 + 1 토픽

모든 토픽은 **module `lifecyclemgr`** (헬스만 `adswmgr`), bare-DDS 이름 `mmf/…` 를 쓴다.
Req/Res 쌍은 **요청 토픽 + 응답 토픽** 2개로 구성된다 (RPC 아님, 단방향 pub 2개로 흉내).

| # | 토픽 (`mmf/…`) | 방향 | 종류 | IDL 타입 | mw_type 필터 |
|---|---|---|---|---|:-:|
| 1 | `lifecyclemgr/UpdateServiceListRequest` | **adsw → lfc** | 등록 req | `UpdateServiceListRequest` | 발행값=AUTOSAR |
|   | `lifecyclemgr/UpdateServiceListResponse` | lfc → adsw | 등록 res | `UpdateServiceListResponse` | — |
| 2 | `lifecyclemgr/NotifyServiceState` | **adsw → lfc** | 상태 통지 | `NotifyServiceState` | 발행값=AUTOSAR |
| 3 | `lifecyclemgr/NotifyContainerState` | **adsw → lfc** | 컨테이너 통지 | `NotifyContainerState` | (해당 시) |
| 4 | `lifecyclemgr/StartServiceRequest` | **lfc → adsw** | 제어 req | `StartServiceRequest` | **수신 필터=AUTOSAR** |
|   | `lifecyclemgr/StartServiceResponse` | adsw → lfc | 제어 res | `StartServiceResponse` | — |
| 5 | `lifecyclemgr/StopServiceRequest` | **lfc → adsw** | 제어 req | `StopServiceRequest` | **수신 필터=AUTOSAR** |
|   | `lifecyclemgr/StopServiceResponse` | adsw → lfc | 제어 res | `StopServiceResponse` | — |
| 6 | `lifecyclemgr/SetServiceModeRequest` | **lfc → adsw** | 모드 req | `SetServiceModeRequest` | **수신 필터=AUTOSAR** |
|   | `lifecyclemgr/SetServiceModeResponse` | adsw → lfc | 모드 res | `SetServiceModeResponse` | — |
| + | `lifecyclemgr/GetServiceStateRequest` | lfc → adsw | 상태조회 req | `GetServiceStateRequest` | 수신 필터=AUTOSAR |
| H | `adswmgr/AdswServiceStatus` | **adsw → diagnostics** | 헬스 | `adswmgr::AdswServiceStatus` | — |

> **핵심 라우팅**: 모든 adsw_manager 가 위 토픽을 **공유**한다. lifecycle_manager 는 제어 메시지에
> 대상 `mw_type` 를 스탬프하고, 각 adsw_manager 는 `req.mw_type() == <자기 MwType>` 인 것만 처리한다.
> (agl 구현 참조: `MessageTransmitter.cpp:48` 발행 스탬프, `:100/:122/:141` 수신 필터)

---

## 3. 흐름별 상세

### 3.1 등록 (Registration) — adsw 가 시작하며 자기 서비스 목록을 알린다

```
adsw(AUTOSAR)  ──UpdateServiceListRequest(service_list[], container_list[])──▶  lifecycle_manager
adsw(AUTOSAR)  ◀──────────UpdateServiceListResponse(ret_code)───────────────  lifecycle_manager
```

- `service_list` 의 각 `Service` 필드:

  | 필드 | 의미 | AUTOSAR 매핑 권고 |
  |---|---|---|
  | `name` | 서비스(앱) 고유 이름 | AA(Adaptive Application) 프로세스명 또는 Function Group 항목명 |
  | `service_type` | `NORMAL`(상주) / `ONESHOT`(1회) | 상주 AA=NORMAL, 초기화용 1회 실행=ONESHOT |
  | `recovery_policy` | 복구 정책 (§3.5) | PHM/EM 재시작 정책과 매핑 |
  | `is_enabled` | 부팅 시 자동 기동 여부 | Function Group 기본 State 포함 여부 |
  | `mw_type` | **`AUTOSAR`** 고정 | — |
  | `container_name` | 소속 컨테이너("" = native) | AP 는 보통 native → `""` |

- **필수**: 등록 성공(`ret_code == SUCCESS`) 확인 후 내부 상태를 RUNNING 으로 올린다(§4).
- **재전송(idempotent)**: 응답이 안 오면 재발행해야 한다. lifecycle_manager 가 늦게 떠도
  adsw 의 주기적 재발행으로 결국 등록이 수렴한다 (agl 의 `PublishAppList` 재시도 로직 참조).

### 3.2 상태 통지 (NotifyServiceState) — adsw 가 실제 상태를 주기/이벤트로 보고

```
adsw(AUTOSAR) ──NotifyServiceState(mw_type=AUTOSAR, service_list[], service_state_list[])──▶ lifecycle_manager
```

- `service_list[i]` 와 `service_state_list[i]` 는 **병렬 배열** (같은 인덱스가 한 서비스).
- `ServiceState` enum: `NOT_RUNNING / INITIALIZING / RUNNING / RUNNING_NO_RESPONSE / TERMINATING`.
- lifecycle_manager 는 이 통지를 받아 `ServiceRegistry` 를 갱신하고 **`LifecycleDiagReport`** 에
  접어 넣어 HMI 로 내보낸다. → **adsw 가 상태를 안 올리면 HMI 에 그 서비스가 안 보인다.**
- 발행 시점: 상태 변화 이벤트 + 주기적 재동기화(late-join 대비) 둘 다 권장.

### 3.3 제어 (Start / Stop) — lifecycle 이 정책 결정 후 adsw 에 위임

```
lifecycle_manager ──StartServiceRequest(mw_type=AUTOSAR, service_list[], is_group)──▶ adsw(AUTOSAR)
lifecycle_manager ◀──────────────StartServiceResponse(ret_code)────────────────────  adsw(AUTOSAR)
```

- adsw 는 **`req.mw_type() == AUTOSAR` 일 때만** 처리하고, 아니면 무시(다른 OS 대상).
- `is_group == true` 면 `service_list` 를 그룹으로 취급 (묶음 기동/종료).
- 처리: AP 에서 실제 앱 기동/종료 수행(§5) → 결과를 `*Response(ret_code)` 로 발행.
- Stop 도 동일 형태 (`StopServiceRequest`/`Response`).

### 3.4 OTA 모드 전환 (SetServiceMode)

```
lifecycle_manager ──SetServiceModeRequest(mw_type=AUTOSAR, service_list[], is_group, service_mode)──▶ adsw
lifecycle_manager ◀───────────────────SetServiceModeResponse(ret_code)──────────────────────────────  adsw
```

- `ServiceMode`: `NORMAL` / `OTA_MODE`.
- OTA 업데이트 중 대상 앱을 안전 상태로 전이시키는 용도 (예: 워크로드 정지·저전력·쓰기잠금).
- AUTOSAR 매핑: State Management 의 별도 Machine/Function Group State(예: `UpdateMode`)로 전이.

### 3.5 복구 정책 (RecoveryPolicy)

`R_DEFAULT / ALWAYS / ON_FAILURE / ON_SUCCESS / ON_NOT_RESPONDING / MANUAL`
- 정책 자체는 등록 시 lifecycle 에 전달되지만, **실제 복구 실행 주체는 adsw_manager** 다
  (프로세스가 죽었을 때 재시작할지 여부). AP 에서는 **PHM(Platform Health Management) 감시 +
  EM 재시작** 또는 adsw 자체 감시 루프로 구현한다.
- 복구 발생 시 `LifecycleDiagReport.ServiceStatus.recovered_count / exit_code` 로 노출되므로,
  adsw 는 재시작 횟수·마지막 exit code 를 상태 통지에 반영하는 게 좋다.

### 3.6 컨테이너 (선택) — AP native 면 대개 불필요

- `container_list`(등록) + `NotifyContainerState`(상태)는 docker/k3s 등 컨테이너 오케스트레이션을
  쓰는 도메인용이다. **AP 앱이 native 프로세스면 `container_list=[]`, NotifyContainerState 미발행**
  으로 두면 된다. (컨테이너 개념 없이도 서비스 단위로 완전히 동작함)

### 3.7 헬스 (AdswServiceStatus) — diagnostics 로 가는 별도 라인

```
adsw(AUTOSAR) ──AdswServiceStatus(status_code, status_msg)──▶ Diagnostic Manager (policy code 12)
```
- **module 이 `adswmgr`** (lifecyclemgr 아님), 토픽 `mmf/adswmgr/AdswServiceStatus`.
- `status_code == 1` → 정상(PASS), `!= 1` → 이상(WARNING) 으로 diagnostic_policy.yaml 이 판정.
- adsw_manager 자체의 "나 살아있고 준비됨" 신호. 시작 완료 시 최소 1회 `status_code=1` 발행 권장.
- ⚠️ 현재 agl adsw 는 이 헬스를 발행하지 않아 진단 사각이다(별도 개선안 참조). **AUTOSAR
  adsw 는 처음부터 이 헬스를 발행하도록 구현할 것을 권장.**

---

## 4. adsw_manager 내부 상태머신 (권장)

agl 구현(`ADSWManager.cpp`) 기준 최소 상태 흐름:

```
ADSW_INIT ──(DDS 준비 + 앱 목록 확정)──▶ ADSW_STARTED
ADSW_STARTED ──UpdateServiceListRequest 발행──▶ (UpdateServiceListResponse=SUCCESS 수신)──▶ ADSW_RUNNING
ADSW_RUNNING ──(제어 수신/상태변화)──▶ NotifyServiceState 발행 (계속)
    ↓ (Stop 전체/종료 신호)
ADSW_TERMINATE
```

- `ADSW_STARTED` 에서 등록 응답을 받기 전까지 **주기적으로 등록 재발행**(late lifecycle 대비).
- `ADSW_RUNNING` 진입 시 `AdswServiceStatus(status_code=1)` 1회 발행 권장.

---

## 5. Adaptive AUTOSAR 매핑 — 무엇으로 구현하나

| MMF 계약 | Adaptive Platform 구현 수단 |
|---|---|
| DDS 토픽 pub/sub (`mmf/lifecyclemgr/*`) | **① `ara::com` DDS 바인딩** 또는 **② FastDDS DomainParticipant 직접 사용**(agl 방식). MMF IDL 과 **동일 타입**을 써야 하므로, ②가 타입 정합·QoS 제어에 유리 (agl adsw 가 이 방식) |
| `StartServiceRequest` / `StopServiceRequest` 처리 | **Execution Management(`ara::exec`) + State Management**. adsw 가 SM 역할로 **Function Group State** 를 전이시켜 EM 이 AA 프로세스를 기동/종료 |
| `NotifyServiceState` 생성 (RUNNING 등) | 각 AA 의 **`ara::exec::ExecutionClient::ReportExecutionState(kRunning)`** + EM/PHM 프로세스 상태를 adsw 가 취합 → `ServiceState` 로 변환 |
| `SetServiceModeRequest`(OTA_MODE) | **State Management** 의 별도 Machine/Function Group State(업데이트 모드)로 전이 |
| `RecoveryPolicy` 실행 | **PHM(Platform Health Management)** 감시 + EM 재시작, 또는 adsw 자체 감시 |
| `AdswServiceStatus` 헬스 | adsw 프로세스 자신의 liveness/ready 를 판단해 발행 (필요 시 PHM supervision 결과 반영) |
| `container_*` | AP native 면 미사용(`""`/빈 배열) |

### 타입 정합 주의 (가장 흔한 실패점)
- MMF 는 **bare-DDS IDL**(module `lifecyclemgr`, 타입명 `lifecyclemgr::StartServiceRequest`)을 쓴다.
- ROS 캐논 이름(`rt/…`, `…::msg::dds_::…`)과 다르다. adsw 는 **bare 이름**으로 붙어야 한다
  (HMI 쪽은 `dds_bridge` 가 `rt/` 로 변환해 rosbridge 에 노출 — adsw 는 이 변환에 관여하지 않음).
- QoS: 요청/응답은 RELIABLE 권장. 등록·상태 통지가 late-join 에도 도달하려면 발행측 재전송 또는
  적절한 durability 를 고려 (현재 프레임워크는 재전송 패턴 사용).

---

## 6. 예제 코드 (Adaptive AUTOSAR C++)

> DDS 는 **FastDDS 직접 사용**(agl 참조 구현과 동일, 타입 정합·QoS 제어 유리)을 기준으로,
> 앱 기동/상태는 **AUTOSAR AP 표준 `ara::exec`** 로 예시한다. 헤더 경로·벤더 API 는 스택에
> 맞게 조정. 아래는 **골격 예제**로, 에러처리·스레딩은 실제 구현에서 보강한다.

### 6.0 공통 셋업 — MMF IDL codegen 타입 + 참가자

```cpp
// MMF IDL 을 그대로 codegen 한 헤더 (bare-DDS, module lifecyclemgr / adswmgr)
#include "lifecycle/LifecycleCommon.hpp"
#include "lifecycle/UpdateServiceListRequest.hpp"
#include "lifecycle/NotifyServiceState.hpp"
#include "lifecycle/StartServiceRequest.hpp"
#include "lifecycle/StopServiceRequest.hpp"
#include "lifecycle/SetServiceModeRequest.hpp"
#include "ros2/AdswServiceStatus.hpp"

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

// ara::exec — Execution Management / State Management (AUTOSAR AP)
#include "ara/exec/execution_client.h"
#include "ara/exec/state_client.h"
#include "ara/exec/function_group.h"

using namespace eprosima::fastdds::dds;
using namespace lifecyclemgr;

// 이 adsw_manager 의 OS 도메인. 발행 스탬프 + 수신 필터의 기준.
static constexpr MwType kMyOs = MwType::AUTOSAR;
```

### 6.1 등록 — `mw_type = AUTOSAR` 스탬프 후 발행

```cpp
// 내부 앱 카탈로그 (ARXML/설정에서 로드한 Adaptive Application 목록)
struct AppEntry {
    std::string name;
    bool        oneshot;      // 1회성이면 ONESHOT
    bool        autostart;    // 부팅 자동 기동
    RecoveryPolicy recovery;  // 예: ON_FAILURE
    std::string fg_path;      // 소속 Function Group, 예: "FG/AdswApps"
};

void AutosarAdsw::PublishRegistration() {
    UpdateServiceListRequest req;
    for (const auto& aa : apps_) {
        Service s;
        s.name(aa.name);
        s.service_type(aa.oneshot ? ServiceType::ONESHOT : ServiceType::NORMAL);
        s.recovery_policy(aa.recovery);
        s.is_enabled(aa.autostart);
        s.mw_type(kMyOs);            // ← AUTOSAR 스탬프
        s.container_name("");        // native AP → 컨테이너 없음
        req.service_list().push_back(s);
    }
    // AP native 면 container_list 는 비움
    update_req_writer_->write(&req);  // topic: mmf/lifecyclemgr/UpdateServiceListRequest
}

// 등록 응답 리스너 — SUCCESS 수신 시 RUNNING 으로
class RegAckListener : public DataReaderListener {
public:
    explicit RegAckListener(AutosarAdsw* o) : owner_(o) {}
    void on_data_available(DataReader* r) override {
        UpdateServiceListResponse res; SampleInfo info;
        if (r->take_next_sample(&res, &info) != RETCODE_OK || !info.valid_data) return;
        if (res.ret_code() == ReturnCode::SUCCESS) owner_->OnRegistered();
    }
private:
    AutosarAdsw* owner_;
};
```

### 6.2 제어 수신(Start) — `mw_type` 필터 → `ara::exec` 기동 → 응답

```cpp
class StartListener : public DataReaderListener {
public:
    explicit StartListener(AutosarAdsw* o) : owner_(o) {}
    void on_data_available(DataReader* r) override {
        StartServiceRequest req; SampleInfo info;
        if (r->take_next_sample(&req, &info) != RETCODE_OK || !info.valid_data) return;

        if (req.mw_type() != kMyOs) return;   // ← 다른 OS 대상 → 무시 (핵심 라우팅)

        ReturnCode rc = ReturnCode::SUCCESS;
        for (const auto& name : req.service_list()) {
            if (!owner_->StartApp(name)) rc = ReturnCode::FAILED;
        }
        StartServiceResponse res; res.ret_code(rc);
        owner_->start_res_writer_->write(&res);  // mmf/lifecyclemgr/StartServiceResponse
    }
private:
    AutosarAdsw* owner_;
};
// StopListener 도 동일 패턴 (StopServiceRequest → StopApp → StopServiceResponse)
```

### 6.3 앱 기동/종료 → Function Group State 전이 (`ara::exec::StateClient`)

```cpp
// Adaptive AUTOSAR 에서 "앱 기동" = 해당 앱을 포함하는 Function Group 을
// 활성 State 로 전이시키는 것. State Management(여기선 adsw)가 요청하면
// Execution Management 가 실제 프로세스를 fork/exec 한다.
bool AutosarAdsw::StartApp(const std::string& name) {
    const auto& aa = apps_by_name_.at(name);
    ara::exec::FunctionGroup fg = ara::exec::FunctionGroup::Create(aa.fg_path).Value();
    ara::exec::FunctionGroupState on =
        ara::exec::FunctionGroupState::Create(fg, "On").Value();

    auto fut = state_client_.SetState(on);      // 비동기
    auto result = fut.GetResult();              // 완료 대기
    return result.HasValue();                   // 성공 여부 → ret_code 로 환원
}

bool AutosarAdsw::StopApp(const std::string& name) {
    const auto& aa = apps_by_name_.at(name);
    ara::exec::FunctionGroup fg = ara::exec::FunctionGroup::Create(aa.fg_path).Value();
    ara::exec::FunctionGroupState off =
        ara::exec::FunctionGroupState::Create(fg, "Off").Value();
    return state_client_.SetState(off).GetResult().HasValue();
}
```

### 6.4 상태 통지 — EM/PHM 프로세스 상태 → `NotifyServiceState`

```cpp
// EM/PHM 이 보고하는 프로세스 실행상태를 MMF ServiceState 로 변환
ServiceState ToServiceState(AppRunState s) {   // AppRunState 는 벤더 EM 상태
    switch (s) {
        case AppRunState::Starting:      return ServiceState::INITIALIZING;
        case AppRunState::Running:        return ServiceState::RUNNING;
        case AppRunState::Terminating:    return ServiceState::TERMINATING;
        case AppRunState::Unresponsive:   return ServiceState::RUNNING_NO_RESPONSE;
        default:                          return ServiceState::NOT_RUNNING;
    }
}

void AutosarAdsw::PublishServiceState() {
    NotifyServiceState msg;
    msg.mw_type(kMyOs);                         // ← AUTOSAR 스탬프
    for (const auto& aa : apps_) {
        msg.service_list().push_back(aa.name);
        msg.service_state_list().push_back(ToServiceState(QueryRunState(aa)));  // 병렬 배열
    }
    state_writer_->write(&msg);                 // mmf/lifecyclemgr/NotifyServiceState
}
```

```cpp
// 참고: 각 Adaptive Application 프로세스 자신은 초기화 완료 후 EM 에 kRunning 보고.
// 이 신호가 EM 을 거쳐 QueryRunState() 의 Running 으로 이어진다.
//   (각 AA 의 main 내부)
ara::exec::ExecutionClient ec;
// ... 앱 초기화 완료 ...
ec.ReportExecutionState(ara::exec::ExecutionState::kRunning);
```

### 6.5 OTA 모드 전환 (`SetServiceMode`)

```cpp
class SetModeListener : public DataReaderListener {
public:
    explicit SetModeListener(AutosarAdsw* o) : owner_(o) {}
    void on_data_available(DataReader* r) override {
        SetServiceModeRequest req; SampleInfo info;
        if (r->take_next_sample(&req, &info) != RETCODE_OK || !info.valid_data) return;
        if (req.mw_type() != kMyOs) return;     // ← OS 필터

        // OTA_MODE → 업데이트용 State, NORMAL → 평시 State 로 전이
        const char* target = (req.service_mode() == ServiceMode::OTA_MODE) ? "UpdateMode" : "On";
        ReturnCode rc = ReturnCode::SUCCESS;
        for (const auto& name : req.service_list()) {
            if (!owner_->SetAppState(name, target)) rc = ReturnCode::FAILED;
        }
        SetServiceModeResponse res; res.ret_code(rc);
        owner_->setmode_res_writer_->write(&res);
    }
private:
    AutosarAdsw* owner_;
};
```

### 6.6 헬스 — `AdswServiceStatus` (module `adswmgr`)

```cpp
void AutosarAdsw::PublishHealth(int32_t code, const std::string& text) {
    adswmgr::AdswServiceStatus s;   // ← 다른 module!
    s.status_code(code);            // 1 = 정상(PASS), !=1 = 이상(WARNING)
    s.status_msg(text);
    health_writer_->write(&s);      // mmf/adswmgr/AdswServiceStatus → Diagnostic Manager(code 12)
}
```

### 6.7 메인 상태머신 골격

```cpp
int main() {
    AutosarAdsw mgr;
    mgr.InitDds();          // 참가자 + 6.0 의 writer/reader(+listener) 생성
    mgr.DiscoverApps();     // ARXML/설정에서 AppEntry 목록 로드

    // ADSW_STARTED: 등록 응답 받을 때까지 주기 재발행 (late lifecycle_mgr 대비)
    mgr.state_ = AdswState::ADSW_STARTED;
    while (mgr.state_ != AdswState::ADSW_RUNNING) {
        mgr.PublishRegistration();
        std::this_thread::sleep_for(std::chrono::seconds(1));
        // RegAckListener::on_data_available → OnRegistered() 가 state_=ADSW_RUNNING 설정
    }

    mgr.PublishHealth(1, "AUTOSAR ADSW ready");   // 준비완료 → diagnostics 가시화

    // ADSW_RUNNING: 주기 상태 통지 (+ 상태 변화 이벤트 시 즉시 통지 권장)
    while (mgr.running_) {
        mgr.PublishServiceState();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    mgr.state_ = AdswState::ADSW_TERMINATE;
    return 0;
}
```

> **주의**: 위 writer/reader 는 6.0 의 참가자에서 각 토픽·타입으로 생성한다. 요청/응답 writer 는
> RELIABLE QoS 권장. `apps_by_name_`/`apps_` 등 카탈로그는 등록·상태·제어에서 공유한다.

---

## 7. 구현 체크리스트

- [ ] FastDDS(또는 ara::com DDS) 참가자 생성, **MMF `idl/dds/lifecycle/*.idl` 타입 그대로** 사용
- [ ] 발행 토픽 3: `UpdateServiceListRequest`, `NotifyServiceState`, (`AdswServiceStatus`)
- [ ] 구독 토픽 3(+2): `StartServiceRequest`, `StopServiceRequest`, `SetServiceModeRequest`
      (+ `*Response` 발행, + `GetServiceStateRequest`)
- [ ] 발행 메시지 전부 `mw_type = AUTOSAR` 스탬프
- [ ] 수신 제어 메시지 `req.mw_type() == AUTOSAR` **필터** (아니면 무시)
- [ ] 상태머신: INIT → STARTED(등록 재시도) → RUNNING → TERMINATE
- [ ] Start/Stop → `ara::exec` Function Group State 전이로 AA 기동/종료
- [ ] AA 상태 → `ServiceState` 변환해 `NotifyServiceState` 주기+이벤트 발행
- [ ] SetServiceMode(OTA_MODE) → State Management 업데이트 모드 전이
- [ ] RecoveryPolicy → PHM/EM 재시작 연동, `recovered_count`/`exit_code` 반영
- [ ] `AdswServiceStatus(status_code=1)` 준비완료 시 발행 (diagnostics 가시성)
- [ ] (컨테이너 미사용 시) `container_list=[]`, `NotifyContainerState` 미발행

---

## 8. 검증 (연동 테스트)

1. **등록 확인**: adsw 기동 → `lfc_monitor`(HMI) Services 패널에 AUTOSAR 서비스가 뜨는지.
   (경로: adsw → NotifyServiceState/UpdateServiceList → lifecycle_mgr → LifecycleDiagReport →
   dds_bridge → rosbridge → lfc_monitor)
2. **제어 확인**: HMI 또는 lifecycle 에서 Start/Stop → 실제 AP 앱이 기동/종료되고 상태가 반영되는지.
3. **mw_type 격리 확인**: ROS2/AGL 대상 제어가 AUTOSAR adsw 에서 무시되는지 (agl 유닛테스트
   `MessageTransmitterTest` 의 `mw_type(ROS2)→무시` 케이스를 AUTOSAR 로 미러).
4. **헬스 확인**: `AdswServiceStatus(status_code=1)` → Diagnostic Manager 가 PASS 판정하는지.
5. **복구 확인**: 대상 앱 강제 종료 → RecoveryPolicy(ALWAYS/ON_FAILURE) 대로 재시작 +
   `recovered_count` 증가가 DiagReport 에 노출되는지.

---

## 9. 부록 — 계약 IDL 원본 (참조)

> 실제 정의는 `oss_multisw_framework/idl/dds/lifecycle/*.idl` 및 `idl/dds/ros2/AdswServiceStatus.idl`.
> **AP 팀은 이 IDL 을 그대로 가져다 codegen 해야 타입이 정합**한다.

```idl
// LifecycleCommon.idl (핵심 enum 발췌)
module lifecyclemgr {
  enum ServiceType   { NORMAL, ONESHOT };
  enum RecoveryPolicy{ R_DEFAULT, ALWAYS, ON_FAILURE, ON_SUCCESS, ON_NOT_RESPONDING, MANUAL };
  enum ReturnCode    { SUCCESS, FAILED };
  enum ServiceMode   { NORMAL, OTA_MODE };
  enum ServiceState  { NOT_RUNNING, INITIALIZING, RUNNING, RUNNING_NO_RESPONSE, TERMINATING };
  enum MwType        { ROS2, AGL, AUTOSAR };   // ← AUTOSAR 이미 존재
};

// UpdateServiceListRequest.idl
struct Service {
  string name; ServiceType service_type; RecoveryPolicy recovery_policy;
  boolean is_enabled; MwType mw_type; string container_name;
};
struct UpdateServiceListRequest  { sequence<Service> service_list; sequence<Container> container_list; };
struct UpdateServiceListResponse { ReturnCode ret_code; };

// NotifyServiceState.idl
struct NotifyServiceState {
  MwType mw_type; sequence<string> service_list; sequence<ServiceState> service_state_list;
};

// StartServiceRequest.idl (Stop 동일 형태)
struct StartServiceRequest  { MwType mw_type; sequence<string> service_list; boolean is_group; };
struct StartServiceResponse { ReturnCode ret_code; };

// SetServiceModeRequest.idl
struct SetServiceModeRequest  { MwType mw_type; sequence<string> service_list; boolean is_group; ServiceMode service_mode; };
struct SetServiceModeResponse { ReturnCode ret_code; };

// AdswServiceStatus.idl  (module adswmgr — 헬스)
module adswmgr { struct AdswServiceStatus { long status_code; string status_msg; }; };
```

---

## 10. 참고 소스 (구현 시 대조용)

- 계약 IDL: `idl/dds/lifecycle/*.idl`, `idl/dds/ros2/AdswServiceStatus.idl`
- **가장 가까운 참조 구현(native)**: `service/extension/agl/adsw_manager/`
  - 상태머신: `ADSWManager.cpp`
  - DDS pub/sub + **mw_type 스탬프/필터**: `message_transmitter/MessageTransmitter.cpp`
  - 등록: `message_transmitter/UpdateAppListHandler.cpp`
  - 제어 수신: `message_transmitter/{StartAppHandler,StopAppHandler}.cpp`
  - 상태 발행: `message_transmitter/NotifyAppStateWriter.cpp`
  - 감시/복구: `service_monitor/ServiceMonitor.cpp`, `service_controller/ServiceController.cpp`
- lifecycle 측 라우팅(mw_type 스탬프): `service/core/lifecycle_mgr/state/StateManager.cpp`
- 집계·발행: `service/core/lifecycle_mgr/state/StateMonitor.cpp` → `LifecycleDiagReport`
