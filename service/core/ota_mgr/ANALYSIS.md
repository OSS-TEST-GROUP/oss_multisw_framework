# ota_mgr 소스 분석

분석일: 2026-03-28

---

## 개요

**OtaManager**: 차량 내 애플리케이션의 OTA(Over-The-Air) 업데이트를 관리하는 서비스.
- DDS(FastDDS) 기반 IPC 통신
- ArgoCD 백엔드 서버와 HTTP 통신
- VISS(Vehicle Information Service Specification)로 차량 상태 조회
- LifecycleMgr와 연동하여 OTA 모드 진입

---

## 전체 아키텍처

```
                    ┌─────────────────────────────────┐
                    │           OtaManager              │
                    │                                   │
  DDS Client ──────►│  OtamActorService (조율 중심)     │
  (외부 요청)        │    │   │   │   │                 │
                    │    ▼   ▼   ▼   ▼                 │
                    │  Server Client Vehicle Diagnostic │
                    │  Service Service Service Service  │
                    └──│──────│──────│──────│──────────┘
                       │      │      │      │
                       ▼      ▼      ▼      ▼
                    ArgoCD  DDS    VISS    DDS
                    (HTTP)  Client (HTTP)  Publisher
```

---

## 서비스 구조 (6개)

### 1. OtamMain (진입점 오케스트레이터)
- 모든 서비스를 생성하고 의존성 주입
- `Initialize()` → `Start()` → `MonitorLoop()` 실행
- MonitorLoop: 5초 폴링, 30초마다 heartbeat → DiagnosticService로 상태 전달
- 서비스 실행 순서: ServerService → ClientService → VehicleService → ActorService

### 2. OtamActorService (핵심 비즈니스 로직)
- 4개 DDS 요청 토픽 구독 (FastDDS DataReaderListener)
- OTA 업데이트 흐름 조율:
  1. `SetAppUpdate` 수신
  2. → LifecycleMgr에 `SetServiceModeRequest(OTA_MODE)` 전송
  3. → `SetServiceModeResponse(SUCCESS)` 수신
  4. → VehicleService에 `Vehicle.LowVoltageSystemState`, `Vehicle.Speed` 요청
  5. → 차량 조건 검증 (acc 상태 or ignition+속도0)
  6. → ServerService에 `RunOtaUpdate()` 호출
  7. → 백그라운드 스레드에서 상태 폴링 (500ms) → ClientService로 알림

**DDS 토픽 (ActorService)**:
| 방향 | 토픽 |
|---|---|
| SUB | `/mmf/otamanager/otaactor_service/GetAppUpdateSettings` |
| SUB | `/mmf/otamanager/otaactor_service/SetAppUpdateSettings` |
| SUB | `/mmf/otamanager/otaactor_service/GetAppUpdateState` |
| SUB | `/mmf/otamanager/otaactor_service/SetAppUpdate` |
| SUB | `mmf/lifecyclemgr/SetServiceModeResponse` |
| PUB | 위 각각의 Response 토픽 |
| PUB | `mmf/lifecyclemgr/SetServiceModeRequest` |

### 3. OtamServerService (ArgoCD 백엔드 연동)
- `config/otam_config.json`에서 서버 URL/인증 정보 로드
  - 기본값: `http://localhost:10000`, user: `admin`
- ArgoCD REST API 클라이언트 (`OtamServerApiClientImpl`, HTTP)
- 주요 메서드:
  - `CreateOtaSession()` / `DestroyOtaSession()`: 인증 토큰 관리
  - `GetOtaSettings(app_name)` → ArgoCD Application 조회
  - `GetOtaUpdateState(app_name)` → 배포 상태 조회 (JSON 반환)
  - `RunOtaUpdate(app_name)` → ArgoCD Sync 트리거
  - `SetOtaSettings(app_name, json)` → 설정 저장

### 4. OtamClientService (클라이언트 등록/알림)
- 외부 OTA 클라이언트의 등록 요청 수신 및 ID 발급
- OTA 진행 상태를 등록된 클라이언트에 알림 전송
- `NotifyAppUpdateStatus(app_name, status)`: 상태 변경 알림 publish

**DDS 토픽 (ClientService)**:
| 방향 | 토픽 |
|---|---|
| SUB | `/mmf/otamanager/otaclient_service/SetOtaClientRegistration` |
| PUB | `/mmf/otamanager/vehicleinfo_service/SetOtaClientRegistrationResponse` |
| PUB | `/mmf/otamanager/otaclient_service/NotifyOtaManagerReady` |
| PUB | `/mmf/otamanager/otaclient_service/NotifyAppUpdateStatus` |

