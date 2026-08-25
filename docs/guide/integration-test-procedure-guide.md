# MMF 및 자율주행 SW 시스템 종합 통합 테스트 절차서

본 문서는 MMF(Multi-SW Framework), Autoware(자율주행 SW), Security/IDS(침입 감지) 및 HMI 앱 간의 종합 통합 테스트를 위한 표준 절차서 및 체크리스트입니다.

---

## 1. 개요 및 시스템 테스트 환경

### 1.1 개요
본 테스트는 공통 Docker Compose 구성을 기반으로 **Orin 타겟 보드(HMI/AGL)**, **2세부 HW(MMF Core/ROS2 Ext/Security/Autoware)**, 그리고 **EVB 보드(우분투 데스크탑/RViz)** 간의 상호 연동 및 동작을 검증하는 것을 목적으로 합니다.

### 1.2 공통 Compose 및 환경 설정 파일
- **레포지토리**: `https://github.com/OSS-TEST-GROUP/oss_multisw_framework.git`
- **컴포즈 파일**: `docker/compose/docker-compose-integration.yaml`
- **환경 변수 파일**: `docker/compose/.env.integration`
- **서비스 등록/설정 파일**: `docker/compose/saved-service-list.yaml`

### 1.3 하드웨어별 역할 분담 및 컴포넌트 구성

| 구분 | 대상 하드웨어 / 환경 | 구동 서비스 / 역할 | 비고 |
|---|---|---|---|
| **Orin 보드** | NVIDIA Jetson Orin (또는 대체 x86_64 환경) | • `mmf-ext-agl` (AGL Extension)<br>• `hmi-launcher` (HMI 애플리케이션 서비스) | `mmf-ext-agl` 기동 시 `hmi-launcher`가 자동 연동 실행됨<br>*(Orin 미보유 환경은 x86_64 데스크탑/노트북으로 대체 가능)* |
| **2세부 HW** | Main Computing HW (x86_64 / AADP 등) | • `mmf-core` (Framework Core)<br>• `mmf-ext-ros2` (ROS2 Extension)<br>• Security 3종 (`security_processor`, `attack_env`, `security_monitor`) <br>• Autoware 3종 (`oss-adsw-decision`, `oss-adsw-control`, `oss-adsw-perception`) | Autoware는 **이미지 다운로드(Pull)**만 완료하고 HMI 제어를 통해 동적 기동 |
| **EVB 보드** | Ubuntu Desktop 환경 보드 | • `oss-adsw-rviz` (RViz 시각화 서비스)<br>• 자율주행 시뮬레이션 | ROS2 토픽 및 주행 데이터 수발신 시뮬레이션 환경 |

> [!NOTE]
> **Orin 미보유 개발 환경 안내 (x86_64 지원)**
> Orin 타겟 보드가 없는 개발사/업체의 경우, 동일한 `docker-compose-integration.yaml` 및 AGL/HMI 구성을 일반 x86_64 우분투 데스크탑/노트북 환경에서 그대로 구동하여 동일하게 테스트를 수행할 수 있습니다.

---

## 2. 사전 준비 및 서비스 기동 절차

### Step 2.0: Orin 보드 AGL 이미지 플래싱 및 초기 인스톨 (최초 1회)

> 이 단계는 **Orin 타겟 보드를 처음 준비할 때만** 수행한다. 이미 구동 중인 보드는 Step 2.3 으로 간다.
> x86_64 대체 환경에는 해당하지 않는다.

AGL Yocto 이미지(`agl-ivi-demo-flutter`)에 **통합 compose 구성 일체가 베이크**되어 있다
(`oss-integration-compose` 패키지, 2026-08-10 기준 이미지). 따라서 플래싱한 보드에서는
Step 2.3 의 파일 배치·오버라이드 지정이 **대부분 불필요**하고, 부팅하면 서비스가 자동으로
기동된다.

| 이미지에 이미 들어 있는 것 | 보드 경로 |
|---|---|
| 통합 compose + 보드 오버라이드 | `/etc/oss/compose/docker-compose-integration.yaml`, `…-agl-board.override.yaml` |
| 보드 런타임 env (`HMI_RUNTIME_UID=1001`, `DOCKER_GID=999`, `ROS_DOMAIN_ID=42`) | `/etc/oss/compose/board.env` |
| AGL 서비스 목록 | `/opt/adsw/config/agl-service-list.yaml` |
| DDS 도메인 파일 | `/opt/mmf/config/domain.env` |
| 부팅 기동 유닛 / 프로비저닝 CLI | `oss-integration.service` / `oss-provision` |
| **컨테이너용 NVIDIA(Tegra) GL 스테이징** — 부팅 시 자동 (`ExecStartPre`) | `oss-nv-gl-stage` → `/opt/nv-gl` |
| **베이크된 컨테이너 3종**(`:provision`) — ext-agl · hmi_launcher · rviz2. 첫 부팅에 `docker load` 후 **`:latest` 로 태깅** → **오프라인 기동** | `/opt/oss/images/{oss-mmf-ext-agl,oss-hmi-launcher,rviz2-agl}.tar.zst`, `oss-container-preload` |
| 서비스 목록 2벌 (활성=minimal: 런처만 / full: 모니터 3종 포함) | `/usr/share/oss/agl-service-list.{minimal,full}.yaml` |
| **k3s** `v1.28.7+k3s1` + server/agent 유닛 (**둘 다 비활성** — 역할은 프로비저닝에서) | `/usr/local/bin/k3s`, `k3s.service`, `k3s-agent.service` |
| Docker 25.0.3 + **Compose v2.26.0** | — |
| 화면 방향 **가로(landscape) 기본** | — |
| **AGL 홈스크린 유지** (`flutter-ics-homescreen`) — 배경(BACKGROUND) 역할 | 런처가 NORMAL 풀화면으로 그 위를 100% 덮는다 |

