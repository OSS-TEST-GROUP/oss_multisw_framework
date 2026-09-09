# DDS / ROS 도메인 가이드 — 두 평면 분리

MMF 는 도메인을 **두 평면**으로 나눠 운용한다. 하나는 순수 Fast-DDS 로 말하는 MMF 내부 버스,
다른 하나는 ROS2(rmw) 그래프다. 각각 자기 env 로 격리되고, 값이 달라도 두 평면을 함께 잡는
`dds_bridge` 를 통해 오간다.

| 평면 | env | 이 평면에 붙는 것 |
|---|---|---|
| **DDS 평면** (순수 Fast-DDS) | **`DDS_DOMAIN_ID`** | `lifecycle_mgr`, `diagnostics`, `adsw_manager`, `ota_mgr`, `ota_client`, `vehicle_info_mgr`(VISS), `dds_bridge` 의 raw 측 |
| **ROS2 평면** (rmw/rclcpp) | **`ROS_DOMAIN_ID`** | `rviz2`, Autoware, `rosbridge_server`, `dds_bridge` 의 rclcpp 측 |

> **핵심**: raw Fast-DDS 는 `ROS_DOMAIN_ID` 를 **보지 않는다**. 그건 ROS2/rmw 관례다.
> 그래서 MMF 서비스의 도메인을 옮기려면 `DDS_DOMAIN_ID` 를 바꿔야 한다.
> 두 평면을 잇는 것은 `dds_bridge` 다(§4).

기본값은 두 평면 모두 **42** 다 (Autoware adsw 파티션이 42 를 쓴다 — `saved-service-list.yaml`).

> **시퀀스 그림**: 도메인 갱신·컨테이너 제어·rviz2 제어·이미지 배포·상태 관측·
> `dds_bridge`·공장 초기화를 슬라이드 7장으로 그린 것이
> [`operation-sequences.html`](./operation-sequences.html) 에 있다 (브라우저로 열면
> 되고, 인쇄하면 장별로 페이지가 나뉜다).

> **분리 운영에는 이중 도메인 `dds_bridge` 가 필요하다** (2026-09-04 적용). 그 이전
> ext-ros2 이미지는 브릿지가 단일 도메인이라 값을 나누면 HMI 가 MMF 데이터를 잃는다.
> 자세한 동작과 실측은 §4.

---

## 1. 우선순위 (한 평면당)

```
DDS 평면 :  MMF_DDS_DOMAIN_ID  >  DDS_DOMAIN_ID  >  0
            VISS_DDS_DOMAIN_ID >  DDS_DOMAIN_ID  >  0   (vehicle_info_mgr)
ROS2 평면:  ROS_DOMAIN_ID                          (rmw 가 직접 읽음)
```

관례대로 **구체적인 키가 일반 키를 이긴다**. `MMF_DDS_DOMAIN_ID`/`VISS_DDS_DOMAIN_ID` 는
서비스별 override 로 남겨 둔다 — SILS 계약은 VISS 를 vECU 도메인에 고정한다
(`config/sils/contracts/mmf-endpoints.yaml` §3.4).

코드 상 기본값 `0` 은 그대로다. 보드에서는 run 스크립트/compose 가 항상 두 평면 값을
주입하므로 기본값이 쓰이는 경우는 없다.

## 2. 설정하는 곳

### ① HMI 런처 Settings (정본)

Settings → **DDS / ROS Domain** 에 두 입력이 있다. 저장하면 도메인 파일
`/opt/mmf/config/domain.env` 가 갱신된다:

```
DDS_DOMAIN_ID=42
ROS_DOMAIN_ID=42
```

두 값을 다르게 넣으면 화면에 경고가 뜬다 — 평면 간 통신이 `dds_bridge` 경유만 된다는 사실을
알린다.

### ② run 스크립트 / compose

```bash
bash docker/run-mmf.sh                                # 파일 → 없으면 42/42
DDS_DOMAIN_ID=5 ROS_DOMAIN_ID=42 bash docker/run-mmf.sh   # 분리 운영
docker compose --env-file /opt/mmf/config/domain.env -f docker/compose/docker-compose-mmf.yaml up -d
```

각 평면 우선순위: **① 명시한 env  ② 도메인 파일의 같은 키  ③ 42**.
유효 범위 0~232 를 벗어나면 경고를 찍고 42 로 되돌린다(조용히 잘못 도는 것보다 낫다).

## 3. 적용 시점 — 재생성이 필요하다

