# agl-adsw-config 사용 가이드

> `agl/adsw_manager`의 서비스 목록 설정 파일을 CLI로 관리하는 도구

---

## 설치

```bash
cd service/extension/agl/adsw_manager/tools/
pip install -e .
```

설치 후 터미널 어디서든 `agl-adsw-config` 명령 사용 가능합니다.

---

## 기본 개념

`agl/adsw_manager`는 시작 시 아래 파일을 로드합니다:

```
./config/agl-service-list.yaml
```

`agl-adsw-config`는 이 파일을 직접 읽고 수정합니다.

### ros2 버전과의 차이

| 항목 | ros2/adsw_manager | agl/adsw_manager |
|------|-------------------|-----------------|
| 구조 | `specs > containers[] > services[]` | `services[]` (flat) |
| Docker 정보 위치 | 컨테이너 레벨 | **서비스 레벨** |
| 추가 필드 | 없음 | `mw_type`, `init_state`, `extra_args` |
| CLI | `adsw-config` | `agl-adsw-config` |

---

## YAML 스키마

```yaml
services:
  - name: adsw_monitor              # 필수, 고유 식별자
    service_type: NORMAL            # NORMAL | ONESHOT
    recovery_policy: ALWAYS         # R_DEFAULT | ALWAYS | ON_FAILURE
                                    # | ON_SUCCESS | ON_NOT_RESPONDING | MANUAL
    is_enabled: true                # true | false
    mw_type: AGL                    # AGL | ROS2 | AUTOSAR
    init_state: NOT_RUNNING         # NOT_RUNNING | RUNNING
    image: adsw-test-app:latest     # Docker 이미지
    extra_args:                     # docker run 추가 인자 (선택)
      - --net=host
```

---

## 공통 옵션

모든 명령 앞에 붙일 수 있는 전역 옵션입니다.

| 옵션 | 기본값 | 설명 |
|------|--------|------|
| `-f, --file <path>` | `config/agl-service-list.yaml` | 대상 YAML 파일 지정 |
| `--dry-run` | off | 변경 없이 미리보기만 (파일 수정 안 함) |
| `--no-backup` | off | 백업 파일 생성 없이 즉시 저장 |
| `-o, --output <fmt>` | `table` | 출력 형식: `table` \| `json` \| `yaml` |

```bash
# 특정 파일 지정
agl-adsw-config -f /opt/adsw/config/agl-service-list.yaml show

# 변경 전 미리보기
agl-adsw-config --dry-run set adsw_monitor --recovery MANUAL

# 백업 없이 저장
agl-adsw-config --no-backup add my_service --image my-app:latest

# JSON 형식으로 출력
agl-adsw-config -o json list
```

---

## 1. 조회 명령

### 전체 트리 보기

```bash
agl-adsw-config show
```

**출력 예시:**
```
agl-adsw-config  v0.1
├── ● adsw_monitor  AGL  docker=adsw-test-app:latest  recovery=ALWAYS
├── ● map           AGL  docker=adsw-test-app:latest  recovery=R_DEFAULT
└── ○ radio         AGL  docker=adsw-test-app:latest  recovery=R_DEFAULT
```

> `●` = enabled, `○` = disabled

---

### 서비스 목록 테이블

```bash
agl-adsw-config list

# JSON 포맷
agl-adsw-config -o json list

# YAML 포맷
agl-adsw-config -o yaml list
```

---

### 특정 서비스 상세 정보

```bash
agl-adsw-config get <name>
```

**예시:**
```bash
agl-adsw-config get adsw_monitor
```

**출력 예시:**
```
  name              adsw_monitor
  enabled           ●
  service_type      NORMAL
  recovery_policy   ALWAYS
  mw_type           AGL
  init_state        NOT_RUNNING
  image             adsw-test-app:latest
  extra_args        --net=host
```

---

## 2. 서비스 관리

### 서비스 추가

```bash
agl-adsw-config add <name> --image <image> [options]
```

| 옵션 | 허용값 | 기본값 |
|------|--------|--------|
| `--image <img>` | 문자열 | *필수* |
| `--type` | `NORMAL \| ONESHOT` | `NORMAL` |
| `--recovery` | `R_DEFAULT \| ALWAYS \| ON_FAILURE \| ON_SUCCESS \| ON_NOT_RESPONDING \| MANUAL` | `R_DEFAULT` |
| `--enabled` | `true \| false` | `true` |
| `--os-type` | `AGL \| ROS2 \| AUTOSAR` | `AGL` |
| `--init-state` | `NOT_RUNNING \| RUNNING` | `NOT_RUNNING` |
| `--extra-args <arg>` | 문자열 (반복 사용 가능) | - |

**예시:**
```bash
# 기본 서비스 추가
agl-adsw-config add navigation \
  --image navigation:latest \
  --type NORMAL \
  --recovery ALWAYS \
  --os-type AGL

# extra_args 포함 추가
agl-adsw-config add camera_node \
  --image camera:latest \
  --extra-args --net=host \
  --extra-args --privileged \
  --extra-args --device=/dev/video0

# 비활성화 상태로 추가
agl-adsw-config add experimental_svc \
  --image experimental:latest \
  --enabled false
```

