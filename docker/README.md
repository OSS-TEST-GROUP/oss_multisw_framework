# Docker 가이드

> 이 문서는 **Dockerfile 구조**를 설명합니다. 소스 빌드 → 이미지 조립 → 보드 설치 →
> 실행까지의 **단계별 절차**는 [docs/build-install-run-guide.md](../docs/build-install-run-guide.md),
> 재부팅 자동시작 구성은 [docs/board-autostart-guide.md](../docs/board-autostart-guide.md) 를 보세요.

`oss_multisw_framework`는 빌드 라이프사이클의 단계와 사용 시점에 따라 **3개의 Dockerfile**을 분리해 관리합니다. 중복이 아니라 역할이 분리되어 있으며, 일반적으로 다음 파이프라인을 따라 사용됩니다.

```
[개발자 머신/IDE]
   └─ Dockerfile         ── 들어가서 코딩/빌드/디버깅
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

> **이전 `Dockerfile.ros`는 `Dockerfile.rel`의 `ros-mmf-base` stage로 흡수되어 삭제되었습니다.** rosbridge 단독 이미지가 필요했던 OTA 통합 검증 스택은 이제 `Dockerfile.rel --target runtime-mmf-ext-ros2`로 만든 같은 이미지를 사용합니다.

---

## 한눈에 비교

| 파일 | 역할 | 베이스 | 사용자 | ENTRYPOINT/CMD | 이미지 성격 |
|---|---|---|---|---|---|
| [`Dockerfile`](./Dockerfile) | 개발자 빌드 환경 (devcontainer) | `ubuntu:22.04` | `ossuser` (1000) | `/bin/bash` | 두꺼움 (toolchain + Fast-DDS-Gen) |
| [`Dockerfile.multi`](./Dockerfile.multi) | CI/배포 빌드용 멀티아키 빌더 (x86_64 + aarch64 크로스) | `ubuntu:22.04` (multi-stage) | `root` | 없음 (산출물을 `/tmp/oss-<arch>`에 떨굼) | 두꺼움, 산출물 추출용 |
| [`Dockerfile.rel`](./Dockerfile.rel) | 런타임(릴리즈) 이미지. 빌드 산출물을 슬림 이미지에 얹음. `ros-mmf-base` 중간 stage가 ROS 2 + rosbridge + IDL→msg를 같이 설치 | `ubuntu:22.04` (런타임 lib만) | `ossuser` (1000) | `/oss/bin/lifecycle_mgr` 또는 `/oss/entrypoint.sh` | 가벼움 (실행 전용) |

---

## 1. `Dockerfile` — 개발자 빌드 환경

- **목적**: 사람이 컨테이너에 들어가서 코드를 빌드/디버깅하는 dev container
- **특징**
  - `--mount=type=cache`로 apt 캐시 재활용
  - 개발 도구 풀세트: `gdb`, `clang-tidy`, `clang-format`, `iwyu`, `cppcheck`, `pre-commit`, `lcov`, `gcovr` 등
  - `Fast-DDS-Gen`을 git에서 빌드해 설치
  - aarch64 크로스 컴파일러(`gcc-aarch64-linux-gnu`) 포함 (sysroot는 없음)
- **ENTRYPOINT**: 없음, `CMD ["/bin/bash"]` → 들어가서 작업
- **사용 예시**
  ```bash
  docker build -f docker/Dockerfile -t oss-mmf-dev:local .
  docker run -it --rm -v "$PWD:/home/ossuser/workspace" oss-mmf-dev:local
  ```

---

## 2. `Dockerfile.multi` — CI/배포 빌드 (멀티아키)

- **목적**: CI에서 x86_64와 aarch64용 산출물을 동일 컨테이너에서 크로스 빌드해 `/tmp/oss-<arch>/`로 추출
- **멀티 스테이지 구성**
  1. `base-setup` — 공통 환경 변수 / 타임존
  2. `builder-x86_64` — 빌드 툴체인 + Fast-DDS x86 빌드
  3. `builder-aarch64` — `debootstrap`으로 aarch64 sysroot 생성 + `qemu-user-static` + 크로스 빌드
  4. `build-stage` — `COPY . .` 후 cmake/make로 실제 프로젝트 빌드
- **주요 ARG**
  - `TARGET_ARCH` (`x86_64` | `aarch64`)
  - `BUILD_EXTENSION` (`TRUE` | `FALSE`)
  - `EXTENSION_PLATFORM` (`ROS2` | `AGL`)
  - `BUILD_TYPE` (`Release` | `Debug`)
  - `CROSSCOMPILING` (`TRUE` | `FALSE`)
- **최종 산출물**: `/tmp/oss-${TARGET_ARCH}/` 안의 바이너리 + .so → `Dockerfile.rel`이 가져다 씀
- **사용 예시**
  ```bash
  ./docker/build_multiarch.sh   # 래퍼 스크립트
  ```

---

## 3. `Dockerfile.rel` — 릴리즈/런타임 이미지

- **목적**: `Dockerfile.multi`가 만든 `output/<arch>/...`을 받아서 **실행만 가능한 슬림 이미지** 생성
- **특징**
  - 빌드 툴체인 없음 → 런타임 .so만 (`libgrpc++1`, `libprotobuf23`, `libtinyxml2-9`, `libssl3`, `libyaml-cpp0.7`, …)
  - 비루트 `ossuser` (uid 1000)로 실행
  - FastDDS 프로파일 / `FASTDDS_LOG_LEVEL=Error` 사전 설정
- **타깃 (총 6개 + 3 multi-arch selector + 1 공유 base)**

  | 타깃 (stage) | 아키 | 산출물 위치 | ENTRYPOINT |
  |---|---|---|---|
  | `runtime-mmf-core-x86_64` | x86_64 | `output/x86_64/.../core/bin` | `/oss/bin/lifecycle_mgr` |
  | `runtime-mmf-core-aarch64` | aarch64 | `output/aarch64/.../core/bin` | `/oss/bin/lifecycle_mgr` |
  | **`runtime-mmf-core`** | 자동 (`${TARGETARCH}`) | — | (위와 동일) |
  | `ros-mmf-base` *(공유)* | 자동 | ROS 2 humble + rosbridge_server + tier4_autoware_msgs + per-module ament 패키지 (`/opt/ros/humble/share/mmf_interfaces`) | — (실행 안 함, ext-ros2의 base) |
  | `runtime-mmf-ext-ros2-x86_64` | x86_64 | `.../extension/ros2/bin` + `ros-mmf-base` | `/oss/entrypoint.sh` |
  | `runtime-mmf-ext-ros2-aarch64` | aarch64 | 동일 (aarch64) | 동일 |
  | **`runtime-mmf-ext-ros2`** | 자동 selector | — | — |
  | `runtime-mmf-ext-agl-x86_64` | x86_64 | `.../extension/agl/bin` | `/oss/entrypoint.sh` (agl_entrypoint.sh) |
  | `runtime-mmf-ext-agl-aarch64` | aarch64 | 동일 | 동일 |
  | **`runtime-mmf-ext-agl`** | 자동 selector | — | — |

- **`ros-mmf-base` 단계가 하는 일** (`runtime-mmf-ext-ros2-*`의 공통 base)
  - ROS 2 apt 소스 + `ros-humble-ros-core`, `ros-humble-rosbridge-server`, `ros-humble-rosidl-default-generators`, `python3-colcon-common-extensions`, autoware/geographic msgs 설치
  - `tier4_autoware_msgs v0.58.0`을 소스에서 빌드 → `/opt/ros/humble/share/tier4_autoware`
  - `idl/`을 복사 → `idl/rosbridge/idl_to_msg.py`로 DDS IDL을 ROS .msg로 변환 → colcon으로 모듈별 ament 패키지(`diagnosticmgr`, `lifecyclemgr`, `otamanager`, `adswmgr`, `security`) 빌드 → `/opt/ros/humble/share/mmf_interfaces` 밑에 5개 sub-package 설치. 이렇게 해야 rosbridge가 DDS wire type(`<module>::msg::dds_::*_`)을 인식해 `adsw_monitor` / `ota_monitor`가 캐노니컬 pkg/msg/Type 이름으로 통신 가능
  - `ros2_entrypoint.sh`는 두 overlay (`mmf_interfaces`, `tier4_autoware`)를 source 함

- **사용 예시**
  ```bash
  # 코어 (lifecycle_mgr)
  docker build -f docker/Dockerfile.rel --target runtime-mmf-core \
    -t oss-mmf-core:local .

  # ROS2 확장 + rosbridge 포함 풀스택 (ros-mmf-base 자동 빌드됨)
  docker build -f docker/Dockerfile.rel --target runtime-mmf-ext-ros2 \
    -t oss-mmf-ext-ros2:local .
  ```
- **주의**: 이 Dockerfile은 빌드를 하지 않습니다 — `core` / `ext-agl` 단계는 `output/<arch>/${BUILD_TYPE}/...`의 산출물이 컨텍스트에 이미 존재해야 `COPY`가 성공합니다. 단, `ros-mmf-base`는 별도 산출물 없이 IDL만으로 자급자족합니다.

---

## 어떤 걸 언제 쓰나

| 시나리오 | Dockerfile |
|---|---|
| 로컬에서 코드 짜고 컴파일 | `Dockerfile` |
| 릴리즈용 바이너리 만들기 (멀티아키) | `Dockerfile.multi` |
| 운영/배포용 가벼운 이미지 패키징 (core / ext-ros2 / ext-agl) | `Dockerfile.rel` |
| OTA 통합 검증의 DDS↔WS 브릿지 | `Dockerfile.rel --target runtime-mmf-ext-ros2` |

---

## 관련 파일

- [`build_local.sh`](./build_local.sh) — 로컬 빌드 래퍼
- [`build_multiarch.sh`](./build_multiarch.sh) — 멀티아키 빌드 래퍼
- [`compose/docker-compose-integration.yaml`](./compose/docker-compose-integration.yaml) — OTA 통합 스택 (mmf-core + mmf-ext-ros2)
- [`compose/docker-compose-aadp.yaml`](./compose/docker-compose-aadp.yaml) — AADP 배포 스택
- [`entrypoint/`](./entrypoint/) — 각 컨테이너의 entrypoint 스크립트
- [`build_environment/fastdds-profile.xml`](./build_environment/fastdds-profile.xml) — FastDDS 공통 프로파일