도메인은 **프로세스 시작 시점에만** 읽힌다. 실행 중 노드의 도메인을 바꾸는 방법은 없다.
게다가 `docker restart` 는 **생성 당시의 env 를 재사용**하므로 도메인 파일만 바꾸고 restart
해도 옛 도메인으로 돌아온다 (실측 2026-08-04). `--restart unless-stopped` 때문에 재부팅해도
같은 컨테이너가 복귀한다.

→ **컨테이너 재생성**(stop+rm+run)이 필요하다. 그리고 **순서가 있다.**

### 3-1. `oss-domain-apply` — 한 번에 반영 (권장)

```bash
sudo systemctl start oss-domain-apply     # 이 호스트의 플랫폼 컨테이너 재생성
oss-domain-apply --dry-run                # 실행할 명령만 확인
```

`oss-domain-apply.path` 유닛이 켜져 있으면 **도메인 파일이 바뀌는 순간 각 보드가 스스로**
실행하므로 손으로 칠 필요가 없다. 두 보드가 각자 자기 호스트만 처리하고, 크로스-호스트
조율은 필요 없다 — 런처가 81 의 파일을 쓰면 `domain_agent` 가 151 로 전파하고(실측 1초)
양쪽 path 유닛이 각각 발화한다.

### 3-2. 왜 순서가 있고, 왜 호스트에서 도는가

**재생성해야 할 대상 안에 재생성을 실행할 주체가 들어 있다** (보드 실측 2026-09-08):

| 실행자 | 도는 위치 |
|---|---|
| `adsw_manager` | `oss-mmf-ext-ros2` **안** |
| `agl_adsw_manager` | `oss-mmf-ext-agl` **안** |

그래서 lfc_monitor 의 Stop/Start 로는 이 둘을 재생성할 수 없다 — 명령을 실행하는 주체가
함께 죽어 Start 를 수행할 것이 남지 않는다. 실행자가 대상 **밖**(호스트)에 있어야 한다.

또한 adsw_manager 는 Autoware 를 `docker run` 할 때 서비스 목록의 `${ROS_DOMAIN_ID:-42}`
를 **자기 셸 env** 로 확장한다. 따라서:

```
① core + ext-ros2 재생성  →  ② ext-agl 재생성  →  ③ Autoware/rviz2 재시작
```

②③ 을 먼저 하면 Autoware 는 **몇 번 재시작해도 옛 도메인에 머문다** (2026-09-08 에 3회 재현).
③ 의 자식 컨테이너는 인자의 정본이 각각 adsw 서비스 목록과 패널 설정이므로
`oss-domain-apply` 가 직접 `docker run` 하지 않는다 — 지워 두고 소유자(lfc_monitor /
rviz2_panel)가 만들게 한다. `--children` 이 그 제거까지 해 준다.

### 3-3. 수동 재생성 (유닛이 없는 구 이미지)

```bash
# 도메인 파일이 정본이 되게 **마지막에** source 한다 — board.env 의 중복 키나
# 사용자 셸 오염(~/.bashrc 의 export ROS_DOMAIN_ID=1)을 이기기 위해서다.
set -a; . /etc/oss/compose/board.env 2>/dev/null; . /opt/mmf/config/domain.env; set +a
bash docker/run-mmf.sh        # core + ext-ros2
bash docker/run-ext-agl.sh    # ext-agl
```

> ⚠️ **셸 env 가 `--env-file` 을 이긴다** (compose 2.26 실측). 151 의 `~/.bashrc` 에
> `export ROS_DOMAIN_ID=1` 이 있어, 대화형 셸에서 `--env-file domain.env` 로 compose 를
> 돌리면 **ROS 평면만 1** 이 되는 반쪽 적용이 발생했다(DDS 는 10, ROS 는 1). 위처럼
> 도메인 파일을 셸에 마지막으로 적용하면 막을 수 있다.

런처의 `MmfRestarter` 는 이 사실을 그대로 보고한다 — 컨테이너의 두 평면 도메인을 읽어
목표와 다르면 restart 를 시도하지 않고 `needsRecreate` + 재생성 명령을 안내한다.

## 4. 두 평면을 나누면 실제로 일어나는 일 — `dds_bridge` 가 잇는다

`dds_bridge` 는 **참가자를 두 개** 만든다 (2026-09-04 적용):

- DDS 평면 참가자 — bare `lifecyclemgr::X` 를 읽고 쓴다 (`MMF_DDS_DOMAIN_ID` > `DDS_DOMAIN_ID`)
- ROS2 평면 참가자 — ROS-정규 `lifecyclemgr::msg::dds_::X_` 를 읽고 쓴다 (`ROS_DOMAIN_ID`)

