# ext-ros2 adsw_manager 운영 가이드 — autoware 관리·기동·트러블슈팅

> ext-ros2 컨테이너의 **adsw_manager** 가 무엇을 관리하고, autoware 를 어떻게 기동/정지하며,
> 실제로 겪은 함정을 어떻게 피하는지 정리한 **운영(사용) 가이드**. 빌드·설치는
> [build-install-run-guide.md](build-install-run-guide.md), 자동시작은
> [board-autostart-guide.md](board-autostart-guide.md) 참조.
>
> ⚠️ 이 문서는 2026-07 다보드(67/70→151/162) 운영 중 실제로 발생한 이슈
> (컨테이너 안 뜸 / 상태 flapping / 맵 없음 / 헤드리스 perception)를 근거로 작성됐다.
> 아래 **§2 필수 전제조건** 을 건너뛰면 대부분 재발한다.

---

## 1. ext-ros2 가 제공/관리하는 것

ext-ros2 컨테이너 1개 안에서 아래가 함께 돈다:

| 구성요소 | 역할 |
|---|---|
| **adsw_manager** | lifecycle_mgr 에 서비스 등록 + **docker 로 컨테이너 워크로드 직접 관리**(start/stop/restart) |
| **dds_bridge** | raw DDS ↔ ROS(rt/) 브리지 — rosbridge/HMI 가 lifecycle 토픽을 받게 함 |
| **ota_client** | OTA 등록/상태 |
| **rosbridge** | WebSocket(:9090) — HMI(lfc_monitor 등)가 붙는 지점 |

**adsw_manager 가 기본 관리하는 서비스** = `saved-service-list.yaml`(이미지에 baked)에 정의된
**parted Autoware 3개** (container-workload):

| 서비스 | 이미지 | 실행 |
|---|---|---|
| autoware-perception | oss-adsw-perception-aarch64:field | `docker run` (adsw가 직접) |
| autoware-decision | oss-adsw-decision-aarch64:field | 동일 |
| autoware-control | oss-adsw-control-aarch64:field | 동일 |

- `exec_type: container` → adsw 가 StartService 시 **`docker run`**, StopService 시 **`docker stop`+`rm`** 한다 (컨테이너 CMD 가 워크로드; 별도 `docker exec ros2 launch` 안 함).
- `recovery: MANUAL` → 운영자가 **lfc_monitor 에서 명시적으로** start/stop (자동 재기동 안 함).
- autoware 는 `ROS_DOMAIN_ID=42` 로 **MMF(domain 0)와 격리**.

---

## 2. 필수 런타임 전제조건 (건너뛰면 안 됨)

ext-ros2 컨테이너를 `docker run` 할 때 **아래가 없으면 조용히 실패**한다. `run-mmf.sh` 에는
모두 들어 있으므로 **반드시 run-mmf.sh(또는 동등 옵션)로 기동**할 것 — 수동으로 일부만
빠뜨리면 아래 증상이 난다.

| 옵션 | 없으면 생기는 증상 (실측) |
|---|---|
| **`-v /var/run/docker.sock:/var/run/docker.sock`** + **`--group-add <docker-gid>`** | ⛔ **adsw 가 컨테이너를 못 만든다.** StartService 는 lifecycle 레벨에선 "Success" 응답하지만 실제 `docker run` 이 `failed to connect to the docker API at unix:///var/run/docker.sock: no such file or directory` 로 실패 → **autoware 컨테이너가 생성 안 됨.** (2026-07 151 보드 실제 사례) |
| **`-v /dev/shm:/dev/shm`** | DDS SHM 전송 불가 / 크로스 프로세스 discovery 저하 |
| **`--network host`** | rosbridge·DDS 가 호스트 네트워크에 안 붙음 (HMI 연결/크로스호스트 DDS 실패) |
| **`--privileged --cap-add NET_RAW --cap-add NET_ADMIN`** | DDS 멀티캐스트/네트워크 권한 부족 |
| **`-e MMF_DDS_DOMAIN_ID=<n>`** (같은 보드 core 와 동일값) | 도메인 불일치 → core↔adsw 통신 끊김 (§4 참조) |

**docker gid 확인**: `getent group docker | cut -d: -f3` (보드마다 다름 — 예 999).

> 검증: `docker exec oss-mmf-ext-ros2 docker ps` 가 동작하면 docker.sock OK.
> 안 되면 `no such file or directory` → docker.sock 마운트 누락.

---

## 3. 보드별 서비스 설정 (saved-service-list.yaml 오버라이드)

baked yaml 은 **개발 보드(goasp) 기준 고정값**이라, 다른 보드에선 **bind-mount 로 교정**해야
한다. 특히 아래 2가지는 보드마다 다르다:

### 3-1. 맵 경로
- baked: `-v /home/goasp/autoware_map:/autoware_map`
- 다른 보드(예 beyhpc)는 맵이 `/home/beyhpc/autoware_map/...` 에 있음 → **경로가 안 맞으면
  빈 디렉토리가 마운트돼 autoware 가 맵을 못 읽는다.** (2026-07 151: `/home/goasp/autoware_map`
  가 빈 디렉토리였음)
- 교정: 보드의 실제 맵 경로로 `saved-service-list.yaml` 을 bind-mount.

### 3-2. 헤드리스 보드의 perception (rviz)
- baked perception 은 **디스플레이 전제**: `-e DISPLAY=:1 -v /tmp/.X11-unix:/tmp/.X11-unix`
  + `adsw-perception.sh`(내부 `rviz:=true`).
