# ros2/ota_client 통합 테스트 결과

## 개요

- **테스트 일자**: 2026-03-29
- **대상**: `ros2/ota_client` ↔ `ota_manager` DDS 통신
- **바이너리**: `output/x86_64/Debug/extension/ros2/bin/ota_client`
- **OtaManager**: `output/x86_64/Debug/core/bin/OtaManager`

---

## 통신 구조

```
ros2/ota_client                          ota_manager (OtamClientService)
─────────────────────────────────────────────────────────────────────────
[Start()] PublishClientRegistration ─────────────────────────────────→
           (500ms 후 1회 재발행)

                         ← NotifyOtaManagerReady (TRANSIENT_LOCAL)
OnManagerReady() → PublishClientRegistration ────────────────────────→
                         ← SetOtaClientRegistrationResponse (client_id)
                         ← NotifyAppUpdateStatus (app_name, status)
                              → OtacStatusStore::UpdateStatus()
```

### DDS 토픽

| 방향 | 토픽 | QoS |
|---|---|---|
| ota_client → OtaManager | `/mmf/otamanager/otaclient_service/SetOtaClientRegistration` | RELIABLE + VOLATILE |
| OtaManager → ota_client | `/mmf/otamanager/otaclient_service/SetOtaClientRegistrationResponse` | RELIABLE + **TRANSIENT_LOCAL** |
| OtaManager → ota_client | `/mmf/otamanager/otaclient_service/NotifyOtaManagerReady` | RELIABLE + **TRANSIENT_LOCAL** |
| OtaManager → ota_client | `/mmf/otamanager/otaclient_service/NotifyAppUpdateStatus` | RELIABLE + VOLATILE |

---

## 실행 방법

```bash
export LD_LIBRARY_PATH=/home/obigo/obigo-sdk/Fast-DDS/output/ros2_x86_64:$LD_LIBRARY_PATH
BASE=/home/obigo/project/oss/oss_multisw_framework

# OtaManager 실행
stdbuf -oL -eL $BASE/output/x86_64/Debug/core/bin/OtaManager &

# ota_client 실행 (기본 config)
cd $BASE/service/extension/ros2/ota_client
stdbuf -oL -eL $BASE/output/x86_64/Debug/extension/ros2/bin/ota_client

# 또는 커스텀 config
stdbuf -oL -eL $BASE/output/x86_64/Debug/extension/ros2/bin/ota_client --config /path/to/config.json
```

### TC-3 트리거 (notify_status_publisher)

```bash
cd $BASE
(sleep 3; echo "adsw_monitor downloading"; sleep 1; echo "adsw_monitor installing"; sleep 1; echo "adsw_monitor success") | \
  stdbuf -oL -eL ./service/core/ota_mgr/test/notify_status_publisher/build/notify_status_publisher
```

---

## 테스트 케이스

### TC-1: OtaManager 먼저 기동 ✅

**시나리오**: OtaManager 완전 기동 후 ota_client 기동

**예상 흐름**:
1. OtaManager 기동 → `NotifyOtaManagerReady` 발행 (TRANSIENT_LOCAL 캐시)
2. ota_client 기동 → `Start()`에서 `SetOtaClientRegistration` 발행
3. ota_client → `OnManagerReady` 수신 (TRANSIENT_LOCAL 캐시에서) → 재등록 발행
4. OtaManager → `SetOtaClientRegistrationResponse (Client ID: 0, SUCCESS)` 응답

**실제 결과**:
```
[ota_client] config path: config/ota_client_config.json
participant create complete
OtacManagerService::Start complete!!
publish success
[SVC] OnManagerReady called
Ota Manager is ready!
Call PublishClientRegistration()
publish success
Successfully Call Publish.
Client id: 0/ result: 1   ← result: 1 = SUCCESS (ReturnValue::SUCCESS = 1)
```

```
[OtaManager] OtamClientService: Processing registration from client 'ros2_ota_client' with 1 applications.
[OtaManager] OtamClientService: Published registration response - Client ID: 0, Result: SUCCESS
```

**결과**: ✅ PASS

---

### TC-2: ota_client 먼저 기동 ✅