> [!NOTE]
> **AGL 홈스크린은 제거하지 않는다** (2026-08-10 실측으로 확정). agl-compositor 는 출력에
> BACKGROUND surface 가 없으면 black curtain 을 걷지 않아 **화면이 완전히 검어진다**.
> 런처를 `--window-type BG` 로 띄워 홈스크린을 없앨 수도 있지만, 그러면 rviz2 실행 중
> 런처가 터치를 받지 못해(포커스가 cage 에 잔류) 패널로 rviz2 를 정지할 수 없다.
> 근거: `oss_hmi_app/docs/rviz2/board-launcher-fullscreen-guide.md` 4절.

#### Step 2.0.1: 이미지 플래싱

이미지는 AGL 빌드 에이전트(`agl_image_agent`)가 생성한 tegraflash 번들이다
(meta-oss `feature/jetson-aiml-trout-20.0.3`). 상세 절차는 해당 저장소의
`docs/guides/jetson-agx-orin-flash-ubuntu.md` 를 따르고, 요약하면 다음과 같다.

```bash
# 개발 PC(Ubuntu/Debian) 에서
sudo apt install -y python3 lbzip2 libxml2-utils
sudo systemctl stop ModemManager            # USB 점유 해제 (Return value 8 예방)

mkdir -p ~/jetson-flash && cd ~/jetson-flash
tar xzf agl-ivi-demo-flutter-p3737-0000-p3701-0005.rootfs-<빌드ID>.tegraflash.tar.gz

# 보드를 Force Recovery 로 진입시킨 뒤(FC REC 버튼 누른 채 전원/리셋) USB-C 연결
lsusb | grep -i nvidia                      # 0955:7023 등 APX 장치가 보여야 한다
sudo ./doflash.sh 2>&1 | tee flash-$(date +%Y%m%d-%H%M%S).log
```

##### 컨테이너 태그 정책 — `:provision`(베이크) → `:latest`(운영)

| 시점 | 동작 |
|---|---|
| **Yocto 빌드** | 레지스트리의 **`:provision`** 3종을 `/opt/oss/images/*.tar.zst` 로 굽는다 (약 774MB) |
| **첫 부팅** (`oss-container-preload.service`) | `docker load` → **`docker tag …:provision …:latest`** — 운영 태그를 오프라인에서 만든다 |
| **프로비저닝** (`oss-provision`) | 레지스트리에서 **`:latest`** 를 pull 해 최신본으로 갱신 |

`:provision` 태그는 보드에 남겨 둔다 — "이미지에 구워진 기준선"이라 롤백·추적에 쓴다
(`docker images` 에 같은 IMAGE ID 로 두 줄 보이지만 디스크는 1회분).
compose·서비스 목록은 모두 **`:latest`** 를 참조하므로, 오프라인 첫 부팅과 온라인 갱신이
**같은 태그 하나로** 동작한다 (목록을 다시 쓰거나 태그를 바꿀 필요 없음).

#### Step 2.0.2: 첫 부팅 후 인스톨

> [!IMPORTANT]
> 프로비저닝 전에는 **AGL 기본 홈스크린이 보이는 것이 정상**이다 (컨테이너 런처 이미지를
> 아직 레지스트리에서 받지 않은 상태). `oss-provision` 후 런처가 그 위를 풀화면으로 덮는다.
> 화면이 **완전히 검다면** 홈스크린이 기동하지 않은 것이므로
> `systemctl status flutter-ics-homescreen` 을 확인한다(§Step 2.0.3 트러블슈팅).

```bash
ssh root@<보드IP>

# 1) rootfs 확장 (플래싱 직후 1회 — 컨테이너 이미지가 수 GB 이므로 반드시 수행)
df -h /                                     # 17G 등으로 작게 나오면 필요
resize2fs /dev/mmcblk0p1
df -h /                                     # ~55G 확인

# 2) (선택, 권장) 레지스트리 로봇 토큰 배치 — 이후 프로비저닝이 **무인**이 된다
#    자격증명은 이미지에 굽지 않으므로 보드에서 1회 배치한다. 파일 권한을 반드시 죈다.
install -d -m 0700 /etc/oss
printf '%s' '<robot-token>' > /etc/oss/registry-token
chmod 600 /etc/oss/registry-token
#   ※ 계정은 Harbor 로봇 계정 `robot$oss+oss-ci` 를 기본으로 쓴다(pull 전용).
#     다른 계정이면 REGISTRY_USER 로 덮는다: REGISTRY_USER='robot$xxx' oss-provision …
#     토큰을 두지 않으면 oss-provision 이 대화형 docker login 으로 넘어간다.
#     (oss 프로젝트가 public 인 동안은 익명 pull 도 동작한다)

# 3) ⭐ 프로비저닝 — 런처 config(rosbridge IP) + 로그인 + pull + 스택 기동을 한 번에
oss-provision --rosbridge <2세부HW-IP>
#   IP 만 주면 ws://IP:9090 으로 확장된다. host:port 나 완전한 ws:// URL 도 받는다.
#   런처 config·설정 디렉토리·소유권(uid 1001)은 **이미지가 이미 갖추고 있어** 수동
#   mkdir/chown 이 필요 없다 (2026-08-12 기준 이미지).
```

