# Docker 가이드

> 이 문서는 **Dockerfile 구조**를 설명합니다. 소스 빌드 → 이미지 조립 → 보드 설치 →
> 실행까지의 **단계별 절차**는 [docs/build-install-run-guide.md](../docs/build-install-run-guide.md),
> 재부팅 자동시작 구성은 [docs/board-autostart-guide.md](../docs/board-autostart-guide.md) 를 보세요.

`oss_multisw_framework`는 빌드 라이프사이클의 단계와 사용 시점에 따라 **3개의 Dockerfile**을 분리해 관리합니다. 중복이 아니라 역할이 분리되어 있으며, 일반적으로 다음 파이프라인을 따라 사용됩니다.

```
[개발자 머신/IDE]
   └─ Dockerfile         ── 컨테이너에 들어가서 코딩/빌드/디버깅
                            (산출물 추출 안 함)

[CI 빌드 단계]
   └─ Dockerfile.multi   ── x86_64 + aarch64 크로스빌드
                            output/<arch>/Release/{core, extension/ros2, extension/agl}/
                              │
                              ▼
[CI 패키징 단계]
   └─ Dockerfile.rel     ── 위 산출물을 슬림 런타임 이미지에 COPY
                            (core / ext-ros2 / ext-agl)
                            ext-ros2 stage는 ros-mmf-base 중간 stage에서
                            rosbridge_server + IDL→msg 변환을 함께 처리
```

> **이전 `Dockerfile.ros`는 `Dockerfile.rel`의 `ros-mmf-base` stage로 흡수되어 삭제되었습니다.**  
> rosbridge 단독 이미지가 필요했던 OTA 통합 검증 스택은 이제 `Dockerfile.rel --target runtime-mmf-ext-ros2`로 만든 같은 이미지를 사용합니다.

---

## 한눈에 비교

| 파일 | 역할 | 베이스 | 사용자 | ENTRYPOINT/CMD | 이미지 성격 |
|---|---|---|---|---|---|
| [`Dockerfile`](./Dockerfile) | 개발자 빌드 환경 (devcontainer) | `ubuntu:22.04` | `ossuser` (1000) | `/bin/bash` | 두꺼움 (toolchain + Fast-DDS-Gen) |
| [`Dockerfile.multi`](./Dockerfile.multi) | CI/배포 빌드용 멀티아키 빌더 (x86_64 + aarch64 크로스) | `ubuntu:22.04` (multi-stage) | `root` | 없음 (산출물을 `/tmp/oss-<arch>`에 떨굼) | 두꺼움, 산출물 추출용 |
| [`Dockerfile.rel`](./Dockerfile.rel) | 런타임(릴리즈) 이미지. 빌드 산출물을 슬림 이미지에 얹음. `ros-mmf-base` 중간 stage가 ROS 2 + rosbridge + IDL→msg를 같이 설치 | `ubuntu:22.04` (런타임 lib만) | `ossuser` (1000) | `/oss/bin/lifecycle_mgr` 또는 `/oss/entrypoint.sh` | 가벼움 (실행 전용) |

---

## 사전 준비 (Prerequisites)

Docker 이미지를 빌드하기 전에 다음을 확인하세요.

### 필수 소프트웨어

| 항목 | 최소 버전 | 확인 명령 |
|---|---|---|
| Docker Engine | 24.0 이상 | `docker --version` |
| Docker Buildx | 0.11 이상 | `docker buildx version` |
| Docker Compose | v2.20 이상 | `docker compose version` |

> **Linux 전용 주의**: `network_mode: host`를 사용하므로 Docker Desktop(macOS/Windows)에서는 통합 스택이 동작하지 않습니다. **Linux 호스트**를 사용하세요.

### BuildKit 활성화 확인

```bash
# BuildKit이 활성화되어 있어야 --mount=type=cache 등의 기능을 사용할 수 있습니다.
export DOCKER_BUILDKIT=1
# 또는 daemon.json에 설정되어 있는지 확인
docker info | grep -i buildkit
```

### 멀티아키 빌드를 위한 추가 설정 (Dockerfile.multi / build_multiarch.sh 사용 시)

```bash
# QEMU 에뮬레이터 등록 (aarch64 크로스빌드용)
docker run --privileged --rm tonistiigi/binfmt --install all

# buildx builder 생성 (build_multiarch.sh가 자동으로 생성하지만, 수동으로도 가능)
docker buildx create --name mmf-buildx --driver docker-container --use
docker buildx inspect --bootstrap
```

