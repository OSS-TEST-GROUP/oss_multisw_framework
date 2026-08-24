# 정적분석 · 유닛테스트 진행 가이드 — lifecycle_mgr / adsw_manager

**대상 모듈 (확정)**

| 모듈 | 경로 |
|---|---|
| lifecycle_manager | [`service/core/lifecycle_mgr`](../../service/core/lifecycle_mgr) |
| adsw_manager (ROS2) | [`service/extension/ros2/adsw_manager`](../../service/extension/ros2/adsw_manager) |
| adsw_manager (GENERAL / SiLS) | [`service/extension/general/adsw_manager`](../../service/extension/general/adsw_manager) |

general 판은 나중에 합류했다. **ros2 판의 복사본이 아니라 포크다** — 파일 58개 중
로거 2개만 바이트 단위로 같고 나머지 54개는 내용이 갈렸다. 그래서 ros2 쪽 수정을
그대로 이식할 수 없고, 측정·수정·테스트를 별도로 했다.
`EXTENSION_PLATFORM=GENERAL` 로 configure 해야 소스가 잡힌다.

**범위 밖**: `service/extension/agl/adsw_manager`

이 문서는 작업을 **다른 머신(x86_64 Linux)에서 이어받기 위한 인수인계 문서**다.
0단계(환경·베이스라인)부터 순서대로 따라가면 된다.

---

## 1. 왜 리눅스 x86_64 여야 하는가

macOS(Apple Silicon)에서는 진행하지 않기로 결정했다. 재논의를 막기 위해 근거를 남긴다.

1. **아키텍처 전제 불일치.** Fast-DDS 는 빌더 컨테이너 안에서 소스 빌드되고
   ([Jenkinsfile.unittest](../../Jenkinsfile.unittest) `Pull Builder Container` 스테이지)
   `output/ros2_x86_64` 는 실제 크로스컴파일이 아니라 **디렉토리 이름 라벨**일 뿐이다.
   반면 [CMakeLists.txt](../../CMakeLists.txt) 의 `OUTPUT_DIR` 은
   `${CMAKE_SYSTEM_PROCESSOR}` 를 쓰므로 arm64 에서는 `output/aarch64/...` 가 되고,
   [docker/scripts/run_unittest.sh](../../docker/scripts/run_unittest.sh) 는
   `output/x86_64/Debug` 를 하드코딩한다 → `MMF_CONFIG_PATH`, `DIAG_IDL_PATH`,
   `SAVED_REGISTRY_BACKUP` 이 전부 없는 경로를 가리켜 테스트가 무더기로 깨진다.
   아키텍처 경로를 패치해야 하는데, 그러면 **측정 대상과 측정 도구를 동시에 바꾸는 셈**이라
   베이스라인의 의미가 없어진다.
2. **런타임 의존성이 리눅스 전용.** CI 는 `--network=host`,
   `-v /var/run/docker.sock`, `-u 1000:1000`, `/run/user/1000` 를 전제한다.
   macOS Docker Desktop 은 컨테이너를 별도 리눅스 VM 에서 돌리므로 host 네트워킹이
   맥 호스트 네트워크가 아니고, 이 스위트는 실 DDS multicast 디스커버리에 의존하는
   케이스가 많다 (`ConcreteEndpointIntegrationTest`, `ADSWManagerTest`,
   `ROSDiscoveryInfoEndpointTest`). autoware E2E 9종은 추가로 docker-in-docker +
   `mmf-autoware-mock:humble`(760MB) 를 요구한다.
3. **정적분석만 떼어내는 것도 불가.** clang-tidy 는 `compile_commands.json` 이 필요하고,
   그건 cmake configure 가 `find_package(fastcdr 2 REQUIRED)` / `fastdds 3` 를
   통과해야 나온다 → 결국 FastDDS SDK 가 있는 컨테이너가 필요하다.

---

## 2. 대상 머신 요구사항

| 항목 | 요구 | 확인 명령 |
|---|---|---|
| 아키텍처 | x86_64 Linux | `uname -m` → `x86_64` |
| docker | 데몬 실행 + 현재 유저가 `docker` 그룹 | `docker info >/dev/null && id -nG \| grep docker` |
| uid | **1000** 권장 | `id -u` → `1000` |
| 디스크 | 50 GiB 이상 여유 | `df -h .` |
| 코어 | 8코어 이상 권장 (Fast-DDS 소스 빌드) | `nproc` |
| 기타 | `python3-venv`, `wget`, `git`, `curl`, `sudo` | — |

**uid 가 1000 이 아니면**: 컨테이너가 `-u 1000:1000` 으로 뜨므로 바인드 마운트한
`output/`, `.build/` 에 쓰기 권한이 없어 빌드가 조용히 실패한다. uid 가 다르면
`build.sh` / `docker run` 의 `-u` 를 `$(id -u):$(id -g)` 로 바꿔야 한다.

---

## 3. Step 0 — 환경 구축 (최초 1회)

### 0-1. 저장소

```bash
git clone git@github.com:obigo-gov/oss_multisw_framework.git
cd oss_multisw_framework
git checkout develop        # 또는 작업 브랜치
```

### 0-2. 빌더 이미지

```bash
# 레지스트리 자격증명이 있으면
export REGISTRY_ID=<id> REGISTRY_PWD=<pw>
docker pull registry.obigo.com/oss/oss-mmf-builder:latest

# 없거나 실패하면 로컬 빌드 (10~20분)
docker build -f docker/Dockerfile -t registry.obigo.com/oss/oss-mmf-builder:latest .
```

> `build.sh` 는 레지스트리 catalog 조회에 실패하면 자동으로 로컬 `docker build` +
> push 시도로 넘어간다. push 실패는 경고만 내고 로컬 이미지를 쓴다.

### 0-3. Fast-DDS SDK (`SDK_DIR`)

`build.sh` 는 Fast-DDS **소스만 내려받고 빌드는 하지 않는다** (colcon 빌드는
Jenkins 전용 스테이지). 그래서 이 한 단계는 수동이다.

```bash
export SDK_DIR=${HOME}/obigo-sdk        # build.sh 기본값

# (1) 소스 fetch — 호스트에서
mkdir -p ${SDK_DIR}
if [ ! -d ${SDK_DIR}/Fast-DDS ]; then
  python3 -m venv ${SDK_DIR}/venv
  . ${SDK_DIR}/venv/bin/activate
  pip3 install --no-cache-dir colcon-common-extensions vcstool
  mkdir -p ${SDK_DIR}/Fast-DDS/src
  cd ${SDK_DIR}/Fast-DDS
  wget https://raw.githubusercontent.com/eProsima/Fast-DDS/3.2.2/fastdds.repos
  vcs import src < fastdds.repos
  sed -i "s#-Wpedantic##g" ${SDK_DIR}/Fast-DDS/src/fastdds/CMakeLists.txt
  deactivate
fi

# (2) 컨테이너 안에서 빌드 — nproc 에 따라 30~60분
docker run --rm -u 1000:1000 -v ${SDK_DIR}:/opt \
  registry.obigo.com/oss/oss-mmf-builder:latest \
  bash -lc 'cd /opt/Fast-DDS && colcon build --merge-install \
      --parallel-workers $(nproc) \
      --build-base build/ros2_x86_64 \
      --install-base output/ros2_x86_64'

# (3) 확인
test -f ${SDK_DIR}/Fast-DDS/output/ros2_x86_64/setup.bash && echo OK
```

