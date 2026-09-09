# OTA Manager Test Applications

이 디렉토리에는 OTA Manager 컴포넌트를 테스트하기 위한 테스트 애플리케이션들이 포함되어 있습니다.

## 테스트 애플리케이션

### 1. test_otam_actor_service

OtamActorService의 DDS 통신을 테스트하는 대화형 애플리케이션입니다.

#### 기능
- GetAppUpdateSettings - 앱 업데이트 설정 조회
- SetAppUpdateSettings - 앱 업데이트 설정 변경
- GetAppUpdateState - 앱 업데이트 상태 조회
- SetAppUpdate - 앱 업데이트 실행 요청

#### 실행 방법
```bash
# 테스트 앱 실행
./bin/test/test_otam_actor_service

# 메뉴에서 원하는 기능을 선택하여 테스트
```

#### 주의사항
- OtamActorService가 실행 중이어야 정상적으로 응답을 받을 수 있습니다.
- DDS discovery를 위해 초기 대기 시간(3초)이 필요합니다.

---

### 2. mock_argocd_server.py

OtamServerService가 호출하는 ArgoCD REST API를 모방하는 FastAPI 기반 Mock HTTP 서버입니다.  
(상세 가이드: [`README_MOCK_SERVER.md`](README_MOCK_SERVER.md))

#### 지원 엔드포인트

1. **POST /api/v1/session**
   - 세션/토큰 생성
   - Request: `{"username": "...", "password": "..."}`
   - Response: `{"token": "..."}`

2. **GET /api/v1/applications/{app_name}**
   - 애플리케이션 정보 조회
   - Response: ArgoCD Application 객체 (JSON)

3. **POST /api/v1/applications/{app_name}/sync**
   - 애플리케이션 동기화 트리거
   - Request: `{}`
   - Response: `{"status": "success", "message": "..."}`

4. **PATCH /api/v1/applications/{app_name}**
   - 애플리케이션 설정 변경
   - Request: `{"patch": "...", "patchType": "merge"}`
   - Response: 업데이트된 Application 객체

#### 실행 방법

Python 의존성 설치:
```bash
pip install -r requirements.txt
```

서버 실행:
```bash
# 기본 포트(8080)로 실행
python3 mock_argocd_server.py

# 커스텀 포트(-p) 및 디버그 모드(-d)로 실행
python3 mock_argocd_server.py -p 9090 -d
```
> 빌드 산출물 디렉토리에서 실행 시: `python3 ./bin/test/mock_argocd_server.py`

#### Mock 데이터 수정

Mock 서버는 `mock_data/` 디렉토리의 JSON 파일들을 로드합니다:

- **session_response.json** - 세션 생성 응답 템플릿
- **sync_response.json** - 동기화 응답 템플릿
- **sample-app.json** - sample-app 애플리케이션 데이터
- **test-app.json** - test-app 애플리케이션 데이터

JSON 파일을 수정하거나 `mock_data/`에 새 애플리케이션 JSON을 추가하여 다양한 시나리오를 테스트할 수 있습니다.

#### OtamServerService 설정

Mock 서버를 사용하려면 OtamServerService의 config.json을 다음과 같이 설정하세요:

```json
{
  "server": {
    "url": "http://localhost:8080",
    "username": "admin",
    "password": "password"
  }
}
```

---

### 3. mock_viss_server

OtamVehicleService가 요청하는 차량 정보(Vehicle Info)에 대해 응답하는 Mock VISS 서버입니다.

#### 기능
- VISS Request 구독 (`/VIN/Vehicle` 토픽)
- YAML 파일에서 차량 데이터 로드
- VISS Response 발행 (`/mmf/otamanager/vehicleinfo_service/GetVehicleInfoResponse` 토픽)

#### 지원하는 차량 정보
`mock_data/vehicle_info.yaml` 파일에 정의된 모든 차량 정보를 지원합니다:
- **차량 식별 정보**: VIN, Model, Year, Brand 등
- **차량 상태 정보**: LowVoltageSystemState, Speed (OtamActorService에서 사용)
- **배터리 정보**: StateOfCharge, StateOfHealth, Temperature
- **위치 정보**: Latitude, Longitude, Altitude
- **기타 상태**: 주행거리계, 트립미터 등

