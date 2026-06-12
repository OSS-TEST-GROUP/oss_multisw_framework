# OTA: ota_mgr ↔ ota_monitor 로컬 x86 통합 (F011)

lifecycle(F002)와 평행 구조의 OTA 통합 검증 노트. 로컬 x86 한 머신에서
`ota_mgr`(core) + `ota_client`(ros2 ext) + `dds_bridge` + `rosbridge` + `ota_monitor`(Flutter)
를 co-locate 구동하여 **OTA 통신·등록·상태 평면**을 검증한다. 실제 업데이트
다운로드/플래싱/앱 재배포는 본 범위 밖(L2 Orin/ADLink).

## 토폴로지

```
ota_monitor(Flutter, roslibdart)
   │  ws://127.0.0.1:9090  (ROS 토픽명 /mmf/otamanager/...)
rosbridge_server (ext-ros2 컨테이너)
   │  ROS-canonical  rt/mmf/otamanager/...  (otamanager::msg::dds_::X_)
dds_bridge  ← trust-on-first-use, raw↔ros 동적 IDL 변환
   │  bare-DDS  /mmf/otamanager/...  (otamanager::X)
ota_mgr(core 컨테이너, OtaManager) ── ota_client(ext-ros2)
```

기존 통합 컨테이너가 이미 OTA 스택을 기동한다:
- `oss-mmf-core` (core_entrypoint.sh): lifecycle_mgr + diagnostics + vehicle_info_manager_app + **OtaManager**
- `oss-mmf-ext-ros2` (ros2_entrypoint.sh): **ota_client** + **dds_bridge** + rosbridge_server + adsw_manager

dds_bridge 는 otamanager 토픽군을 모두 등록:
- `otaactor_service`: 6 req/resp 쌍 (GetAppList, GetAppUpdateSettings, GetAppUpdateState, SetAppUpdate, … + Delete)
- `otaclient_service`: SetOtaClientRegistration(+Response), NotifyOtaManagerReady, NotifyAppUpdateStatus
- `diagnostic_service`: NotifyOtaMgrHeartbeat

## 검증된 제어 평면 (websocket 스모크, ota_monitor 계약 그대로)

| 항목 | 토픽 / 타입 | 방향(bridge) | 결과 |
|---|---|---|---|
| Heartbeat | diagnostic_service/NotifyOtaMgrHeartbeat · `otamanager/NotifyOtaMgrHeartbeat` | raw→ros | PASS (seq 단조 증가, ts_ms) |
| 등록 왕복 | otaclient_service/SetOtaClientRegistration → …Response | ros→raw, raw→ros | PASS (client_id 반환) |
| 액터 질의 왕복 | otaactor_service/GetAppList → GetAppListResponse · `otamanager/msg/*` | ros→raw, raw→ros | PASS (등록 앱 목록 반환) |

→ 양방향(ros→raw 2종, raw→ros 3종) 모두 dds_bridge 경유로 정상 왕복.

## 아키텍처 소견 1 — NotifyOtaManagerReady 는 end-to-end VOLATILE (one-shot)

`OtaManager` 는 NotifyOtaManagerReady 를 **TRANSIENT_LOCAL** 로 발행한다
(`otam_client_service.cc` — late-join DDS 구독자 수신 가능). 그러나 dds_bridge 는
republish writer 를 **의도적으로 VOLATILE 로 강제**한다
(`dds_bridge.cc:123-128` — 응답류 TRANSIENT_LOCAL replay storm 방지 목적).
브리지 reader 도 기본값(VOLATILE).

**결과**: TRANSIENT_LOCAL 내구성이 브리지를 넘어가지 못한다. 매니저 부팅 시 한 번
발행되는 NotifyOtaManagerReady 는, 그 순간 브리지+rosbridge 가 살아있고 모니터가
구독 중일 때만 전달된다. **늦게 접속한 ota_monitor 는 ready one-shot 을 받지 못한다.**

설계상 문제 아님 — ota_monitor 는 **연속 발행되는 NotifyOtaMgrHeartbeat**(RELIABLE)로
매니저 생존을 판단한다. `client_service_controller.dart` 의 `isHeartbeatStale`(기본
9s = 3 × 3s 발행주기)가 Manager Status 카드를 구동하며, `NotifyOtaManagerReady`
기반 `_managerReady` 는 best-effort 보조 신호다.