### 0-4. 유닛테스트 빌드

```bash
cd <repo>
./build.sh --build-unittest      # Debug 강제 + --coverage 계측
```

> **주의**: `build.sh` 는 첫 실행에서 `setup_sudo_nopasswd()` 를 호출해
> `/etc/sudoers.d/nopasswd_cmds` 에 현재 유저의 **NOPASSWD: ALL** 항목을 심는다.
> 공용 머신이라면 이 함수 호출을 주석 처리하고 진행할 것.
> (x86_64 네이티브 경로에서는 `do_install_ros_build_environment` 는 즉시 return 하므로
> aarch64 sysroot debootstrap 은 일어나지 않는다.)

확인:

```bash
ls .build/Debug/ros2_x86_64                       # 빌드 트리
ls output/x86_64/Debug/core/bin/unittest          # 테스트 바이너리
find .build -name '*.gcno' | head                 # 커버리지 계측 확인
```

---

## 4. Step 1 — 베이스라인 측정

여기까지가 **이번 단계의 산출물**이다. 아래 3개를 돌리고 숫자를 보고하면
Phase 1 의 수정 대상을 확정할 수 있다.

### 1-1. 정적분석

`scripts/` 는 `build.sh` 의 마운트 목록에 없으므로 별도 `docker run` 이 필요하다.
저장소 전체를 마운트한다.

```bash
docker run --rm -it -u 1000:1000 \
  -w /home/ossuser/workspace \
  -v ${PWD}:/home/ossuser/workspace \
  -v ${HOME}/obigo-sdk:/opt \
  registry.obigo.com/oss/oss-mmf-builder:latest \
  ./scripts/analyze-modules.sh
```

산출물: `output/reports/module-analysis/`

| 파일 | 내용 |
|---|---|
| `SUMMARY.md` | **보고용 요약** — 모듈별 총계 + 체크별/파일별 상위 |
| `clang-tidy-<mod>.txt` | clang-tidy 원본 |
| `clang-tidy-<mod>.by-check.txt` | 체크별 건수 (수정 우선순위 판단 근거) |
| `clang-tidy-<mod>.by-file.txt` | 파일별 건수 |
| `cppcheck-<mod>.xml` | cppcheck 원본 (Jenkins `recordIssues` 호환) |
| `cppcheck-<mod>.summary.txt` | severity / id 별 건수 |

옵션: `--only lifecycle` / `--only adsw` / `--only general` / `--skip-cppcheck` / `--reconfigure`

### 1-2. 유닛테스트

**`./docker/scripts/run_unittest.sh` 를 그 경로 그대로 실행하면 안 된다.**
스크립트는 `WORK_DIR` 을 `dirname ${BASH_SOURCE[0]}` 로 잡으므로 그렇게 실행하면
`WORK_DIR=<repo>/docker/scripts` 가 되어 `MMF_CONFIG_PATH` / `DIAG_IDL_PATH` /
`SAVED_REGISTRY_BACKUP` 이 전부 없는 경로를 가리키고, 결과도
`docker/scripts/output/` 밑에 떨어진다. (실측: lifecycle_mgr 263건, adsw 99건이
이 이유만으로 실패했다.) Jenkins 는 스크립트를 **워크스페이스 루트에 마운트**해서
쓴다 ([Jenkinsfile.unittest](../../Jenkinsfile.unittest) `Run Unittest` 스테이지).
같은 방식으로 실행한다.

```bash
docker run --rm -it -u 1000:1000 --network=host \
  -w /home/ossuser/workspace \
  -v ${PWD}:/home/ossuser/workspace \
  -v ${PWD}/docker/scripts/run_unittest.sh:/home/ossuser/workspace/run_unittest.sh \
  -v ${HOME}/obigo-sdk:/opt \
  -v /var/run/docker.sock:/var/run/docker.sock \
  -e MMF_E2E_ROOT=/home/ossuser/workspace \
  -e MMF_E2E_OUTPUT=/home/ossuser/workspace/output/x86_64/Debug \
  -e MMF_AUTOWARE_TEST_IMAGE=mmf-autoware-mock:humble \
  -e MMF_E2E_FASTDDS_LIB=/opt/Fast-DDS/output/ros2_x86_64/lib \
  registry.obigo.com/oss/oss-mmf-builder:latest \
  ./run_unittest.sh
```

> 이 방식은 호스트 저장소 루트에 빈 `run_unittest.sh` 를 root 소유로 남긴다
> (docker 가 마운트 지점을 만든다). 끝나면 지울 것.

산출물: `output/unittest-results/<label>/junit_results.xml`

autoware E2E 를 실제로 돌리려면 mock 이미지가 먼저 필요하다 (없으면 fast-skip 된다):

```bash
docker build -f docker/test/Dockerfile.autoware-mock -t mmf-autoware-mock:humble .
```

### 1-3. 커버리지

```bash
docker run --rm -it -u 1000:1000 \
  -w /home/ossuser/workspace \
  -v ${PWD}:/home/ossuser/workspace \
  -v ${HOME}/obigo-sdk:/opt \
  registry.obigo.com/oss/oss-mmf-builder:latest \
  ./scripts/coverage-modules.sh
```

산출물: `output/coverage/modules/`

| 파일 | 내용 |
|---|---|
| `SUMMARY.md` | **보고용 요약** — 모듈별 라인/브랜치 % + 0% 파일 목록 |
| `<mod>/by-file.txt` | 파일별 라인/브랜치 커버리지 (낮은 순) |
| `<mod>/zero-coverage.txt` | 라인 커버리지 0% 파일 — Phase 2 최우선 대상 |
| `<mod>/report/index.html` | genhtml 리포트 |
| `<mod>/coverage.xml` | cobertura |

옵션:
- `--reuse-capture` — `lcov --capture` 재사용 (반복 실행 시 훨씬 빠름)
- `--with-logger` — `logger/` 포함. CI 는 제외하는데 두 모듈 모두
  `logger/Logger.cpp` 를 갖고 있어 수치에서 빠진다. 차이를 보려면 이 플래그로 한 번 더.
- `--with-simulator` — `lifecycle_mgr/simulator/` 포함 (기본 제외, 이유는 아래 6절)

### 1-4. 보고할 숫자

Phase 1 대상 확정에 필요한 입력이다.

- [x] `output/reports/module-analysis/SUMMARY.md` 의 요약 표
      (특히 **`fatal/include-error` 가 0 인지** — 0 이 아니면 분석이 정상 파싱되지 않은
      파일이 있다는 뜻이고, 그 건수는 베이스라인으로 쓸 수 없다)
- [x] 두 모듈의 `by-check.txt` 상위 — `bugprone-*` / `clang-analyzer-*` 건수
- [x] cppcheck severity 분포 (`error` 건수가 핵심)
- [x] `output/coverage/modules/SUMMARY.md` 의 라인/브랜치 % 와 0% 파일 목록
- [x] `run_unittest.sh` 의 pass / fail / skip 집계

---

## 4-A. 측정 결과

x86_64 Linux / 28코어 / uid 1000 / 빌더 이미지 `oss-mmf-builder:latest` /
clang-tidy 14.0.0 / cppcheck 2.7 에서 실측한 값이다.

**두 시점**을 나눠 적는다.

- **베이스라인** — 커밋 58557c2, 손대기 전 상태
- **현재** — 6-5 링크 버그 수정 + Phase 2 테스트 보강 + 로거 정리를 마친 뒤

### 한눈에