#### 실행 방법
```bash
# 기본 설정 파일(mock_data/vehicle_info.yaml)로 실행
./bin/test/mock_viss_server

# 커스텀 YAML 파일로 실행
./bin/test/mock_viss_server /path/to/custom_vehicle_info.yaml
```

#### YAML 설정 파일 수정

`mock_data/vehicle_info.yaml` 파일을 수정하여 다양한 차량 데이터를 테스트할 수 있습니다:

```yaml
vehicle_info:
  # 차량 식별 정보
  Vehicle.VehicleIdentification.VIN: "KMHXX00XXXX000000"
  Vehicle.VehicleIdentification.Model: "Test Vehicle Model"

  # OtamActorService가 요청하는 정보
  Vehicle.LowVoltageSystemState: "Active"
  Vehicle.Speed: "0"

  # 추가 차량 정보
  Vehicle.Powertrain.TractionBattery.StateOfCharge.Current: "85.5"
```

#### OtamActorService와의 연동

OtamActorService의 `OnSetServiceModeResponseReceived` 함수가 호출되면:
1. `otam_vehicle_service_->RequestVehicleInfo(id, "Vehicle.LowVoltageSystemState")` 호출
2. `otam_vehicle_service_->RequestVehicleInfo(id, "Vehicle.Speed")` 호출
3. Mock VISS Server가 이 요청들을 수신하여 YAML에서 값을 읽고 응답

#### 테스트 시나리오

```bash
# 터미널 1: Mock VISS Server 시작
./bin/test/mock_viss_server

# 터미널 2: OTA Manager 실행
./bin/OtaManager

# 터미널 3: 테스트 클라이언트로 SetAppUpdate 요청
./bin/test/test_otam_actor_service
# 메뉴에서 4번 선택 (SetAppUpdate)

# 예상 결과:
# - OtamActorService가 Service Mode 변경 요청
# - Service Mode 응답 수신 후 차량 정보 요청
# - Mock VISS Server가 Vehicle.LowVoltageSystemState와 Vehicle.Speed 응답
# - OtamVehicleService가 응답 수신 및 처리
```

---

### 4. mock_lifecycle_service

OtamActorService가 요청하는 Service Mode 변경에 대해 응답하는 Mock Lifecycle Manager 서비스입니다.

#### 기능
- SetServiceModeRequest 구독 (`mmf/lifecyclemgr/SetServiceModeRequest` 토픽)
- YAML 파일에서 응답 설정 로드
- SetServiceModeResponse 발행 (`mmf/lifecyclemgr/SetServiceModeResponse` 토픽)

#### 지원하는 Return Code
`mock_data/lifecycle_config.yaml` 파일에서 설정 가능:
- **SUCCESS**: Service mode 변경 성공
- **FAILED**: Service mode 변경 실패

#### 실행 방법
```bash
# 기본 설정 파일(mock_data/lifecycle_config.yaml)로 실행
./bin/test/mock_lifecycle_service

# 커스텀 YAML 파일로 실행
./bin/test/mock_lifecycle_service /path/to/custom_lifecycle_config.yaml
```

#### YAML 설정 파일 구조

`mock_data/lifecycle_config.yaml` 파일을 수정하여 다양한 시나리오를 테스트할 수 있습니다:

```yaml
# 서비스 모드 응답 설정
service_mode_response:
  return_code: SUCCESS  # SUCCESS, FAILED
  response_delay_ms: 0  # 응답 지연 시간 (밀리초)
  mode: automatic

# 시나리오 정의
scenarios:
  success:
    return_code: SUCCESS
    response_delay_ms: 0

  failure:
    return_code: FAILED
    response_delay_ms: 0

  slow_success:
    return_code: SUCCESS
    response_delay_ms: 2000

# 활성 시나리오 선택
active_scenario: success
```

#### OtamActorService와의 연동

