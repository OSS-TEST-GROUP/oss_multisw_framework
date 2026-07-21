# MMF 빌드·설치·실행 가이드 (x86_64 / arm64)

> core · ext-ros2 · ext-agl 를 소스에서 빌드해 타겟(PC x86_64 시뮬레이션, arm64 보드)에
> 설치·실행하는 전 과정. 2026-07 실보드(67/70/162) 검증 절차 기준.
> Dockerfile 구조 상세는 [docker/README.md](../docker/README.md),
> 재부팅 자동시작은 [board-autostart-guide.md](board-autostart-guide.md) 참조.

---

## 0. 전체 흐름

```
[공통] ./build.sh 로 바이너리 크로스컴파일 (빌더 컨테이너 안에서 colcon)
   ↓ output/<arch>/<BUILD_TYPE>/{core, extension/{ros2,agl}}/...
[이미지] docker buildx 로 runtime-mmf-{core,ext-ros2,ext-agl} 조립
   ↓
[설치] registry push→pull 또는 docker save|ssh|load
   ↓
[실행] 기동 스크립트(run-mmf.sh 등) — --restart unless-stopped 로 자동시작까지
```

빌드는 호스트에 툴체인 없이 **빌더 컨테이너**(`registry.obigo.com/oss/oss-mmf-builder`,
최초 1회 pull 후 캐시)에서 수행된다. 증분 캐시는 `.build/`(arch 별 하위 디렉토리로
분리— `.build/Debug/ros2_aarch64` 등)라 arch 를 오가도 오염되지 않는다.

---

## 1. x86_64 (PC 시뮬레이션)

### 1-1. 빌드
```bash
./build.sh --set-extension-platform=ROS2     # core + ext-ros2 (Debug)
./build.sh --set-extension-platform=AGL      # ext-agl 까지 필요하면
./build.sh --release ...                     # Release 빌드
# → output/x86_64/<BUILD_TYPE>/...
```

### 1-2. 로컬 런타임 이미지
```bash
BUILD_TYPE=Debug ./docker/build_local.sh     # oss-mmf-core:local, oss-mmf-ext-ros2:local
#   --core-only / --ext-ros2-only 옵션 있음
```

### 1-3. 실행
```bash
docker compose -f docker/compose/docker-compose-integration.yaml up -d
# 재기동 시 stale /dev/shm/fastdds_* 청소가 필요하면:
./docker/restart-mmf.sh     # compose stop → SHM sweep → core → 8s → ext-ros2
```

---

## 2. arm64 (실보드)

### 2-1. 크로스컴파일
```bash
./build.sh --aarch64 --set-extension-platform=ROS2   # core + ext-ros2 → output/aarch64/Debug/
./build.sh --aarch64 --set-extension-platform=AGL    # ext-agl        → output/aarch64/Debug/extension/agl/
```
- 증분 빌드: 이전 `.build` 캐시가 있으면 변경분만 재컴파일 (수 분).
- 로그 끝의 `Finish.` + `error:`/`Failed <<<` 0건으로 성공 판정.

### 2-2. 런타임 이미지 조립
```bash
# ⚠️ TARGETARCH 는 Docker 자동값(arm64)이 아니라 커널 arch(aarch64) 를 명시해야 한다
#    (Dockerfile.rel 의 스테이지 접미사가 x86_64/aarch64 이기 때문 — docker/README.md 참조)
for t in core ext-ros2 ext-agl; do
  docker buildx build --platform=linux/arm64 \
    --build-arg BUILD_TYPE=Debug --build-arg TARGETARCH=aarch64 \
    -f docker/Dockerfile.rel --target runtime-mmf-$t --load \
    -t oss-mmf-$t:arm64-local .
done
```
> 멀티아키(amd64+arm64) 매니페스트 + registry push 를 한 번에 하려면
> `BUILD_TYPE=Release ./docker/build_multiarch.sh` 사용.

> **⚠️ 레지스트리 계정 — push 는 push 권한 계정 필요 (중요)**
> registry.obigo.com(Harbor)은 최소권한 정책이다. **pull 계정으로는 push 가 안 된다**
> (`unauthorized to access repository ... action: push`).
> - **pull**: `ossuser`(pull 전용) — build.sh/Jenkins 기본 로그인 계정.
> - **push**: **push 권한 계정**이 별도로 필요하다. `docker/registry-auth.sh` 의
>   push 루틴이 push 시점에만 그 계정으로 전환하고, 끝나면 ossuser 로 복귀한다.
>   자격증명은 **리포에 하드코딩하지 말고 env 로 주입**한다:
>   ```bash
>   export REGISTRY_PUSH_ID=<push계정>  REGISTRY_PUSH_PWD='<비번/토큰>'
>   BUILD_TYPE=Release ./docker/build_multiarch.sh   # push 시 자동 로그인→push→복귀
>   ```
>   env 미설정이면 push 는 가드로 스킵/실패한다(pull 은 ossuser 로 정상).
> - **push 계정 만들기(권장)**: Harbor 웹UI → 프로젝트 `oss` → **Robot Accounts** →
>   Push+Pull 권한으로 생성(`robot$oss+...` + 토큰), 또는 유저 + **Developer** 롤.
>   관리자(builder)가 아닌 최소권한 계정을 쓴다. (임시로 builder 사용은 가능하나 지양)
> - **Jenkins**: `REGISTRY_PUSH_ID`/`REGISTRY_PUSH_PWD` 를 Jenkins 자격증명(credentials
>   /withCredentials)으로 주입한다 — Jenkinsfile 에 평문 금지.