| | lifecycle_mgr | | ros2/adsw | | general/adsw | |
|---|---|---|---|---|---|---|
| | 전 | 후 | 전 | 후 | 전 | 후 |
| 테스트 총수 | 575 | **623** | 591 | **727** | 8 | **169** |
| pass | 527 | **575** | 579 | **716** | 0 | **161** |
| fail | 0 | **0** | 1\* | **0** | 0 | **0** |
| 라인 커버리지 | 71.63% | **84.04%** | 48.18% | **83.39%** | **0%** | **78.70%** |
| 브랜치 (LOG_ 제외) | 46.34% | **56.53%** | 38.58% | **51.29%** | 0% | **48.84%** |
| 라인 0% 파일 | 2 | **0** | 3 | **0** | 2 | **0** |
| clang-tidy unique | 663 | **640** | 754 | **728** | 621 | **605** |
| cppcheck | 117 | **99** | 97 | **73** | 75 | **56** |
| **P1** | 18 | **0** | 33 | **0** | 15 | **0** |

\* `ADSWServiceRegistryTest_NOT_BUILT` 플레이스홀더. 실제 테스트 실패는 아니었다 (6-5).

general 은 테스트가 사실상 없는 상태(8건 전부 skip, 계측 파일 2개가 모두 0%)에서
시작했다. 161건을 새로 붙여 78.7% 까지 올렸고 0% 파일은 사라졌다.

### 정적분석 상세 (현재)

| 모듈 | TU | clang-tidy unique | raw | fatal/include-error | cppcheck |
|---|---|---|---|---|---|
| `service/core/lifecycle_mgr` | 47 | 640 | 2,997 | **0** | 99 |
| `service/extension/ros2/adsw_manager` | 29 | 728 | 4,521 | **0** | 73 |
| `service/extension/general/adsw_manager` | 25 | 605 | 3,630 | **0** | 56 |

P1(`bugprone-*` / `clang-analyzer-*` / cppcheck `error`)은 **두 모듈 모두 0건**이다.
처리 내역은 7절 Phase 1 참조.

### 유닛테스트 상세 (현재)

`junit_results.xml` 기준. **두 모듈 모두 실패 0건.**

| 레인 | 총 | pass | fail | skip |
|---|---|---|---|---|
| `lifecycle_mgr` | 623 | 575 | 0 | 48 |
| `ros2_adsw_manager` | 727 | 716 | 0 | 11 |
| `general_adsw_manager` | 169 | 161 | 0 | 8 |

- lifecycle skip 48건: autoware E2E 39건(mock 이미지가 있어도 조건부 skip) +
  `EndPointFactoryTest` 9건.
- adsw skip 11건: `ContainerDockerTest` 8, `ContainerHealthMonitorDockerTest` 3.
  베이스라인의 `ADSWServiceRegistryTest_NOT_BUILT` 1건은 6-5 수정으로 사라졌다.
- autoware mock 이미지(`mmf-autoware-mock:humble`)를 빌드해 둔 상태에서 측정했다.
  그래도 E2E 39건은 skip 이므로 이 수치는 mock 유무와 무관하다.
- **범위 밖 주의**: `diagnostics` 레인은 591건 중 50건 실패다. 베이스라인과
  같은 수치이고 두 대상 모듈과 무관한 기존 실패다. `vehicle_info_mgr` 은
  `dds_provider_test.cpp` 가 `TrajectoryPubSubTypes.hpp` 를 못 찾아 빌드 자체가
  안 된다 — 이것도 기존 문제다. 둘 다 별도 이슈로 분리할 것.

### 커버리지 상세 (현재, logger / simulator 제외)

| 모듈 | 파일 | 0% 파일 | 라인 | 라인% | 브랜치% | 브랜치%(LOG_ 제외) |
|---|---|---|---|---|---|---|
| `lifecycle_mgr` | 44 | **0** | 1827/2174 | **84.04%** | 43.15% | **56.53%** |
| `ros2/adsw_manager` | 27 | **0** | 1581/1896 | **83.39%** | 42.58% | **51.29%** |
| `general/adsw_manager` | 14 | **0** | 1068/1357 | **78.70%** | 40.34% | **48.84%** |

### 브랜치 % 를 라인 % 와 같은 잣대로 보면 안 된다

브랜치 분모의 절반 이상이 **`LOG_*` 매크로**에서 나온다. `LOG_DEBUG(a << b << c)`
한 줄이 인라인 확장되며 브랜치 수십 개를 만들고, 전부 매크로 첫 줄에 귀속된다.
그 브랜치는 우리 로직이 아니라 `std::stringstream` / `operator<<` 의 libstdc++
내부(locale·width 등)라 어떤 테스트로도 대부분 실행되지 않는다.

실측 예: `monitor/ContainerHealthMonitor.cpp` 는 163줄에 브랜치 698개인데
상위 10개 라인(각 28~50 브랜치)이 전부 `LOG_DEBUG` 다.

참고로 `gcovr --exclude-throw-branches` 는 거의 효과가 없다 (5303 → 5289).
예외 엣지가 아니라 **로깅 매크로**가 원인이라는 뜻이다.

→ `coverage-modules.sh` 가 `LOG_` 라인의 브랜치를 뺀 값을 함께 낸다
(`SUMMARY.md` 의 마지막 열, 파일별로는 `<mod>/branches-nolog.txt`).
**게이트는 라인% 에 걸고 브랜치는 참고치로 둘 것.** 굳이 브랜치로 걸면 LOG_ 제외 열을 쓴다.

## 5. 확정된 사전 조사 결과

측정 없이 정적 리딩으로 이미 확인된 사실이다. 다시 조사할 필요 없다.

### 규모

| | lifecycle_mgr | ros2/adsw_manager |
|---|---|---|
| 프로덕션 LOC | 8,730 | 6,901 |
| 테스트 LOC | 14,015 | 12,146 |
| gtest 케이스 수 | 606 | 747 |
| unittest CMake 에 연결된 소스 | 38 / 42 | 23 / 29 |

### unittest CMake 에 연결되지 않은 소스 (Phase 2 대상)

**lifecycle_mgr** — `main.cc` 는 엔트리포인트라 대상 제외:
- `manager/LifecycleFactory.cpp`
- `manager/ManagerBase.cpp`
- `transmitter/MessageQueue.cpp`

mock 은 [`unittest/mock/`](../../service/core/lifecycle_mgr/unittest/mock) 에
`mock_lifecycle_factory.hpp`, `mock_manager_base.hpp`, `mock_message_queue.hpp` 로
**이미 존재**한다 → 프로덕션 코드 수정 없이 테스트를 붙일 수 있다.

**ros2/adsw_manager** — `main.cc` 제외:
- `endpoint/EndPointFactory.cpp`
- `endpoint/NotifyContainerStateEndPoint.cpp`
- `endpoint/SetRecoveryPolicyEndPoint.cpp`
- `endpoint/StartContainerEndPoint.cpp`
- `endpoint/StopContainerEndPoint.cpp`

참고할 기존 패턴:
[`lifecycle_mgr/unittest/EndPointFactoryTest.cpp`](../../service/core/lifecycle_mgr/unittest/EndPointFactoryTest.cpp),
[`adsw_manager/unittest/StartServiceEndpointTest.cpp`](../../service/extension/ros2/adsw_manager/unittest/StartServiceEndpointTest.cpp)

### 이미 있는 CI 인프라 (새로 만들 것 없음)

