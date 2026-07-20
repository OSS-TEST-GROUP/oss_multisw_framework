# adsw-config 사용 가이드

> `ros2/adsw_manager`의 서비스 목록 설정 파일을 CLI로 관리하는 도구

---

## 설치

```bash
cd service/extension/ros2/adsw_manager/tools/
pip install -e .
```

설치 후 터미널 어디서든 `adsw-config` 명령 사용 가능합니다.

---

## 기본 개념

`adsw_manager`는 시작 시 아래 순서로 설정 파일을 읽습니다:

```
1순위: $SAVED_REGISTRY_BACKUP/saved-service-list.yaml
2순위: $SAVED_REGISTRY_BACKUP/sils-sample-speclist.yaml (fallback, k3s Pod)
환경변수 미설정 시: ./config/saved-service-list.yaml
```

`adsw-config`는 이 파일을 직접 읽고 수정합니다.

---

## 공통 옵션

모든 명령 앞에 붙일 수 있는 전역 옵션입니다.

| 옵션 | 기본값 | 설명 |
|------|--------|------|
| `-f, --file <path>` | `$SAVED_REGISTRY_BACKUP/saved-service-list.yaml` | 대상 YAML 파일 지정 |
| `--dry-run` | off | 변경 없이 미리보기만 |
| `--no-backup` | off | 백업 파일 생성 없이 즉시 저장 |
| `-o, --output <fmt>` | `table` | 출력 형식: `table` \| `json` \| `yaml` |

```bash
# 특정 파일 지정
adsw-config -f /path/to/my-service-list.yaml show

# 변경 전 미리보기
adsw-config --dry-run service set my-container my_svc --recovery MANUAL

# 백업 없이 저장
adsw-config --no-backup service add ...

# JSON 형식으로 출력
adsw-config -o json list services
```

---

## 1. 조회 명령

### 전체 트리 보기

```bash
adsw-config show
```

**출력 예시:**
```
adsw-config  v0.1
├── autoware  docker-compose
│   ├── image: autoware-universe:latest
│   └── services
│       ├── ● sensing       autoware_launch  type=NORMAL  recovery=MANUAL
│       └── ○ localization  autoware_launch  type=NORMAL  recovery=MANUAL
└── adsw-monitor  docker
    ├── image: adsw-monitor:latest
    └── services
        └── ● adsw_monitor  adsw_pkg  type=NORMAL  recovery=ALWAYS
```

> `●` = enabled, `○` = disabled

---

### 컨테이너 목록

```bash
adsw-config list containers
```

```bash
# JSON으로 출력
adsw-config -o json list containers
```

---

### 서비스 목록

```bash
# 전체 서비스
adsw-config list services

# 특정 컨테이너의 서비스만
adsw-config list services --container autoware
```

---

## 2. 컨테이너 관리

### 컨테이너 추가

```bash
# docker 타입
adsw-config container add <name> --image <image> --type docker

# docker-compose 타입 (deploy-filepath 필수)
adsw-config container add <name> \
  --image <image> \
  --type docker-compose \
  --deploy-filepath docker-compose.yaml

# k3s 타입 (deploy-filepath, namespace 필수)
adsw-config container add <name> \
  --image <image> \
  --type k3s \
  --deploy-filepath pod.yaml \
  --namespace autoware
```

**예시:**
```bash
adsw-config container add autoware \
  --image autoware-universe:latest \
  --type docker-compose \
  --deploy-filepath docker-compose.autoware.yaml
```

---

### 컨테이너 속성 변경

```bash
adsw-config container set <name> [--image] [--type] [--deploy-filepath] [--namespace]
```

**예시:**
```bash
# 이미지 버전 업데이트
adsw-config container set autoware --image autoware-universe:v1.1

# docker → docker-compose 타입 변경
adsw-config container set my-app \
  --type docker-compose \
  --deploy-filepath docker-compose.yml
```

---

### 컨테이너 삭제

```bash
# 서비스가 없는 경우
adsw-config container remove <name>

# 서비스가 있는 경우 --force 필요 (하위 서비스 모두 삭제)
adsw-config container remove <name> --force
```

---

## 3. 서비스 관리

### 서비스 추가

```bash
adsw-config service add <container> <name> \
  --package <pkg> \
  [--launchfile <file>] \
  [--type NORMAL|ONESHOT] \
  [--recovery ALWAYS|ON_FAILURE|ON_SUCCESS|ON_NOT_RESPONDING|MANUAL] \
  [--enabled true|false] \
  [--max <int>]
```

**예시:**
```bash
# 기본 서비스 추가
adsw-config service add autoware sensing \
  --package autoware_launch \
  --launchfile sensing.launch.xml \
  --type NORMAL \
  --recovery MANUAL \
  --enabled true

# 비활성화 상태로 추가
adsw-config service add autoware localization \
  --package autoware_launch \
  --launchfile localization.launch.xml \
  --enabled false
```

