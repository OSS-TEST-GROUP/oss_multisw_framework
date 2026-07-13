# Diagnostics 컴포넌트 연동 확장 및 Monitor UI 분리 검토

> 대상: `oss_multisw_framework` Diagnostics Manager 및 ADSW Monitor 연계 방향  
> 검토일: 2026-07-13  
> 검토 범위: 현행 연동 코드, 전체 컴포넌트 확장 필요성, Diagnostics Info UI 분리, 단계별 개발안

---

## 1. 검토 결론

### 1.1 전체 컴포넌트 연동

**확장은 필요하지만 모든 컴포넌트를 일괄 연결하는 방식은 권장하지 않는다.**

현재 Diagnostics Manager는 Lifecycle Manager, OTA Manager, ROS2 ADSW Manager, Security 계열 메시지 네 종류를 policy로 분석한다. 이 구조를 모든 내부 서비스와 모든 업무 토픽으로 확대하면 다음 문제가 생긴다.

- Diagnostics Manager가 모든 컴포넌트의 내부 메시지 구조를 알아야 한다.
- 같은 장애가 lifecycle 상태와 component 진단에서 중복 보고된다.
- IDL 변경마다 Diagnostics policy와 DynamicData parser가 함께 영향을 받는다.
- 고빈도 업무 데이터를 진단 목적으로 구독하면 부하와 오탐이 증가한다.
- component 수가 늘수록 code, topic, freshness, severity 관리가 어려워진다.

따라서 확장 원칙은 다음과 같이 잡는 것이 적절하다.

> 프로세스 생존 여부는 Lifecycle Manager가 담당하고, 컴포넌트 내부의 degraded/error 원인은 표준 Component Health 계약으로 Diagnostics Manager에 보고한다.

### 1.2 ADSW Monitor의 Diagnostics Info 분리

**논리적 화면 분리는 필요하지만 별도 배포 앱 분리는 현 단계에서 권장하지 않는다.**

우선 ADSW Monitor 안에 다음 구조를 권장한다.

```text
Overview
  └─ 전체 상태와 WARNING/FATAL 개수

Services / Containers
  └─ Lifecycle 기반 현재 실행 상태와 제어

Diagnostics
  ├─ 현재 장애
  ├─ Component별 상태
  ├─ 발생/해제 이력
  └─ 원본 데이터와 상세 원인
```

즉 데이터 모델, navigation, 화면은 분리하되 설치 파일이나 실행 프로세스까지 별도 앱으로 분리하지 않는다. 별도 앱은 장기 보관, 별도 권한, 여러 차량 통합 관제 등 요구가 실제로 생길 때 재검토한다.

> 참고: 현재 저장소에는 ADSW Monitor UI 소스가 포함되어 있지 않고 `lfc_monitor` 및 DDS bridge 연계 흔적만 존재한다. 따라서 정확한 화면 컴포넌트 변경량은 Monitor 앱 저장소에서 별도 확인이 필요하다.

---

## 2. 현재 구현 확인

### 2.1 현재 등록된 진단 대상

`service/core/diagnostics/config/diagnostic_policy.yaml`에 다음 네 policy가 등록되어 있다.

| Code | 대상 | 입력 메시지 | 현재 판정 |
|---|---|---|---|
| 10 | Lifecycle Manager | `LifecycleDiagReport` | 전체/일부 서비스 실행 여부 |
| 11 | OTA Manager | `NotifyOtaMgrInfo` | `err_code` 성공 여부 |
| 12 | ROS2 ADSW Manager | `AdswServiceStatus` | `status_code` 성공 여부 |
| 13 | Security | `DetectionReport` | 탐지 topic 문자열 여부 |

Diagnostics Analyzer는 YAML의 `refer_messages`를 읽고 해당 DDS topic을 동적으로 구독한다. 새 component는 IDL과 policy를 추가하면 연결할 수 있는 형태지만, 현재 구조는 arbitrary source message에 강하게 결합된다.

### 2.2 현재 데이터 흐름

```text
Lifecycle / OTA / ADSW / Security DDS message
                │
                ▼
        DiagnosticAnalyzer
        · DynamicData 역직렬화
        · YAML condition 평가
                │
                ▼
         DiagInfoRegistry
                │
         ┌──────┴────────┐
         ▼               ▼
 DiagnosticInfo 요청/응답  DiagnosticNotification
                         (Monitor/ROS 측)
```

### 2.3 현재 진단 정보 모델

현재 진단 결과는 다음 필드 중심이다.

- `diag_code`
- `diag_name`
- `status`: UNKNOWN, PASS, FATAL, WARNING
- `message`
- `source_message_name`
- `raw_data`: JSON 문자열