> [!CAUTION]
> **토큰 취급** — `/etc/oss/registry-token` 은 이미지에 포함되지 않는다(보드마다 배치).
> 셸 히스토리에 남기지 않으려면 위처럼 `printf` 로 쓰거나, 개발 PC 에서
> `scp token.txt root@<보드>:/etc/oss/registry-token && ssh root@<보드> chmod 600 …` 로 옮긴다.
> 토큰이 유출되면 Harbor 에서 로봇 계정을 재발급하고 보드의 파일을 교체한다.
> 임시로 1회만 쓰려면 파일 대신 환경변수도 된다: `REGISTRY_TOKEN=… oss-provision --rosbridge …`

> [!TIP]
> 환경 의존 값이 rosbridge URL **하나로 모여 있다.** 네트워크가 바뀌거나 새 보드를 셋업할
> 때 이미지를 다시 만들 필요 없이 `oss-provision --rosbridge <IP>` 만 다시 실행하면 된다.
> 현재 값 확인은 `oss-provision --status`.

`oss-provision` 이 수행하는 일:
1. `--rosbridge` 를 주면 런처 config 의 rosbridge URL 갱신 (+ `/cfg` 사본·소유권 정리)
2. `docker login registry.obigo.com` — 로봇 토큰(`/etc/oss/registry-token` 또는
   `REGISTRY_TOKEN`)이 있으면 **무인**, 없으면 대화형 (oss 프로젝트가 public 이면 익명 pull)
3. `mmf-ext-agl` · **런처**(서비스 목록의 태그를 읽어 자동 선택) · 모니터 앱 · `rviz2-agl` pull
4. 서비스 목록을 **full** 로 전환 (베이크 이미지는 minimal=런처만으로 출하 — 이전 목록은 `.bak`)
5. `oss-integration.service` 재시작 → 스택 기동

완료되면 화면에 **hmi_launcher 가 표출**된다. 이후 재부팅은 로컬 이미지 캐시와
`restart: unless-stopped` 로 **오프라인에서도 자동 복원**된다.

#### Step 2.0.3: 검증

```bash
systemctl status oss-integration     # active (exited) 가 정상 (Type=oneshot)
docker ps                            # oss-mmf-ext-agl, hmi_launcher 확인
docker compose version               # Docker Compose version v2.26.0

# 베이크 컨테이너 적재 + :latest 태깅 확인 (오프라인 기동의 근거)
systemctl status oss-container-preload   # active (exited) 가 정상
docker images | grep -E 'oss-mmf-ext-agl|oss-hmi-launcher|rviz2-agl'
#  기대: 각 repo 가 :provision 과 :latest **두 줄**(같은 IMAGE ID = 디스크 1회분)

# GPU 경로 확인 — 스테이징된 GL 스택과 런처의 EGL 상태
ls /opt/nv-gl/libEGL_nvidia.so.0     # 없으면: systemctl restart oss-integration (ExecStartPre 재실행)
docker logs hmi_launcher 2>&1 | grep -iE 'WAYLAND_DISPLAY|MESA-LOADER|libEGL warning'
#  기대: `WAYLAND_DISPLAY=wayland-0` 한 줄만. `MESA-LOADER … failed` 가 보이면
#  GL 스택이 컨테이너에 주입되지 않은 것 → 아래 트러블슈팅 참조

# 화면 캡처로 확인 (원격에서도 실제 IVI 화면 그대로 확인 가능)
sudo -u agl-driver env XDG_RUNTIME_DIR=/run/user/1001 WAYLAND_DISPLAY=wayland-0 agl-screenshooter
ls /tmp/agl-screenshot-*.png
```

##### 트러블슈팅 — 화면/GPU (보드 실측 2026-08-10)

| 증상 | 원인 | 해결 |
|---|---|---|
| 런처는 `docker ps` 에 보이는데 **화면이 검다** | 출력에 BACKGROUND surface 가 없다 | 2026-08-13 이후 이미지는 **런처가 BG 롤**을 맡는다 — `docker inspect hmi_launcher --format '{{.Args}}'` 에 `--window-type BG` 가 있어야 한다. 없으면 서비스 목록의 `command_args` 확인. 구 이미지라면 AGL 홈스크린이 BG 를 맡으므로 `systemctl start flutter-ics-homescreen` |
| 런처가 **좌측 1080px 만** 그리거나 기동 15초 뒤 사라짐 (`xdg_surface geometry (1080x1712) is larger than … (1920x1080)`) | AGL 홈스크린이 켜져 있다 — 세로 1080x1920 전용이라 가로 출력에서 activation area 를 1080x1712 로 고정한다 | `systemctl disable --now flutter-ics-homescreen` + 런처를 `--window-type BG` 로. 2026-08-13 이후 이미지는 기본 적용 |
| 홈스크린·런처가 **둘 다 크래시 루프** (`agl_shell extension already in use by other shell client`) | 두 클라이언트가 BG 슬롯을 다툰다. 죽은 클라이언트가 슬롯을 잡은 채 남는다 | 홈스크린을 끄고 `systemctl restart agl-compositor` 로 슬롯을 비운 뒤 `docker rm -f hmi_launcher` (adsw_manager 가 재생성) |
| 로그에 `libEGL warning: MESA-LOADER: failed to retrieve device information` / `egl: failed to create dri2 screen` | 컨테이너에 **Tegra 유저스페이스 GL 이 없어** glvnd 가 mesa 로 폴백. `--device /dev/dri` 는 커널 노드 접근권만 준다 | `/opt/nv-gl` 스테이징 확인 + 서비스 목록에 `LD_LIBRARY_PATH=/opt/nv-gl`, `/opt/nv-gl`·glvnd·egl_external_platform.d 마운트가 있는지 확인 |
| 로그 첫 줄이 `[INFO] WAYLAND_DISPLAY=` (빈 값) | 소켓 디렉토리만 마운트되고 `XDG_RUNTIME_DIR`/`WAYLAND_DISPLAY` env 가 없어 컴포지터에 붙지 못함 | 서비스 목록의 env 2종 확인 |
| 런처 UI 는 뜨는데 rviz2 실행 후 **타일·Stop 이 무반응** | 런처가 BG 롤이라 입력 포커스가 cage 에 잔류 (BG 구성의 알려진 트레이드오프) | 현재는 SSH 로 `docker stop rviz2`. 가로 출력에서 NORMAL 롤은 위 행의 geometry 충돌로 쓸 수 없다 — 앱 팀과 재조정 필요 |

