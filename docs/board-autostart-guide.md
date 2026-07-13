# 보드 재부팅 자동시작 가이드 — core / ext-ros2 / ext-agl

> MMF 도커 컨테이너(core, ext-ros2, ext-agl)를 보드 재부팅 시 자동으로 기동시키는 방법.
> 2026-07 실보드(172.16.10.67 평가, 172.16.10.70 시제품, 172.16.10.162 HMI) 검증 구성 기준.

---

## 1. 원리 — systemd 유닛 없이 Docker restart policy 로

```
부팅 → ① systemd 가 docker 데몬 시작        (systemctl enable docker)
     → ② docker 데몬이 restart policy 가 걸린 컨테이너를 자동 복원
          (--restart unless-stopped → core / ext-ros2 / ext-agl 기동)
```

컨테이너별 systemd 유닛·cron 이 필요 없다. **두 가지만 갖추면 된다:**

| 요소 | 설정 | 확인 |
|---|---|---|
| docker 데몬 부팅 시작 | `sudo systemctl enable docker` (1회) | `systemctl is-enabled docker` → `enabled` |
| 컨테이너 restart policy | `docker run --restart unless-stopped ...` | `docker inspect <c> --format '{{.HostConfig.RestartPolicy.Name}}'` |

restart policy 는 컨테이너 속성으로 저장되므로, **기동 스크립트를 1회 실행해 컨테이너를
만들어 두면 이후 재부팅마다 docker 가 알아서 복원**한다.

---

## 2. 최초 설치 절차

### 2-1. 이미지 준비 (둘 중 하나)
```bash
# (a) registry 접근 가능 보드 (예: 70)
docker pull registry.obigo.com/oss/oss-mmf-core:latest
docker pull registry.obigo.com/oss/oss-mmf-ext-ros2:latest

# (b) registry 미인증 보드 — 빌드 호스트에서 전송
docker save oss-mmf-core:arm64-local oss-mmf-ext-ros2:arm64-local \
  | gzip -1 | ssh <user>@<board> 'gunzip | docker load'
```

### 2-2. 기동 스크립트 1회 실행
core+ext-ros2 보드는 `~/run-mmf.sh`(보드에 배치), ext-agl 보드(HMI)는
`/opt/adsw/run-ext-agl.sh` 를 사용한다. 스크립트가 하는 일:

1. 기존 컨테이너 `docker stop` + `rm`
2. **stale `/dev/shm/fastdds_*` sweep** — FastDDS 좀비 세그먼트 제거 (수동 재기동 시 필수)
3. `docker run -d --restart unless-stopped ...` 로 생성
   - core → 12초 대기 → ext-ros2 순서 (DDS discovery 여유)

핵심 실행 옵션 (세 컨테이너 공통 골격):
```bash
docker run -d --name oss-mmf-core \
  --restart unless-stopped \
  --privileged --network host \
  --cap-add NET_RAW --cap-add NET_ADMIN \
  -v /dev/shm:/dev/shm \
  -e ROS_DOMAIN_ID=0 -e MMF_DDS_DOMAIN_ID=0 \
  <core-image>

# ext-ros2 는 추가로: -v /var/run/docker.sock:/var/run/docker.sock --group-add <docker-gid>
# ext-agl  은 추가로: docker.sock (adsw_manager 가 hmi_launcher 컨테이너 제어)
```
> `MMF_DDS_DOMAIN_ID` 는 보드별 DDS domain 격리용(기본 0). **같은 보드의
> core/ext-ros2/ext-agl 은 반드시 동일 값**이어야 하며, 같은 서브넷의 다른 보드와
> 격리하려면 보드마다 다른 값을 준다 (`docs/` 의 domain 격리 관련 커밋 참조).

---

## 3. 주의사항

1. **`unless-stopped` 의 의미** — 수동으로 `docker stop` 한 컨테이너는 **재부팅해도
   되살아나지 않는다** (수동 정지를 존중). 복구:
   ```bash
   docker start oss-mmf-core oss-mmf-ext-ros2   # 또는 기동 스크립트 재실행(권장 — SHM sweep 포함)
   ```
2. **부팅 시 기동 순서는 미보장** — 스크립트는 core→ext-ros2 순서를 지키지만, 재부팅
   복원 시엔 docker 가 거의 동시에 띄운다. DDS discovery + dds_bridge self-heal 이 늦은
   합류를 처리하므로 실사용 문제 없음 (67 재부팅 실검증).