| 스테이지 | 위치 |
|---|---|
| SlocCount → Clang-tidy → Cppcheck → CppLint | [Jenkinsfile](../../Jenkinsfile) |
| Build → Run Unittest → Coverage | [Jenkinsfile.unittest](../../Jenkinsfile.unittest) |
| 통합 테스트 레인 | [Jenkinsfile.integration](../../Jenkinsfile.integration) |
| clang-tidy 체크 설정 | [.clang-tidy](../../.clang-tidy) — `bugprone-*`, `clang-analyzer-*` 가 이미 `WarningsAsErrors` |
| 커버리지 수집 | [docker/scripts/coverage.sh](../../docker/scripts/coverage.sh) |
| 유닛/통합 레인 분리 | [docker/scripts/integration-tests.regex](../../docker/scripts/integration-tests.regex) |

### 빌더 이미지에 포함된 도구

`clang-tidy`, `run-clang-tidy`(clang-tools), `cppcheck`, `cpplint`, `sloccount`,
`lcov`, `gcovr`, `lcov_cobertura`, `fastddsgen`, `colcon`, `python3`.
**`jq` 는 없다** — Jenkins 는 호스트에서 설치해 쓴다. 그래서
`analyze-modules.sh` 는 compile db 필터링을 `python3` 로 한다.

### 기타 확인 사항

- IDL 생성은 [idl/dds/CMakeLists.txt](../../idl/dds/CMakeLists.txt) 의 `execute_process`,
  즉 **configure 시점**이다. → clang-tidy 용 `compile_commands.json` 을 만드는
  `cmake` 한 번으로 생성 헤더가 확보된다 (`make` 불필요).
- [ros2/adsw_manager/CMakeLists.txt](../../service/extension/ros2/adsw_manager/CMakeLists.txt)
  는 `pthread fastcdr fastdds yaml-cpp` 만 링크한다 → **실 ROS2 설치 불필요**.

---

## 6. 알려진 함정

측정 결과를 해석할 때 반드시 감안해야 한다.

### 6-1. 빌드 디렉토리 공유

[docker/scripts/build.sh](../../docker/scripts/build.sh) 의 `do_build_cmd` 는
core / ext-ros2 / ext-agl 세 번의 configure 를 모두
`.build/Debug/ros2_x86_64` **한 디렉토리**에 순차로 쓴다
(`TARGET_MF_NAME` 이 AGL 루프에서 갱신되지 않음). 마지막 configure 가 살아남고
앞선 타겟의 `CTestTestfile.cmake` / 오브젝트는 stale 하게 남는다.
`run_unittest.sh` 가 `find .build/ -name CTestTestfile.cmake` 로 훑는 구조라 동작은 하지만,
모듈별 커버리지 귀속과 깨끗한 재실행이 취약하다.

→ `analyze-modules.sh` 는 `.build/analyze/<mod>` 에 **전용 configure 디렉토리**를 써서
   이 문제를 우회한다. 문제 자체는 별도 이슈로 분리할 것.

### 6-2. 커버리지 기본 제외 항목

- `*/logger/*` — CI [coverage.sh](../../docker/scripts/coverage.sh) 가 제외한다.
  두 모듈 모두 `logger/Logger.cpp` 를 갖고 있어 수치에서 빠진다. **의도인지 확인 필요.**
- `*/simulator/*` — `coverage-modules.sh` 가 추가로 제외한다.
  `lifecycle_mgr/simulator/` 는 별도 `main()` 을 가진 개발용 퍼블리셔/서브스크라이버
  하네스이므로 프로덕션 커버리지 분모에서 빼는 것이 맞다. **CI 수치와는 달라진다.**

### 6-3. 유닛 레인에서 제외된 테스트

[integration-tests.regex](../../docker/scripts/integration-tests.regex) 가 유닛 레인에서
제외하는 것 중 두 모듈 관련분:

| 테스트 | 건수 | 제외 이유 |
|---|---|---|
| `ConcreteEndpointIntegrationTest.*` | 다수 | 실 DDS pub→sub |
| `LifecycleAdswIntegrationTest.*` | 다수 | 시뮬레이터 서브프로세스 fork |
| `ADSWManagerTest` | 5 | 실 ROS2 DDS participant + 그래프 |
| `ContainerDockerComposeTest` | 4 | `docker compose` 서브프로세스 |
| `ROSDiscoveryInfoEndpointTest` | 6 | 실 FastDDS DataReader participant |

이들이 커버하는 코드는 **유닛 커버리지 수치에 잡히지 않는다.** 커버리지가 낮게 나오는
파일 중 일부는 실제로 이 테스트들이 커버하고 있을 수 있으니, Phase 2 대상 선정 전에
대조해야 한다.

### 6-4. `analyze-modules.sh` 가 Jenkins 와 다른 점

Jenkins 는 `run-clang-tidy` 에 `*.hpp`/`*.h` 까지 위치인자로 넘긴다. 헤더는
compilation database 에 컴파일 커맨드가 없어 사실상 분석되지 않는다.
`analyze-modules.sh` 는 TU(`*.cpp`/`*.cc`)만 넘기고 `-header-filter` 로 모듈 헤더를 잡는다.
→ **같은 코드인데 Jenkins 리포트보다 건수가 많이 나올 수 있다. 정상이다.**

또한 헤더 경고는 그 헤더를 include 하는 TU 마다 중복 보고되므로,
`SUMMARY.md` 는 `raw` 와 `unique` 를 나눠 낸다. **실제 고쳐야 할 지점 수는 `unique`** 다.

### 6-5. `ADSWServiceRegistryTest` 는 링크가 깨져 있다

`ros2_adsw_manager` 레인의 fail 1건, 그리고 `service/ServiceRegistry.cpp` 434줄
0% 커버리지의 원인이 같다. 오브젝트 3개는 정상 컴파일되지만 **링크에서 실패**해
바이너리가 안 만들어지고, ctest 가 `ADSWServiceRegistryTest_NOT_BUILT`
플레이스홀더를 대신 넣어 그게 실패로 잡힌다.

```
undefined reference to `ros2::adsw_manager::Container::GetExtraArgs() const'
undefined reference to `ros2::adsw_manager::Container::GetCommandArgs() const'
undefined reference to `ros2::adsw_manager::Container::SetExtraArgs(...)'
undefined reference to `ros2::adsw_manager::Container::SetCommandArgs(...)'
undefined reference to `ros2::adsw_manager::Service::IsContainerWorkload() const'
undefined reference to `ros2::adsw_manager::Service::SetContainerWorkload(bool)'
```

**주의: `CMakeLists.txt` 에 `Container.cpp` / `Service.cpp` 를 추가하는 건 오답이다.**
[mock/MockContainer.hpp](../../service/extension/ros2/adsw_manager/unittest/mock/MockContainer.hpp)
와 [mock/MockService.hpp](../../service/extension/ros2/adsw_manager/unittest/mock/MockService.hpp)
는 gmock 서브클래스가 아니라 **실제 `Container::` / `Service::` 메서드를 인라인
정의하는 링크 심(seam)** 이다. 실제 .cpp 를 같이 링크하면 중복 정의가 된다.

빠진 건 그 심에 있는 메서드 6개다. `ServiceRegistry::Save` / `LoadServiceSaved`
가 호출하는데 심에 정의가 없어서 링크가 깨졌다.