#### Step 2.0.4: 컨테이너 업데이트 (레지스트리에 새 이미지가 올라온 뒤)

```bash
oss-provision            # pull + 컨테이너 재생성 (--rosbridge 없이도 된다)
oss-provision --status   # 실행 이미지 / 목록 상태 / 로그인 여부 확인
```

> ⚠️ **pull 만으로는 화면이 갱신되지 않는다** (보드 실측 2026-08-14).
> `oss-integration.service` 는 `up -d --no-recreate` 이고 런처는 `--restart unless-stopped`
> 컨테이너라, 새 이미지를 받아도 기존 컨테이너가 **구 이미지로 계속** 뜬다.
> 그래서 `oss-provision` 이 pull 후 `hmi_launcher`·`oss-mmf-ext-agl` 을 제거하고 재생성한 뒤,
> 실행 이미지가 목록의 태그와 일치하는지 90초까지 폴링해 보고한다.
> 2026-08-14 이전 이미지라면 수동으로:
> `docker rm -f hmi_launcher oss-mmf-ext-agl && systemctl restart oss-integration`

#### Step 2.0.5: 공장 초기화 (프로비저닝 이전 상태로 되돌리기)

```bash
oss-provision --help                      # 옵션 전체 (사용법·환경변수·예시)
oss-provision --factory-reset --dry-run   # 무엇이 바뀌는지 먼저 확인 (변경 없음)
oss-provision --factory-reset             # 대화형 확인 후 실행
oss-provision --factory-reset --yes --prune-dangling --purge-token   # 무인 + 완전 정리
```

**재플래싱이 필요 없다** — `oss-provision` 이 바꾼 것만 정확히 원복한다.

| 대상 | 복원 결과 |
|---|---|
| 활성 서비스 목록 | 공장 기본(런처만) + `.bak-*` 정리 |
| 런처 config | rosbridge URL → 베이크된 placeholder (cfg 사본까지, uid 1001) |
| `domain.env` | 런처 Settings 가 바꾼 `ROS_DOMAIN_ID` → 공장값(42) |
| 컨테이너 | `hmi_launcher`·`oss-mmf-ext-agl`·`rviz2` 제거 후 재기동 |
| 이미지 | `:latest` → 베이크 `:provision` 재태깅, 스냅샷 태그(`develop-*-arm64`)·모니터 앱 이미지 제거 |
| 레지스트리 로그인 | 로그아웃 (`--purge-token` 이면 `/etc/oss/registry-token` 도 삭제) |

건드리지 않는 것: rootfs, `/opt/nv-gl` 스테이징(부팅 때 재생성), 로그, 네트워크 설정.
초기화 후에는 Step 2.0.2 (토큰 배치 → `oss-provision --rosbridge <IP>`) 를 그대로 반복한다.

#### Step 2.0.6: k3s 노드 역할 설정 (OTA 클러스터에 합류할 때)

이미지에는 k3s 바이너리(`v1.28.7+k3s1` 고정)와 **server/agent 유닛이 모두** 들어 있고
**둘 다 비활성(disabled)** 으로 출하된다 — 이미지는 역할 중립이다. 부팅만으로는 k3s 가
뜨지 않으므로, 노드 역할을 프로비저닝에서 정한다 (차량 1대 = 클러스터 1개, 151=server / 81=agent).

> 🔒 **join 토큰과 server 주소는 이미지에 넣지 않는다.** 유출 시 회수가 불가능하고 모든 보드가
> 같은 토큰을 갖게 된다. 아래처럼 보드에서 주입한다.

```bash
# ① agent 로 합류시킬 때 (예: 81)
cat > /etc/systemd/system/k3s-agent.service.env <<'ENV'
K3S_URL=https://172.16.10.151:6443
K3S_TOKEN=<server 의 /var/lib/rancher/k3s/server/node-token 값>
ENV
chmod 600 /etc/systemd/system/k3s-agent.service.env
systemctl enable --now k3s-agent

# ② server 로 쓸 때 (예: 151)
systemctl enable --now k3s          # 필요시 --disable traefik --disable servicelb 는 config.yaml 로

# 검증
systemctl is-enabled k3s k3s-agent  # 역할에 맞는 하나만 enabled
/usr/local/bin/k3s --version        # v1.28.7+k3s1 계열 (server 와 버전 스큐 금지)
ssh <server> 'kubectl get nodes'    # 노드가 Ready 로 보이면 합류 완료
```

> ⚠️ **버전은 이미지 단독으로 올리지 않는다.** server(151)와 스큐가 생기면 합류가 깨진다 —
> 올릴 때는 양쪽을 함께 올린다.

#### 이후 운영