- **디스플레이 없는 보드(예 151)** 에선 rviz 가 못 떠 perception 이 실패/불안정 → **perception
  을 `rviz:=false`(헤드리스)로** 실행해야 한다. rviz 시각화는 **디스플레이 있는 별도 보드**에서
  rviz2 를 로컬 실행하고 cross-host DDS(domain 42)로 구독한다 (분산 rviz — §5).

### 3-3. 오버라이드 방법
```bash
# 보드에 교정한 yaml 배치 후, ext-ros2 를 bind-mount 로 재생성:
docker run -d --name oss-mmf-ext-ros2 --restart unless-stopped \
  --privileged --network host --cap-add NET_RAW --cap-add NET_ADMIN \
  --group-add "$(getent group docker | cut -d: -f3)" \
  -v /dev/shm:/dev/shm \
  -v /var/run/docker.sock:/var/run/docker.sock \
  -v ~/adsw-service-list.yaml:/oss/bin/config/saved-service-list.yaml:ro \
  -e ROS_DOMAIN_ID=0 -e MMF_DDS_DOMAIN_ID=0 \
  registry.obigo.com/oss/oss-mmf-ext-ros2:latest
```

---

## 4. autoware 기동/정지 (lfc_monitor / ctrl)

adsw 가 관리하는 서비스는 **lifecycle 제어(ctrl/StartService·StopService)** 로 조작한다.
HMI 의 **lfc_monitor Start/Stop 버튼**이 이 경로를 쓴다.

- **흐름**: lfc_monitor(또는 rosbridge publish) → `ctrl/StartService{service_list:[name]}`
  → lifecycle_mgr 가 registry 의 mw_type 으로 라우팅 → `StartServiceRequest(mw_type=ROS2)`
  → ext-ros2 adsw 가 `docker run` (container-workload).
- **rosbridge 로 직접 트리거**(디버깅용):
  ```bash
  # ws://<board>:9090 에 advertise 후 publish
  #   topic /mmf/lifecyclemgr/ctrl/StartService  type lifecyclemgr/CtrlStartServiceRequest
  #   msg   {"service_list":["autoware-perception"]}
  ```
- **검증**: 보드에서 `docker ps | grep autoware` 로 컨테이너 생성 확인 +
  `docker exec oss-mmf-core grep "running service num" /oss/lifecycle_mgr.log | tail`.
- start 를 눌렀는데 컨테이너가 안 생기면 → **§2 docker.sock 부터 확인** (core 로그엔 Success 로
  보여도 adsw 가 docker run 못 한 것).

---

## 5. 분산 rviz (헤드리스 보드 + 별도 디스플레이)

- autoware = 헤드리스 보드(151)에서 실행 (perception `rviz:=false`).
- rviz2 = 디스플레이 있는 보드(예 49)에서 **로컬 실행** + 같은 `ROS_DOMAIN_ID=42` 로
  cross-host DDS 구독. **보드→보드 X11 forwarding 금지**(OGRE/GLX 실패).
- 상세 레시피는 별도 문서(HMI 리포 `autoware-distributed-rviz-guide`)의 패턴과 동일.

---

## 6. 트러블슈팅 (2026-07 실측 이슈 → 원인 → 조치)

| 증상 | 원인 | 조치 |
|---|---|---|
| lfc_monitor 에서 **autoware 가 Running↔NotRunning 반복(flapping)** | **같은 DDS domain·서브넷에 MMF core 가 2대 이상**(예: 보드 + 개발 PC 로컬 MMF). lfc 가 두 core 의 DiagReport 를 번갈아 받음 | 한 domain 에 **core 1대만**. 개발 PC 로컬 MMF(core/ext-ros2/ext-agl/autoware)를 **stop**, 또는 보드별 `MMF_DDS_DOMAIN_ID` 분리 |
| lfc_monitor 에서 autoware 가 **셋업 중 깜빡임** | 보드에서 컨테이너가 반복 생성/제거(churn) 중 → lfc 연결/해제 반복 | 셋업 완료(안정 running)까지 대기. `docker inspect ... {{.State.Status}}` 5회 관측으로 안정 확인 |
| **Start 눌러도 autoware 컨테이너 안 생김** | ext-ros2 에 **docker.sock 미마운트** → adsw 가 docker run 실패(Success 응답은 lifecycle 레벨) | §2 대로 docker.sock+group-add 포함해 ext-ros2 재생성 |
| autoware 는 뜨는데 **맵/planning 이 비어있음** | yaml 맵 경로가 실제 맵과 불일치(빈 디렉토리 마운트) | §3-1 맵 경로 교정 bind-mount |
| **perception 만 실패/불안정** | 헤드리스 보드인데 rviz:=true(DISPLAY=:1) | §3-2 perception `rviz:=false` + rviz2 는 별도 디스플레이 보드 |
| core↔adsw 가 서로 못 봄 | 같은 보드인데 core 와 ext-ros2 의 `MMF_DDS_DOMAIN_ID` 불일치 | 같은 보드는 **동일 domain** |

**진단 로그 위치 (중요)**:
- lifecycle_mgr DEBUG 는 **stdout 이 아니라 컨테이너 내 파일** → `docker exec oss-mmf-core cat /oss/lifecycle_mgr.log` (running service num, SetServiceState, trusting adsw prefix 등).
- adsw DEBUG → `docker exec oss-mmf-ext-ros2 cat /oss/adsw.log` (docker run 실패 사유 등).
- dds_bridge / rosbridge → `docker logs oss-mmf-ext-ros2` (stdout: `[bridge] ... forwarded`, rosbridge 에러).
- lfc 가 실제 받는 것 = 보드 rosbridge(:9090)에서 `LifecycleDiagReport`/`NotifyServiceState` 구독(rosbridge subscribe 시 **type 생략**하면 추론돼 수신됨).