---

## 빌드 순서 (전체 플로우)

처음 도커 이미지를 만드는 경우, 아래 순서대로 진행하세요.

```
Step 1. (개발용) Dockerfile로 개발 환경 이미지 생성 → 내부에서 소스 빌드
    또는
Step 1. (CI용) Dockerfile.multi로 크로스빌드 산출물 생성 → output/ 디렉터리에 추출

Step 2. Dockerfile.rel로 런타임 이미지 생성 (Step 1의 산출물 필요)

Step 3. Docker Compose로 통합 스택 실행
```

---

## 1. `Dockerfile` — 개발자 빌드 환경

- **목적**: 사람이 컨테이너에 들어가서 코드를 빌드/디버깅하는 dev container
- **특징**
  - `--mount=type=cache`로 apt 캐시 재활용 (반복 빌드 속도 향상)
  - 개발 도구 풀세트: `gdb`, `clang-tidy`, `clang-format`, `iwyu`, `cppcheck`, `pre-commit`, `lcov`, `gcovr` 등
  - `Fast-DDS-Gen`을 GitHub에서 클론 후 Gradle로 빌드·설치
  - aarch64 크로스 컴파일러(`gcc-aarch64-linux-gnu`, `g++-aarch64-linux-gnu`) 포함 (sysroot는 없음)
  - 비루트 `ossuser` (uid/gid 1000)로 실행, 워크스페이스: `/home/ossuser/workspace`
- **ENTRYPOINT**: 없음, `CMD ["/bin/bash"]` → 들어가서 작업

### 이미지 빌드

```bash
# 프로젝트 루트(oss_multisw_framework/)에서 실행
docker build \
  -f docker/Dockerfile \
  -t oss-mmf-dev:local \
  .
```

> **첫 빌드는 Fast-DDS-Gen Gradle 빌드 포함으로 10~20분 소요될 수 있습니다.**

### 컨테이너 실행 및 소스 빌드

```bash
# 소스 디렉터리를 컨테이너 워크스페이스에 마운트하여 실행
docker run -it --rm \
  -v "$PWD:/home/ossuser/workspace" \
  oss-mmf-dev:local

# 컨테이너 내부에서 소스 빌드 (예: x86_64 Debug)
./docker/scripts/build.sh

# Release 빌드
./docker/scripts/build.sh --release

# ROS2 Extension 포함 빌드
./docker/scripts/build.sh --set-extension-platform=ROS2

# AGL Extension 빌드
./docker/scripts/build.sh --set-extension-platform=AGL

# 유닛 테스트 포함 빌드
./docker/scripts/build.sh --build-unittest
```

빌드가 완료되면 `output/x86_64/Debug/` (또는 `Release/`) 디렉터리에 산출물이 생성됩니다.

---

## 2. `Dockerfile.multi` — CI/배포 빌드 (멀티아키)

- **목적**: CI에서 x86_64와 aarch64용 산출물을 동일 컨테이너에서 크로스 빌드해 `/tmp/oss-<arch>/`로 추출
- **멀티 스테이지 구성**
  1. `base-setup` — 공통 환경 변수 / 타임존 설정
  2. `builder-x86_64` — 빌드 툴체인 + Fast-DDS x86_64 빌드
  3. `builder-aarch64` — `debootstrap`으로 aarch64 sysroot 생성 + `qemu-user-static` + 크로스 빌드
  4. `build-stage` — `COPY . .` 후 cmake/make로 실제 프로젝트 빌드

### 주요 빌드 인자 (ARG)

| ARG | 기본값 | 설명 |
|---|---|---|
| `TARGET_ARCH` | `x86_64` | 빌드 대상 아키텍처 (`x86_64` \| `aarch64`) |
| `BUILD_EXTENSION` | `FALSE` | Extension 빌드 여부 (`TRUE` \| `FALSE`) |
| `EXTENSION_PLATFORM` | `ROS2` | Extension 플랫폼 (`ROS2` \| `AGL`) |
| `BUILD_TYPE` | `Release` | 빌드 타입 (`Release` \| `Debug`) |
| `CROSSCOMPILING` | `FALSE` | 크로스컴파일 여부 (`TRUE` \| `FALSE`) |
| `BUILD_UNITTEST` | `FALSE` | 유닛 테스트 빌드 여부 (`TRUE` \| `FALSE`) |