### 2-3. 설치 (보드로 전달 — 둘 중 하나)
```bash
# (a) registry 경유 — 배포 표준
REG=registry.obigo.com/oss
for t in core ext-ros2 ext-agl; do
  docker tag oss-mmf-$t:arm64-local $REG/oss-mmf-$t:latest
  docker push $REG/oss-mmf-$t:latest
done
# 보드에서: docker pull $REG/oss-mmf-{core,ext-ros2}:latest

# (b) 직접 전송 — registry 미인증 보드
docker save oss-mmf-core:arm64-local oss-mmf-ext-ros2:arm64-local \
  | gzip -1 | ssh <user>@<board> 'gunzip | docker load'
```
> ⚠️ **wire 호환 주의**: lifecycle IDL 은 wire-breaking 변경(예: os_type→mw_type)이
> 있었으므로 **core/ext-ros2/ext-agl 은 같은 develop 커밋에서 빌드한 세트로 함께 배포**
> 해야 한다. 구/신 혼용 시 DDS 타입 불일치로 lifecycle 통신이 조용히 끊긴다.

### 2-4. 실행

기동 스크립트는 리포에 포함되어 있다 — 보드로 복사해 실행한다:
- [docker/run-mmf.sh](../docker/run-mmf.sh) — core + ext-ros2
- [docker/run-ext-agl.sh](../docker/run-ext-agl.sh) — ext-agl (HMI 보드)

```bash
# core+ext-ros2 보드
scp docker/run-mmf.sh <user>@<board>:~/
ssh <user>@<board> 'bash ~/run-mmf.sh'          # stop/rm → SHM sweep → core → 12s → ext-ros2
ssh <user>@<board> 'ROS_DOMAIN_ID=1 bash ~/run-mmf.sh'   # 보드별 DDS domain 격리
# 로컬/검증 이미지로:  EXTROS2_IMG=oss-mmf-ext-ros2:diag2-local bash ~/run-mmf.sh
# 서비스목록 오버라이드: SAVED_SERVICE_LIST=~/adsw-service-list.yaml bash ~/run-mmf.sh

# ext-agl 보드(HMI)
scp docker/run-ext-agl.sh <user>@<hmi-board>:~/
ssh <user>@<hmi-board> 'bash ~/run-ext-agl.sh'
```
- 스크립트가 `--restart unless-stopped` 를 부여하므로 이후 재부팅 자동시작 —
  상세·주의사항·compose 대안은 [board-autostart-guide.md](board-autostart-guide.md).
- ⚠️ **ext-ros2 는 반드시 run-mmf.sh(또는 동등 옵션)로 기동** — 특히
  **`-v /var/run/docker.sock:/var/run/docker.sock` + `--group-add <docker-gid>`** 가 없으면
  adsw_manager 가 컨테이너(autoware 등)를 **docker run 하지 못한다**(StartService 가 Success 로
  보여도 실제 생성 실패). 수동 `docker run` 으로 일부 옵션을 빠뜨리면 이 문제가 재발한다.
  ext-ros2 로 autoware 를 관리·기동하는 운영법·필수 전제조건·트러블슈팅은
  **[ext-ros2-adsw-operation-guide.md](ext-ros2-adsw-operation-guide.md)** 참조.
- **DDS domain**: raw Fast-DDS 서비스는 `ROS_DOMAIN_ID` 가 아닌 **`MMF_DDS_DOMAIN_ID`**
  (lifecycle/diagnostics/adsw/ota) 와 **`VISS_DDS_DOMAIN_ID`**(vehicle_info) env(기본 0)를
  읽는다. run-mmf.sh/run-ext-agl.sh 는 `ROS_DOMAIN_ID` 값을 이 셋 모두에 동일하게 주입하므로
  `ROS_DOMAIN_ID=<n>` 하나만 지정하면 전 스택이 한 도메인에 격리된다. 같은 서브넷에 ROS2
  mw_type adsw 보드가 2대 이상이면 보드별로 다른 domain 을 줘야 서비스 상태 flapping 이
  안 생긴다.

### 2-5. 검증
```bash
docker ps                                              # 컨테이너 Up
docker exec oss-mmf-core     ps -eo comm | sort -u     # lifecycle_mgr/diagnostics/OtaManager/vehicle_info/viss-server
docker exec oss-mmf-ext-ros2 ps -eo comm | sort -u     # adsw_manager/dds_bridge/ota_client/rosbridge(python3)
ss -tln | grep 9090                                    # rosbridge 리슨
# lifecycle 집계 확인 (DEBUG 로그는 stdout 이 아니라 파일!):
docker exec oss-mmf-core grep "running service num" /oss/lifecycle_mgr.log | tail -3
```

---

## 3. 자주 밟는 함정 요약

| 함정 | 증상 | 대처 |
|---|---|---|
| TARGETARCH 를 arm64 로 줌 | 스테이지 해석 실패/엉뚱한 산출물 | `TARGETARCH=aarch64` (커널 arch) |
| 세트 아닌 이미지 혼용 | lifecycle 등록/상태가 조용히 안 옴 | 같은 커밋 세트로 lockstep 배포 |
| 수동 stop 후 재부팅 | 컨테이너 안 뜸 | unless-stopped 특성 — `docker start` 또는 스크립트 재실행 |
| 수동 재기동 후 이상 | bridge 팬텀/플래핑 | 기동 스크립트 사용(SHM sweep 포함), `docker start` 단독 지양 |
| lifecycle 로그가 안 보임 | `docker logs` 에 DEBUG 없음 | 컨테이너 내 `/oss/lifecycle_mgr.log` 확인 |