OtamActorService의 `SendSetServiceModeRequest` 함수가 호출되면:
1. SetServiceModeRequest가 `mmf/lifecyclemgr/SetServiceModeRequest` 토픽으로 발행
2. Mock Lifecycle Service가 요청 수신
3. YAML 설정에 따라 SUCCESS/FAILED 응답 생성
4. `mmf/lifecyclemgr/SetServiceModeResponse` 토픽으로 응답 발행
5. OtamActorService의 `OnSetServiceModeResponseReceived` 콜백 호출
6. SUCCESS인 경우 → 차량 정보 요청 진행
7. FAILED인 경우 → 업데이트 중단 및 에러 응답

#### 테스트 시나리오

**성공 케이스:**
```bash
# 터미널 1: Mock Lifecycle Service (SUCCESS 응답)
./bin/test/mock_lifecycle_service

# 터미널 2: Mock VISS Server
./bin/test/mock_viss_server

# 터미널 3: OTA Manager 실행
./bin/OtaManager

# 터미널 4: SetAppUpdate 요청
./bin/test/test_otam_actor_service
# 메뉴에서 4번 선택

# 예상 결과:
# - SetServiceModeRequest 전송
# - SetServiceModeResponse SUCCESS 수신
# - Vehicle info 요청/응답
# - OTA 업데이트 진행
```

**실패 케이스:**
```bash
# lifecycle_config.yaml에서 active_scenario를 'failure'로 변경
# Mock Lifecycle Service 재시작
./bin/test/mock_lifecycle_service

# 동일한 테스트 수행 시 예상 결과:
# - SetServiceModeRequest 전송
# - SetServiceModeResponse FAILED 수신
# - OTA 업데이트 중단
# - 에러 응답 발행
```

---

## 빌드 방법

```bash
# BUILD_TEST 옵션을 활성화하여 빌드
cmake -DBUILD_TEST=ON ..
make

# 테스트 애플리케이션들이 bin/test 디렉토리에 생성됨
ls bin/test/
# test_otam_actor_service
# mock_argocd_server
# mock_viss_server
# mock_lifecycle_service
```

## 테스트 시나리오

### 전체 통합 테스트

1. **Mock 서버들 시작**
   ```bash
   # 터미널 1: Mock ArgoCD 서버
   ./bin/test/mock_argocd_server

   # 터미널 2: Mock VISS 서버
   ./bin/test/mock_viss_server

   # 터미널 3: Mock Lifecycle Service
   ./bin/test/mock_lifecycle_service
   ```

2. **OTA Manager 메인 애플리케이션 시작**
   ```bash
   # 터미널 4
   ./bin/OtaManager
   ```

3. **테스트 클라이언트로 기능 테스트**
   ```bash
   # 터미널 5
   ./bin/test/test_otam_actor_service
   ```

4. 테스트 시나리오:
   - 세션 생성 확인 (OTA Manager 로그)
   - GetAppUpdateSettings 요청 → 설정 정보 확인
   - SetAppUpdate 요청 → Service Mode 변경 요청/응답 → 차량 정보 요청/응답 → 동기화 트리거 확인
   - GetAppUpdateState 요청 → 상태 변경 확인

### 전체 흐름도

```
SetAppUpdate 요청
    ↓
OtamActorService::OnAppUpdateRequested
    ↓
SendSetServiceModeRequest (OTA_MODE)
    ↓
Mock Lifecycle Service 수신
    ↓
YAML 설정 확인 (return_code)
    ↓
SetServiceModeResponse 발행
    ↓
OtamActorService::OnSetServiceModeResponseReceived
    ↓
[SUCCESS 경로]                    [FAILED 경로]
    ↓                                  ↓
RequestVehicleInfo                SetAppUpdateResponse (FAILED)
    ↓                                  ↓
Mock VISS Server 응답              종료
    ↓
OnVehicleInfoResponseReceived
    ↓
OTA 업데이트 진행
```

---

## 문제 해결

### DDS Discovery 실패
- 방화벽에서 DDS 멀티캐스트 포트가 차단되어 있는지 확인
- 네트워크 인터페이스 설정 확인

### Mock 서버 연결 실패
- Mock 서버가 실행 중인지 확인
- config.json의 URL이 올바른지 확인 (http:// 또는 https://)
- 포트 번호가 일치하는지 확인

### JSON 파싱 오류
- mock_data/*.json 파일의 형식이 올바른지 확인
- JSON 유효성 검사 도구 사용 (예: jsonlint)