### 빌드 방법

#### 방법 A: 래퍼 스크립트 사용 (권장)

```bash
# 프로젝트 루트(oss_multisw_framework/)에서 실행
# 기본값: Release 빌드, x86_64 + aarch64 모두 빌드, registry.obigo.com에 push

BUILD_TYPE=Release ./docker/build_multiarch.sh
```

이 스크립트는 다음을 자동으로 수행합니다:
- `mmf-buildx` buildx 빌더 생성/재사용
- x86_64(linux/amd64)와 aarch64(linux/arm64) 각각 빌드
- `core`, `ext-ros2`, `ext-agl` 총 3개 타깃 × 2 아키텍처 = 6개 이미지 빌드
- `registry.obigo.com/oss/oss-mmf-<target>:latest-amd64/arm64` 각각 push
- 멀티아키 매니페스트 `:latest` 조립

#### 방법 B: docker buildx 직접 사용

```bash
# x86_64 코어만 빌드하는 예시
docker buildx build \
  --platform=linux/amd64 \
  --build-arg TARGET_ARCH=x86_64 \
  --build-arg BUILD_TYPE=Release \
  --build-arg BUILD_EXTENSION=FALSE \
  -f docker/Dockerfile.multi \
  --target build-stage \
  --load \
  -t oss-mmf-builder:x86_64 \
  .

# aarch64 크로스빌드 예시
docker buildx build \
  --platform=linux/amd64 \
  --build-arg TARGET_ARCH=aarch64 \
  --build-arg BUILD_TYPE=Release \
  --build-arg BUILD_EXTENSION=TRUE \
  --build-arg EXTENSION_PLATFORM=ROS2 \
  --build-arg CROSSCOMPILING=TRUE \
  --build-arg TOOLCHAIN_FILE=toolchain-ros2-aarch64.cmake \
  -f docker/Dockerfile.multi \
  --target build-stage \
  --load \
  -t oss-mmf-builder:aarch64 \
  .
```

### 산출물 추출

`Dockerfile.multi` 빌드는 이미지 내부의 `/tmp/oss-<arch>/`에 산출물을 생성합니다.  
`Dockerfile.rel`이 이를 `COPY`하기 위해 호스트의 `output/<arch>/` 디렉터리로 추출해야 합니다.

```bash
# 컨테이너를 생성하고 산출물 복사 후 삭제
CONTAINER_ID=$(docker create oss-mmf-builder:x86_64)
docker cp ${CONTAINER_ID}:/tmp/oss-x86_64/. ./output/x86_64/
docker rm ${CONTAINER_ID}
```

> **최종 산출물 위치**: `output/<arch>/<BUILD_TYPE>/{core, extension/ros2, extension/agl}/` → `Dockerfile.rel`이 이 경로에서 COPY합니다.

---

## 3. `Dockerfile.rel` — 릴리즈/런타임 이미지

- **목적**: `Dockerfile.multi` 또는 호스트 빌드가 만든 `output/<arch>/...`을 받아서 **실행만 가능한 슬림 이미지** 생성
- **특징**
  - 빌드 툴체인 없음 → 런타임 .so만 (`libgrpc++1`, `libprotobuf23`, `libtinyxml2-9`, `libssl3`, `libyaml-cpp0.7`, …)
  - 비루트 `ossuser` (uid 1000)로 실행
  - FastDDS 프로파일 / `FASTDDS_LOG_LEVEL=Error` 사전 설정

> **⚠️ 주의**: `Dockerfile.rel`은 직접 빌드를 하지 않습니다.  
> `core` / `ext-agl` / `ext-ros2` 단계는 `output/<arch>/${BUILD_TYPE}/...`의 산출물이 **빌드 컨텍스트(프로젝트 루트)에 이미 존재**해야 `COPY`가 성공합니다.  
> **이미지를 빌드하기 전에 반드시 소스를 먼저 빌드하세요.**

### 타깃(Stage) 목록

