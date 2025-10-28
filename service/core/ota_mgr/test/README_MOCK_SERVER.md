# Mock ArgoCD API Server

FastAPI 기반의 ArgoCD API Mock Server입니다. 실제 ArgoCD 인스턴스 없이 OTA Manager를 테스트할 수 있습니다.

## 특징

- **FastAPI 기반**: 빠르고 현대적인 Python 웹 프레임워크
- **자동 API 문서**: Swagger UI와 ReDoc 지원
- **타입 안정성**: Pydantic 모델을 통한 요청/응답 검증
- **간단한 설정**: Python 3.7+ 이상에서 바로 실행 가능

## 지원하는 API 엔드포인트

| Method | Endpoint | Description |
|--------|----------|-------------|
| POST | `/api/v1/session` | 세션 생성 및 토큰 발급 |
| GET | `/api/v1/applications` | 모든 애플리케이션 목록 조회 |
| GET | `/api/v1/applications/{app_name}` | 특정 애플리케이션 정보 조회 |
| PATCH | `/api/v1/applications/{app_name}` | 애플리케이션 설정 업데이트 (Auto Sync) |
| POST | `/api/v1/applications/{app_name}/sync` | 애플리케이션 동기화 트리거 |
| GET | `/health` | 서버 상태 확인 |
| GET | `/docs` | Swagger UI 문서 |
| GET | `/redoc` | ReDoc 문서 |

## 설치

### 1. 의존성 설치

```bash
cd test
python3 -m venv argocd_env
source argocd_env/bin/activate
```

```bash
pip install -r requirements.txt
```

또는 직접 설치:

```bash
pip install fastapi==0.104.1 uvicorn[standard]==0.24.0 pydantic==2.5.0
```

### 2. 서버 실행

기본 설정으로 실행 (포트 8080):

```bash
python3 mock_argocd_server.py
```

커스텀 설정으로 실행:

```bash
# 포트 변경
python3 mock_argocd_server.py -p 9090

# 디버그 모드
python3 mock_argocd_server.py -d

# 자동 재로드 활성화 (개발 시)
python3 mock_argocd_server.py --reload

# 모든 옵션 조합
python3 mock_argocd_server.py -H 0.0.0.0 -p 8080 -d --reload
```

### 3. API 문서 확인

서버 실행 후 브라우저에서 다음 URL로 접속:

- **Swagger UI**: http://localhost:8080/docs
- **ReDoc**: http://localhost:8080/redoc

## 사용 예제

### 1. 세션 생성 (토큰 발급)

```bash
curl -X POST \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"admin123"}' \
  http://localhost:8080/api/v1/session
```

**응답 예제:**
```json
{
  "token": "mock_token_1234567890",
  "username": "admin",
  "iat": 1234567890,
  "exp": 1234654290
}
```

### 2. 애플리케이션 목록 조회

```bash
TOKEN="mock_token_1234567890"

curl -s -H "Authorization: Bearer $TOKEN" \
  http://localhost:8080/api/v1/applications | jq -r '.items[].metadata.name'
```

**응답 예제:**
```
sample-app
test-app
```

### 3. 특정 애플리케이션 조회

```bash
curl -s -H "Authorization: Bearer $TOKEN" \
  http://localhost:8080/api/v1/applications/sample-app | jq '.'
```

### 4. Auto Sync 상태 확인

```bash
curl -s -H "Authorization: Bearer $TOKEN" \
  http://localhost:8080/api/v1/applications/sample-app | jq '.spec.syncPolicy.automated'
```

### 5. Auto Sync 활성화

```bash
curl -X PATCH \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "patch": "{\"spec\":{\"syncPolicy\":{\"automated\":{\"prune\":true,\"selfHeal\":true}}}}",
    "patchType": "merge"
  }' \
  http://localhost:8080/api/v1/applications/sample-app
```

### 6. Auto Sync 비활성화

```bash
curl -X PATCH \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "patch": "{\"spec\":{\"syncPolicy\":null}}",
    "patchType": "merge"
  }' \
  http://localhost:8080/api/v1/applications/sample-app
```

### 7. Sync 상태 확인

```bash
curl -s -H "Authorization: Bearer $TOKEN" \
  http://localhost:8080/api/v1/applications/sample-app | jq -r '.status.sync.status'
```

**응답 예제:**
```
Synced
```

### 8. Sync 트리거

```bash
curl -X POST \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{}' \
  http://localhost:8080/api/v1/applications/sample-app/sync
```