→ mock 헤더 두 개에 실제 멤버를 그대로 쓰는 정의를 추가했다. 기존
`GetContainerType` / `GetProcessMatchPattern` (F013 주석) 과 같은 패턴이다.
`MockService.hpp` 의 `Service::Service` 초기화 목록에 `is_container_workload_(false)`
도 함께 넣었다 — 실제 `Service.cpp` 생성자에는 있는데 심에는 빠져 있어
초기화되지 않은 bool 을 읽게 된다.

**적용 완료.** 결과는 위 4-A 참조 (테스트 591 → 658, adsw 라인 커버리지
48.18% → 67.51%).

### 6-5-1. 왜 아무도 몰랐나 — 빌드 실패가 삼켜진다

[build.sh](../../docker/scripts/build.sh) `do_build_cmd` 가
`make -k -j${CPU_CORES} || true` / `make install -k ... || true` 다.
`-k` 로 계속 진행하고 `|| true` 로 종료코드까지 지운다. 그래서
**타겟 하나가 링크에 실패해도 빌드는 성공으로 끝난다.**
ctest 는 없는 바이너리 자리에 `_NOT_BUILT` 를 넣고, Jenkins 성공률 기준은
50% 라 1건 실패로는 아무 알람도 안 울린다.

→ Phase 3 에서 `make` 종료코드를 확인하거나, 최소한
`gtest_discover_tests` 대상 수와 실제 생성된 바이너리 수를 대조하는
체크를 넣을 것. 이번에 쓴 대조 방법:

```bash
grep -oP 'gtest_discover_tests\s*\(\s*\K\w+' <mod>/unittest/CMakeLists.txt | sort > /tmp/decl
ls .build/Debug/ros2_x86_64/.../unittest/out/bin | sort > /tmp/built
comm -23 /tmp/decl /tmp/built    # 선언됐는데 안 만들어진 것
```

### 6-6. 유닛테스트 레이블 충돌 (수정함)

`run_unittest.sh` 가 결과 레이블을 `basename` 으로만 만들어서
`extension/{ros2,general,agl}/adsw_manager` 세 레인이 모두 `adsw_manager` 가 되고
같은 `junit_results.xml` 에 덮어썼다. 마지막 레인만 남아 **ros2/adsw_manager 결과가
통째로 유실**된다 (`ota_client` 도 ros2/agl 두 벌이라 같은 문제).
extension 레인에 플랫폼 세그먼트를 붙이도록 고쳤다 → `ros2_adsw_manager`,
`general_adsw_manager`, `agl_adsw_manager`, `ros2_ota_client`, `agl_ota_client`.
core 레인 레이블(`lifecycle_mgr` 등)은 그대로다.

### 6-7. `analyze-modules.sh` 에서 고친 것

셋 다 조용히 틀린 숫자를 내던 버그다. 이 문서의 이전 버전 절차 그대로 돌리면
재현된다.

1. **SDK 를 source 하지 않았다.** 가드가 `command -v fastddsgen` 이었는데
   빌더 이미지는 `fastddsgen` 을 `/usr/local/bin` 에 따로 심어 둬서 항상 참이 된다.
   → `CMAKE_PREFIX_PATH` 가 빈 채로 configure 에 들어가
   `find_package(fastcdr 2 REQUIRED)` 실패. 가드를 `CMAKE_PREFIX_PATH` 검사로 바꿨다.
2. **`set -u` 와 colcon `setup.bash` 충돌.** `COLCON_TRACE` unbound 로 죽는다.
   source 구간만 `set +u`.
3. **ANSI 이스케이프 때문에 집계가 0 이었다.** `run-clang-tidy` 가 `--use-color` 를
   붙여 넘겨 원본에 색코드가 섞이고, 진단 추출 정규식이 한 건도 매칭하지 못해
   `raw=0 unique=0 fatal=0` 이 나왔다. **1.1MB 짜리 원본이 있는데 0건이면 이걸 의심할 것.**
   파싱 전에 ANSI 를 제거한다.
4. 같은 헤더가 `a/b.hpp` 와 `a/./b.hpp` 두 표기로 잡혀 dedupe 가 안 먹었다
   (unique 가 lifecycle 216건, adsw 156건 부풀었다). `/./` 를 접는다.

### 6-8. 저장소 루트의 in-source cmake 잔재

루트에 `CMakeCache.txt` / `CMakeFiles/` 가 있으면 `analyze-modules.sh` 의
전용 디렉토리 configure 가 `The current CMakeCache.txt directory ... is different`
로 실패한다. 둘 다 `.gitignore` 대상이라 `git status` 에 안 보인다. 지우고 시작할 것.

---

### 6-9. 유닛 레인 회수를 위해 연 seam (수정함)

세 곳은 "테스트가 없어서" 가 아니라 **가로챌 수가 없어서** 미커버였다.
프로덕션 동작은 그대로 두고 가로챌 지점만 열었다.

| 위치 | 원래 | 바꾼 것 |
|---|---|---|
| `ContainerController` | `RunCommand` 가 private, popen 은 함수 안에 인라인 | `RunCommand` / `CaptureCommand` 를 `protected virtual` 로. popen 블록 2곳을 `CaptureCommand` 로 추출 (버퍼 절단 규약 포함 동작 동일) |
| `ContainerHealthMonitor` | `IsServiceAliveInContainer` / `LookupServicePidInContainer` 가 익명 네임스페이스 자유 함수. 폴링은 3초 sleep 스레드 루프 안 | 둘을 `protected virtual` 멤버로. `PollLoop` 한 사이클을 `PollOnce()` 로 분리 (`continue` → `return` 외 변경 없음) |
| `ROSServiceController` | `GetContainer()` 가 non-virtual | `virtual` 로. `MockROSServiceController` 가 이미 `MOCK_METHOD` 로 선언해 뒀지만 오버라이드가 아니라 가려짐이라 주입이 안 됐다 |

**virtual 로 바꾸면 vtable 이 정의를 요구한다.** 그 헤더를 쓰지만 해당 `.cpp` 를
링크하지 않던 타겟(`ADSWManager`, `ADSWServiceRegistryTest`)이 깨진다.
이 저장소의 mock 은 gmock 서브클래스가 아니라 **링크 심**이므로,
실제 `.cpp` 를 링크하지 말고 `MockContainerController.hpp` /
`MockContainerHealthMonitor.hpp` 에 심 정의를 추가하는 것이 맞다. 그렇게 했다.

### 6-10. 로거 정리 (수정함)

세 가지가 섞여 있었다.

1. **dangling-else.** 매크로 본문이 `do { } while (0)` 이 아니라 그냥 `{ }` 였다.
   `if (c) LOG_DEBUG(x); else y;` 가 깨진다. 그렇게 쓴 곳은 아직 없었다.
2. **레벨 가드가 너무 늦었다.** 필터는 `Logger::log()` 의 `if (level > logLevel) return;`
   인데, 거기 닿기까지 stringstream 생성 + `getpid()` 시스콜 + 전체 포맷팅 +
   `str()` 할당 + `debug(std::string)` 값전달 복사 + `log(std::string)` 또 복사가
   전부 끝난 상태였다. 버려질 DEBUG/VERBOSE 도 값을 다 치렀다.
   → `Logger::ShouldLog(LogLevel)` 을 추가하고 매크로가 **조립 전에** 물어보게 했다.
3. **값 전달.** `critical/error/info/debug/verbose/log` 가 `const std::string` 을
   값으로 받았다 (cppcheck `passedByValue` 로 두 모듈 각각 12건). `const std::string&` 로.