| 작업 | 명령 |
|---|---|
| 컨테이너 이미지 갱신 | `oss-provision --pull-only` |
| **rosbridge(MMF) 주소 변경** | `oss-provision --rosbridge <새IP>` — 이미지 재빌드 불필요 |
| 현재 설정 확인 | `oss-provision --status` (활성 목록 · 런처 태그 · rosbridge URL · 베이크 이미지) |
| **레지스트리 토큰 교체** | `/etc/oss/registry-token` 갱신(0600) → 다음 `oss-provision` 부터 적용 |
| 런처 태그 변경 | `/opt/adsw/config/agl-service-list.yaml` 의 `image:` 수정 → `systemctl restart oss-integration` |
| DDS 도메인 변경 | `/opt/mmf/config/domain.env` 수정 → `systemctl restart oss-integration` (⚠️ `docker restart` 로는 반영 안 됨 — 재생성 필요) |
| 수동 기동/중지 | `systemctl start|stop oss-integration` |

---

### Step 2.1: 2세부 HW 대상 Docker 이미지 다운로드 (Pull)

**2세부 HW**에서 실행 및 동적 기동에 필요한 Docker 이미지들을 환경 변수 파일(`.env.integration`) 옵션과 함께 지정하여 다운로드합니다.

```bash
# 2세부 HW에서 실행
cd ~/source/oss_multisw_framework
docker compose --env-file docker/compose/.env.integration -f docker/compose/docker-compose-integration.yaml pull \
  mmf-core mmf-ext-ros2 security_processor attack_env security_monitor \
  oss-adsw-decision oss-adsw-control oss-adsw-perception
```

#### [터미널 실행 결과 첨부 양식]
```text
[+] pull 71/71
 ✔ Image ghcr.io/oss-test-group/oss-ids/security_processor:v0.1.0 Pulled          19.6s
 ✔ Image ghcr.io/oss-test-group/oss-ids/attack_env:v0.1.0         Pulled          20.8s
 ✔ Image registry.obigo.com/oss/oss-mmf-ext-ros2:latest           Pulled          64.8s
 ✔ Image junohb/autoware-partition:sample-adsw-control            Pulled          65.3s
 ✔ Image junohb/autoware-partition:sample-adsw-perception         Pulled          19.6s
 ✔ Image ghcr.io/oss-test-group/oss-ids/security_monitor:v0.1.0   Pulled          30.8s
 ✔ Image junohb/autoware-partition:sample-adsw-decision           Pulled          60.4s
 ✔ Image registry.obigo.com/oss/oss-mmf-core:latest               Pulled          31.1s
```

---

### Step 2.2: 2세부 HW 메인 프레임워크 및 보안 서비스 기동

2세부 HW에서는 MMF Core, ROS2 Extension 및 Security 서비스들을 기동합니다. (Autoware 서비스는 런타임 수동/HMI 제어를 위해 이미지 다운로드만 해두고 아직 기동하지 않음)

```bash
# 2세부 HW에서 실행
docker compose --env-file docker/compose/.env.integration -f docker/compose/docker-compose-integration.yaml up -d \
  mmf-core mmf-ext-ros2 security_processor attack_env security_monitor
```

#### [터미널 실행 결과 첨부 양식]
```text
[+] up 6/6
 ✔ Volume oss_monitor_db        Created             0.0s
 ✔ Container attack_env         Started             0.3s
 ✔ Container oss-mmf-core       Healthy             17.9s
 ✔ Container security_processor Started             0.3s
 ✔ Container security_monitor   Started             0.3s
 ✔ Container oss-mmf-ext-ros2   Started             18.1s
```

**실행된 컨테이너 상태 확인 명령어:**
```bash
docker ps
```

#### [터미널 실행 결과 및 스크린샷 첨부 양식]
```text
CONTAINER ID   IMAGE                                                      COMMAND                CREATED         STATUS                   NAMES
6a26e7b1e7eb   registry.obigo.com/oss/oss-mmf-ext-ros2:latest             "/oss/entrypoint.sh"   2 minutes ago   Up 2 minutes             oss-mmf-ext-ros2
c153f39fffcf   ghcr.io/oss-test-group/oss-ids/security_monitor:v0.1.0     "/entrypoint.sh"       2 minutes ago   Up 2 minutes             security_monitor
495a13bb6c37   registry.obigo.com/oss/oss-mmf-core:latest                 "/oss/entrypoint.sh"   2 minutes ago   Up 2 minutes (healthy)   oss-mmf-core
53ecbefd489c   ghcr.io/oss-test-group/oss-ids/security_processor:v0.1.0   "/entrypoint.sh"       2 minutes ago   Up 2 minutes             security_processor
f079e210a5c1   ghcr.io/oss-test-group/oss-ids/attack_env:v0.1.0           "/entrypoint.sh"       2 minutes ago   Up 2 minutes             attack_env
```

---

### Step 2.3: Orin 보드 (또는 x86_64 대체 환경) AGL 및 HMI 기동

Orin 타겟 보드에서 AGL Extension 서비스(`mmf-ext-agl`)를 기동합니다.
`mmf-ext-agl` 서비스가 정상 기동되면 internal manager를 통해 `oss-hmi-launcher` (HMI 앱 서비스)가 자동으로 연동 기동됩니다.

```bash
# x86_64 대체 PC(nvidia 런타임 보유)에서 실행 — base 그대로 사용
docker compose -f docker/compose/docker-compose-integration.yaml up -d mmf-ext-agl
```

#### Orin 보드(arm64/AGL)에서 기동할 때 — 오버라이드 + 사전 준비 필요