**응답 예제:**
```json
{
  "status": "success",
  "message": "Sync operation started",
  "application": "sample-app",
  "timestamp": 1234567890.123
}
```

### 9. Health Check

```bash
curl http://localhost:8080/health | jq '.'
```

**응답 예제:**
```json
{
  "status": "healthy",
  "applications": 2,
  "authenticated": true
}
```

## 자동 테스트 스크립트

모든 API 엔드포인트를 테스트하는 스크립트가 제공됩니다:

```bash
./test_mock_server.sh
```

이 스크립트는 다음을 자동으로 테스트합니다:
1. 서버 상태 확인
2. 세션 생성
3. 애플리케이션 목록 조회
4. 특정 애플리케이션 조회
5. Auto Sync 활성화
6. Auto Sync 상태 확인
7. Sync 상태 조회
8. Sync 트리거
9. Auto Sync 비활성화
10. Health check

## Mock 데이터

서버 시작 시 다음 애플리케이션이 자동으로 로드됩니다:

### sample-app
- **상태**: Synced
- **Health**: Healthy
- **Auto Sync**: Disabled (기본값)
- **Namespace**: default

### test-app
- **상태**: OutOfSync
- **Health**: Progressing
- **Auto Sync**: Enabled (prune: true, selfHeal: true)
- **Namespace**: test

## 인증

모든 API 엔드포인트(세션 생성 제외)는 Bearer 토큰 인증이 필요합니다:

```bash
Authorization: Bearer <token>
```

토큰은 `/api/v1/session` 엔드포인트를 통해 발급받을 수 있습니다.

## 에러 처리

### 401 Unauthorized
토큰이 없거나 유효하지 않은 경우:
```json
{
  "detail": "Invalid token"
}
```

### 404 Not Found
애플리케이션을 찾을 수 없는 경우:
```json
{
  "error": "Application not found",
  "code": 5
}
```

### 400 Bad Request
잘못된 요청 형식:
```json
{
  "detail": "Invalid JSON in patch: ..."
}
```

## 개발

### 디버그 모드로 실행

```bash
python3 mock_argocd_server.py -d
```

### 자동 재로드 활성화

코드 변경 시 자동으로 서버를 재시작합니다:

```bash
python3 mock_argocd_server.py --reload
```

### 로그 확인

모든 API 요청이 로그로 출력됩니다:
- 세션 생성
- 애플리케이션 조회/수정
- Auto Sync 상태 변경
- Sync 트리거

## Python으로 사용하기

Python 코드에서 직접 사용할 수도 있습니다:

```python
import requests

BASE_URL = "http://localhost:8080"

# 1. 세션 생성
response = requests.post(
    f"{BASE_URL}/api/v1/session",
    json={"username": "admin", "password": "admin123"}
)
token = response.json()["token"]

# 2. 헤더에 토큰 포함
headers = {"Authorization": f"Bearer {token}"}

# 3. 애플리케이션 목록 조회
response = requests.get(f"{BASE_URL}/api/v1/applications", headers=headers)
apps = response.json()["items"]

for app in apps:
    print(app["metadata"]["name"])

# 4. Auto Sync 활성화
patch_data = {
    "patch": '{"spec":{"syncPolicy":{"automated":{"prune":true,"selfHeal":true}}}}',
    "patchType": "merge"
}
response = requests.patch(
    f"{BASE_URL}/api/v1/applications/sample-app",
    json=patch_data,
    headers=headers
)

# 5. Sync 트리거
response = requests.post(
    f"{BASE_URL}/api/v1/applications/sample-app/sync",
    json={},
    headers=headers
)
print(response.json())
```

## 문제 해결

### 서버가 시작되지 않는 경우

1. 의존성이 설치되어 있는지 확인:
   ```bash
   pip list | grep -E "fastapi|uvicorn|pydantic"
   ```

2. 포트가 이미 사용 중인지 확인:
   ```bash
   lsof -i :8080
   ```

3. 다른 포트로 시작:
   ```bash
   python3 mock_argocd_server.py -p 9090
   ```

### 토큰 인증 실패

1. 세션을 먼저 생성했는지 확인
2. 토큰이 올바르게 헤더에 포함되었는지 확인:
   ```bash
   curl -H "Authorization: Bearer <token>" ...
   ```

## 라이센스

이 프로젝트는 OTA Manager 테스트 목적으로 제공됩니다.
