# dds_bridge — lifecycle 단일 aggregator 정책 (이중 responder 해소)

## 배경 / 문제

lfc_monitor(HMI) 는 rosbridge → dds_bridge 를 거쳐 lifecycle_mgr 의 bare-DDS 토픽을 소비한다.
`mmf/lifecyclemgr/*` 토픽 네임스페이스는 lifecycle_mgr 와 **모든 adsw_manager(ros2 + agl)** 가
공유한다. pub/sub 역할:

| 토픽 | lifecycle_mgr | adsw_manager(ros2/agl) |
|---|---|---|
| GetServiceStateRequest | PUB | SUB |
| **GetServiceStateResponse** | SUB | **PUB** |
| **NotifyServiceState** | SUB | **PUB** |
| UpdateServiceListRequest | SUB | PUB |
| UpdateServiceListResponse | **PUB** | SUB |
| LifecycleDiagReport | **PUB** | — |
| ctrl/*Response | **PUB** | — |

→ `GetServiceStateResponse` / `NotifyServiceState` 는 **각 adsw_manager 가 발행**한다.
ros2 + agl adsw_manager 를 같은 DDS 도메인에서 함께 돌리면 이 두 토픽에 **publisher 가 2개 이상**
생기고, dds_bridge 의 ForwardListener trust-on-first-use 가드가 먼저 본 publisher 하나만 신뢰하고
나머지 샘플을 드롭한다 → **monitor 가 한 변형의 서비스 상태를 잃는다(이중 responder).**

(측정: ros2+agl 동시 기동 시 `GetServiceStateResponse` 60s 기준 dropped 31 / forwarded 17.)

## 결정 (Option A — lifecycle_mgr 단일 aggregator)

monitor 는 **lifecycle_mgr 가 단일 발행하는 집계 토픽 `LifecycleDiagReport`** 만 서비스 상태의
권위 소스로 사용한다. dds_bridge 는 per-responder 토픽(`NotifyServiceState`,
`GetServiceStateResponse`)을 monitor 로 **브리지하지 않는다**.

- lfc_monitor 의 `_onDiagReport` 가 이미 service_list(name/service_state/recovery_policy/
  container_name)를 순회해 Services 패널을 채우는 권위 소스다 — 코드 변경 불필요.
- adsw_manager 들은 여전히 raw DDS 토픽에 Notify/GetServiceStateResponse 를 발행하고,
  lifecycle_mgr 가 SUBSCRIBE 해 DiagReport 로 집계한다.
- `GetServiceStateRequest`(ros→raw)는 유지 — monitor 폴링이 adsw 를 깨워 DiagReport 갱신을 유도.

## 변경

`service/extension/ros2/ext_comm/dds_bridge/main.cc` 의 lifecyclemgr 토픽 테이블에서
`NotifyServiceState`, `GetServiceStateResponse` 의 `add_lc` 등록 2줄 제거(주석 처리 + 사유 기록).

## 검증 (로컬 x86, ros2 + agl adsw_manager 동시 기동)

- 재빌드 후 dds_bridge lifecycle 포워딩: LifecycleDiagReport / NotifyContainerState /
  UpdateServiceListResponse 만 (Notify/GetServiceStateResponse 사라짐).
- rosbridge websocket: NotifyServiceState=0, GetServiceStateResponse=0,
  LifecycleDiagReport 수신(agl 등록 서비스 포함), UpdateServiceListResponse=ret_code:0.
- lfc_monitor GUI: connected, Services 패널 정상, Events 에 "update list SUCCESS".
  데이터 손실/oscillation 없음.

## 한계 / 후속

- AGL 실앱 NotifyServiceState(실제 앱 상태)는 afm-util/systemd 가 있는 실 AGL 타겟(Orin)에서만
  완전 검증 가능.
- ros2/agl UpdateServiceList 가 lifecycle_mgr 레지스트리를 **병합이 아니라 교체**하는 것으로
  관측됨(DiagReport service_list 가 마지막 등록자 기준으로 바뀜) — 별도 후속 확인 필요.
- DiagReport/UpdateServiceListResponse 에 잔여 trust-pin 드롭이 보이나, 이는 stray/zombie
  participant 억제(기존 동작)이며 monitor 일관성은 유지됨.

## F008 후속: monitor ↔ lifecycle_mgr 전용 (adsw-직접 경로 제거)

원칙: **lfc_monitor 는 lifecycle_mgr 하고만 통신한다. ros2/agl adsw_manager 와는 직접 통신하지 않는다.**

dds_bridge 의 lifecycle 토픽을 publisher/subscriber 주체 기준으로 감사해, monitor-facing
경로 중 상대가 adsw_manager 인 것을 모두 제거:

| 토픽 | 방향 | 상대 | 처리 |
|---|---|---|---|
| StartServiceResponse / StopServiceResponse | raw→ros | adsw 발행 | 제거 (monitor 는 ctrl/*Response 사용) |
| GetServiceStateRequest / StartServiceRequest / StopServiceRequest | ros→raw | adsw 구독 | 제거 (monitor 는 ctrl/* + DiagReport) |
| NotifyServiceState / GetServiceStateResponse | raw→ros | adsw 발행 | 이미 제거(F002) |
| LifecycleDiagReport, SetServiceModeResponse, UpdateServiceListResponse, ctrl/*Response, ContainerStateReport→NotifyContainerState | raw→ros | **lifecycle_mgr 발행** | 유지 |
| ctrl/* 요청, SetServiceModeRequest | ros→raw | **lifecycle_mgr 구독** | 유지 |

lifecycle_mgr 가 adsw 를 GetServiceStateRequest 로 주기 폴링(StateMonitor)하고 NotifyServiceState/
GetServiceStateResponse/NotifyContainerState 를 구독·집계해 DiagReport + ContainerStateReport 로
monitor 에 단일 발행한다. → monitor 는 lifecycle_mgr 단일 창구만 본다.

검증: monitor 수신 = LifecycleDiagReport + NotifyContainerState[ros2+agl], NotifyServiceState/
GetServiceStateResponse = 0.