mock 심(`mock_logger.hpp` / `MockLogger.hpp`)도 같이 고쳤다. 테스트에서
`ShouldLog` 는 항상 true 를 반환한다 — 그래야 기존 `EXPECT_CALL(debug/info)` 가
그대로 유효하고 LOG_ 본문 커버리지도 이전과 같게 유지된다.

> 커버리지 영향: 가드가 LOG_ 라인에 `if` 를 하나 더 만든다. 하지만
> `coverage-modules.sh` 의 LOG_ 제외 지표(6-2 / 4-A)가 그 라인을 빼므로
> 게이트용 수치는 흔들리지 않는다.

### 6-11. general(SiLS) 판에서 새로 찾은 것

general 을 범위에 넣으며 세 가지가 드러났다.

**(a) 널 트랜스미터 역참조 — 두 판 모두 (수정함)**

`ProcessService{Start,Stop}Request` 마지막 줄이 트랜스미터를 널 검사 없이
역참조했다. 같은 함수 안의 registry / monitor 는 전부 검사하는데 여기만 빠져
있었다. 테스트를 붙이자마자 코어를 떨궈서 발견했다.
`general/ContainerServiceController.cpp` 2곳, `ros2/ROSServiceController.cpp` 2곳
모두 가드를 넣었다.

**(b) NotifyServiceState 가 항상 빈 목록을 보낸다 — 두 판 모두 (미수정)**

`ServiceRegistry::AddService` 는 group 이 비면 **거부한다.** 따라서 `group_[""]`
는 절대 생기지 않는데, `ServiceMonitor` 는 두 곳에서 `GetServicesByGroup("")` 로
"전체 서비스"를 얻으려 한다 (`ServiceMonitor.cpp:87`, `:219`).

- `NotifyServiceState()` 가 `service_list` 를 **빈 채로 publish** 한다.
- 인자 없는 `UpdateServiceState()` 는 아무 서비스도 순회하지 않는다.

lifecycle_mgr 의 `ServiceRegistry` 는 빈 그룹을 허용해서 `""` 가 실제로 "전체"로
동작한다. 그쪽 코드를 adsw_manager 로 가져오면서 `AddService` 의 규약 차이가
따라오지 않은 것으로 보인다.

고치면 DDS 로 나가는 내용이 바뀌므로 **별도 이슈로 둔다.** 현재 동작은
`KnownDefect_*` 테스트 2건으로 고정해 뒀다 — 고칠 때 그 둘이 먼저 깨지며 알려 준다.

**(c) `Save()` 를 `LoadServiceList()` 전에 부르면 조용히 실패한다 (미수정)**

`yaml_path_` 가 인자 없는 `LoadServiceList()` 안에서만 정해진다. 그 전에 `Save()`
를 부르면 빈 경로로 파일을 열어 실패한다. 실사용 순서는 로드 → 저장이라 문제가
드러나지 않았다. 이것도 테스트로 고정만 해 뒀다.

### 6-12. general 테스트를 붙이며 쓴 방법

ros2 판과 같은 두 가지 장치를 썼고, 새 mock 인프라는
[unittest/mock/](../../service/extension/general/adsw_manager/unittest/mock) 에 있다.

| 파일 | 역할 |
|---|---|
| `MockLogger.hpp` | Logger 링크 심 + `ScopedMockLogger` RAII |
| `MockADSWManager.hpp` | ADSWManager 싱글턴 링크 심. general 은 `SetShared()` 가 private 이라 심이 `Get()` 반환 포인터를 직접 들고 `SetMockADSWManager()` 로 갈아끼운다 |
| `FakeCmdContainerController.hpp` | `RunCommand`/`CaptureCommand` 를 가로채는 fake |

**링크 심은 반드시 외부 링키지여야 한다.** 정의에 `inline` 을 붙이면 그 헤더를
include 하지 않는 TU(`ContainerController.cpp` 등)에서 심볼을 못 찾는다.
그래서 이 헤더들은 **테스트 바이너리당 정확히 한 TU 에서만** include 해야 한다.

---

## 7. 이후 계획

### Phase 1 — 정적분석 선별 수정 — **완료 (P1 43건 → 0건)**

[.clang-tidy](../../.clang-tidy) 가 `bugprone-*`, `clang-analyzer-*` 를
`WarningsAsErrors` 로 지정해 둔 것이 P1 의 경계선이다.

| 우선순위 | 대상 | 처리 |
|---|---|---|
| P1 | `bugprone-*`, `clang-analyzer-*`, cppcheck `error` | **전건 처리** — 실제 결함 가능성 |
| P2 | `performance-*`, cppcheck `warning` | 실질 영향 있는 것만 수정 |
| P3 | `readability-*`, `cppcoreguidelines-*`, cpplint | 수정하지 않음. `.clang-tidy` 억제 목록 또는 `// NOLINT(check)` 로 정리하고 **억제 이유를 문서화** |

**43건을 액면 그대로 다 고친 게 아니다.** 분류부터 했다.

| 체크 | 건수 | 처리 |
|---|---|---|
| `clang-analyzer-deadcode.DeadStores` | 4 | **수정** — 실제 결함 신호 |
| `bugprone-narrowing-conversions` | 9 | **수정** — 명시적 캐스트 |
| `bugprone-branch-clone` | 9 | **정리 2 / 억제 7** |
| `bugprone-macro-parentheses` | 2 | **억제** — 괄호를 치면 컴파일이 깨진다 |
| `bugprone-easily-swappable-parameters` | 15 | **검사 제외** — 결함이 아니라 설계 지적 |
| cppcheck `error` | 4 | **분석 제외** — 생성된 IDL 헤더 |

#### 수정한 것

**DeadStores 4건 — 가장 의심스러웠던 항목.**

- `ROS2LifecycleController::RestartNode` 가 deactivate / cleanup 의 반환값을
  받아만 두고 안 읽었다. 클래스의 다른 메서드는 전부 `system()` 뒤에 `rc != 0`
  을 검사하는데 여기만 빠져 있어 "검사를 빠뜨린 것"처럼 보였다.
  실제로는 의도가 맞다 — 노드가 이미 inactive/unconfigured 면 두 전이는 정당하게
  실패하고, Restart 는 어떤 상태였든 configure → activate 로 되돌리는 게 목적이다.
  `RestartNodeFailureOn{Configure,Activate}` 테스트가 그 계약을 고정하고 있다.
  → 동작은 그대로 두고 rc 를 로그에 남겨 **관용이 의도임을 코드에 드러냈다.**
- `ROSServiceController::ProcessService{Start,Stop}Request` 가
  `message->is_group()` 을 읽자마자 두 줄 뒤에서 `false` 로 덮어썼다.
  요청 필드가 읽히자마자 버려지고 있었다. `const bool is_group = false;` 로
  죽은 저장만 없앴다.
  → 부수 발견: `is_group` 이 항상 false 라 **아래 `if (is_group)` 그룹 분기는
    도달하지 않는다.** 지우거나 실제 환경 판정을 붙이는 건 동작 변경이라
    별도 이슈로 남긴다.

**narrowing-conversions 9건** — 전부 컨테이너 크기·카운터 → `int32_t` 다
(`map::size()`, `vector::size()`, `uint32_t` 재시작 횟수, `uint64_t` 서비스 수).
실제로 넘칠 수 있는 값이 아니라 `static_cast<int32_t>` 로 의도를 명시했다.

**branch-clone 중 정리한 2건** — 이건 진짜 중복이었다.