3. **부팅 시 /dev/shm 은 비어 있음** (tmpfs) — sweep 불필요. sweep 은 **수동 재기동**
   시에만 의미 있다. 이상 동작 시 `docker start` 보다 스크립트 재실행이 안전한 이유.

---

## 4. 검증 방법

```bash
systemctl is-enabled docker                                   # enabled
docker inspect oss-mmf-core     --format '{{.HostConfig.RestartPolicy.Name}}'   # unless-stopped
docker inspect oss-mmf-ext-ros2 --format '{{.HostConfig.RestartPolicy.Name}}'   # unless-stopped
sudo reboot   # → 재부팅 후:
docker ps     # 두 컨테이너 Up + 내부 서비스 확인 (lifecycle_mgr, adsw_manager, rosbridge :9090 등)
```

---

## 5. 대안 — docker compose 기반

### 방법 A: compose 파일의 restart policy (원리는 §1 과 동일)
```yaml
services:
  core:
    image: registry.obigo.com/oss/oss-mmf-core:latest
    container_name: oss-mmf-core
    restart: unless-stopped
    privileged: true
    network_mode: host
    cap_add: [NET_RAW, NET_ADMIN]
    volumes: [/dev/shm:/dev/shm]
    environment: [ROS_DOMAIN_ID=0, MMF_DDS_DOMAIN_ID=0, HEARTBEAT_INTERVAL_SEC=3]
  ext-ros2:
    image: registry.obigo.com/oss/oss-mmf-ext-ros2:latest
    container_name: oss-mmf-ext-ros2
    restart: unless-stopped
    depends_on: [core]          # up 시점 순서만 보장 (부팅 복원엔 미적용)
    privileged: true
    network_mode: host
    cap_add: [NET_RAW, NET_ADMIN]
    group_add: ["999"]          # 보드 docker gid (getent group docker)
    volumes: [/dev/shm:/dev/shm, /var/run/docker.sock:/var/run/docker.sock]
    environment: [ROS_DOMAIN_ID=0, MMF_DDS_DOMAIN_ID=0]
```
`docker compose up -d` 1회 → 이후 재부팅마다 자동 복원. 한계는 §3-1/§3-2 와 동일.

### 방법 B: systemd 유닛이 compose 를 실행 (순서·상태 보장)
```ini
# /etc/systemd/system/oss-mmf.service
[Unit]
Description=OSS MMF core + ext-ros2 (docker compose)
Requires=docker.service
After=docker.service network-online.target

[Service]
Type=oneshot
RemainAfterExit=yes
WorkingDirectory=/home/<user>/mmf
# ExecStartPre=/usr/bin/docker compose pull   # 부팅 시 최신 이미지 갱신 원하면
ExecStart=/usr/bin/docker compose up -d
ExecStop=/usr/bin/docker compose stop
TimeoutStartSec=300

[Install]
WantedBy=multi-user.target
```
`systemctl enable oss-mmf` → 부팅마다 compose 오케스트레이션 수행: `depends_on` 순서
보장, 수동 down 후에도 다음 부팅에 복구, pull 로 이미지 자동 갱신 가능.

**보드 적용성**: compose 플러그인이 있는 보드(70: v5.2.0)는 A/B 모두 가능. 없는
보드(67)는 `docker-compose-plugin` 설치가 선행돼야 하며, 그 전까지는 §1 docker run
방식을 쓴다. PC 시뮬레이션용 compose 는 `docker/compose/docker-compose-integration.yaml` 참조.

---

## 6. 요약

| 보드 | 컨테이너 | 이미지 소스 | 기동 스크립트 | 자동시작 |
|---|---|---|---|---|
| core+ext-ros2 보드 | oss-mmf-core, oss-mmf-ext-ros2 | registry pull 또는 save/load | `~/run-mmf.sh` | docker enabled + unless-stopped |
| HMI 보드 (AGL) | oss-mmf-ext-agl | save/load | `/opt/adsw/run-ext-agl.sh` | 동일 |

**"docker enabled + `--restart unless-stopped`" 가 전부다.** 스크립트로 1회 생성해 두면
재부팅마다 자동으로 올라온다.
