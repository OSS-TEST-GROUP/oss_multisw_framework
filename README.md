# oss_multisw_framework

OSS Multi-Middleware Framework (MMF) 빌드 및 실행 가이드입니다.  
Docker 관련 세부 내용은 [`docker/README.md`](./docker/README.md)를 참조하세요.

---

## Fetch the repository

```bash
git clone git@github.com:obigo-gov/oss_multisw_framework.git
```

---

## How to build the OSS Multi-Middleware Framework

> **전제**: 개발 컨테이너(`docker/Dockerfile`) 또는 동등한 빌드 환경이 갖춰져 있어야 합니다.  
> 개발 컨테이너 빌드 및 실행 방법은 [아래 섹션](#how-to-run-the-dev-container)을 참조하세요.

### x86-64 (유닛테스트 / 정적 분석 / 디버깅)

```bash
# Core + Extension (Debug, ROS2 기본)
./build.sh

# 유닛 테스트 포함 빌드 (Debug 강제)
./build.sh --build-unittest

# Release 빌드
./build.sh --release

# Extension 포함 빌드
./build.sh --build-extension --set-extension-platform=ROS2
./build.sh --build-extension --set-extension-platform=AGL
```

### aarch64 크로스빌드 (AGL, AADP 등)

```bash
# ROS2 Extension 크로스빌드
./build.sh --aarch64 --set-extension-platform=ROS2

# AGL Extension 크로스빌드
./build.sh --aarch64 --set-extension-platform=AGL
```

### 빌드 옵션 전체 목록

| 옵션 | 설명 |
|---|---|
| `--release` | Release 모드로 빌드 (기본: Debug) |
| `--build-unittest` | 유닛 테스트 포함 빌드 (Debug 강제) |
| `--build-extension` | Extension 프레임워크 빌드 활성화 |
| `--set-extension-platform=<PLATFORM>` | Extension 대상 플랫폼 지정 (`ROS2` \| `AGL` \| `AAutoSAR`) |
| `--aarch64` | aarch64 크로스빌드 대상으로 설정 |
| `--help` | 도움말 출력 |

빌드 산출물 위치:
```
output/x86_64/<Debug|Release>/core/bin         # Core 바이너리
output/x86_64/<Debug|Release>/extension/ros2/bin  # Extension - ROS2
output/x86_64/<Debug|Release>/extension/agl/bin   # Extension - AGL
output/aarch64/<Debug|Release>/...             # aarch64 산출물
```

---

## How to run the dev container

개발 컨테이너는 빌드 툴체인, 정적 분석 도구, Fast-DDS-Gen 등이 포함된 두꺼운 이미지입니다.  
소스를 호스트에서 마운트하여 컨테이너 내부에서 빌드하는 방식으로 사용합니다.

### 개발 컨테이너 이미지 빌드 (최초 1회, 10~20분 소요)

```bash
# 프로젝트 루트(oss_multisw_framework/)에서 실행
docker build -f docker/Dockerfile -t oss-mmf-dev:local .
```

### 개발 컨테이너 실행

```bash
docker run --privileged \
    --rm -it \
    -w /home/ossuser/workspace \
    -v ${PWD}:/home/ossuser/workspace \
    -v /tmp/.X11-unix:/tmp/.X11-unix \
    -v /run/user/1000:/run/user/1000 \
    --network=host --cap-add=NET_RAW --cap-add=NET_ADMIN \
    oss-mmf-dev:local
```

> 또는 프로젝트 루트의 [`docker_run.sh`](./docker_run.sh)를 참고하세요.  
> (레지스트리에서 이미지를 pull하여 사용하는 방식으로, 직접 빌드한 `oss-mmf-dev:local`로 대체 가능합니다.)

컨테이너 내부 빌드 산출물 경로:
```
/home/ossuser/workspace/output/<arch>/<Debug|Release>/core/bin
/home/ossuser/workspace/output/<arch>/<Debug|Release>/extension/ros2/bin
/home/ossuser/workspace/output/<arch>/<Debug|Release>/extension/agl/bin
```

---

## How to build the container images to release

빌드 산출물(`output/`)이 준비된 후 런타임 이미지를 생성합니다.

### 로컬 x86_64 런타임 이미지 (통합 테스트용)

```bash
# Debug 빌드 산출물로 로컬 이미지 생성
./docker/build_local.sh

# Release 빌드 산출물로 로컬 이미지 생성
BUILD_TYPE=Release ./docker/build_local.sh

# core 또는 ext-ros2 중 하나만 빌드
./docker/build_local.sh --core-only
./docker/build_local.sh --ext-ros2-only
```

빌드 결과: `oss-mmf-core:local`, `oss-mmf-ext-ros2:local`

### 멀티아키 릴리즈 이미지 (x86_64 + aarch64, CI 배포용)

```bash
# x86_64 + aarch64 모두 빌드하여 registry.obigo.com에 push
BUILD_TYPE=Release ./docker/build_multiarch.sh
```

Push 결과: `registry.obigo.com/oss/oss-mmf-{core,ext-ros2,ext-agl}:latest`

### docker buildx 직접 사용 (개별 이미지 빌드)

```bash
# buildx 빌더 생성 (최초 1회)
docker buildx create --name mmf-buildx --driver docker-container --use || true

# aarch64 core 이미지
docker buildx build \
    --no-cache --platform="linux/arm64" \
    --build-arg BUILD_TYPE=Release \
    --build-arg TARGETARCH=aarch64 \
    -f docker/Dockerfile.rel \
    --target runtime-mmf-core \
    --load \
    -t registry.obigo.com/oss/oss-mmf-core:latest \
    .

# aarch64 ROS2 Extension 이미지
docker buildx build \
    --no-cache --platform="linux/arm64" \
    --build-arg BUILD_TYPE=Release \
    --build-arg TARGETARCH=aarch64 \
    -f docker/Dockerfile.rel \
    --target runtime-mmf-ext-ros2 \
    --load \
    -t registry.obigo.com/oss/oss-mmf-ext-ros2:latest \
    .

# aarch64 AGL Extension 이미지
docker buildx build \
    --no-cache --platform="linux/arm64" \
    --build-arg BUILD_TYPE=Release \
    --build-arg TARGETARCH=aarch64 \
    -f docker/Dockerfile.rel \
    --target runtime-mmf-ext-agl \
    --load \
    -t registry.obigo.com/oss/oss-mmf-ext-agl:latest \
    .
```

> **`TARGETARCH` 주의**: `Dockerfile.rel`은 Docker 표준(`amd64`/`arm64`) 대신 커널 아키텍처명(`x86_64`/`aarch64`)을 사용합니다. 반드시 `--build-arg TARGETARCH=aarch64`처럼 명시하세요.

---

## How to run with Docker Compose

### 로컬 통합 검증 환경 (x86_64, 로컬 이미지)

```bash
# 프로젝트 루트(oss_multisw_framework/)에서 실행
docker compose -f docker/compose/docker-compose-integration.yaml up -d

# 로그 확인
docker compose -f docker/compose/docker-compose-integration.yaml logs -f

# 종료
docker compose -f docker/compose/docker-compose-integration.yaml down
```

### AADP 배포 환경 (aarch64, 레지스트리 이미지, NVIDIA GPU 필요)

```bash
# 프로젝트 루트(oss_multisw_framework/)에서 실행
docker compose -f docker/compose/docker-compose-aadp.yaml up -d
```

### AGL 환경 (aarch64, 레지스트리 이미지)

```bash
# 프로젝트 루트(oss_multisw_framework/)에서 실행
docker compose -f docker/compose/docker-compose-agl.yaml up -d
```

---

## How to use DDS IDL files

### DDS IDL 파일 추가

커스텀 IDL 파일을 아래 경로에 추가하면 빌드 시 자동으로 처리됩니다.

```
oss_multisw_framework/idl/dds/
```

### IDL 구조

| 경로 | 용도 |
|---|---|
| `idl/dds/` | DDS 공통 IDL 파일 (모듈별 서브디렉터리: `diagnostic/`, `lifecycle/`, `ota_mgr/` 등) |
| `idl/rosbridge/` | ROS 2 rosbridge용 IDL→msg 변환 스크립트 및 설정 (`idl_to_msg.py`) |

### 빌드 산출물

DDS 빌드 생성 소스 파일은 아래 경로에 생성됩니다.

```
output/<arch>/<Debug|Release>/generated/idl/
```

### CMakeLists.txt에서 사용하는 방법

```cmake
# FastDDS 라이브러리 링크
link_libraries(fastcdr fastdds)

# 생성된 IDL 파일 경로 설정
set(GENERATED_DIR "${CMAKE_INSTALL_PREFIX}/generated/idl")

# 생성된 DDS 소스 파일 목록 (예시)
set(GENERATED_DDS_FILES
    ${GENERATED_DIR}/DataTypeTestPubSubTypes.cxx
    ${GENERATED_DIR}/DataTypeTestTypeObjectSupport.cxx
    ${GENERATED_DIR}/DataTypeTestSimplePubSubTypes.cxx
)
```

---

## About unit tests

각 서비스별로 gtest/gmock 기반 유닛 테스트를 작성할 수 있습니다.  
서비스의 `CMakeLists.txt`에 아래를 추가하세요.

```cmake
link_libraries(gtest gmock)
```

유닛 테스트 빌드 및 실행:

```bash
# 유닛 테스트 포함 빌드
./build.sh --build-unittest

# 빌드된 테스트 바이너리 실행 (예)
./output/x86_64/Debug/tests/<service_name>_test
```

---

## Related documentation

가이드 문서는 [`docs/guide/`](./docs/guide) 에 모아둡니다. (Docker 이미지 빌드/실행 상세는 위 각 섹션 및 `docker/README.md` 참조)

### 가이드 문서 (`docs/guide/`)
| 문서 | 설명 |
|---|---|
| [`build-install-run-guide.md`](./docs/guide/build-install-run-guide.md) | MMF 빌드·설치·실행 (x86_64 / arm64) |
| [`board-autostart-guide.md`](./docs/guide/board-autostart-guide.md) | 보드 재부팅 자동시작 — core / ext-ros2 / ext-agl |
| [`ext-ros2-adsw-operation-guide.md`](./docs/guide/ext-ros2-adsw-operation-guide.md) | ext-ros2 adsw_manager 운영 — autoware 관리·기동·트러블슈팅 |
| [`adsw-manager-autosar-lifecycle-integration-guide.md`](./docs/guide/adsw-manager-autosar-lifecycle-integration-guide.md) | Adaptive AUTOSAR `adsw_manager` ↔ `lifecycle_manager` 통합 |
| [`ota-client-autosar-integration-guide.md`](./docs/guide/ota-client-autosar-integration-guide.md) | Adaptive AUTOSAR OTA Client 구현 (OTA Manager 연동) |

### 설계 · 검토 문서 (참고)
| 문서 | 설명 |
|---|---|
| [`docs/sils/sils-virtual-ecu-architecture-v3.md`](./docs/sils/sils-virtual-ecu-architecture-v3.md) | SiLS 가상 ECU(vECU) 아키텍처 (v3) |
| [`docs/diagnostics-integration-expansion-review.md`](./docs/diagnostics-integration-expansion-review.md) | Diagnostics 연동 확장 및 Monitor UI 분리 검토 |