---

### 서비스 속성 변경

```bash
adsw-config service set <container> <name> [options]
```

**예시:**
```bash
# recovery 정책 변경
adsw-config service set autoware sensing --recovery ALWAYS

# 여러 속성 동시 변경
adsw-config service set autoware sensing \
  --recovery ON_FAILURE \
  --max 3
```

---

### 서비스 활성화 / 비활성화

```bash
# 활성화
adsw-config service enable <container> <name>

# 비활성화
adsw-config service disable <container> <name>
```

**예시:**
```bash
adsw-config service enable autoware localization
adsw-config service disable autoware sensing
```

---

### 서비스 삭제

```bash
adsw-config service remove <container> <name>
```

---

## 4. 파일 관리

### 검증

YAML 구조와 값의 유효성을 검사합니다.

```bash
adsw-config validate
```

검사 항목:
- `container-type`이 `docker-compose` / `k3s`이면 `deploy-filepath` 존재 여부
- `recovery` 값이 허용 enum 범위 내인지
- `type` 값이 `NORMAL` / `ONESHOT`인지
- 같은 컨테이너 내 서비스 `name` 중복 여부

---

### 백업

```bash
adsw-config backup
# → config/saved-service-list.yaml.bak.20260330-120000
```

변경 작업 시 기본적으로 자동 백업이 생성됩니다.  
`--no-backup` 옵션으로 비활성화 가능합니다.

---

### 변경 내용 비교 (diff)

마지막 백업 파일과 현재 파일의 차이를 보여줍니다.

```bash
adsw-config diff
```

**출력 예시:**
```diff
--- backup (saved-service-list.yaml.bak.20260330-115642)
+++ current (saved-service-list.yaml)
@@ -15,6 +15,6 @@
       name: sensing
       launchfile: sensing.launch.xml
       type: NORMAL
-      recovery: MANUAL
+      recovery: ALWAYS
       enabled: 'true'
       max: '0'
```

---

## 5. 실전 시나리오

### 새 컨테이너 + 서비스 셋업

```bash
# 1. 컨테이너 추가
adsw-config container add my-ros2-app \
  --image my-ros2-app:latest \
  --type docker-compose \
  --deploy-filepath docker-compose.yaml

# 2. 서비스 추가
adsw-config service add my-ros2-app sensor_node \
  --package sensor_pkg \
  --launchfile sensor.launch.xml \
  --type NORMAL \
  --recovery ALWAYS

adsw-config service add my-ros2-app planner_node \
  --package planner_pkg \
  --launchfile planner.launch.xml \
  --type NORMAL \
  --recovery ON_FAILURE

# 3. 결과 확인
adsw-config show
```

---

### 운영 중 recovery 정책 일괄 검토

```bash
# 현재 상태 JSON으로 출력 → 스크립트 파이프라인 활용
adsw-config -o json list services | jq '.[] | select(.recovery == "ALWAYS")'
```

---

### 배포 전 안전 체크

```bash
# 검증
adsw-config validate

# dry-run으로 변경 미리보기
adsw-config --dry-run service set my-app my_svc --recovery MANUAL

# 백업 후 적용
adsw-config backup
adsw-config --no-backup service set my-app my_svc --recovery MANUAL

# 변경 내용 확인
adsw-config diff
```

---

## 6. YAML 스키마 참고

```yaml
version: 0.1
specs:
  - containers:
      - name: <string>                    # 필수, 고유 식별자
        image: <string>                   # 필수
        container-type: docker            # 필수: docker | docker-compose | k3s
        deploy-filepath: <path>           # docker-compose / k3s 필수
        namespace: <ns>                   # k3s 필수
        services:
          - package: <pkg>                # 필수
            name: <string>               # 필수, 컨테이너 내 고유
            launchfile: <file>
            type: NORMAL                 # NORMAL | ONESHOT
            recovery: ALWAYS             # ALWAYS | ON_FAILURE | ON_SUCCESS
                                         # | ON_NOT_RESPONDING | MANUAL
            enabled: "true"              # "true" | "false"
            max: "0"                     # 0 = 무제한
```

---

## 7. 환경 변수

| 변수 | 설명 |
|------|------|
| `SAVED_REGISTRY_BACKUP` | `saved-service-list.yaml`이 위치한 디렉토리 경로 |

```bash
export SAVED_REGISTRY_BACKUP=/opt/adsw/config
adsw-config show
```

---

*대상 프로젝트: `oss_multisw_framework/service/extension/ros2/adsw_manager`*  
*작성일: 2026-03-30*