---

### 서비스 속성 변경

```bash
agl-adsw-config set <name> [options]
```

**예시:**
```bash
# recovery 정책 변경
agl-adsw-config set navigation --recovery MANUAL

# 이미지 버전 업데이트
agl-adsw-config set navigation --image navigation:v2.0

# 여러 속성 동시 변경
agl-adsw-config set navigation \
  --image navigation:v2.0 \
  --recovery ON_FAILURE \
  --init-state RUNNING
```

---

### 서비스 활성화 / 비활성화

```bash
# 활성화 (is_enabled=true)
agl-adsw-config enable <name>

# 비활성화 (is_enabled=false)
agl-adsw-config disable <name>
```

**예시:**
```bash
agl-adsw-config enable navigation
agl-adsw-config disable experimental_svc
```

---

### 서비스 삭제

```bash
agl-adsw-config remove <name>
```

---

## 3. extra_args 관리

docker run 시 전달할 추가 인자를 개별적으로 관리합니다.

```bash
# 목록 확인
agl-adsw-config args <name> --list

# 항목 추가
agl-adsw-config args <name> --add <arg>

# 항목 제거
agl-adsw-config args <name> --remove <arg>
```

**예시:**
```bash
# 현재 extra_args 확인
agl-adsw-config args adsw_monitor --list

# 볼륨 마운트 추가
agl-adsw-config args navigation --add "--volume=/dev:/dev"

# 네트워크 설정 추가
agl-adsw-config args navigation --add "--net=host"

# 특정 항목 제거
agl-adsw-config args navigation --remove "--privileged"
```

---

## 4. 파일 관리

### 검증

YAML 구조와 값의 유효성을 검사합니다.

```bash
agl-adsw-config validate
```

검사 항목:
- `service_type` 허용 enum 범위 (`NORMAL | ONESHOT`)
- `recovery_policy` 허용 enum 범위
- `mw_type` 허용 enum 범위 (`AGL | ROS2 | AUTOSAR`)
- `init_state` 허용 enum 범위 (`NOT_RUNNING | RUNNING`)
- `name` 중복 여부

---

### 백업

```bash
agl-adsw-config backup
# → config/agl-service-list.bak.20260330-150000
```

변경 작업 시 기본적으로 자동 백업이 생성됩니다.
`--no-backup` 옵션으로 비활성화 가능합니다.

---

### 변경 내용 비교 (diff)

마지막 백업 파일과 현재 파일의 차이를 보여줍니다.

```bash
agl-adsw-config diff
```

**출력 예시:**
```diff
--- config/agl-service-list.bak.20260330-150925
+++ config/agl-service-list.yaml
@@ -3,7 +3,7 @@
   name: adsw_monitor
   service_type: NORMAL
-  recovery_policy: R_DEFAULT
+  recovery_policy: ALWAYS
   is_enabled: true
   mw_type: AGL
```

---

## 5. 실전 시나리오

### 새 서비스 셋업

```bash
# 1. 서비스 추가
agl-adsw-config add lidar_node \
  --image lidar:latest \
  --recovery ALWAYS \
  --extra-args --net=host \
  --extra-args --privileged

# 2. 결과 확인
agl-adsw-config get lidar_node

# 3. 전체 확인
agl-adsw-config show
```

---

### 배포 전 안전 체크

```bash
# 1. 검증
agl-adsw-config validate

# 2. dry-run으로 변경 미리보기
agl-adsw-config --dry-run set adsw_monitor --recovery ON_FAILURE

# 3. 백업 생성
agl-adsw-config backup

# 4. 실제 변경 (백업 skip)
agl-adsw-config --no-backup set adsw_monitor --recovery ON_FAILURE

# 5. diff로 변경 확인
agl-adsw-config diff
```

---

### 특정 서비스만 일시 비활성화

```bash
# 비활성화
agl-adsw-config disable radio

# 확인
agl-adsw-config show

# 재활성화
agl-adsw-config enable radio
```

---

### mw_type별 서비스 현황 확인

```bash
# JSON으로 출력 후 jq로 필터링
agl-adsw-config -o json list | jq '.[] | select(.mw_type == "AGL")'
```

---

## 6. 허용 값 빠른 참고

| 필드 | 허용값 |
|------|--------|
| `service_type` | `NORMAL` \| `ONESHOT` |
| `recovery_policy` | `R_DEFAULT` \| `ALWAYS` \| `ON_FAILURE` \| `ON_SUCCESS` \| `ON_NOT_RESPONDING` \| `MANUAL` |
| `mw_type` | `AGL` \| `ROS2` \| `AUTOSAR` |
| `init_state` | `NOT_RUNNING` \| `RUNNING` |
| `is_enabled` | `true` \| `false` |

---

*대상 프로젝트: `oss_multisw_framework/service/extension/agl/adsw_manager`*
*작성일: 2026-03-30*