| 타깃 (stage) | 아키 | 산출물 위치 | ENTRYPOINT |
|---|---|---|---|
| `runtime-mmf-core-x86_64` | x86_64 | `output/x86_64/.../core/bin` | `/oss/entrypoint.sh` (core_entrypoint.sh) |
| `runtime-mmf-core-aarch64` | aarch64 | `output/aarch64/.../core/bin` | `/oss/entrypoint.sh` (core_entrypoint.sh) |
| **`runtime-mmf-core`** | 자동 (`${TARGETARCH}`) | — | (위와 동일) |
| `ros-mmf-base` *(공유 base)* | 자동 | ROS 2 Humble + rosbridge_server + tier4_autoware_msgs + mmf_interfaces | — (실행 안 함) |
| `runtime-mmf-ext-ros2-x86_64` | x86_64 | `.../extension/ros2/bin` + `ros-mmf-base` | `/oss/entrypoint.sh` (ros2_entrypoint.sh) |
| `runtime-mmf-ext-ros2-aarch64` | aarch64 | 동일 (aarch64) | 동일 |
| **`runtime-mmf-ext-ros2`** | 자동 selector | — | — |
| `runtime-mmf-ext-agl-x86_64` | x86_64 | `.../extension/agl/bin` | `/oss/entrypoint.sh` (agl_entrypoint.sh) |
| `runtime-mmf-ext-agl-aarch64` | aarch64 | 동일 | 동일 |
| **`runtime-mmf-ext-agl`** | 자동 selector | — | — |

### `ros-mmf-base` 단계가 하는 일

`runtime-mmf-ext-ros2-*`의 공통 베이스 이미지로, 다음을 수행합니다.

- ROS 2 apt 소스 추가 + `ros-humble-ros-core`, `ros-humble-rosbridge-server`, `ros-humble-rosidl-default-generators`, `python3-colcon-common-extensions`, autoware/geographic msgs 설치
- `tier4_autoware_msgs v0.58.0`을 소스에서 빌드 → `/opt/ros/humble/share/tier4_autoware`
- `idl/`을 복사 → `idl/rosbridge/idl_to_msg.py`로 DDS IDL을 ROS .msg로 변환 → colcon으로 모듈별 ament 패키지 빌드 → `/opt/ros/humble/share/mmf_interfaces` 밑에 5개 서브패키지 설치
- `ros2_entrypoint.sh`가 두 overlay(`mmf_interfaces`, `tier4_autoware`)를 source

### 빌드 방법

#### 방법 A: 래퍼 스크립트 사용 (로컬 x86_64, 권장)

```bash
# 프로젝트 루트(oss_multisw_framework/)에서 실행
# 전제: output/x86_64/Debug/{core,extension/ros2}/{bin,lib}이 존재해야 함

./docker/build_local.sh

# Release 빌드
BUILD_TYPE=Release ./docker/build_local.sh

# core 이미지만 빌드
./docker/build_local.sh --core-only

# ext-ros2 이미지만 빌드
./docker/build_local.sh --ext-ros2-only
```

빌드 결과: `oss-mmf-core:local`, `oss-mmf-ext-ros2:local` 이미지가 로컬에 생성됩니다.

#### 방법 B: docker buildx 직접 사용

```bash
# 프로젝트 루트(oss_multisw_framework/)에서 실행

# 코어 이미지 (x86_64, Debug)
docker buildx build \
  -f docker/Dockerfile.rel \
  --target runtime-mmf-core \
  --build-arg BUILD_TYPE=Debug \
  --build-arg TARGETARCH=x86_64 \
  --platform=linux/amd64 \
  --load \
  -t oss-mmf-core:local \
  .

# ROS2 Extension 이미지 (x86_64, Debug) — ros-mmf-base 자동 빌드됨
docker buildx build \
  -f docker/Dockerfile.rel \
  --target runtime-mmf-ext-ros2 \
  --build-arg BUILD_TYPE=Debug \
  --build-arg TARGETARCH=x86_64 \
  --platform=linux/amd64 \
  --load \
  -t oss-mmf-ext-ros2:local \
  .

# AGL Extension 이미지 (x86_64, Release)
docker buildx build \
  -f docker/Dockerfile.rel \
  --target runtime-mmf-ext-agl \
  --build-arg BUILD_TYPE=Release \
  --build-arg TARGETARCH=x86_64 \
  --platform=linux/amd64 \
  --load \
  -t oss-mmf-ext-agl:local \
  .
```

> **`TARGETARCH` 주의**: `Dockerfile.rel`의 스테이지명은 Docker 표준(`amd64`/`arm64`)이 아닌  
> 커널 아키텍처명(`x86_64`/`aarch64`)을 사용합니다. `--build-arg TARGETARCH=x86_64`와 같이 명시적으로 지정하세요.

