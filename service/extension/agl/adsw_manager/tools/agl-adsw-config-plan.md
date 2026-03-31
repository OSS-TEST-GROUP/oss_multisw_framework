# agl-adsw-config CLI 기획서

> `agl/adsw_manager`의 설정 파일을 CLI로 관리하는 도구

---

## 1. 개요

`agl/adsw_manager`가 사용하는 `agl-service-list.yaml` 설정 파일을 직접 편집하지 않고,
CLI 명령어를 통해 안전하게 조회·수정·검증할 수 있는 도구.

---

## 2. YAML 구조 (agl-service-list.yaml)

```yaml
services:
  - name: adsw_monitor
    service_type: NORMAL         # NORMAL | ONESHOT
    recovery_policy: ALWAYS      # R_DEFAULT | ALWAYS | ON_FAILURE | ON_SUCCESS
                                 # | ON_NOT_RESPONDING | MANUAL
    is_enabled: true             # true | false
    os_type: AGL                 # AGL | ROS2 | AUTOSAR
    init_state: NOT_RUNNING      # NOT_RUNNING | RUNNING
    image: adsw-test-app:latest
    extra_args:
      - --net=host
```

> ros2 버전과 달리 **flat 구조** (컨테이너 계층 없음). Docker 정보(image, extra_args)가 서비스 레벨에 존재.

---

## 3. ros2 버전과 구조 비교

```
ros2 버전                    agl 버전
─────────────────────        ────────────────────
specs                        services
└── containers[]             └── service (flat)
    ├── name                     ├── name
    ├── image                    ├── image        ← 서비스 레벨
    ├── container-type           ├── extra_args   ← 서비스 레벨
    └── services[]               ├── service_type
        ├── package              ├── recovery_policy
        ├── launchfile           ├── is_enabled
        ├── type                 ├── os_type      ← agl만
        ├── recovery             └── init_state   ← agl만
        ├── enabled
        └── max
```

---

## 4. CLI 구조

```
agl-adsw-config <command> [args] [options]
```

### 4.1 조회

| 명령 | 설명 |
|------|------|
| `agl-adsw-config show` | 전체 서비스 목록 트리 출력 |
| `agl-adsw-config list` | 서비스 목록 테이블 출력 |
| `agl-adsw-config get <name>` | 특정 서비스 상세 정보 |

### 4.2 서비스 관리

| 명령 | 설명 |
|------|------|
| `agl-adsw-config add <name> --image <img> [options]` | 서비스 추가 |
| `agl-adsw-config remove <name>` | 서비스 삭제 |
| `agl-adsw-config set <name> [options]` | 서비스 속성 변경 |
| `agl-adsw-config enable <name>` | `is_enabled=true` |
| `agl-adsw-config disable <name>` | `is_enabled=false` |
| `agl-adsw-config args <name> --add <arg>` | extra_args 항목 추가 |
| `agl-adsw-config args <name> --remove <arg>` | extra_args 항목 제거 |
| `agl-adsw-config args <name> --list` | extra_args 목록 확인 |

### 4.3 파일 관리

| 명령 | 설명 |
|------|------|
| `agl-adsw-config validate` | YAML 구조/값 검증 |
| `agl-adsw-config backup` | 타임스탬프 백업 생성 |
| `agl-adsw-config diff` | 마지막 백업과 현재 diff |

---

## 5. 서비스 필드 옵션

`add` / `set` 공통:

| 옵션 | 허용값 | 기본값 |
|------|--------|--------|
| `--image <img>` | 문자열 | - |
| `--type <type>` | `NORMAL \| ONESHOT` | `NORMAL` |
| `--recovery <policy>` | `R_DEFAULT \| ALWAYS \| ON_FAILURE \| ON_SUCCESS \| ON_NOT_RESPONDING \| MANUAL` | `R_DEFAULT` |
| `--enabled <bool>` | `true \| false` | `true` |
| `--os-type <os>` | `AGL \| ROS2 \| AUTOSAR` | `AGL` |
| `--init-state <state>` | `NOT_RUNNING \| RUNNING` | `NOT_RUNNING` |
| `--extra-args <arg>` | 문자열 (반복 가능) | - |

---

## 6. 공통 옵션

| 옵션 | 기본값 | 설명 |
|------|--------|------|
| `-f, --file <path>` | `./config/agl-service-list.yaml` | 대상 YAML 파일 |
| `--dry-run` | off | 미리보기만, 파일 수정 없음 |
| `--no-backup` | off | 백업 없이 즉시 저장 |
| `-o, --output <fmt>` | `table` | `table \| json \| yaml` |

---

## 7. 검증 규칙

| 규칙 | 내용 |
|------|------|
| service_type | `NORMAL \| ONESHOT` 만 허용 |
| recovery_policy | `R_DEFAULT \| ALWAYS \| ON_FAILURE \| ON_SUCCESS \| ON_NOT_RESPONDING \| MANUAL` |
| os_type | `AGL \| ROS2 \| AUTOSAR` 만 허용 |
| init_state | `NOT_RUNNING \| RUNNING` 만 허용 |
| name 중복 금지 | services 내 name 중복 불가 |

---

## 8. 소스 구조

```
tools/agl_adsw_config/
├── __main__.py      # 진입점 (python -m agl_adsw_config)
├── cli.py           # click 커맨드 정의
├── model.py         # pydantic 스키마 (ServiceEntry)
├── store.py         # YAML 로드/저장 (ruamel.yaml, 백업 포함)
├── validator.py     # 검증 로직
└── formatter.py     # rich 출력 (table, tree, diff)
```

---

## 9. 기술 스택

| 항목 | 선택 |
|------|------|
| 언어 | Python 3 |
| YAML 파서 | `ruamel.yaml` (주석 보존) |
| CLI 프레임워크 | `click` |
| 출력 | `rich` |
| 스키마 검증 | `pydantic` v2 |
| entry point | `agl-adsw-config` |

---

## 10. 구현 우선순위

| Phase | 내용 |
|-------|------|
| **Phase 1** | `show`, `list`, `get`, `validate` — 읽기 전용 |
| **Phase 2** | `add`, `remove`, `set`, `enable`, `disable` |
| **Phase 3** | `args` — extra_args CRUD |
| **Phase 4** | `backup`, `diff`, `--dry-run` |

---

*작성일: 2026-03-30*
*대상 프로젝트: `oss_multisw_framework/service/extension/agl/adsw_manager`*