### 5. OtamVehicleService (VISS 차량 정보 조회)
- VISS 프로토콜로 차량 신호 조회 (`/VIN/Vehicle` 토픽)
- 콜백 패턴: `RegisterCallback()` → 응답 수신 시 ActorService 콜백
- 주요 신호: `Vehicle.LowVoltageSystemState`, `Vehicle.Speed`

### 6. OtamDiagnosticService (진단/모니터링)
- OTA 매니저 상태를 DDS로 publish: `/mmf/otamanager/diagnostic_service/NotifyOtaMgrInfo`
- **late-subscriber 지원**: 구독자가 없을 때 메시지를 내부 버퍼에 저장, 구독자 연결 시 flush
- 모든 서비스에 주입되어 에러 리포팅 중앙화

---

## DDS 통신 계층

### OtamDds (추상 인터페이스)
```cpp
Create(topic, type, listener)
SetQos(qos)
Write(message)
Read(message)
Destroy()
```

### OtamDdsImpl<T> (FastDDS 구현체)
- DomainParticipant, Publisher, Subscriber, Topic, DataWriter, DataReader 관리
- 타입별 템플릿 특화

### IDL 메시지 타입 (generated/idl)
- `OtamClientServiceMsg`: 클라이언트 등록/알림
- `OtamDiagnosticServiceMsg`: 진단 메시지
- `OtamVehicleServiceMsg`: 차량 정보
- `OtamActorServiceMsg`: OTA 액터 요청/응답
- `common/ReturnValue`: 공통 결과 코드 (SUCCESS/FAIL/UNKNOWN)

---

## OTA 업데이트 시퀀스

```
Client                 ActorService        LifecycleMgr    VehicleService    ServerService
  │                        │                    │               │                 │
  │── SetAppUpdate ────────►│                    │               │                 │
  │                        │── SetServiceMode ──►│               │                 │
  │                        │                    │               │                 │
  │                        │◄── Response(OK) ───│               │                 │
  │                        │── RequestVehicleInfo ──────────────►│                 │
  │                        │◄── vehicle_state, vehicle_speed ───│                 │
  │                        │                                                       │
  │                        │  [차량 조건 검증]                                     │
  │                        │  acc OR (ignition AND speed==0)                       │
  │                        │── RunOtaUpdate ──────────────────────────────────────►│
  │                        │◄── Result ──────────────────────────────────────────│
  │◄── SetAppUpdateResponse│                                                       │
  │                        │  [백그라운드 폴링 500ms]                             │
  │◄── NotifyAppUpdateStatus (Running → Synced 등)
```

---

## 주요 설정 파일

`config/otam_config.json`:
```json
{
  "server": {
    "url": "http://localhost:10000",
    "username": "admin",
    "password": "start$44"
  }
}
```

---

## 빌드 구조

```
ota_mgr/
├── CMakeLists.txt          ← 최상위 빌드 (OtaManager 실행파일)
├── main.cc                 ← 진입점, 시그널 처리
├── otam_main/              ← 오케스트레이터
├── otam_common/            ← OtamDds 인터페이스, OtamDdsImpl, 공통 타입
├── otam_actor_service/     ← 핵심 OTA 조율
├── otam_server_service/    ← ArgoCD HTTP 클라이언트
├── otam_client_service/    ← 클라이언트 DDS 서비스
├── otam_vehicle_service/   ← VISS 차량 정보
├── otam_diagnostic_service/← 진단 pub
├── config/                 ← otam_config.json
├── test/                   ← 통합 테스트 (mock 서버)
└── unittest/               ← 단위 테스트 (gtest)
```

---

## 의존성

- **FastDDS** (eProsima): DDS 통신
- **nlohmann/json**: JSON 파싱
- **libcurl** (추정): HTTP REST 통신 (OtamServerApiClientImpl)
- **pthread**: 멀티스레딩
- IDL 생성 코드: `generated/idl/` (별도 빌드 스텝)

---

## 알려진 이슈 / 개선 포인트

1. `main.cc`의 시그널 핸들러가 `std::signal()` 사용 → FastDDS(boost::asio)가 signal_set 재등록 시 무효화 가능 (동일 버그가 someip_sensor_publisher에서 발견됨)
2. `PublishSetAppUpdateResponse`의 백그라운드 스레드가 `detach()` → 수명 관리 불명확
3. `OnVehicleInfoReceived`에서 mutex lock/unlock을 수동으로 관리하는 부분 존재 (잠재적 예외 안전성 문제)
4. `pending_updates_` map에서 iterator 무효화 가능성 (erase 후 접근)

