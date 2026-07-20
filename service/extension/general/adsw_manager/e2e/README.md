# 경로 B e2e 스모크 — 실 lifecycle_mgr ↔ general adsw_manager ↔ docker

실 `lifecycle_mgr`(core) 와 실 `adsw_manager_general`(general 확장) 을 같은 DDS
도메인에 띄우고, **인젝터/ctrl 명령 없이** `recovery: ALWAYS` speclist 로 컨테이너
자동 기동을 유발해 경로 B 전체를 라이브로 검증한다.

## 동작 원리 (왜 트리거가 필요 없나)

```
adsw_manager_general 기동
  → UpdateServiceList(container_list + service_list) 발행
lifecycle_mgr
  → ServiceRegistry.UpdateServiceList 로 컨테이너/서비스 등록
  → ProcessServiceHandling (등록 직후 + StateMonitor 주기)
  → PolicyAlways.Evaluate: 서비스 NOT_RUNNING → START
  → StartServiceRequest → adsw_manager
adsw_manager (ContainerServiceController.ProcessServiceStartRequest)
  → ContainerController.StartContainer → docker run <sils-vecu 이미지>
```

`recovery: MANUAL` 이면 PolicyManual 이 항상 NONE 을 반환해 자동 기동하지 않는다
(운영자 ctrl 명령 필요). 그래서 이 e2e 전용 speclist(`config/saved-service-list.yaml`)
는 `ALWAYS` 로 둔다.

## 전제

1. **core 빌드**(lifecycle_mgr): `BUILD_EXTENSION=FALSE` (기본). → `output/<arch>/<build>/core/bin/lifecycle_mgr`
2. **general 빌드**(adsw_manager_general): `EXTENSION_PLATFORM=GENERAL`. → `output/<arch>/<build>/extension/general/bin/adsw_manager_general`
3. **docker 데몬** 접근 가능
4. **FastDDS 런타임(.so)** 로드 가능 (`FASTDDS_SETUP` 로 setup.bash 지정 가능)
5. `oss-mmf-sils-vecu` 이미지 (speclist 참조; 없으면 `SILS_VECU_IMAGE` 로 override + pull)

## 실행

```bash
# (선택) FastDDS 런타임 경로
export FASTDDS_SETUP=$HOME/obigo-sdk/Fast-DDS/output/ros2_x86_64/setup.bash

service/extension/general/adsw_manager/e2e/run_lifecycle_e2e.sh
```

성공 시 `docker ps` 에 `sils-vecu-viss-probe` 가 running 으로 나타나고 `PASS ✅`.
실패 시 두 프로세스 로그(마지막 40줄)를 출력한다.

## 환경변수 (선택)

| 변수 | 기본 | 설명 |
|---|---|---|
| `OUTPUT_ROOT` | `<repo>/output/x86_64/Debug` | 빌드 산출물 루트 |
| `MMF_DDS_DOMAIN_ID` | `0` | 두 프로세스 공통 DDS 도메인 |
| `SILS_VECU_IMAGE` | (speclist 값) | 이미지 override + pull |
| `FASTDDS_SETUP` | (없음) | source 할 setup.bash |
| `TIMEOUT_SEC` | `60` | 컨테이너 기동 대기 상한 |

## 범위 밖

- **정지 검증**: ALWAYS 는 컨테이너가 죽으면 자동 재시작하므로 깔끔한 stop 검증이
  안 된다. start/stop 정밀 제어는 MANUAL + ctrl 퍼블리셔가 필요(후속).
- viss_probe 가 **의미있는** VISS 트래픽을 내려면 SUT(viss-server) peer 가 별도로
  같은 도메인에 있어야 한다. 이 스모크는 "컨테이너가 뜨는가"까지만 본다.