---

## 어떤 걸 언제 쓰나

| 시나리오 | 사용할 것 |
|---|---|
| 로컬에서 코드 짜고 컴파일 | `Dockerfile` |
| 릴리즈용 바이너리 만들기 (멀티아키, CI) | `Dockerfile.multi` / `build_multiarch.sh` |
| 로컬 x86_64 런타임 이미지 빠르게 만들기 | `build_local.sh` |
| 운영/배포용 가벼운 이미지 패키징 (core / ext-ros2 / ext-agl) | `Dockerfile.rel` |
| OTA 통합 검증의 DDS↔WS 브릿지 | `Dockerfile.rel --target runtime-mmf-ext-ros2` |

---

## Docker Compose로 통합 스택 실행

### 통합 검증 환경 (로컬 x86_64)

`oss-mmf-core:local` + `oss-mmf-ext-ros2:local` 이미지가 빌드되어 있어야 합니다.

```bash
# 스택 시작
docker compose -f docker/compose/docker-compose-integration.yaml up -d

# 상태 확인
docker compose -f docker/compose/docker-compose-integration.yaml ps

# 로그 확인
docker compose -f docker/compose/docker-compose-integration.yaml logs -f mmf-core
docker compose -f docker/compose/docker-compose-integration.yaml logs -f mmf-ext-ros2

# 스택 중지
docker compose -f docker/compose/docker-compose-integration.yaml down
```

**환경변수 커스터마이징**:

```bash
# ROS DDS domain ID 지정 (기본값: 0)
ROS_DOMAIN_ID=5 docker compose -f docker/compose/docker-compose-integration.yaml up -d

# Docker 소켓 GID 지정 (기본값: 999, 호스트에 따라 다를 수 있음)
DOCKER_SOCK_GID=$(stat -c '%g' /var/run/docker.sock) \
  docker compose -f docker/compose/docker-compose-integration.yaml up -d
```

### 재시작 (SHM 정리 포함)

FastDDS SHM 세그먼트가 남아 재시작 후 DDS 통신이 깨지는 경우 사용합니다.

```bash
# 정상 재시작 (SHM 자동 정리)
./docker/restart-mmf.sh

# SHM 정리 없이 단순 재시작
./docker/restart-mmf.sh --no-sweep
```

### AADP 배포 환경 (레지스트리 이미지 사용, NVIDIA GPU 필요)

```bash
# registry.obigo.com의 :latest 이미지를 사용
docker compose -f docker/compose/docker-compose-aadp.yaml up -d
```

### AGL 환경

```bash
docker compose -f docker/compose/docker-compose-agl.yaml up -d
```

---

## 전체 빌드 및 실행 예시 (처음부터 끝까지)

### 시나리오 1: 로컬 개발 환경에서 빌드 후 통합 테스트

```bash
# 1. 프로젝트 루트로 이동
cd oss_multisw_framework

# 2. 개발 환경 이미지 빌드 (최초 1회, 10~20분 소요)
docker build -f docker/Dockerfile -t oss-mmf-dev:local .

# 3. 개발 컨테이너 실행 후 소스 빌드
docker run -it --rm -v "$PWD:/home/ossuser/workspace" oss-mmf-dev:local

# -- 컨테이너 내부에서 --
./docker/scripts/build.sh                             # core + ext (x86_64, Debug)
./docker/scripts/build.sh --set-extension-platform=ROS2  # ROS2 extension
exit

# 4. 런타임 이미지 빌드 (산출물이 output/에 있어야 함)
./docker/build_local.sh

# 5. 통합 스택 실행
docker compose -f docker/compose/docker-compose-integration.yaml up -d

# 6. rosbridge 연결 확인 (포트 9090)
curl -s --max-time 5 http://localhost:9090 || echo "rosbridge not ready yet"
```

### 시나리오 2: CI에서 릴리즈 이미지 빌드 후 레지스트리 Push

```bash
# 멀티아키 빌드 및 push (x86_64 + aarch64, Release)
BUILD_TYPE=Release ./docker/build_multiarch.sh

# 결과: registry.obigo.com/oss/oss-mmf-{core,ext-ros2,ext-agl}:latest
```

---

## 트러블슈팅

### `COPY` 실패 — "no such file or directory"