각 엔드포인트는 자기 평면 참가자에 붙고, **평면을 넘는 일은 리스너 콜백**에서 일어난다
(한쪽 평면의 reader 가 받은 샘플을 반대 평면 writer 에 넘긴다). ROS2 토픽도 rmw 위에서는
`rt/` 접두사 + 이름 변환된 DDS 토픽일 뿐이라 **rclcpp 없이** 성립한다.

> 두 값이 **같으면** 참가자를 하나만 만든다 — 흔한 구성이고, 이 변경 이전과 동작이 동일하다.

**실측 (151+81 보드, 2026-09-04)** — `DDS_DOMAIN_ID=5`, `ROS_DOMAIN_ID=42`:

```
[bridge] dds domain=5 ros domain=42 (two participants)   ← 34 pair 등록
```

| 관찰 도메인 | `/mmf/*` | Autoware |
|---|---:|---:|
| 5 (DDS 평면) | 1 | 1 |
| **42 (ROS 평면)** | **39** | 326 |

발행자 확인 — `/mmf/lifecyclemgr/LifecycleDiagReport` pub=1, `/mmf/lifecyclemgr/NotifyContainerState`
pub=1, `/app_status/hmi_launcher` pub=1/sub=1 (81 의 ext-agl 이 DDS 5 에 발행 → 브릿지 → ROS 42).

**수정 전(단일 참가자)에는** 같은 설정에서 `/mmf/*` 가 도메인 5 에만 나타나고 42 에서 사라져
rosbridge 가 보지 못했고, HMI 가 MMF 데이터를 전부 잃었다.

> 브릿지가 중계하지 않는 토픽은 평면을 넘지 못한다 — **에러 없이 그냥 안 온다.**
> 분리 운영 시 가장 흔한 함정이다. 중계 목록은 `dds_bridge/main.cc` 의 pair 테이블.

같은 패턴의 선례: `adapters/ss2/src/aw_*_bridge.cpp` 는 `rclcpp::Node` + 별도 raw 참가자로
두 평면에 동시에 존재한다.

## 5. 여러 호스트에 전파하기 — `domain_agent`

런처는 **자기가 도는 호스트의** `domain.env` 만 쓴다. Autoware·MMF core·ext-ros2 가 다른
호스트(차량 컴퓨트)에 있으면 그쪽은 옛 값으로 남는다 — 실측 2026-09-04: 81 보드 Settings 를
5/5 로 저장했지만 151 보드는 42/42 그대로였다.

그 경로를 `domain_agent` (`service/extension/ros2/ext_comm/domain_agent`) 가 잇는다:

```
hmi_launcher(81) --ws--> rosbridge(151) --ROS 평면--> domain_agent(151)
                                                       → /opt/mmf/config/domain.env
```

- 런처는 Apply 때 `/mmf/domain/set` (std_msgs/String) 에 `DDS_DOMAIN_ID=<n> ROS_DOMAIN_ID=<m>`
  을 발행한다. 로컬 파일에 쓴 값과 **같은 값**이 나간다.
- agent 는 ROS 평면(`ROS_DOMAIN_ID`)에 붙는다 — 명령이 rosbridge 로 오고, 또 **도메인을
  바꾸기 전** 도메인으로 도착해야 하기 때문이다(바꾼 뒤엔 서로 안 보인다).
- 받은 문자열을 그대로 쓰지 않는다. 값만 취해 정규 형식으로 렌더하고 **원자적으로**
  기록한다(임시파일 → rename). 읽는 쪽이 반쯤 쓰인 파일을 볼 일이 없다.
- 검증: 0~232 정수만, **두 키가 모두** 있어야 반영. 부분 갱신은 거부한다(절반만 적용된
  도메인 상태를 만들지 않기 위한 의도적 제약). 거부해도 파일은 건드리지 않는다.
- `/mmf/domain/state` 에 결과를 latch 발행한다:
  `file_dds=5;file_ros=42;running_dds=42;running_ros=42;note=applied: needs container recreate`
  → **파일 값과 실행 중 값이 다르면 재생성이 아직 안 된 것**이다.

**agent 가 하지 않는 일**: 컨테이너를 재기동하지 않는다. 설정 파일을 쓰는 권한과 서비스를
죽이는 권한은 다르다 — 컨테이너 제어는 lifecycle_mgr/adsw_manager 의 책임이고, 사용자에게는
런처가 "재생성 필요"를 알린다.

### 쓰기 경로 두 가지 — "컨테이너만 업데이트하면 동작"