단일 PoC에는 충분하지만 전체 컴포넌트 운영 진단에는 component instance, 발생 시각, 마지막 관측 시각, stale 상태, 장애 식별자, 발생/해제 여부가 부족하다.

---

## 3. 확장 전에 먼저 해결할 기반 문제

전체 component policy를 추가하기 전에 아래 문제를 우선 해결해야 한다. 그렇지 않으면 연동 대상만 늘고 신뢰할 수 없는 “초록색 상태”가 많아질 수 있다.

### 3.1 [Critical] 메시지 미수신과 stale 상태를 장애로 판단하지 않음

현재 Analyzer는 참조 메시지가 도착할 때만 분석을 요청한다. 마지막 PASS 이후 publisher가 중단되면 새로운 메시지가 없으므로 이전 PASS가 계속 Registry에 남을 수 있다.

필요한 기능:

- reference message별 `expected_interval`과 `stale_after`
- 마지막 수신 시각 저장
- 주기적인 freshness 평가
- `STALE` 또는 `UNAVAILABLE` 상태
- 최초 메시지 수신 전 `UNKNOWN` 유지

예시:

```yaml
refer_messages:
  - name: NotifyOtaMgrHeartbeat
    message: /mmf/otamanager/diagnostic_service/NotifyOtaMgrHeartbeat
    protocol: dds
    expected_interval_ms: 3000
    stale_after_ms: 10000
```

OTA Manager는 이미 heartbeat를 발행하지만 현재 diagnostic policy는 `NotifyOtaMgrInfo`만 참조하므로 liveness 판정에 이를 사용하지 않는다.

### 3.2 [High] 초기화 실패를 기록만 하고 성공 처리

`DiagnosticManager::Initialize()`는 Analyzer 또는 Publisher 초기화가 실패해도 로그만 남긴 뒤 `true`를 반환한다. 이 경우 프로세스는 살아 있지만 실제 진단은 동작하지 않는 부분 초기화 상태가 된다.

권장:

- Analyzer 또는 Publisher 초기화 실패 시 전체 Initialize 실패
- 생성한 DDS entity와 task rollback
- readiness가 성공하기 전 lifecycle RUNNING 처리 금지

### 3.3 [High] Stop/Finalize 경로 부재

현재 `DiagnosticManager::Stop()`은 비어 있다. subscriber, periodic task, DDS participant를 명시적으로 정리하는 종료 순서를 구현해야 한다.

권장 순서:

```text
새 요청 차단
 → periodic publication task 중지
 → subscriber 제거
 → Analyzer/Publisher Finalize
 → DDS protocol Finalize
 → executor stop
```

### 3.4 [High] ROS 응답 protocol 선택값이 실제 Publish에 사용되지 않음

`DiagnosticPublisher`는 `rt/` 요청이면 `response_protocol_type = DDS_ROS`를 계산하지만 실제 `Message` 생성에는 항상 `MessageProtocolType::DDS`를 전달한다. List response도 동일하다.

또한 notification은 topic 이름은 `rt/mmf/...`인데 protocol type은 DDS다. Monitor 연동을 확대하기 전에 raw DDS와 ROS-facing topic의 publisher/type/QoS를 일관되게 정리해야 한다.

### 3.5 [High] Security 연동은 “완료”로 보기 어려움

현재 core `security_mgr`와 ROS2 `security_client`의 main은 테스트 문자열만 출력한다. 또한 Security IDL에는 같은 module/type 이름의 `DetectionReport`가 서로 다른 string/char-array 형태로 중복 존재한다.

따라서 현재 상태는 “Security Manager 정상 상태 연동”이라기보다 DetectionReport policy와 IDL 흔적이 존재하는 수준으로 분류해야 한다.

권장:

- canonical `DetectionReport` IDL 하나로 통합
- 실제 publisher 소유자 명시
- Security Manager 자체 health와 보안 탐지 event를 분리
- 보안 raw data의 Monitor 노출 시 masking/권한 적용

### 3.6 [Medium] 진단 계약과 설정 품질

- YAML key가 `diagnosic_policies`로 오타지만 parser도 같은 오타에 종속되어 있다.
- ADSW PASS message가 `OTA manager service success.`로 잘못 작성되어 있다.
- 진단 code 10~13의 namespace/할당 규칙이 없다.
- status에 INFO, DEGRADED, UNAVAILABLE, STALE 또는 RESOLVED 개념이 없다.
- `DiagnosticInfoListResponse`는 code/name만 반환해 상세 목록을 얻으려면 항목별 추가 요청이 필요하다.
- `raw_data` 크기 제한, schema version, 민감 정보 정책이 없다.
- `PublishUpdatedInfo()`가 빈 함수로 남아 있다.

---

## 4. 전체 컴포넌트 확장 판단

### 4.1 판단 기준