**시나리오**: ota_client 먼저 기동 후 OtaManager 나중에 기동

**예상 흐름**:
1. ota_client 기동 → `Start()`에서 registration 발행 (OtaManager 없어서 미수신)
2. OtaManager 기동 → `NotifyOtaManagerReady` 발행
3. ota_client → `OnManagerReady` 수신 → `PublishClientRegistration` 재발행
4. OtaManager → response 발행 → ota_client 수신

**실제 결과**:
```
[SVC] OnManagerReady called
Ota Manager is ready!
Call PublishClientRegistration()
publish success
Successfully Call Publish.
Client id: 0/ result: 1
```

```
[OtaManager] Processing registration from client 'ros2_ota_client' with 1 applications.
[OtaManager] Published registration response - Client ID: 0, Result: SUCCESS
```

**결과**: ✅ PASS

---

### TC-3: NotifyAppUpdateStatus 수신 ✅

**시나리오**: registration 완료 후 `notify_status_publisher`로 앱 상태 변경 통보

**입력**:
```
adsw_monitor downloading
adsw_monitor installing
adsw_monitor success
```

**실제 결과**:
```
App Update Status Update complete.   ← adsw_monitor downloading
App Update Status Update complete.   ← adsw_monitor installing
App Update Status Update complete.   ← adsw_monitor success
```

**결과**: ✅ PASS (3개 상태 메시지 모두 수신)

---

### TC-4: 중복 등록 ✅

**시나리오**: 이미 등록된 상태에서 동일 `client_name`으로 재등록

**예상 동작**: `FindClient()`에서 기존 클라이언트 탐지 → 동일 `client_id` 반환

**실제 결과**:
```
[OtaManager] OtamClientService: Processing registration from client 'ros2_ota_client' with 1 applications.
[OtaManager] OtamClientService: Found client name=ros2_ota_client
[OtaManager] OtamClientService: Published registration response - Client ID: 0, Result: SUCCESS
```

**결과**: ✅ PASS

---

## 수정 사항 (사전 작업)

| 항목 | 내용 |
|---|---|
| 토픽 이름 버그 수정 | `OtamClientService::TOPIC_REGISTRATION_RESPONSE`: `vehicleinfo_service` → `otaclient_service` |
| namespace 수정 | `OtacManagerService`: `OtaManager::` → `ota_manager::msg::` |
| TRANSIENT_LOCAL QoS | `NotifyOtaManagerReady`, `SetOtaClientRegistrationResponse` Writer/Reader |
| JSON config 지원 | `OtacStatusStore`: 하드코딩 → `config/ota_client_config.json` 로드 |
| segfault 수정 | `std::cerr` → `fprintf(stderr, ...)` (FastDDS 심볼 불일치 환경) |

---

## 설정 파일

### config/ota_client_config.json

```json
{
  "client_name": "ros2_ota_client",
  "apps": [
    { "name": "adsw_monitor", "status": "installed" }
  ]
}
```

### 설정 경로 우선순위

```
--config <path>  >  OTA_CLIENT_CONFIG (환경변수)  >  config/ota_client_config.json (기본)
```

### Fallback 동작

- 파일 없거나 파싱 실패 시 Warning 출력 후 기본값 (`adsw_monitor`, client_name: `ros2_ota_client`) 사용

---

## 비고

### ReturnValue enum 정의

```cpp
enum class ReturnValue : int32_t {
    UNKNOWN = 0,
    SUCCESS = 1,   // result: 1 = SUCCESS (정상)
    FAIL    = 2
};
```

`result: 1`은 `ReturnValue::SUCCESS`로 **정상** 동작.

### FastDDS 심볼 경고

```
Symbol `PARTICIPANT_QOS_DEFAULT' has different size in shared object, consider re-linking
```

빌드 시 사용한 FastDDS(`/opt/Fast-DDS/...`)와 런타임 시스템 FastDDS(`/usr/local/lib/`) 버전 불일치로 발생하는 경고.  
SDK 라이브러리를 우선 로드(`LD_LIBRARY_PATH`)하면 해소됨. 동작에는 영향 없음.