> [!NOTE]
> **Step 2.0 의 이미지로 플래싱한 보드는 이 절의 수동 절차가 필요 없다.**
> compose·오버라이드·서비스 목록이 `/etc/oss/compose`, `/opt/adsw/config` 에 이미 설치돼 있고
> `oss-integration.service` 가 부팅 시 기동한다. 아래는 **구 이미지 보드**(베이크 이전)나
> 이미지 없이 수동 구성할 때의 참조 절차다.

보드는 base compose 만으로는 기동되지 않는다(2026-08-06 실측). **오버라이드 파일과
호스트 준비 3종**이 필요하다.

```bash
# 0) 파일 배치 (보드에서)
mkdir -p /root/oss /opt/adsw/config
cp docker/compose/docker-compose-integration.yaml            /root/oss/
cp docker/compose/docker-compose-agl-board.override.yaml     /root/oss/
cp docker/compose/.env.integration.agl-board.example         /root/oss/.env.integration
cp service/extension/agl/adsw_manager/config/agl-service-list.yaml /opt/adsw/config/

# 1) 런처가 쓰는 호스트 디렉토리 — **AGL 런타임 uid(1001) 소유**여야 한다
mkdir -p /root/oss/hmi_launcher_cfg /opt/mmf/config /tmp/models
chown -R 1001:1001 /root/oss/hmi_launcher_cfg /opt/mmf/config
#    root 소유면 flutter-auto 가 XDG_CONFIG_HOME 하위 생성에 실패해 앱이 죽고,
#    /opt/mmf/config 는 런처 Settings 의 도메인 파일 기록이 실패한다.

# 2) 런처 config (rosbridge = 2세부 HW IP)
cat > /root/oss/hmi_launcher_config.json <<'JSON'
{ "rosbridge": { "url": "ws://<2세부HW-IP>:9090" },
  "statusTopic": "/app_status/hmi_launcher",
  "rosReconnectIntervalSec": 2, "heartbeatIntervalSec": 1 }
JSON
cp /root/oss/hmi_launcher_config.json /root/oss/hmi_launcher_cfg/hmi_launcher/config.json  # (디렉토리 먼저 생성)
chown -R 1001:1001 /root/oss/hmi_launcher_cfg

# 2-1) rviz2_panel 설정 — **따로 둘 파일이 없다**
#      패널이 첫 실행에 기기 기본값 `$XDG_CONFIG_HOME/rviz2_panel/config_def.json`
#      (보드: /root/oss/hmi_launcher_cfg/rviz2_panel/config_def.json) 을 스스로 만든다.
#      화면에서 저장한 값은 같은 디렉토리의 config.json 에 쌓이고, "기본값으로" 를
#      누르면 config_def.json 으로 돌아간다. 기기 기본값을 바꾸고 싶으면 이미지를
#      다시 굽지 말고 config_def.json 을 고치면 된다.

# 3) 기동 — **오버라이드와 env 파일을 함께** 지정한다
cd /root/oss
docker compose -f docker-compose-integration.yaml \
               -f docker-compose-agl-board.override.yaml \
               --env-file .env.integration up -d mmf-ext-agl

# 4) rviz2 이미지 프로비저닝 (약 2GB — 패널 Start 전에 미리 받아 둔다)
#    rviz2-agl 은 `profiles: [rviz2]` 라서 위 `up` 에 포함되지 않는다. 서비스 이름을
#    지목하면 해당 profile 이 자동 활성돼 pull 만 수행된다(compose v2).
docker compose -f docker-compose-integration.yaml \
               -f docker-compose-agl-board.override.yaml \
               --env-file .env.integration pull rviz2-agl
```

##### 보드에서 실제로 걸렸던 3 가지 (모두 위 절차에 반영돼 있다)

| 증상 | 원인 | 해결 |
|---|---|---|
| `could not select device driver "nvidia" with capabilities: [[gpu]]` 로 컨테이너 생성 거부 | base 의 `deploy.resources.reservations.devices`(nvidia GPU 예약). 보드 docker 런타임은 `runc` 뿐이다 | 보드 오버라이드가 `deploy: !reset null` 로 제거한다. `devices: []` 로는 지워지지 않는다(compose 가 빈 시퀀스를 미설정으로 보고 base 유지) |
| 런처가 `flutter-auto 를 찾을 수 없습니다: /usr/bin/flutter-auto` 로 즉시 종료 → 무한 재시작 | flutter-auto 는 **호스트 바이너리**를 쓰는데 마운트가 없었다 | AGL 서비스 목록에 `-v /usr/bin/flutter-auto:/usr/bin/flutter-auto:ro` 추가 |
| 런처가 `번들 디렉토리를 찾을 수 없습니다: /usr/share/flutter/hmi_launcher/<ver>/release` | 호스트 `/usr/share/flutter` 를 통째로 ro 마운트해 **이미지 안의 앱 번들**을 가렸다(호스트엔 엔진만, 이미지엔 번들만 있다) | 엔진 버전 디렉토리만 마운트: `-v /usr/share/flutter/<ver>:/usr/share/flutter/<ver>:ro` |

##### rviz2 컨테이너의 위치 — compose 에 있지만 `up` 대상은 아니다

`docker-compose-integration.yaml` 에 **`rviz2-agl` 서비스**가 있다(2026-08-07 추가).
다만 `profiles: [rviz2]` 로 게이트돼 **`up` 으로 자동 기동되지 않는다**. 생명주기는
HMI 의 **rviz2_panel 이 소유**하기 때문이다 — 패널이 `docker run/stop/rm -f rviz2` 로
직접 제어하고(F010 direct-docker) agl-shell gRPC(:14005) 로 화면을 전환한다. compose 가
부팅 때 함께 띄우면 ① 이름 충돌(패널 Start 가 `rm -f` 로 지운다) ② rviz2 가 런처를 덮은
상태로 부팅되는 문제가 생긴다.