다음 질문 중 하나 이상이 참일 때만 별도 진단 연동을 추가한다.

1. 프로세스가 RUNNING이어도 기능이 degraded될 수 있는가?
2. 장애 원인이 Lifecycle 상태만으로 설명되지 않는가?
3. 운영자가 즉시 조치할 수 있는 정보인가?
4. 안정적인 health/event 계약을 제공할 수 있는가?
5. 같은 정보를 다른 Manager가 이미 집계하고 있지 않은가?

### 4.2 권장 우선순위

| 우선순위 | 대상 | 판단 | 권장 진단 항목 |
|---|---|---|---|
| P0 | Diagnostics Manager 자체 | 필수 | policy load, DDS readiness, analyzer/publisher 상태, queue backlog |
| P0 | Vehicle Info Manager | 필요 | VISS transport readiness, provider freshness, queue/backpressure, set 실패 |
| P0 | DDS/ROS bridge(`ext_comm`) | 필요 | endpoint match, parse 실패, forward 실패, trust pin/drop 수 |
| P0 | 기존 4개 연동 안정화 | 필수 | freshness, canonical topic/type, 실제 producer 확인 |
| P1 | AGL ADSW Manager | 선택 확장 | container control 실패, app state collection 실패. 단순 alive는 lifecycle 재사용 |
| P1 | OTA Client(ROS2/AGL) | 선택 확장 | Manager 연결, download/apply 상태, 마지막 heartbeat. OTA 업무 상태와 health 구분 |
| P1 | ROS2 ADSW Manager 상세 | 확장 | discovery 상태, container controller, 명령 실패. 기존 단일 status code 세분화 |
| P2 | Adaptive AUTOSAR | 구현 후 | 현재 실질 구현이 없어 연동 선행 가치 낮음 |
| P2 | SiLS component | 필요 시 | Run Coordinator, Clock Master, vECU readiness. SiLS run에 한정 |

### 4.3 연동하지 말아야 할 대상

- 모든 업무 DDS topic
- 고빈도 센서/vehicle data 원본
- Lifecycle Manager가 이미 제공하는 단순 process alive 정보의 중복
- 운영 조치와 연결되지 않는 debug-only metric
- 안정된 schema와 ownership이 없는 임시 topic

---

## 5. 권장 Diagnostics 아키텍처

### 5.1 표준 Component Health 계약

장기적으로 Diagnostics Manager가 각 component의 arbitrary 업무 IDL을 직접 해석하기보다 공통 health report를 받는 구조가 적합하다.

```text
각 Component
  ├─ 업무 topic
  └─ ComponentHealthReport ─────┐
                               ▼
LifecycleDiagReport ──▶ Diagnostics Manager
Security Event ────────▶ · freshness/stale
OTA Event ─────────────▶ · policy/correlation
                               │
                     ┌─────────┴─────────┐
                     ▼                   ▼
             Current Health Snapshot   Diagnostic Event
                     │                   │
                     └─────────┬─────────┘
                               ▼
                      ADSW Monitor Diagnostics
```

개념 IDL:

```idl
enum HealthStatus {
    UNKNOWN,
    HEALTHY,
    DEGRADED,
    UNAVAILABLE,
    FAILED
};

struct ComponentHealthReport {
    string component_id;
    string instance_id;
    HealthStatus status;
    long error_code;
    string reason;
    long long observed_at_ms;
    unsigned long long sequence;
    string detail_json;
};
```

주의:

- 모든 event를 위 health snapshot 하나로 표현하지 않는다.
- 보안 탐지, OTA 단계 실패처럼 이력이 중요한 것은 별도 event로 유지한다.
- Lifecycle은 container/service state의 authoritative source로 유지한다.

### 5.2 State와 Event 분리

| 유형 | 예 | 보관/표시 |
|---|---|---|
| Current State | component HEALTHY/DEGRADED | 최신 snapshot, Overview badge |
| Incident Event | OTA apply 실패, security 탐지 | 발생/해제 이력, Diagnostics 화면 |
| Metric | queue depth, forward error count | threshold 판정용, 필요 시 상세 |
| Raw Evidence | 원본 JSON | 제한 크기, 상세 화면, 권한 적용 |

---

## 6. ADSW Monitor Diagnostics 화면 분리안

### 6.1 권장: 같은 앱 안의 독립 화면

```text
ADSW Monitor
  ├─ Overview
  │   ├─ 전체 상태
  │   ├─ FATAL / WARNING 개수
  │   └─ 최근 주요 incident
  ├─ Services
  │   └─ start/stop/recovery 등 lifecycle 제어
  ├─ Containers
  │   └─ 실행 상태와 resource
  └─ Diagnostics
      ├─ Active Issues
      ├─ Component Health
      ├─ History
      └─ Raw Detail
```