- `ContainerController::BaseRos2Command` 의 switch 는 K3S / DOCKER /
  DOCKER_COMPOSE 세 분기가 **전부 같은 문자열**을 반환했다. `default` 분기도
  도달하지 않는다 (알 수 없는 타입이면 `BaseCommand` 가 "" 를 주고 그 위
  조기 반환에 걸린다). switch 를 걷어냈다.
- `adsw Service::SetRunningNodes` 의 `if (prev > cur) / else if (prev < cur)` 는
  **두 분기 본문이 모두 비어** 있었다. 빈 분기와 거기서만 쓰이던 `prev_nodes` 를
  함께 제거했다.
- `adsw ServiceRegistry` 의 `recovery_to_str` 은 `MANUAL` 과 `R_DEFAULT/default`
  가 같은 문자열이라 라벨을 묶었다.

#### 억제한 것 — 이유

**branch-clone 5건 (`policy/`)** → [policy/.clang-tidy](../../service/core/lifecycle_mgr/policy/.clang-tidy)
정책 클래스들은 `ServiceState` 를 빠짐없이 열거하고 각 case 에 "왜 그 동작인지"
개별 주석을 단다. 결과가 같은 상태가 여럿이지만 (PolicyManual 은 5개 전부 NONE)
합치면 안 된다 — 같은 NONE 이라도 이유가 다르고, `default` 로 접으면 `ServiceState`
에 값이 추가될 때 **정책 검토 없이 조용히 흡수된다.** exhaustive switch 가 그걸
막는 장치이고 이 검사와 정면으로 상충한다.
NOLINT 를 쓰지 않은 이유: 진단이 switch 문이 아니라 **첫 번째 중복 case 줄**에
붙어서 억제 주석을 switch 한가운데 끼워 넣어야 하고 파일마다 위치도 다르다.

**branch-clone 1건 (`ServiceMonitor`)** → `NOLINTNEXTLINE`
`CREATED` 와 `STATE_UPDATED` 가 지금은 같은 동작이다. CREATED 가 전달받은 state 를
그대로 존중하는 건 버그를 잡고 얻은 결론이고 (코드의 긴 주석이 그 경위를 설명한다),
두 이벤트는 의미가 달라 언제든 다시 갈릴 수 있다.

**macro-parentheses 2건 (`Log.hpp`)** → `NOLINTBEGIN/END`
`msg` 는 완결된 식이 아니라 `"a" << x << "b"` 같은 스트림 삽입 조각이다.
`(msg)` 로 감싸면 `("a" << x)` 가 되어 스트림이 아닌 `const char*` 에
`operator<<` 를 적용하게 되고 **컴파일이 깨진다.** 인자를 문자열 하나로
제한하지 않는 한 고칠 수 없는 지적이다.
`NOLINTNEXTLINE` 이 아니라 `NOLINTBEGIN/END` 인 이유: 진단이 `#define` 줄이 아니라
매크로 본문의 `<< msg` 줄에 붙는데, 그 줄은 백슬래시로 이어진 매크로 내부라
주석을 끼우면 줄 이음이 깨진다.

#### 검사에서 뺀 것

**`bugprone-easily-swappable-parameters` 15건** → [.clang-tidy](../../.clang-tidy)
이름은 `bugprone-*` 이지만 실제 결함이 아니라 "같은 타입 인자가 인접해 순서를
바꿔 넣기 쉽다" 는 설계 지적이다. P1 43건 중 최대 덩어리인데, 고치려면 시그니처와
모든 호출부를 바꾸거나 strong typedef 를 도입해야 한다. 이 코드베이스는
`(name, image, deploy_file, namespace)` 처럼 문자열 여러 개를 받는 생성자가
도메인상 자연스러워 새 함수마다 계속 잡힌다. `bugprone-*` 는 `WarningsAsErrors`
라 남겨두면 **실제 결함이 이 소음에 묻힌다.**

> 주의: `Checks` 목록은 **뒤에 오는 항목이 이긴다.** 부정을 `bugprone-*` 앞에
> 두면 뒤의 `bugprone-*` 가 도로 켠다. 반드시 뒤에 둘 것.

**cppcheck `error` 4건** → [analyze-modules.sh](../../scripts/analyze-modules.sh) 의
`--suppress=preprocessorErrorDirective:*/generated/idl/*`
fastddsgen 이 만든 `*PubSubTypes.hpp` 상단의 FastDDS 버전 가드 `#error` 다.
cppcheck 가 버전 매크로를 못 풀어서 나는 것이고 우리 코드가 아니다.

#### 결과

| | lifecycle_mgr | ros2/adsw_manager |
|---|---|---|
| P1 | 18 → **0** | 29 → **0** |
| clang-tidy unique | 663 → **643** | 754 → **729** |
| cppcheck | 117 → **104** | 97 → **74** |

테스트는 두 모듈 모두 실패 0건으로 동일하고, 커버리지도 유지된다
(lifecycle 84.04% / adsw 83.39%).

### Phase 2 — 유닛테스트 갭 메우기 — **대부분 완료**

미커버 라인 수(% 아님) 기준으로 우선순위를 매기고 위에서부터 처리했다.