배포 요구가 **이미지 교체만으로 동작**하는 것이다. 그런데 도메인 파일은 호스트에 있고,
기존 보드의 ext-ros2 에는 그 디렉토리 마운트가 없다(실측 2026-09-04, 151 보드:
`/dev/shm`, `docker.sock`, `${SAVED_REGISTRY_BACKUP:-/dev/null}:/config` 뿐). 그래서 agent 는
두 경로를 순서대로 시도한다:

1. **direct** — 컨테이너 안에서 그 경로에 직접 쓸 수 있으면 그렇게 한다
   (`run-mmf.sh`/compose 가 `/mmf-config` 를 마운트한 배포. 빠르고 docker 를 안 쓴다.)
2. **docker** — 못 쓰면 이미 마운트된 `docker.sock` 으로 짧게 뜨는 컨테이너에 호스트
   디렉토리를 붙여 쓴다. 본문은 **stdin** 으로만 넘겨 payload 로 인한 셸 주입이 없고,
   헬퍼 이미지는 **자기 이미지**라 보드에 반드시 존재한다. 호스트 도메인 디렉토리가
   root 소유(실측 `/opt/mmf/config` = root:root 755)라 헬퍼는 `--user 0:0` 로 띄운다 —
   docker.sock 을 쓸 수 있는 시점에 이미 호스트 root 급 권한이므로 새로 얻는 권한은 없다.

어느 경로를 썼는지는 로그와 상태 토픽에 남는다: `applied(direct)` / `applied(docker)`.

마운트가 없으면 상태 토픽의 `file_*` 도 읽을 수 없으므로 **읽기도 같은 통로**로 폴백한다
(그러지 않으면 `file_dds=?` 만 나와 "반영됐는지" 를 알 수 없다).

## 5-1. Autoware 컨테이너도 따라온다

`saved-service-list.yaml` 의 Autoware args 는 이제 하드코딩이 아니다:

```yaml
- "-e"
- "ROS_DOMAIN_ID=${ROS_DOMAIN_ID:-42}"
```

`ContainerController::RunCommand` 가 **`system()`** 이라 이 args 는 `/bin/sh` 를 통과한다
→ 치환 코드 없이 셸이 확장하고, adsw_manager 의 `ROS_DOMAIN_ID`(= 그 호스트 domain.env 값)를
따라간다. unset 이어도 `:-42` 폴백으로 종전 동작이 유지된다.

> ⚠️ 셸을 통과한다는 것은 그 yaml 의 args 에 `$`·백틱·따옴표를 쓰면 셸이 해석한다는 뜻이다
> (이 변경 이전에도 이미 그랬다). 보드 로컬 신뢰 설정 파일이라 위험은 낮지만 임의 문자열을
> 넣지 말 것.

Autoware 는 adsw_manager 가 Stop 시 `stop+rm`, Start 시 `run` 하므로 **MMF 컨테이너보다
반영이 쉽다**(재생성이 원래 동작).

## 5-2. 남은 한계

- **SILS/ss2 는 `VISS_DDS_DOMAIN_ID` 를 계약으로 고정**한다 — `DDS_DOMAIN_ID` 로 일괄
  변경되지 않는다(의도된 것).
- 브릿지가 중계하지 않는 토픽은 평면을 넘지 못한다(§4).
- `domain_agent` 가 없는 구버전 ext-ros2 에서는 원격 전파가 되지 않는다 — 런처 스낵바가
  "원격 호스트 요청 실패"로 알린다.

## 6. 문제 해결

| 증상 | 원인 | 조치 |
|---|---|---|
| MMF 서비스끼리 서로 못 봄 | 같은 보드인데 `DDS_DOMAIN_ID`(또는 `MMF_DDS_DOMAIN_ID`) 불일치 | 같은 보드는 동일 DDS 도메인 |
| adsw_monitor 화면이 전부 `null` | Autoware 는 42, MMF DDS 평면은 다른 값 → 발행자 없음 | 두 평면을 맞추거나 브릿지 중계 확인 |
| rviz2 에 토픽이 안 보임 | `ROS_DOMAIN_ID` 가 Autoware 와 다름 | ROS 평면을 Autoware 값(42)에 맞춤 |
| 저장했는데 안 바뀜 | 컨테이너 재생성을 안 함 | §3 의 run 스크립트로 재생성 |
| VISS 데이터만 안 옴 | `VISS_DDS_DOMAIN_ID` override 가 남아 있음 | 그 env 를 지우거나 의도한 값인지 확인 |

## 7. 확인 방법

```bash
# 컨테이너가 실제로 어느 평면에 있나
docker inspect oss-mmf-core --format '{{range .Config.Env}}{{println .}}{{end}}' | grep DOMAIN

# 도메인 파일 정본
cat /opt/mmf/config/domain.env
```