```
COPY failed: file not found in build context: output/x86_64/Release/core/bin
```

**원인**: 런타임 이미지를 빌드하기 전에 소스가 빌드되지 않았습니다.  
**해결**: 소스를 먼저 빌드하여 `output/` 디렉터리를 생성하세요.

```bash
# 개발 컨테이너 내부 또는 호스트 빌드 환경에서
./docker/scripts/build.sh --release
```

### 재시작 후 DDS 통신 불가

**원인**: FastDDS SHM 세그먼트(`/dev/shm/fastdds_*`)가 이전 컨테이너 실행 후 남아 있습니다.  
**해결**: `restart-mmf.sh`를 사용하거나 수동으로 정리하세요.

```bash
# 자동 정리 재시작
./docker/restart-mmf.sh

# 수동 정리
sudo rm -f /dev/shm/fastdds_* /dev/shm/fast_datasharing_*
```

### rosbridge 포트(9090) 연결 불가

**원인**: `mmf-core`가 완전히 기동되기 전에 `mmf-ext-ros2`가 시작되거나, ROS_DOMAIN_ID 불일치.  
**해결**:

```bash
# mmf-core가 먼저 올라왔는지 확인
docker logs oss-mmf-core | tail -20

# ROS_DOMAIN_ID 일치 여부 확인
docker inspect oss-mmf-core | grep ROS_DOMAIN_ID
docker inspect oss-mmf-ext-ros2 | grep ROS_DOMAIN_ID

# restart-mmf.sh로 올바른 순서로 재시작 (core 기동 후 8초 대기 후 ext-ros2 시작)
./docker/restart-mmf.sh
```

### `DOCKER_SOCK_GID` 오류 (docker.sock 접근 거부)

**원인**: 호스트의 docker 그룹 GID가 기본값(999)과 다릅니다.  
**해결**:

```bash
DOCKER_SOCK_GID=$(stat -c '%g' /var/run/docker.sock)
echo "Docker socket GID: $DOCKER_SOCK_GID"
DOCKER_SOCK_GID=$DOCKER_SOCK_GID \
  docker compose -f docker/compose/docker-compose-integration.yaml up -d
```

### `runtime-mmf-ext-ros2` 빌드 중 IDL 변환 실패

**원인**: `idl/` 디렉터리가 빌드 컨텍스트에 없거나 `idl_to_msg.py` 스크립트가 없습니다.  
**해결**: 프로젝트 루트에서 빌드 명령을 실행하는지 확인하세요 (`.`이 빌드 컨텍스트여야 함).

---

## 관련 파일

| 파일 | 설명 |
|---|---|
| [`build_local.sh`](./build_local.sh) | 로컬 x86_64 런타임 이미지 빌드 래퍼 |
| [`build_multiarch.sh`](./build_multiarch.sh) | x86_64 + aarch64 멀티아키 빌드 & 레지스트리 push 래퍼 |
| [`restart-mmf.sh`](./restart-mmf.sh) | 통합 스택 재시작 (FastDDS SHM 정리 포함) |
| [`scripts/build.sh`](./scripts/build.sh) | 컨테이너/호스트 내부 소스 빌드 스크립트 |
| [`compose/docker-compose-integration.yaml`](./compose/docker-compose-integration.yaml) | OTA 통합 스택 (mmf-core + mmf-ext-ros2, 로컬 이미지) |
| [`compose/docker-compose-aadp.yaml`](./compose/docker-compose-aadp.yaml) | AADP 배포 스택 (레지스트리 이미지, NVIDIA GPU) |
| [`compose/docker-compose-agl.yaml`](./compose/docker-compose-agl.yaml) | AGL 배포 스택 (레지스트리 이미지) |
| [`entrypoint/core_entrypoint.sh`](./entrypoint/core_entrypoint.sh) | core 컨테이너 entrypoint |
| [`entrypoint/ros2_entrypoint.sh`](./entrypoint/ros2_entrypoint.sh) | ext-ros2 컨테이너 entrypoint (rosbridge + ota_client + dds_bridge 기동) |
| [`entrypoint/agl_entrypoint.sh`](./entrypoint/agl_entrypoint.sh) | ext-agl 컨테이너 entrypoint |
| [`build_environment/fastdds-profile.xml`](./build_environment/fastdds-profile.xml) | FastDDS 공통 프로파일 (런타임 이미지에 포함됨) |
