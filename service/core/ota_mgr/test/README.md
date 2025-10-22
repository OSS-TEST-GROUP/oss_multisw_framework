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

### 2. mock_argocd_server

OtamServerService가 호출하는 ArgoCD REST API를 모방하는 Mock HTTP 서버입니다.

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
```bash
# 기본 포트(8080)로 실행
./bin/test/mock_argocd_server

# 커스텀 포트로 실행
./bin/test/mock_argocd_server 9090
```

#### Mock 데이터 수정

Mock 서버는 `mock_data/` 디렉토리의 JSON 파일들을 사용합니다:

- **session_response.json** - 세션 생성 응답 템플릿
- **sync_response.json** - 동기화 응답 템플릿
- **sample-app.json** - sample-app 애플리케이션 데이터
- **test-app.json** - test-app 애플리케이션 데이터

JSON 파일을 수정하여 다양한 시나리오를 테스트할 수 있습니다.

##### 예제: 새 애플리케이션 추가

1. `mock_data/` 디렉토리에 새 JSON 파일 생성 (예: `my-app.json`)
2. ArgoCD Application 형식에 맞게 데이터 작성
3. `mock_argocd_server.cc`의 `InitializeMockData()` 함수에서 파일 경로 추가:
```cpp
std::vector<std::string> app_files = {
  "mock_data/sample-app.json",
  "mock_data/test-app.json",
  "mock_data/my-app.json"  // 추가
};
```

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

## 빌드 방법

```bash
# BUILD_TEST 옵션을 활성화하여 빌드
cmake -DBUILD_TEST=ON ..
make

# 테스트 애플리케이션들이 bin/test 디렉토리에 생성됨
ls bin/test/
# test_otam_actor_service
# mock_argocd_server
```

## 테스트 시나리오

### 전체 통합 테스트

1. **Mock ArgoCD 서버 시작**
   ```bash
   ./bin/test/mock_argocd_server
   ```

2. **OTA Manager 메인 애플리케이션 시작**
   ```bash
   ./bin/OtaManager
   ```

3. **테스트 클라이언트로 기능 테스트**
   ```bash
   ./bin/test/test_otam_actor_service
   ```

4. 테스트 시나리오:
   - 세션 생성 확인 (OTA Manager 로그)
   - GetAppUpdateSettings 요청 → 설정 정보 확인
   - SetAppUpdate 요청 → 동기화 트리거 확인
   - GetAppUpdateState 요청 → 상태 변경 확인

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
