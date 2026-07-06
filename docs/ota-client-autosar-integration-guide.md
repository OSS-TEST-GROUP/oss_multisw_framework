# Adaptive AUTOSAR OTA Client 구현 가이드 (MMF `OTA Manager` 연동)

> 작성일: 2026-07-05
> 대상 독자: Adaptive AUTOSAR 플랫폼에서 OTA Client 를 신규 구현하는 팀
> 범위: MMF `OTA Manager`(core service `ota_mgr`) 와의 **연동 계약(contract)** 정의 및
>       Adaptive Platform(AP, 특히 `ara::ucm`) 매핑 + C++ 예제
> 기준 소스: `idl/dds/ota_mgr/OtamClientServiceMsg.idl`,
>            `service/extension/{agl,ros2}/ota_client/*`, `service/core/ota_mgr/*`

---

## 0. 요약 (TL;DR)

- OTA Client 는 **한 노드(ECU/보드)에 설치된 앱(app)들을 OTA Manager 에 등록하고, 그 앱들의
  업데이트 상태를 Manager 와 동기화하는 노드측 에이전트** 다.
- OTA Manager 는 **업데이트 평면의 중앙 오케스트레이터** 로, 실제 업데이트 소스는 **ArgoCD**
  (`OtamServerService`, libcurl REST) 다. Client 는 ArgoCD 를 직접 보지 않는다 — **Manager 만 본다.**
- Client ↔ Manager 계약은 **`/mmf/otamanager/otaclient_service/` 네임스페이스의 4개 토픽**이 전부다.
  Client 는 그중 **1개만 발행**(등록), **3개는 구독**(응답·Manager ready·업데이트 상태).
- **현재 계약은 "인벤토리 등록 + 상태 미러"** 에 집중되어 있다. AP 에서 실제 패키지 적용은
  **`ara::ucm`(Update & Configuration Management)** 로 구현하고, 그 상태를 이 DDS 계약에 브릿지한다.
- IDL 변경 없이 기존 4토픽에 붙으면 된다.

---

## 1. 컴포넌트 관계 (근본)

```
        ArgoCD (cd.obigo.com)  ◀── REST/libcurl ──  OTA Manager (core, 단일)  ── DDS ──▶  OTA Client (노드별)
        (업데이트 소스·배포 상태)                     otam_actor/client/diag       otaclient_service     (설치 앱 소유·상태 미러)
                                                    /vehicle_service                              │
                                                          │                                       │
                                                    ota_monitor(HMI) ◀ actor_service(GetAppList 등)┘
```

- **OTA Manager 는 하나.** 모든 OTA Client 가 여기에 등록한다.
- **OTA Client 는 노드마다 하나.** 이 가이드의 구현 대상.
- **업데이트 결정·소스는 Manager+ArgoCD 몫**, **노드 로컬 적용·상태보고는 Client 몫** (책임 분리).
- HMI(`ota_monitor`)는 **actor_service** 로 Manager 에게 조회(GetAppList 등)한다 — **Client 와 직접
  통신하지 않는다.** Client 의 계약면은 오직 **client_service** 다.

---

## 2. 통합 계약 (SSOT) — otaclient_service 4 토픽

모든 토픽 module `otamanager`, bare-DDS 이름 `/mmf/otamanager/otaclient_service/…`.

| # | 토픽 | 방향 | Client 역할 | IDL 타입 |
|---|---|---|:-:|---|
| 1 | `.../SetOtaClientRegistration` | **client → manager** | **발행(write)** | `SetOtaClientRegistration` |
| 2 | `.../SetOtaClientRegistrationResponse` | manager → client | 구독(read) | `SetOtaClientRegistrationResponse` |
| 3 | `.../NotifyOtaManagerReady` | manager → client | 구독(read) | `NotifyOtaManagerReady` |
| 4 | `.../NotifyAppUpdateStatus` | manager → client | 구독(read) | `NotifyAppUpdateStatus` |

