# adsw_manager 테스트 가이드

## 목차

1. [테스트 구조 개요](#1-테스트-구조-개요)
2. [사전 준비](#2-사전-준비)
3. [단위 테스트 (Unit Test)](#3-단위-테스트-unit-test)
4. [Docker 통합 테스트 (Integration Test)](#4-docker-통합-테스트-integration-test)
5. [테스트 케이스 목록](#5-테스트-케이스-목록)
6. [트러블슈팅](#6-트러블슈팅)

---

## 1. 테스트 구조 개요

```
unittest/
├── README.md                                      ← 이 파일
│
├── [단위 테스트 - mock 기반, Docker 불필요]
├── ContainerTest.cpp                              ← Container 클래스 단위 테스트
├── ContinerControllerTest.cpp                     ← ContainerController 레지스트리/명령 테스트
├── ContainerHealthMonitorTest.cpp                 ← ContainerHealthMonitor 상태 추적 테스트
├── ServiceTest.cpp                                ← Service 클래스 단위 테스트
├── ServiceMonitorTest.cpp                         ← ServiceMonitor 단위 테스트
├── ServiceRegistryTest.cpp                        ← ServiceRegistry 단위 테스트
│   └── ... (기타 DDS endpoint 테스트)
│
├── [Docker 통합 테스트 - 실제 Docker 필요]
├── ContainerControllerDockerIntegrationTest.cpp   ← 실제 Docker 컨테이너 제어 테스트
│
├── docker/
│   ├── Dockerfile                                 ← 테스트용 샘플 Docker 이미지
│   └── docker-compose.test.yaml                   ← docker-compose 테스트 정의
│
├── config/
│   ├── saved-service-list.yaml                    ← 기본 서비스 목록 설정
│   └── docker-integration-test-service-list.yaml  ← 통합 테스트용 서비스 목록
│
├── mock/
│   ├── MockADSWManager.hpp / .cpp                 ← ADSWManager mock
│   ├── MockContainerController.hpp                ← ContainerController mock
│   ├── MockServiceMonitor.hpp                     ← ServiceMonitor mock
│   └── ... (기타 mock)
│
└── test/
    └── run_docker_integration_test.sh             ← Docker 통합 테스트 실행 스크립트
```

---

## 2. 사전 준비

### 2.1 빌드 환경

| 항목 | 버전 |
|---|---|
| CMake | 3.5 이상 |
| GCC / G++ | C++17 지원 |
| Google Test / Mock | 설치 필요 |
| FastDDS | 설치 필요 |
| yaml-cpp | 설치 필요 |

### 2.2 빌드 방법

프로젝트 최상위 디렉토리에서:

```bash
# BUILD_UNITTEST 옵션 활성화
cmake -B build -DBUILD_UNITTEST=ON
cmake --build build
cmake --install build
```

설치 후 테스트 바이너리 위치: `<install_prefix>/bin/unittest/adsw_manager/`

### 2.3 Docker 통합 테스트 추가 준비

```bash
# Docker 설치 확인
docker --version
docker info

# 테스트 이미지 빌드 (최초 1회)
docker build -t adsw-test-app:latest \
    <install_prefix>/bin/unittest/adsw_manager/docker/
```

---

## 3. 단위 테스트 (Unit Test)

Docker, DDS, 외부 프로세스 없이 실행됩니다.

### 3.1 전체 단위 테스트 실행

```bash
cd <install_prefix>/bin/unittest/adsw_manager

# 전체 실행
for bin in ADSW*Test; do
    echo "=== $bin ==="
    ./$bin
done
```

### 3.2 개별 테스트 실행

```bash
# Container 클래스 테스트
./ADSWContainerControllerTest --gtest_filter="ContainerTest*"

# ContainerController 레지스트리/명령 테스트
./ADSWContainerControllerTest --gtest_filter="ContainerControllerTest*"

# ContainerHealthMonitor 상태 추적 테스트
./ADSWContainerHealthMonitorTest

# ServiceMonitor 테스트
./ADSWServiceMonitorTest

# ServiceRegistry 테스트
./ADSWServiceRegistryTest
```

### 3.3 ContainerHealthMonitor 단위 테스트 상세

`ContainerHealthMonitorTest` 는 `TestableContainerHealthMonitor` 서브클래스를 사용하여
실제 Docker 없이 상태 전환 로직을 검증합니다.

```
TestableContainerHealthMonitor
 ├── prev_state_  (protected)  ← 이전 컨테이너 상태 맵
 ├── NotifyContainerState()    ← override → notifications 벡터에 기록
 ├── InjectStateChange()       ← 외부에서 상태 변경 주입
 └── SetPrevState()            ← 초기 상태 설정
```

```bash
./ADSWContainerHealthMonitorTest --gtest_color=yes --gtest_verbose=yes
```

---

## 4. Docker 통합 테스트 (Integration Test)

### 4.1 테스트 이미지 구조

`docker/Dockerfile` — Alpine 기반 최소 이미지:

```dockerfile
FROM alpine:3.18
# 종료 시그널을 받으면 정상 종료하는 서비스 스크립트
ENTRYPOINT ["/usr/local/bin/adsw_sample_service.sh"]
```

`docker/docker-compose.test.yaml` — docker-compose 타입 테스트:

```yaml
services:
  adsw-test-compose-svc:
    image: adsw-test-app:latest
    container_name: adsw-test-compose
```

### 4.2 실행 방법

#### 방법 A: 자동 스크립트 (권장)

```bash
cd <install_prefix>/bin/unittest/adsw_manager

# 이미지 빌드 + 테스트 실행 + 정리 자동화
./test/run_docker_integration_test.sh ./ADSWContainerDockerIntegrationTest
```

스크립트 동작 순서:
1. Docker 데몬 가용 여부 확인
2. `adsw-test-app:latest` 이미지 빌드
3. 이전 테스트 컨테이너 정리
4. 통합 테스트 바이너리 실행
5. 사후 컨테이너 정리

#### 방법 B: 수동 실행

```bash
# 1. 이미지 빌드
docker build -t adsw-test-app:latest ./docker/

# 2. 테스트 실행 (unittest 디렉토리에서 실행해야 상대 경로가 맞음)
cd <install_prefix>/bin/unittest/adsw_manager
./ADSWContainerDockerIntegrationTest --gtest_color=yes

# 3. 잔여 컨테이너 정리
docker rm -f adsw-test-docker adsw-test-compose adsw-test-hm 2>/dev/null || true
docker compose -f ./docker/docker-compose.test.yaml down 2>/dev/null || true
```

#### 방법 C: 특정 테스트만 실행

```bash
# Docker run 타입 테스트만
./ADSWContainerDockerIntegrationTest \
    --gtest_filter="ContainerDockerTest*"

# Docker Compose 타입 테스트만
./ADSWContainerDockerIntegrationTest \
    --gtest_filter="ContainerDockerComposeTest*"

# ContainerHealthMonitor 감지 테스트만
./ADSWContainerDockerIntegrationTest \
    --gtest_filter="ContainerHealthMonitorDockerTest*"
```

### 4.3 Docker 없는 환경에서의 동작

Docker 데몬이 없거나 이미지가 없는 경우 `GTEST_SKIP()` 으로 자동 건너뜁니다:

```
[ SKIPPED ] ContainerDockerTest.StartContainer_Success_IT001
>> Docker를 사용할 수 없습니다. 테스트를 건너뜁니다.
```

---

## 5. 테스트 케이스 목록

### 5.1 Container 단위 테스트 (`ContainerTest`)

| TC | 테스트명 | 검증 내용 |
|---|---|---|
| TC001 | `Constructor_InitialValues` | 생성자 인자 저장 확인 |
| TC002 | `GetPodName` | pod name = container name |
| TC003~010 | 다양한 값 테스트 | 빈 값, 긴 이름, 특수문자, 레지스트리 URL 등 |
| TC011 | `PodNameMatchesContainerName` | GetPodName() == GetName() |
| TC013 | `DefaultContainerType_IsDocker` | 5번째 인자 생략 시 DOCKER |
| TC014 | `SetContainerType_DockerCompose` | DOCKER_COMPOSE 타입 설정 |
| TC015 | `SetContainerType_K3S` | K3S 타입 설정 |
| TC016 | `SetContainerType_Unknown` | UNKNOWN 타입 설정 |
| TC017 | `SetContainerType_Mutate` | SetContainerType() 변경 동작 |
| TC018 | `AllContainerTypes_Distinct` | 4가지 타입 값이 모두 다름 |

### 5.2 ContainerController 단위 테스트 (`ContainerControllerTest`)

| TC | 테스트명 | 검증 내용 |
|---|---|---|
| — | `ConstructorDestructor` | 생성/소멸 크래시 없음 |
| — | `InitializeAndFinalize` | 반복 호출 안전성 |
| — | `ContainerSupported_DefaultFalse` | 초기 false |
| — | `ContainerSupported_TrueAfterAdd` | 컨테이너 추가 후 true |
| — | `AddContainer_NullContainer` | null → false |
| — | `AddContainer_ValidContainer` | 정상 추가 → true |
| — | `AddContainer_DuplicateContainer` | 중복 → false |
| — | `GetContainer_AfterAdding` | 추가 후 조회 → not null |
| — | `GetContainer_NotFound` | 미존재 → nullptr |
| — | `GetAllContainers_Empty` | 초기 빈 벡터 |
| — | `GetAllContainers_AfterAdd` | 추가 후 크기 확인 |
| — | `BaseCommand_DockerContainer` | `docker exec <name>` 포함 |
| — | `BaseCommand_K3sContainer` | `kubectl exec -n <ns>` 포함 |
| — | `BaseRos2Command_DockerContainer` | `source /opt/ros/humble/setup.bash` 포함 |
| — | `GetContainerStatusStr_NotRegistered` | 미등록 → `"unknown"` |
| — | `GetContainerExitCode_NotRegistered` | 미등록 → `-1` |
| — | `IsContainerRunning_NotRegistered` | 미등록 → `false` |
| — | `StartContainer_NotRegistered` | 미등록 → `false` |
| — | `StopContainer_NotRegistered` | 미등록 → `false` |

### 5.3 ContainerHealthMonitor 단위 테스트 (`ContainerHealthMonitorTest`)

| TC | 테스트명 | 검증 내용 |
|---|---|---|
| TC001 | `ConstructorDestructor` | 생성/소멸 정상 |
| TC002 | `StopWithoutStart` | Start 없이 Stop 안전 |
| TC003 | `PrevState_InitiallyEmpty` | prev_state_ 초기 비어있음 |
| TC004 | `SetPrevState_CanRetrieve` | 상태 저장/조회 |
| **TC005** | `StateChange_StoppedToRunning` | **중지→실행: 알림 발생, is_running=true** |
| **TC006** | `StateChange_RunningToStopped` | **실행→중지: 알림 발생, exit_code 포함** |
| TC007 | `NoStateChange_NoNotification` | 상태 동일: 알림 없음 |
| **TC008** | `MultipleContainers_Independent` | **여러 컨테이너 독립 추적** |
| TC009 | `PrevStateUpdatedAfterChange` | 변경 후 prev_state_ 갱신 |
| **TC010** | `SequentialStateChanges` | **연속 3회 전환 → 알림 3회** |
| TC011 | `ExitCode_Zero_NormalExit` | exit_code=0 정상 종료 |
| TC012 | `ExitCode_Nonzero_ErrorExit` | exit_code=137 (SIGKILL) |
| TC013 | `StartStop_ThreadSafe` | 스레드 시작/종료 안전성 |
| TC014 | `DoubleStop_Safe` | 이중 Stop 안전성 |

### 5.4 Docker 통합 테스트 (`ContainerControllerDockerIntegrationTest`)

#### ContainerDockerTest (docker run 타입)

| IT | 테스트명 | 검증 내용 |
|---|---|---|
| IT001 | `StartContainer_Success` | 컨테이너 시작 → IsContainerRunning=true |
| IT002 | `GetContainerStatusStr_Running` | 상태 문자열 = `"running"` |
| IT003 | `GetContainerExitCode_WhileRunning` | 실행 중 exit code = 0 |
| IT004 | `StopContainer_Success` | 컨테이너 중지 → IsContainerRunning=false |
| IT005 | `GetContainerStatusStr_AfterStop` | 중지 후 = `"unknown"` 또는 `"exited"` |
| IT006 | `RestartContainer_StaysRunning` | 재시작 후 IsContainerRunning=true |
| IT007 | `StartContainer_AlreadyRunning_Idempotent` | 실행 중 재시작 → 성공 (멱등) |
| IT008 | `ContainerStatus_BeforeStart` | 시작 전 = `"unknown"`, Running=false |

#### ContainerDockerComposeTest (docker-compose 타입)

| IT | 테스트명 | 검증 내용 |
|---|---|---|
| IT010 | `StartComposeContainer_Success` | compose up → IsContainerRunning=true |
| IT011 | `GetContainerStatusStr_Running` | 상태 = `"running"` |
| IT012 | `StopComposeContainer_Success` | compose down → IsContainerRunning=false |
| IT013 | `RestartComposeContainer` | restart → IsContainerRunning=true |

#### ContainerHealthMonitorDockerTest (실제 상태 변경 감지)

| IT | 테스트명 | 검증 내용 |
|---|---|---|
| **IT020** | `DetectsContainerStart` | **컨테이너 시작 시 중지→실행 변경 감지** |
| **IT021** | `DetectsContainerStop` | **컨테이너 종료 시 실행→중지 변경 감지** |
| IT022 | `NoNotification_WhenStateSame` | 상태 동일 시 알림 없음 |

---

## 6. 트러블슈팅

### Q. `adsw-test-app:latest` 이미지가 없다는 오류

```
[ SKIPPED ] ContainerDockerTest - 이미지가 없습니다.
```

**해결:**

```bash
docker build -t adsw-test-app:latest \
    <install_prefix>/bin/unittest/adsw_manager/docker/
```

### Q. Docker 데몬 연결 실패

```
[ SKIPPED ] - Docker를 사용할 수 없습니다.
```

**해결:**

```bash
# Docker 데몬 시작
sudo systemctl start docker

# 현재 사용자에게 권한 부여 (재로그인 필요)
sudo usermod -aG docker $USER
```

### Q. 이전 테스트 컨테이너가 남아 있어 테스트 실패

```
Error: Conflict. The container name "/adsw-test-docker" is already in use.
```

**해결:**

```bash
docker rm -f adsw-test-docker adsw-test-compose adsw-test-hm
docker compose -f ./docker/docker-compose.test.yaml down
```

### Q. ContainerHealthMonitorTest 빌드 오류 (ADSWManager 심볼 미해결)

`mock/MockADSWManager.cpp` 가 CMakeLists.txt 타겟에 포함되어 있는지 확인:

```cmake
add_executable( ADSWContainerHealthMonitorTest ...
                mock/MockADSWManager.cpp   # ← 이 줄이 있어야 함
)
```

### Q. docker-compose 테스트에서 컨테이너 이름 불일치

docker-compose는 기본적으로 `<project>-<service>-1` 형식으로 컨테이너를 생성합니다.
`docker-compose.test.yaml` 에서 `container_name: adsw-test-compose` 를 명시적으로
지정하여 이름을 고정합니다.

### Q. 테스트 타임아웃 (컨테이너 기동 지연)

컨테이너 기동에 시간이 걸리는 경우 `WaitMs()` 값을 조정하거나, 다음과 같이 재시도 로직을 사용하세요:

```bash
# 특정 테스트만 타임아웃 늘려 실행
./ADSWContainerDockerIntegrationTest \
    --gtest_filter="ContainerDockerComposeTest*" \
    --gtest_repeat=1
```