| 용도 | 명령 |
|---|---|
| 이미지 프로비저닝(권장) | `docker compose … pull rviz2-agl` — 위 절차 4) |
| 런처 없이 rviz2 만 스모크 | `docker compose … --profile rviz2 up -d rviz2-agl` → 확인 후 `docker stop rviz2 && docker rm rviz2` 로 패널에 넘긴다 |

서비스 파라미터(net/ipc host, uid 1001, `/run/user/1001` 마운트, `LIBGL_ALWAYS_SOFTWARE`)는
패널의 docker run 인자(`app_config.dart` → `dockerRunArgs`)와 **동일하게 유지**한다.
갈라지면 "패널로 띄운 rviz2" 와 "compose 로 띄운 rviz2" 가 다르게 동작한다.

compose 의 `oss-adsw-rviz` 는 EVB 보드(x86 데스크탑)용 Autoware 파티션 rviz 로 별개 서비스다.

#### [터미널 실행 결과 및 스크린샷 첨부 양식]
```text
[+] up 15/15
 ✔ Image registry.obigo.com/oss/oss-mmf-ext-agl:latest Pulled    17.4s
 ✔ Container oss-mmf-ext-agl                           Started     0.2s
```

수초 후 `docker ps` 실행 시 아래와 같이 hmi_launcher가 기동됨을 확인.
```text
CONTAINER ID   IMAGE                                            COMMAND                   CREATED         STATUS         NAMES
7d1698495106   registry.obigo.com/oss/oss-hmi-launcher:latest   "/app/entrypoint"         3 minutes ago   Up 3 minutes   hmi_launcher
41e4fcf91a60   registry.obigo.com/oss/oss-mmf-ext-agl:latest    "/bin/bash -c 'if [ …"   3 minutes ago   Up 3 minutes   oss-mmf-ext-agl
```
![alt text](image.png)
---

## 3. 세부 테스트 절차 및 검증 항목

### 테스트 1: Orin HMI 앱 네트워크 IP 설정, 초기 상태 및 모니터링 수신 검증

**목적**: HMI 앱 구동 후 2세부 HW(MMF ROS2 Ext 구동 타겟)와의 데이터 통신을 위한 Bridge IP를 설정하고 모니터링 표출 상태를 검증합니다.

1. Orin 디스플레이(또는 x86_64 화면)에서 자동 실행된 HMI 화면으로 접속합니다.
2. HMI 화면 내 **[Settings (설정)]** 메뉴에 진입합니다.
3. **Bridge IP 주소** 입력란에 `mmf-ext-ros2` 서비스가 구동 중인 **2세부 HW의 실제 IP 주소**를 입력하고 저장합니다.
4. HMI 메인 대시보드 화면에서 Life Cycle Monitor로 이동하여 다음 상태를 확인합니다:
   - **Bridge Network 상태**: 정상 (Connected)
   - **Autoware 상태**: RUNNING / Stopped (미기동 상태)
   - **Event 수신 상태**: Received / No Event

#### [결과 확인 스크린샷 첨부 양식]
![alt text](settings.png) 

![alt text](life_cycle.png)

- [ ] **체크사항 1.1**: HMI 앱 Settings 메뉴에서 2세부 HW의 Bridge IP 주소가 정상 설정되고 저장되는가?
- [ ] **체크사항 1.2**: IP 설정 후 2세부 HW의 MMF Core 및 ROS2 Extension과의 통신 상태가 '정상(Active/Connected)'으로 표시되는가?

---

### 테스트 2: Orin HMI 앱에서의 Autoware 제어(실행/중지) 검증

**목적**: Orin HMI 앱의 버튼 조작으로 2세부 HW상의 Autoware 도커 컨테이너 3종이 동적으로 실행 및 중지되는지 검증합니다.

#### 2.1 Autoware 실행 검증
1. Orin HMI 앱에서 **[Start]** 버튼을 클릭합니다.
2. 2세부 HW 터미널에서 아래 명령을 실행하여 Autoware 도커 컨테이너 3종이 새로 기동되었는지 확인합니다:
   ```bash
   # 2세부 HW 터미널에서 실행
   docker ps
   ```

##### [터미널 실행 결과 첨부 양식]
```text
CONTAINER ID   IMAGE                                          COMMAND                  CREATED         STATUS         NAMES
a1b2c3d4e5f6   ghcr.io/oss-test-group/autoware-partition:adsw-decision   "/autoware/start_sc…"   5 seconds ago   Up 4 seconds   oss-adsw-decision-container
b2c3d4e5f6a1   ghcr.io/oss-test-group/autoware-partition:adsw-control    "/autoware/start_sc…"   5 seconds ago   Up 4 seconds   oss-adsw-control-container
c3d4e5f6a1b2   ghcr.io/oss-test-group/autoware-partition:adsw-perception "/autoware/start_sc…"   5 seconds ago   Up 4 seconds   oss-adsw-perception-container
```

#### 2.2 Autoware 중지 검증
1. Orin HMI 앱에서 **[Stop]** 버튼을 클릭합니다.
2. 2세부 HW 터미널에서 Autoware 도커 컨테이너들이 정상 종료되었는지 확인합니다:
   ```bash
   # 2세부 HW 터미널에서 실행
   docker ps
   ```

##### [터미널 실행 결과 첨부 양식]
```text
CONTAINER ID   IMAGE     COMMAND   CREATED   STATUS   PORTS   NAMES
(출력 결과 없음 - 컨테이너 정상 종료됨)
```