Services 화면에 Diagnostics raw data를 계속 추가하면 lifecycle 상태와 원인 분석 정보가 섞인다. 서비스 카드에는 최악 severity badge와 “상세 보기” link만 제공하고 진단 본문은 Diagnostics 화면에서 보여주는 것이 좋다.

### 6.2 별도 앱으로 분리할 조건

다음 조건이 실제로 생기기 전에는 별도 executable/app을 만들지 않는다.

- 차량 여러 대를 장기간 통합 관제
- 운영자와 보안 담당자의 접근 권한 분리
- 대용량 이력 검색 및 외부 DB 필요
- ADSW 제어 기능 없이 진단만 사용하는 별도 사용자군
- Monitor 장애와 독립적으로 진단 화면을 유지해야 하는 요구

### 6.3 UI 데이터 요구사항

- component/instance 기준 grouping
- severity, 발생 시각, 마지막 관측 시각, stale 표시
- active/resolved 구분
- 같은 원인의 notification deduplication
- 검색과 severity/component filter
- raw JSON 기본 접기, 크기 제한, escape 처리
- Security detail masking
- 연결 단절 시 마지막 상태를 초록색으로 유지하지 않고 STALE 표시

---

## 7. 단계별 개발 권고

### Phase 0 — 현재 기반 안정화

- freshness/stale 평가 추가
- Analyzer/Publisher 초기화 실패 전파
- Stop/Finalize 구현
- DDS/DDS_ROS 응답 protocol 오류 수정
- Security DetectionReport IDL 단일화
- YAML typo/message 수정 및 schema test
- timestamp/instance/status 모델 확정

**통과 기준:** source publisher 중단 후 설정된 timeout 안에 STALE/UNAVAILABLE로 전환되고 Monitor에 표시된다.

### Phase 1 — Vertical Slice

- Vehicle Info Manager 한 개를 신규 대상로 선택
- 최소 ComponentHealthReport 또는 안정된 전용 health topic 구현
- Diagnostics Manager freshness/policy 연결
- ADSW Monitor 안에 Diagnostics 독립 tab/page 추가
- active issue와 resolved 상태 한 건씩 종단 검증

**통과 기준:** Vehicle Info Manager는 살아 있지만 provider가 끊긴 degraded 상태를 lifecycle RUNNING과 별도로 표시한다.

### Phase 2 — 운영 핵심 확장

- DDS/ROS bridge
- AGL ADSW Manager의 고유 제어 오류
- OTA Client 연결/적용 상태
- ROS2 ADSW 상세 상태
- code namespace와 component registry 도입

### Phase 3 — 선택 확장

- Adaptive AUTOSAR 실제 구현 연동
- SiLS 전용 health source
- 이력 저장, 검색, 외부 telemetry export
- 별도 Diagnostics 앱 필요성 재평가

---

## 8. 테스트 전략

### Contract test

- 모든 health publisher가 canonical IDL/type/topic/QoS와 일치
- component_id/instance_id/code 중복 검증
- schema version 호환성

### Freshness test

- 정상 heartbeat → HEALTHY
- publisher 중단 → STALE/UNAVAILABLE
- 재연결 → HEALTHY 또는 RESOLVED event
- clock 변경에도 timeout 오판 없음

### End-to-end test

```text
Component fault 주입
 → health/event DDS publish
 → Diagnostics policy 평가
 → snapshot/notification 생성
 → ROS bridge 또는 Monitor client 수신
 → UI Active Issues 표시
 → 복구
 → RESOLVED 및 상태 정상화
```

### Failure test

- 잘못된 IDL 및 policy
- raw_data oversized/invalid UTF-8
- duplicate notification
- Diagnostics Manager 재시작
- Monitor 늦은 접속
- DDS domain 불일치

---

## 9. 최종 권고

1. **전체 component를 일괄 연결하지 않는다.** Diagnostics 자체, Vehicle Info Manager, DDS/ROS bridge부터 우선한다.
2. **프로세스 생존은 Lifecycle, 내부 degradation은 Component Health**로 책임을 분리한다.
3. 연동 확대 전에 freshness, 초기화 실패, shutdown, DDS/ROS publish 경로를 수정한다.
4. ADSW Monitor에서 Diagnostics는 **별도 tab/page로 분리**하되 별도 앱 배포는 보류한다.
5. Phase 1은 Vehicle Info Manager 한 개의 vertical slice로 제한하고 종단 동작을 증명한 뒤 확장한다.

현재 판정:

> **조건부 추진 권장 — Diagnostics 기반 안정화 후 선택적 component 확장. Monitor 내부 화면 분리는 권장, 별도 앱 분리는 현 단계에서 불필요.**

---

_문서 끝._