→ 라이브 매니저-ready 표시는 heartbeat 로 충족.

### 소견 1-후속 — bridge durable QoS 확장 (F011 2차) + 잔여 한계

ready one-shot 의 신뢰 전달을 위해 dds_bridge 에 **per-pair `durable` 플래그**를 추가했다
(`dds_bridge.h` PairConfig.durable, `dds_bridge.cc` reader/writer TRANSIENT_LOCAL +
KEEP_LAST(1), `main.cc` 에서 NotifyOtaManagerReady 만 durable=true). 검증 결과:

- ✅ 브리지 republish writer 가 `Durability: TRANSIENT_LOCAL` 로 광고됨 (`ros2 topic info -v`).
- ✅ rosbridge_server 는 TRANSIENT_LOCAL 퍼블리셔를 감지하면 구독을 자동으로
  TRANSIENT_LOCAL 로 승격한다 (`rosbridge_library/internal/subscribers.py:146`). → rosbridge hop 은 병목 아님.
- ✅ 디스커버리 안정 후 수동 기동한 bridge 인스턴스는 매니저의 retained ready 를
  `trust pinned`+`forwarded` 로 정상 포착.
- ⚠️ **잔여 한계 (근본 원인은 소스의 one-shot 성격)**: NotifyOtaManagerReady 는 매니저 Init 에서
  단 1회 발행된다. end-to-end 전달은 "브리지의 durable reader 가 그 단 하나의 샘플을 브리지 자신의
  기동 시점에 포착" 하는 데 의존하는데, 자동 기동된 백그라운드 브리지는 dynamic-type + trust-pin +
  디스커버리 타이밍 때문에 이를 신뢰성 있게 포착하지 못한다(네이티브 TL 리더로도 retained 샘플 0).
  **bridge QoS 만으로는 소스가 1회성인 신호를 신뢰 전달로 만들 수 없다.**

**신뢰 전달을 위한 소스측 옵션(권장, 후속 결정 필요)**:
- (A) ota_mgr 가 NotifyOtaManagerReady 를 주기 재발행(heartbeat 와 동조) — 늦게 접속한
  모니터도 다음 주기에 ready 수신. bridge TL + rosbridge auto-TL 이미 갖춰져 있어 소스 주기화만 더하면 됨.
- (B) ota_monitor 가 첫 heartbeat 수신을 "manager ready" 로 간주 — 앱측 변경, 추가 토픽 불필요
  (heartbeat 는 이미 RELIABLE·연속, 본 통합에서 검증됨).

bridge durable 플래그는 그대로 유지(올바르고 필요한 개선 — 타이밍이 맞을 때 ready 전달을 가능케 하며
회귀 없음). 소스측 (A)/(B) 중 하나를 더해야 late-join 신뢰 전달이 완성된다.

## 아키텍처 소견 2 — 매니저 재기동 시 dds_bridge 재기동 필요 (trust-on-first-use)

dds_bridge 는 토픽별로 첫 publisher 를 trust-pin 한다. **OtaManager(또는
lifecycle_mgr)가 재기동되면** 새 DDS participant 가 되는데, 브리지는 죽은 구세대
publisher 를 계속 pin 한 채 신규 publisher 메시지를 `dropped (untrusted publisher)`
로 버린다 — 자동 re-pin 이 신뢰성 있게 일어나지 않음.

검증 중 재현: `docker restart oss-mmf-core` 직후 heartbeat/등록/질의 전부 drop →
`docker restart oss-mmf-ext-ros2`(브리지 재기동) 로 신규 publisher 재-pin → 복구.

**운영 수칙**: 매니저 컨테이너 재기동 후에는 브리지(ext-ros2) 도 재기동한다.
또는 기동 순서를 매니저 → 브리지로 둔다. (lifecycle 의 zombie/trust-pin 이슈와 동일
근본 원인 — `docs/dds-bridge-single-aggregator.md` 참조.)

## 범위 밖 (L2 타겟)

- NotifyAppUpdateStatus 이벤트: 실제 앱 업데이트 흐름(다운로드→적용)에서만 발행.
- 실제 OTA 아티팩트 다운로드/플래싱/앱 재배포 검증 → Orin/ADLink 타겟.