- [ ] **체크사항 2.1**: HMI 앱에서 '실행' 조작 시 2세부 HW에서 `oss-adsw-decision`, `oss-adsw-control`, `oss-adsw-perception` 컨테이너가 생성 및 Up 상태가 되는가?
- [ ] **체크사항 2.2**: HMI 앱 화면의 Autoware 상태 표시가 'Running'으로 변경되는가?
- [ ] **체크사항 2.3**: HMI 앱에서 '중지' 조작 시 2세부 HW의 Autoware 컨테이너 3종이 정상적으로 Exited/Down 처리되는가?

---

### 테스트 3: Orin HMI 앱 진단 화면 및 침입 감지(Security/IDS) 팝업 검증

**목적**: HMI 진단(Diagnostics) 화면 진입 시 침입 감지 보안 팝업이 1회 정상 발생하고 닫히는지 검증합니다.

1. Orin HMI 앱의 메뉴에서 **[진단 (Diagnostics)]** 화면으로 이동합니다.
2. 진단 화면 진입 즉시 침입 감지 팝업(Security/IDS Alert)이 **1회** 팝업되는지 확인합니다.
3. 팝업 메시지 내용 및 확인 버튼 클릭 시 정상 닫힘 동작을 확인합니다.

#### [스크린샷 첨부 양식]
![HMI 진단 화면 침입 감지 팝업 화면](file:///home/zeus/source/oss_hmi_app/docs/images/test-hmi-diagnostics-ids-popup.png)

- [ ] **체크사항 3.1**: 진단 화면 이동 시 침입 감지 팝업이 지연 없이 **1회만** 표출되는가? (중복/무한 팝업 방지 확인)
- [ ] **체크사항 3.2**: 팝업창 닫기/확인 버튼 클릭 시 정상적으로 닫히고 진단 데이터 표출 화면으로 돌아가는가?

---

### 테스트 4: EVB 보드 RViz 시뮬레이션 및 HMI 자율주행 데이터 연동 검증

**목적**: EVB 보드(우분투 데스크탑)에서 `oss-adsw-rviz` 컴포즈 서비스를 구동하여 RViz 시뮬레이션을 실행하고, 자율 주행이 시뮬레이션 되는지 검증합니다.

1. **EVB 보드 (Ubuntu Desktop)** 환경에서 Compose를 사용하여 RViz 서비스(`oss-adsw-rviz`)를 기동합니다:
   ```bash
   # EVB 보드 터미널에서 실행. sudo 권한 필요.
   sudo docker compose -f docker/compose/docker-compose-integration.yaml up -d oss-adsw-rviz
   ```
2. RViz 화면에서 차량의 시작/목표 위치를 지정하여 주행 시뮬레이션을 구동합니다.
3. 차량이 목표 위치까지 정상적으로 자율 주행되는지 점검합니다:

#### [HMI 주행 데이터 연동 스크린샷 첨부 양식]
![Orin HMI 자율주행 데이터 실시간 수신 화면](file:///home/zeus/source/oss_hmi_app/docs/images/test-hmi-driving-data-sync.png)

- [ ] **체크사항 4.1**: EVB 보드에서 `oss-adsw-rviz` 서비스 실행 시 RViz2 환경이 정상 오픈되는가?
- [ ] **체크사항 4.2**: RViz 주행 시뮬레이션시 주행 궤적이 정상적으로 계산이 되고, 자율 주행이 목표 위치까지 수행되는가?

---

## 4. 종합 테스트 결과 체크리스트 요약표

| 번호 | 검증 항목 | 세부 확인 내용 | 결과 (P/F) | 비고 / 첨부 자료 |
|---|---|---|---|---|
| 1 | **이미지 준비** | `--env-file` 옵션 포함 2세부 HW 대상 이미지 명시적 `pull` 완료 여부 | [ ] Pass / [ ] Fail | 터미널 로그 확인 |
| 2 | **2세부 HW 서비스** | `mmf-core`, `mmf-ext-ros2`, Security 3종 컨테이너 `Up` 상태 | [ ] Pass / [ ] Fail | `docker ps` 확인 |
| 3 | **Orin/x86_64 HMI** | `mmf-ext-agl` 기동 시 `oss-hmi-launcher` 자동 로딩 및 연동 | [ ] Pass / [ ] Fail | AGL/HMI 스크린샷 |
| 4 | **Bridge IP 설정** | HMI Settings 메뉴에서 2세부 HW의 Bridge IP 설정 후 연결 확인 | [ ] Pass / [ ] Fail | Settings 스크린샷 |
| 5 | **Autoware 동적 기동** | HMI '실행' 클릭 시 2세부 HW Autoware 3종 컨테이너 기동 | [ ] Pass / [ ] Fail | `docker ps` 및 스크린샷 |
| 6 | **Autoware 동적 중지** | HMI '중지' 클릭 시 2세부 HW Autoware 3종 컨테이너 종료 | [ ] Pass / [ ] Fail | `docker ps` 로그 |
| 7 | **진단 및 보안 팝업** | HMI 진단 화면 접속 시 침입 감지 팝업 1회 발생 확인 | [ ] Pass / [ ] Fail | IDS 팝업 스크린샷 |
| 8 | **RViz 컴포즈 실행** | EVB 보드에서 `oss-adsw-rviz` 컴포즈 서비스 정상 실행 | [ ] Pass / [ ] Fail | RViz 구동 스크린샷 |
| 9 | **주행 데이터 연동** | RViz 주행 시뮬레이션 데이터가 HMI 화면에 실시간 표출 | [ ] Pass / [ ] Fail | HMI 데이터 연동 스크린샷 |

---