- Client 는 **writer 1개(등록) + reader 3개**. (참조 구현 `OtacManagerService.cpp:72~84`)
- **공유 토픽 브로드캐스트 주의**: 응답(#2)·상태(#4)는 모든 Client 가 구독하는 공유 토픽이다.
  → **응답은 echo 된 `client_name`(#2) / `client_id`(#4) 가 자기 것일 때만 채택**해야 한다
  (타 클라이언트·잔존 응답 오채택 방지 — `OtacManagerService.cpp:138` 의 실제 방어 로직).

> 참고: 클라이언트 삭제는 `actor_service` 의 `DeleteOtaClientRegistration`(monitor→manager)로
> Manager 레지스트리에서 이뤄진다. **Client 자신은 이 토픽을 다루지 않는다.**

---

## 3. 동작 흐름

### 3.1 부팅 → 등록 → client_id 수신

```
Client 부팅: config 로드(client_name, apps[])  →  3개 토픽 구독 시작
        │
Manager  ──NotifyOtaManagerReady()──▶  Client
        │                                  │ (Manager 준비 신호 = 등록 트리거)
Client   ──SetOtaClientRegistration(client_name, app_list[])──▶  Manager
        │                                  │
Manager  ──SetOtaClientRegistrationResponse(client_id, ret_code, client_name)──▶  Client
        │                                  │ client_name 일치 확인 → client_id 저장
```

- **트리거는 `NotifyOtaManagerReady`**: Manager 가 (재)기동해 준비되면 이 신호를 낸다.
  Client 는 이걸 받을 때마다 **재등록**해야 한다 (Manager 재시작 대응).
- 부팅 직후에도 능동적으로 1회 등록 시도 + 응답 없으면 **재시도** 권장 (참조: `OtacMain.cpp:31~33`).
- 응답의 `client_id` 는 이후 상태 매칭 키다. `ret_code == SUCCESS` 확인.

### 3.2 업데이트 상태 동기화 (NotifyAppUpdateStatus)

```
Manager (ArgoCD sync 상태 기반)  ──NotifyAppUpdateStatus(client_id, app_name, status)──▶  Client
                                                                    │ client_id 일치 시 로컬 status store 갱신
```

- **방향은 manager → client.** Manager 가 ArgoCD 로 구동한 앱 업데이트의 상태를 Client 에 통지한다.
- Client 는 `client_id` 가 자기 것인 통지만 채택해 로컬 상태 저장소를 갱신한다
  (참조: `OtacManagerService.cpp:157` `OnAppUpdateStatus` → `status_store_->UpdateStatus`).
- **여기가 AP `ara::ucm` 연결점**: 실제 패키지 적용이 필요하면 이 통지를 받아 UCM 처리(§5)를
  기동하고, 결과를 로컬 상태에 반영한다. (현재 MMF 계약 자체는 상태 미러가 기본)

---

## 4. 설정 (config)

`ota_client_config.json` (참조 구현):

```json
{
  "client_name": "autosar_ota_client",
  "apps": [
    { "name": "adsw_monitor", "status": "installed" }
  ]
}
```

- `client_name`: 이 노드의 고유 식별자. 등록·응답 필터의 기준. **노드마다 유일**해야 한다.
- `apps[]`: 이 노드가 소유한 앱 목록과 초기 상태(`installed` 등). 등록 시 `app_list` 로 전송.
- 파일 부재/파싱 실패 시 기본값으로 폴백하되(참조 `OtacStatusStore`), 운영에선 반드시 제공.

---

## 5. Adaptive AUTOSAR 매핑 — 무엇으로 구현하나

| MMF 계약 | Adaptive Platform 구현 수단 |
|---|---|
| DDS pub/sub (`otaclient_service/*`) | **FastDDS DomainParticipant 직접**(참조 구현과 동일, 타입 정합·QoS 유리) 또는 `ara::com` DDS 바인딩. **MMF IDL 타입 그대로 codegen** 필수 |
| 앱 인벤토리(`app_list`, 버전/상태) | **`ara::ucm::GetSwClusterInfo()`** 로 설치된 SW Cluster/버전을 조회해 `app_list` 구성 |
| 업데이트 적용(옵션, `NotifyAppUpdateStatus` 수신 시) | **`ara::ucm`**: `TransferStart → TransferData → TransferExit → ProcessSwPackage → Activate → Finish` 시퀀스 |
| 업데이트 상태 → 로컬 반영 | UCM 진행상태(`ara::ucm` state)를 로컬 store 에 기록, 필요 시 Manager 에 재보고 |
| Client 프로세스 수명 | **`ara::exec::ExecutionClient::ReportExecutionState(kRunning)`** 로 EM 에 보고 |
| 노드/앱 안전 상태 전이 | 업데이트 중이면 **State Management** 로 관련 Function Group 을 안전 State 로 (adsw 가이드의 OTA_MODE 와 연동) |

### 타입 정합 주의 (가장 흔한 실패점)
- **bare-DDS IDL**(module `otamanager`, 타입 `otamanager::SetOtaClientRegistration`)을 그대로 써야 한다.
  ROS 캐논 이름(`rt/…`, `…::msg::dds_::…`)이 아니다. (HMI 노출용 `rt/` 변환은 `dds_bridge` 몫)
- 요청/등록·응답 토픽은 RELIABLE QoS 권장. 등록이 late-join Manager 에도 닿도록, Client 는
  `NotifyOtaManagerReady` 수신 + 부팅 재시도 **둘 다** 구현한다.

---

## 6. 예제 코드 (Adaptive AUTOSAR C++)

> DDS 는 FastDDS 직접 사용(참조 구현과 동형) 기준. `ara::ucm`/`ara::exec` 는 AP 표준 API 예시.
> 골격 예제이므로 에러처리·스레딩·QoS 세부는 실제 구현에서 보강한다.

### 6.0 공통 셋업 — MMF IDL codegen 타입 + 참가자

```cpp
// MMF IDL codegen 헤더 (module otamanager)
#include "ota_mgr/OtamClientServiceMsg.hpp"   // SetOtaClientRegistration, ...Response,
                                              // NotifyOtaManagerReady, NotifyAppUpdateStatus
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

// AP: UCM(업데이트 적용) / EM(수명 보고)
#include "ara/ucm/ucm_client.h"
#include "ara/exec/execution_client.h"

using namespace eprosima::fastdds::dds;
using namespace otamanager;

static constexpr char kNS[] = "/mmf/otamanager/otaclient_service/";
```

### 6.1 부팅 — config 로드 + 구독 시작 + EM 보고

```cpp
struct OtaClientConfig {
    std::string client_name;                                  // 노드 유일 식별자
    std::vector<std::pair<std::string,std::string>> apps;     // (app_name, status)
};

void AutosarOtaClient::Init() {
    cfg_ = LoadConfig("ota_client_config.json");     // client_name + apps[]
    client_id_ = -1;                                 // 등록 전

    // reader 3개(응답·ready·상태) + writer 1개(등록) 생성 (아래 리스너 연결)
    reg_writer_       = MakeWriter<SetOtaClientRegistration>(kNS "SetOtaClientRegistration");
    resp_reader_      = MakeReader<SetOtaClientRegistrationResponse>(
                            kNS "SetOtaClientRegistrationResponse", &resp_listener_);
    ready_reader_     = MakeReader<NotifyOtaManagerReady>(kNS "NotifyOtaManagerReady", &ready_listener_);
    status_reader_    = MakeReader<NotifyAppUpdateStatus>(kNS "NotifyAppUpdateStatus", &status_listener_);

    ara::exec::ExecutionClient ec;
    ec.ReportExecutionState(ara::exec::ExecutionState::kRunning);   // EM 에 준비완료 보고
}
```

### 6.2 등록 발행 (client → manager)

```cpp
bool AutosarOtaClient::PublishRegistration() {
    SetOtaClientRegistration reg;
    reg.client_name(cfg_.client_name);
    // 실제 설치 인벤토리는 UCM 에서 취합 권장 (아래는 config 기반 최소 예)
    for (const auto& [name, _status] : cfg_.apps)
        reg.app_list().push_back(name);
    return reg_writer_->write(&reg) == RETCODE_OK;   // /mmf/otamanager/otaclient_service/SetOtaClientRegistration
}

// 선택: ara::ucm 으로 실제 설치 SW Cluster 를 조회해 app_list 구성
std::vector<std::string> AutosarOtaClient::QueryInstalledApps() {
    ara::ucm::UCMClient ucm;
    std::vector<std::string> names;
    auto info = ucm.GetSwClusterInfo();               // 설치된 SW Cluster 목록
    if (info.HasValue())
        for (const auto& c : info.Value()) names.push_back(c.Name);
    return names;
}
```

### 6.3 Manager ready 수신 → (재)등록

```cpp
class ReadyListener : public DataReaderListener {
public:
    explicit ReadyListener(AutosarOtaClient* o) : owner_(o) {}
    void on_data_available(DataReader* r) override {
        NotifyOtaManagerReady msg; SampleInfo info;
        if (r->take_next_sample(&msg, &info) != RETCODE_OK || !info.valid_data) return;
        // Manager (재)기동 준비됨 → 재등록 (Manager 재시작 대응)
        owner_->PublishRegistration();
    }
private:
    AutosarOtaClient* owner_;
};
```

### 6.4 등록 응답 수신 — client_name 필터 후 client_id 저장

```cpp
class RegRespListener : public DataReaderListener {
public:
    explicit RegRespListener(AutosarOtaClient* o) : owner_(o) {}
    void on_data_available(DataReader* r) override {
        SetOtaClientRegistrationResponse res; SampleInfo info;
        if (r->take_next_sample(&res, &info) != RETCODE_OK || !info.valid_data) return;

        // 공유 토픽 브로드캐스트 → 내 응답만 채택 (핵심 방어)
        if (res.client_name() != owner_->cfg_.client_name) return;
        if (res.ret_code() != ReturnValue::SUCCESS) return;

        owner_->client_id_ = res.client_id();          // 이후 상태 매칭 키
    }
private:
    AutosarOtaClient* owner_;
};
```

### 6.5 업데이트 상태 수신 → (옵션) UCM 적용 + 로컬 반영

```cpp
class StatusListener : public DataReaderListener {
public:
    explicit StatusListener(AutosarOtaClient* o) : owner_(o) {}
    void on_data_available(DataReader* r) override {
        NotifyAppUpdateStatus msg; SampleInfo info;
        if (r->take_next_sample(&msg, &info) != RETCODE_OK || !info.valid_data) return;

        if (msg.client_id() != owner_->client_id_) return;    // 내 것만
        owner_->status_store_[msg.app_name()] = msg.status();  // 로컬 미러 갱신

        // 실제 패키지 적용이 필요한 상태라면 UCM 시퀀스 기동
        if (msg.status() == "update_available")
            owner_->ApplyUpdate(msg.app_name());
    }
private:
    AutosarOtaClient* owner_;
};
```

### 6.6 (옵션) 실제 패키지 적용 — `ara::ucm` 시퀀스

```cpp
bool AutosarOtaClient::ApplyUpdate(const std::string& app_name) {
    ara::ucm::UCMClient ucm;
    // 1) 전송 시작 → 데이터 전송 → 종료
    auto transfer = ucm.TransferStart(/*size*/ pkgSizeOf(app_name));
    if (!transfer.HasValue()) return false;
    auto id = transfer.Value().TransferId;
    for (auto& block : ReadPackageBlocks(app_name))
        if (!ucm.TransferData(id, block, /*offset*/ block.offset).HasValue()) return false;
    if (!ucm.TransferExit(id).HasValue()) return false;

    // 2) 처리 → 활성화 → 마무리
    if (!ucm.ProcessSwPackage(id).HasValue()) return false;
    if (!ucm.Activate().HasValue())            return false;
    if (!ucm.Finish().HasValue())              return false;

    status_store_[app_name] = "installed";
    return true;
}
```

### 6.7 메인 골격

```cpp
int main() {
    AutosarOtaClient c;
    c.Init();                       // config + 구독 + EM 보고

    // 부팅 재시도: Manager 가 늦게 떠도 결국 등록되도록
    while (c.client_id_ < 0) {
        c.PublishRegistration();
        std::this_thread::sleep_for(std::chrono::seconds(2));
        // RegRespListener 가 client_id_ 를 채우거나, ReadyListener 가 재등록 유발
    }

    // 이후 이벤트 구동: NotifyOtaManagerReady(재등록) / NotifyAppUpdateStatus(상태·UCM)
    while (c.running_) std::this_thread::sleep_for(std::chrono::seconds(1));
    return 0;
}
```

---

## 7. 구현 체크리스트

- [ ] FastDDS(또는 ara::com DDS) 참가자 + **MMF `OtamClientServiceMsg.idl` 타입 그대로** 사용
- [ ] writer 1: `SetOtaClientRegistration`
- [ ] reader 3: `SetOtaClientRegistrationResponse`, `NotifyOtaManagerReady`, `NotifyAppUpdateStatus`
- [ ] `client_name` 을 노드 유일값으로 config 화
- [ ] 응답(#2) `client_name` echo 필터, 상태(#4) `client_id` 필터 **필수**
- [ ] `NotifyOtaManagerReady` 수신 시 **재등록** (Manager 재시작 대응)
- [ ] 부팅 등록 **재시도** 루프 (late Manager 대응)
- [ ] (옵션) `ara::ucm` 으로 실제 앱 인벤토리 조회(`GetSwClusterInfo`) → `app_list`
- [ ] (옵션) `NotifyAppUpdateStatus` 트리거 → `ara::ucm` 적용 시퀀스 → 로컬 상태 반영
- [ ] `ara::exec::ExecutionClient` 로 EM 에 kRunning 보고
- [ ] 업데이트 중 안전 전이 필요 시 State Management(adsw OTA_MODE)와 연계

---

## 8. 검증 (연동 테스트)

1. **등록 확인**: Client 기동 → Manager 로그에 `Assigned client_id=…` + `ota_monitor`(HMI) 의
   GetAppList 트리에 이 노드의 앱이 뜨는지 (경로: Client 등록 → Manager 레지스트리 → actor_service
   GetAppListResponse → dds_bridge → rosbridge → ota_monitor).
2. **재등록 확인**: Manager 재시작 → `NotifyOtaManagerReady` → Client 자동 재등록되는지.
3. **응답 격리 확인**: 두 Client(다른 client_name) 동시 기동 시, 각자 자기 응답만 채택하는지.
4. **상태 미러 확인**: Manager 가 `NotifyAppUpdateStatus(client_id, app, status)` 발행 →
   해당 Client 만 로컬 상태 갱신하는지 (다른 client_id 는 무시).
5. **(옵션) UCM 적용 확인**: `update_available` 상태 수신 → `ara::ucm` 시퀀스 완료 → `installed` 반영.

---

## 9. 부록 — 계약 IDL 원본 (참조)

> 실제 정의: `oss_multisw_framework/idl/dds/ota_mgr/OtamClientServiceMsg.idl`.
> **AP 팀은 이 IDL 을 그대로 codegen 해야 타입이 정합**한다.

```idl
// OtamClientServiceMsg.idl  (module otamanager, 네임스페이스 /mmf/otamanager/otaclient_service/)
#include "common/ReturnValue.idl"

struct SetOtaClientRegistration {          // client → manager (발행)
    string client_name;
    sequence<string> app_list;
};

struct SetOtaClientRegistrationResponse {  // manager → client (구독)
    long client_id;
    ReturnValue ret_code;
    string client_name;                    // echo — 자기 것만 채택
};

struct NotifyOtaManagerReady { };          // manager → client (구독) — (재)등록 트리거

struct NotifyAppUpdateStatus {             // manager → client (구독)
    long client_id;                        // 자기 것만 채택
    string app_name;
    string status;
};
```

---

## 10. 참고 소스 (구현 시 대조용)

- 계약 IDL: `idl/dds/ota_mgr/OtamClientServiceMsg.idl`
- **참조 구현**: `service/extension/agl/ota_client/` (native, AP 와 가장 유사)
  - 메인/재시도: `otac_main/OtacMain.cpp`
  - 등록·구독·필터: `otac_manager_service/OtacManagerService.cpp`
    (등록 발행 `PublishClientRegistration`, 응답 필터 `OnClientRegistrationResponse`,
     ready 재등록 `OnManagerReady`, 상태 미러 `OnAppUpdateStatus`)
  - 로컬 상태 저장: `otac_store/OtacStatusStore.cpp`
  - DDS 래퍼: `common/OtacManagerDdsImpl.cpp`
- Manager 측(대조용): `service/core/ota_mgr/otam_client_service/otam_client_service.cc`
  (client_id 할당, 등록 응답 발행), ArgoCD 연동은 `otam_*server*` 참조.