| 대상 | 미커버(전) | 처리 | 추가 테스트 |
|---|---|---|---|
| ~~`adsw/service/ServiceRegistry.cpp`~~ | ~~434줄~~ | **완료** — 6-5 링크 버그 수정 | (기존 67건이 살아남) |
| ~~`adsw/control/ContainerController.cpp`~~ | ~~184줄~~ | **완료** — 6-9 실행 seam 회수 | 47건 |
| ~~`lifecycle/state/StateManager.cpp`~~ | ~~186줄~~ | **완료** — ctrl/* 핸들러 9종 | 28건 |
| ~~`adsw/monitor/ContainerHealthMonitor.cpp`~~ | ~~143줄~~ | **완료** — 6-9 폴링 분리 | 22건 |
| ~~`lifecycle/service/ServiceRegistry.cpp`~~ | ~~103줄~~ | **완료** — F013 컨테이너 API | 12건 |
| ~~`lifecycle/include/TypeConvert.hpp`~~ | ~~43줄~~ | **완료** — 전수 왕복 테이블 | 8건 |
| `adsw/ADSWManager.cpp` | 58줄 | **보류** — 아래 참조 | — |
| `lifecycle/manager/LifecycleManager.cpp` | 46줄 | 미착수 | — |

결과는 4-A 참조. 라인 커버리지 lifecycle 71.63% → **84.03%**,
adsw 48.18% → **83.32%**, 0% 파일 5개 → **0개**.

**`ADSWManager.cpp` 를 보류한 이유.** `ADSWManagerTest` 5건이 유닛 레인에서
제외된 건 실제 ROS2 DDS participant 가 필요해서다. 유닛으로 회수하려면
`ADSWManager::Initialize()` 가 `std::make_shared<DDSTransmitter>()` 로 직접
만드는 것을 주입 가능하게 바꿔야 하는데, 여긴 애플리케이션 루트 싱글턴이다.
**58줄을 얻으려고 앱 루트 생성 경로를 흔드는 건 수지가 안 맞는다.**
통합 레인에 그대로 둔다. 나중에 DI 를 도입한다면 그때 같이 옮길 것.

**남은 것**

1. `lifecycle/manager/LifecycleManager.cpp` 46줄
2. 5절의 미연결 소스 중 아직 남은 것 (mock 은 이미 다 있다)
3. 6-3 의 제외 테스트 중 회수 가능한 나머지

**목표치** — 1차 75%, 2차 80% 는 두 모듈 모두 달성했다. 다음 목표를 잡는다면:

- 라인 **85%** + 0% 파일 0개 유지
- 브랜치는 게이트로 쓰지 말 것 (4-A 참조). 참고치로만 기록한다.
  굳이 걸면 `SUMMARY.md` 의 LOG_ 제외 열(현재 lifecycle 56.59% / adsw 51.29%)을 쓴다.

### Phase 3 — CI 게이트 — **판정 스크립트 완료 / Jenkins 연결 미착수**

#### 왜 필요한가

지금까지 만든 상태(P1 0건, 라인 84/83/79%, 0% 파일 0개)는 **게이트가 없어서
다음 커밋 한 번에 조용히 되돌아간다.** 현재 CI 는 이렇다.

| 게이트 | 현재 설정 | 실측 | 문제 |
|---|---|---|---|
| clang-tidy | [Jenkinsfile](../../Jenkinsfile) `threshold: 1000, TOTAL, unstable` | 모듈 3개만 재도 1,973 | 트리 전체는 더 많다. 이미 켜져 있는데 무시되는지, 리포트가 다른 걸 세는지 **확인 필요** |
| 테스트 성공률 | [Jenkinsfile.unittest](../../Jenkinsfile.unittest) `< 50%` | 97.81% | 절반이 깨져도 통과 |
| 커버리지 | `recordCoverage` 기록만, `failOnError: false` | 78~84% | 하한 없음 |
| 빌드 무결성 | 없음 | — | `make -k … \|\| true` 가 링크 실패를 삼킨다 (6-5-1) |

#### [scripts/ci-gate.sh](../../scripts/ci-gate.sh) — 완료

판정 로직을 Groovy 가 아니라 셸에 뒀다. **로컬에서 그대로 돌려볼 수 있어서**다 —
개발자가 커밋 전에 확인할 수 있고, Jenkins 는 같은 스크립트를 호출만 하면 된다.
`analyze-modules.sh` / `coverage-modules.sh` / `run_unittest.sh` 가 이미 만든
산출물을 읽기만 하므로 컨테이너도 필요 없다 (python3 만 있으면 된다).

```bash
./scripts/ci-gate.sh              # 전체 판정 (미달이면 exit 1)
./scripts/ci-gate.sh --warn-only  # 미달이어도 exit 0 (도입 초기)
./scripts/ci-gate.sh --skip-build # .build 없을 때
```

판정 5종:

| 항목 | 기준 | 근거 |
|---|---|---|
| 측정 유효성 | `fatal/include-error == 0` | 0 이 아니면 파싱 실패한 파일이 있고 그 건수는 애초에 기준으로 못 쓴다. 다른 판정보다 먼저 본다 |
| P1 | **0건** | 세 모듈 모두 0 이라 하나만 새로 생겨도 잡힌다. 이 게이트에서 가장 값어치 있는 항목 |
| 라인 커버리지 | lifecycle 80 / ros2 80 / general 75 | 실측(84.04 / 83.39 / 78.70)에서 3~4%p 여유. 여유를 두는 이유는 분모가 흔들릴 때마다 붉어지면 게이트를 꺼버리게 되어서다 |
| 라인 0% 파일 | 0개 | 전체 %가 기준을 넘어도 통째로 안 도는 파일이 새로 생겼다면 테스트가 링크에서 빠졌다는 신호다 |
| 테스트 성공률 | 95% (실행분 기준) | Jenkins 의 `successRateRun` 과 **같은 정의**를 쓴다. 두 곳이 다른 숫자를 보면 판정이 어긋난다 |
| 빌드 무결성 | 등록 타겟마다 바이너리 존재 | 6-5 재발 방지 |

기준값은 `GATE_*` 환경변수로 덮어쓸 수 있다 (`GATE_LINE_MIN_general=70 ./scripts/ci-gate.sh`).

**브랜치 커버리지는 게이트로 쓰지 않는다.** 분모의 절반 이상이 `LOG_*` 매크로에서
나오고(4-A) 그 대부분은 libstdc++ 내부라 어떤 테스트로도 실행되지 않는다.

**빌드 무결성의 불변식**은 "등록 == 생성" 이 아니라
"**`gtest_discover_tests` 로 등록한 타겟마다 바이너리가 있어야 한다**" 다.
반대(바이너리는 있는데 등록은 안 된 것)는 정상이다 — `gtest_discover_tests` 를
일부러 주석 처리해 둔 타겟이 실제로 있다 (`ADSWROSServiceControllerTest`).
주석 줄을 먼저 걷어내지 않으면 없는 바이너리를 찾게 된다.

#### 검증한 것

통과만 확인하면 게이트가 아무것도 안 잡아도 모른다. 실패 조건을 하나씩 주입해
확인했다.

| 주입 | 결과 |
|---|---|
| 커버리지 하한 상향 (`GATE_LINE_MIN_general=85`) | FAIL, exit 1 |
| `--warn-only` 로 같은 조건 | FAIL 출력 + exit 0 |
| 성공률 하한 101% | 3개 모듈 전부 FAIL |
| `by-check.txt` 에 `bugprone-*` 1건 추가 | P1 FAIL |
| cppcheck XML 에 `severity="error"` 1건 추가 | P1 FAIL |
| 테스트 바이너리 1개 숨김 (링크 실패 재현) | 빌드 무결성 FAIL + 원인 안내 |
| `fatal_or_missing_include=3` 주입 | 측정 무효 FAIL |
| 산출물 디렉토리 삭제 | exit 2 + 무엇을 먼저 돌릴지 안내 |
| 원복 후 | 전부 통과, exit 0 |

#### 남은 것 — Jenkins 연결 (미착수)

아래 4곳을 고쳐야 한다. **Jenkins 가 없어 여기서 검증할 수 없으므로 실제 빌드를
한 번 돌려봐야 한다.**

1. [Jenkinsfile](../../Jenkinsfile) Clang-tidy 스테이지 — P1(`bugprone-*` /
   `clang-analyzer-*`)만 뽑은 리포트를 하나 더 만들고 `threshold: 1` 을 건다.
   전체 threshold 는 실측에 맞춰 올린 뒤 P2/P3 처리하며 내린다.
2. [Jenkinsfile.unittest](../../Jenkinsfile.unittest) 289줄 — `successRate < 50.0`
   → `< 95.0`.
3. 같은 파일 Coverage 스테이지 뒤 — `./scripts/ci-gate.sh` 호출.
4. 같은 파일 Build 스테이지 뒤 — 빌드 무결성 검사 (또는 3번의 게이트에 포함).

도입 초기에는 `--warn-only` 로 두어 기존 빌드를 붉히지 않고 수치만 보다가,
안정되면 떼는 방식을 권한다.

## 8. 참고

- 스크립트: [`scripts/analyze-modules.sh`](../../scripts/analyze-modules.sh),
  [`scripts/coverage-modules.sh`](../../scripts/coverage-modules.sh)
  (각 스크립트 `--help` 에 상세 사용법)
- 산출물 경로 `output/`, `.build/` 는 `.gitignore` 에 포함되어 커밋되지 않는다.
- 통합 테스트 절차: [integration-test-procedure-guide.md](./integration-test-procedure-guide.md)
- 빌드·설치·실행: [build-install-run-guide.md](./build-install-run-guide.md)
