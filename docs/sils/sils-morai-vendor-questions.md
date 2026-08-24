# MORAI 벤더 확인 요청 (루프 A 선행)

> **한 줄**: MORAI 연동을 시작하기 전에 **벤더만 답할 수 있는 것**을 모은 문서. 소스로 풀 수 있는
> 것은 이미 풀었고(루프 B, [ss2-bringup](sils-ss2-bringup.md)), 여기 남은 것은 MORAI 내부를 볼 수
> 없어 막힌 항목이다.
>
> 관련: [simulator-loops](sils-simulator-loops.md)(루프 A/B 구조) ·
> [real-bridge-requirements](sils-real-bridge-requirements.md)(BR-M/BR-C) ·
> [morai-plant-adapter-design](sils-morai-plant-adapter-design.md)(버스 결합 Option 2)
>
> 작성 2026-08-06, 개정 2026-08-07(공개 자료 조사 반영). **§7 이 그대로 보내는 표다.**

---

## 1. 왜 이 질문들인가 — 답이 무엇을 가르나

우리는 vECU(가상 ECU)를 **시뮬레이터 교체 가능하게** 설계했고, MORAI 는 그 플랜트 중 하나로 붙는다.
어댑터 한 컨테이너만 교체되고 우리 코드는 바뀌지 않는다. 따라서 **우리가 알아야 하는 것은
MORAI 의 내부가 아니라 경계면 4가지**다: 시간 · 명령 · 단위 · 배포.

| 답 | 무엇이 갈리나 |
|---|---|
| **Q1 시간** | **검증 등급이 갈린다.** 스텝 제어가 되면 실행마다 값이 동일한 **회귀 기준선(golden)** 을 만들 수 있고, 안 되면 "동작은 한다"까지만 증명된다 |
| **Q2 종방향 명령** | 어댑터가 **무손실**인지, 차량 파라미터로 **튜닝된 근사**인지 |
| **Q3 단위·프레임** | 어댑터 난이도. 여기서 틀리면 **아무 데서도 에러가 안 나고 판정만 조용히 틀린다** |
| **Q4 시나리오 자산** | 요구 판정 기준을 **자동 추출**할 수 있는지, **따로 저작**해야 하는지 |
| **Q5 통신·배포** | 붙는가 · CI 에 넣을 수 있는가 (RMW · 헤드리스 · 라이선스 · GPU) |

### ★ 개정 이유 (2026-08-07) — 질문의 층위가 내려갔다

공개된 `morai_msgs` 저장소를 읽어 **Sync Mode 가 존재하고 우리가 필요한 형태의 lockstep 이라는
것을 확인했다**(§2). 따라서 Q1 은 더 이상 *"스텝 제어를 지원합니까"* 가 아니다 — 답을 아는 채로
그렇게 물으면 원론적인 답이 돌아올 여지만 준다.

| | 개정 전 | 개정 후 |
|---|---|---|
| Q1 | 지원합니까? (4지선다 a~d) | **ROS 2 에서 쓸 수 있습니까?** + 동작 세부 |
| 근거 | 없음 | 메시지·서비스 정의 인용 |

**Q1 이 여전히 최우선이다.** 다만 병목이 "지원 여부"에서 **"우리 ROS 2 구성에서의 가용 경로"** 로
바뀌었다. 나머지는 배관이고 병렬 진행 가능하다.

---

## 2. Q1 · 시간 동기 — Sync Mode ★최우선

우리 구조는 **중앙 시계(ClockMaster)가 tick 을 발급하고, 모든 참가자가 "이번 tick 끝냈다"고
응답해야 다음 tick 으로 넘어간다.** 아무도 앞서가지 못하므로 실행을 반복해도 값이 같다.

### ★ 공개 자료로 이미 확인한 것 (2026-08-07)

공개된 `morai_msgs` 저장소를 읽어 **Sync Mode 가 존재하고, 우리가 필요한 형태의 lockstep 이라는
것을 확인**했습니다. 따라서 아래 질문은 "지원합니까?" 가 아니라 **"이 경로를 우리 구성에서 쓸 수
있습니까?"** 입니다.

확인한 정의 ([MORAI-ROS_morai_msgs](https://github.com/MORAI-Autonomous/MORAI-ROS_morai_msgs), ROS 1):

```
SyncModeCmd      { user_id, start_sync_mode, time_step [ms] }     # 고정 시간 스텝으로 시작
SyncModeCtrlCmd  { frame, CtrlCmd command, sensor_capture }       # 제어를 frame 에 태그
WaitForTick      { user_id, frame }
WaitForTickResponse { tick_status, pause_status, frame,
                      EgoVehicleStatus vehicle_status, time }     # 틱 완료 후 상태를 응답에 실어 반환
```

서비스: `MoraiSyncModeCmdSrv` · `MoraiWaitForTickSrv` · `MoraiSyncModeCtrlCmdSrv` ·
`MoraiSyncModeSetGearSrv` · `MoraiSyncModeSLSrv` · `MoraiSyncModeAddObjectSrv` ·
`MoraiSyncModeRemoveObjectSrv`

**이 설계는 우리 barrier 와 사실상 같습니다** — 역할만 뒤집혀 있습니다(우리가 호출, MORAI 가 응답).
`time_step` 이 ms 고정이라 시간이 논리적이고, 제어가 `frame` 에 태그되어 "최신값 사용"이 아니며,
`WaitForTick` 응답에 **틱 이후 차량 상태가 함께** 실려 오므로 상태 수신 경쟁이 없습니다.

### ⚠ 그런데 ROS 2 메시지 집합에는 없습니다

| 저장소 | 메시지 수 | Sync Mode / WaitForTick |
|---|---|---|
| [MORAI-ROS_morai_msgs](https://github.com/MORAI-Autonomous/MORAI-ROS_morai_msgs) (ROS 1) | 137 | ✅ 있음 |
| [MORAI-ROS2_morai_msgs](https://github.com/MORAI-Autonomous/MORAI-ROS2_morai_msgs) (ROS 2, main, 2026-06 갱신) | 32 | ❌ **없음** |

또한 [gRPC 절차](https://morai-sim--drive-user-manual--en-22-r2.scrollhelp.site/msdume2/grpc-procedures)
목록에는 `Pause`/`Resume` 만 있고 step/tick 이 없으며,
[ROS quick start](https://morai-sim--drive-user-manual--en-22-r2.scrollhelp.site/msdume2/ros-quick-start-manual)
는 ROS 1(melodic) 기준입니다.

**저희는 ROS 2 기반입니다.** 그래서 실제 질문은 아래입니다.

### Q1-1. Sync Mode 를 **ROS 2 에서** 쓸 수 있습니까? ★가장 중요

| | 상황 | 우리 대응 |
|---|---|---|
| **(a)** | ROS 2 에서도 지원 — 공개 `morai_msgs` 저장소만 뒤처진 것 | ✅ 최선. 메시지 정의만 받으면 즉시 진행 |
| **(b)** | Sync Mode 는 **네트워크(UDP/gRPC) API** 기능이고 ROS 는 래퍼일 뿐 → ROS 버전 무관 | ✅ 좋음. **네트워크 API 규격 요청** |
| **(c)** | ROS 1 전용 | 🟡 ROS 1 브리지를 별도 컨테이너로 두어야 함 — 가능한지 확인 필요 |
| **(d)** | 더 이상 지원하지 않음 / 계획 없음 | ❌ 결정론 포기, 동작 검증까지만 |

**(b) 라면**: 네트워크 API 문서(또는 포트·패킷 규격)를 받을 수 있습니까?

### Q1-2. Sync Mode 의 동작을 확인하고 싶습니다

- `SyncModeCmd.time_step` (ms) 은 **시뮬레이션 시각이 정확히 그만큼 증가**한다는 뜻입니까,
  아니면 목표 주기(벽시계)입니까?
- `WaitForTick` 호출은 **틱 완료까지 블로킹**입니까?
- `WaitForTickResponse.time` 이 문자열인데, **포맷과 기준**(시뮬 시작 이후 경과 / 절대시각)은?
- 한 번의 `WaitForTick` 이 **정확히 1 프레임**을 진행시킵니까?

### Q1-3. 제어와 프레임의 결합

- `SyncModeCtrlCmd.frame` 에 넣은 값이 **그 프레임의 물리 계산에 반영**됩니까?
- 해당 프레임의 제어를 보내지 않고 `WaitForTick` 을 호출하면 어떻게 됩니까
  (직전 값 사용 / 대기 / 오류)?
- `sensor_capture` 플래그의 정확한 의미는?

> 이것이 결정론의 핵심입니다. 제어가 프레임에 확실히 결합되지 않으면, 스텝 제어가 되더라도
> 프레임마다 반영되는 명령이 실행마다 달라집니다.

### Q1-4. Sync Mode 에서 **다른 출력들**은 어떻게 됩니까?

- Sync Mode 중에도 일반 상태 토픽(`EgoVehicleStatus` 등)이 **자유 실행으로 계속 발행**됩니까?
- 발행된다면 그 값은 `WaitForTickResponse.vehicle_status` 와 **같은 프레임**입니까?
- 센서 데이터(카메라·LiDAR)는 `sensor_capture` 로만 나옵니까?

> **왜 묻는가**: 다른 시뮬레이터에서 시계 토픽이 BEST_EFFORT 로 자유 실행되어, 동기화된 경로와
> 자유 실행 경로가 섞이는 것을 실측한 적이 있습니다. 두 경로가 공존하면 어느 쪽을 믿어야 하는지가
> 계약으로 정해져 있어야 합니다.

### Q1-5. Sync Mode 의 제약

- 지원되지 않는 기능이 있습니까 (특정 센서·다중 Ego·시나리오 기능 등)?
- 성능은 어느 정도입니까 (실시간 대비 배속, 프레임당 소요)?
- `user_id` 의 역할은 무엇입니까 — 다중 클라이언트를 지원합니까?

---

## 3. Q2 · 종방향 명령 방식

우리 정규 명령은 **부호 있는 가속도 `m/s²`** 입니다 (양수=가속, 음수=감속).

### Q2-1. `CtrlCmd` 의 `longCmdType` 에 **가속도(m/s²) 직접 입력** 모드가 있습니까?

| | 지원 | 결과 |
|---|---|---|
| **(a)** | 가속도(m/s²) 모드 있음 | ✅ 우리 값을 **무손실 직결** |
| **(b)** | throttle/brake 정규화(0~1)만 | 🟡 변환 필요 → **차량 최대 가·감속 파라미터**를 알려주셔야 합니다 |
| **(c)** | 목표 속도(m/s) 모드만 | 🟡 우리 쪽에서 적분·제어기 추가 필요 |

**(b) 라면**: 차량 모델별 최대 가속도·최대 감속도 값을 어디서 확인합니까?

### Q2-2. 조향 입력의 **단위와 기준**은 무엇입니까?

- 단위: **도(deg)** 입니까 **라디안(rad)** 입니까?
- 기준: **앞바퀴 조향각** 입니까 **스티어링 휠 각** 입니까?
- 휠 각이라면 조향비(steering ratio)는?
- 부호 규약: **좌회전이 양수**입니까?

> 우리 IDL 주석에 이 항목이 `[확인]` 으로 남아 있는 유일한 필드입니다. 단위를 틀리면 값이
> 57.3 배로 들어오는데 **어디서도 에러가 나지 않습니다.**

### Q2-3. 기어 enum 값을 알려주십시오

우리 규약은 `0=P, 1=R, 2=N, 3=D` 입니다. MORAI 측 값은?

---

## 4. Q3 · 상태 출력의 단위·프레임

`EgoVehicleStatus`(또는 상태 메시지)의 각 필드에 대해 확인 부탁드립니다.

| 필드 | 확인 사항 |
|---|---|
| **position** | 좌표계 기준(map/world 원점 정의), 단위 m 맞습니까? |
| **heading / orientation** | **deg 입니까 rad 입니까?** 기준축과 회전 방향(CCW 양수?) |
| **velocity** | **km/h 입니까 m/s 입니까?** 그리고 **차량 좌표계**입니까 **전역 좌표계**입니까? |
| **acceleration** | 상태 메시지에 **포함됩니까?** 없다면 IMU 토픽으로 제공됩니까, 아니면 속도 미분이 유일한 경로입니까? |
| **yaw rate** | 제공됩니까? 단위는 deg/s 입니까 rad/s 입니까? |
| **steering angle (현재값)** | 제공됩니까? 단위·기준은 Q2-2 와 동일합니까? |
| **타임스탬프** | 시뮬레이션 시각입니까 벽시계입니까? |

> **왜 이렇게 세세히 묻는가**: 이 계층의 오류는 **런타임에 아무 증상이 없습니다.** 컴파일도 되고
> 통신도 되고 시뮬레이션도 돕니다. 다만 검증 판정만 조용히 틀립니다. 그래서 배선 전에 문서로
> 확정하려 합니다.

---

## 5. Q4 · 시나리오 자산

### Q4-1. **OpenSCENARIO(`.xosc`) 파일을 직접 실행**할 수 있습니까?

| | | 결과 |
|---|---|---|
| **(a)** | `.xosc` 를 직접 로드·실행 | ✅ 시나리오와 **판정 기준을 한 파일에서** 뽑을 수 있음 |
| **(b)** | 자체 포맷만, 변환기 제공 | 🟡 변환 경로 확인 필요 |
| **(c)** | 자체 포맷만, 변환 없음 | ❌ 판정 기준을 **따로 저작**해야 함 |

지원한다면 **OpenSCENARIO 버전**(1.0/1.1/1.2)과 지원 범위는?

### Q4-2. 시나리오 실행을 **CLI/API 로 자동화**할 수 있습니까?

GUI 조작 없이 "시나리오 파일 지정 → 실행 → 종료" 를 스크립트로 구동할 수 있어야 CI 에 넣을 수
있습니다.

### Q4-3. 실행 결과(궤적·이벤트 로그)를 **파일로 내보낼** 수 있습니까? 포맷은?

---

## 6. Q5 · 통신 · 배포

### Q5-0. ROS 2 지원 범위와 로드맵 ★ (Q1-1 과 직결)

공개 저장소를 비교하면 ROS 2 메시지 집합이 ROS 1 의 **부분집합**입니다.

| 저장소 | 메시지 수 | 빠져 있는 부류 |
|---|---|---|
| ROS 1 `MORAI-ROS_morai_msgs` | 137 | — |
| ROS 2 `MORAI-ROS2_morai_msgs` | 32 | **Sync Mode**, Fault Injection, 로봇 플랫폼(Dilly·ERP42·선박·UGV 등) |

- ROS 2 메시지 집합은 **순차 포팅 중**이라고 이해하면 됩니까?
- **Sync Mode 의 ROS 2 포팅 계획·시기**가 있습니까?
- ROS 2 에서 지원되는 기능 범위를 정리한 문서가 있습니까?

> 남은 것들이 한 부류로 묶여 보여(Drive 핵심은 있고 확장·특수 기능이 없음) **포팅 진행 중**으로
> 읽었습니다. 맞는지, 그리고 Sync Mode 가 그 대기열에 있는지 확인 부탁드립니다.

### Q5-1. ROS 2 브리지의 미들웨어 구성

- ROS 2 배포판은 무엇입니까 (Humble / Jazzy / 기타)?
- **RMW 구현은 무엇입니까** — `rmw_fastrtps_cpp` / `rmw_cyclonedds_cpp` / 기타?
- **다른 RMW 로 교체 가능**합니까 (환경변수 재정의)?
- DDS 도메인 ID 를 지정할 수 있습니까?

> 우리 정규 버스는 **Fast DDS** 입니다. 다른 벤더라도 어댑터에서 흡수할 수 있지만, 미리 알면
> 상호운용 시험 범위를 좁힐 수 있습니다.

### Q5-2. 주요 토픽의 QoS 설정을 알려주시거나, 변경 가능한지 알려주십시오

특히 상태 출력과 시간 관련 토픽의 reliability(RELIABLE/BEST_EFFORT)·durability.

### Q5-3. 배포 형태

- **컨테이너 이미지**로 제공됩니까? (Docker/OCI)
- **헤드리스(GUI 없이)** 실행이 가능합니까? — CI 자동화의 전제입니다
- **GPU 가 필수**입니까? 필수라면 최소 사양은?
- 아키텍처: **x86_64** 전용입니까, arm64 도 있습니까?

### Q5-4. 라이선스

- CI 서버에서 **무인 자동 실행**이 가능한 라이선스 형태입니까?
- **동시 실행 인스턴스 수** 제한이 있습니까? (병렬 회귀 시험 계획)
- 라이선스 서버가 필요합니까?

---

## 7. ★ 한 장 요약 — 이 표만 채워 주셔도 됩니다

| # | 질문 | 답 |
|---|---|---|
| **Q1-1** | **Sync Mode 를 ROS 2 에서 쓸 수 있는가**: (a) ROS 2 도 지원 / (b) 네트워크 API 기능이라 ROS 무관 / (c) ROS 1 전용 / (d) 미지원 | |
| **Q1-1b** | (b)인 경우 — **네트워크 API 규격 문서를 받을 수 있는가** | |
| **Q1-2** | `SyncModeCmd.time_step` (ms) = 시뮬 시각 증가량인가, 벽시계 목표 주기인가 | |
| Q1-2b | `WaitForTick` 이 정확히 1 프레임을 진행시키는가 · 블로킹인가 | |
| Q1-2c | `WaitForTickResponse.time` 포맷·기준 | |
| **Q1-3** | `SyncModeCtrlCmd.frame` 의 제어가 **그 프레임에 반영**되는가 · 안 보내면 어떻게 되는가 | |
| **Q1-4** | Sync Mode 중 **일반 상태 토픽이 자유 실행으로 계속 나오는가** · 나온다면 같은 프레임인가 | |
| Q1-5 | Sync Mode 제약(미지원 기능·성능·`user_id` 의미) | |
| **Q2-1** | 종방향: (a) m/s² / (b) 0~1 정규화 / (c) 목표속도 | |
| Q2-2 | 조향 단위(deg/rad) · 기준(앞바퀴/휠) · 부호 | |
| Q2-3 | 기어 enum 값 | |
| **Q3** | 상태 필드 단위표 (§4) | |
| **Q4-1** | `.xosc` 직접 실행: (a) 가능 / (b) 변환기 / (c) 불가 | |
| Q4-2 | CLI/API 자동화 가능 여부 | |
| **Q5-0** | **ROS 2 포팅 진행 중인가 · Sync Mode 포팅 계획·시기** | |
| Q5-1 | ROS 2 배포판 · RMW · 교체 가능 여부 | |
| Q5-3 | 컨테이너 / 헤드리스 / GPU 필수 / 아키텍처 | |
| Q5-4 | CI 무인 실행 라이선스 · 동시 인스턴스 제한 | |

**굵은 항목이 우선순위입니다.** 특히 **Q1-1 과 Q5-0** 두 개만 먼저 답을 주셔도 저희 쪽 설계
방향을 확정할 수 있습니다 — 나머지는 그 위에서 병렬로 진행합니다.

---

## 8. 참고 — 우리 쪽 계약 (MORAI 측 검토용)

어댑터가 맞춰야 할 우리 인터페이스입니다. 변환은 **저희 쪽 어댑터가 수행**하므로 MORAI 측 변경은
필요하지 않습니다. 매핑 가능 여부 판단에만 참고해 주십시오.

**상태 입력** (`EgoState`) — 전부 SI 단위, 각도는 rad

| 필드 | 단위 / 프레임 |
|---|---|
| `position` (x,y,z) | m, map 프레임 |
| `orientation_rpy` | **rad** |
| `velocity` (x,y,z) | **m/s**, 차량 프레임 |
| `acceleration` | **m/s²**, 차량 프레임 |
| `yaw_rate` | **rad/s** |
| `steering_angle` | **rad** (앞바퀴 기준) |
| `gear` | 0=P 1=R 2=N 3=D |

**명령 출력** (`VehicleControl`)

| 필드 | 단위 |
|---|---|
| `steering_angle` | **rad** (앞바퀴 기준, 좌회전 양수) |
| `acceleration` | **m/s² 부호 있음** (양수=가속, 음수=감속) |
| `gear` | 0=P 1=R 2=N 3=D |

상세: [`idl/dds/sils/EgoState.idl`](../../idl/dds/sils/EgoState.idl) ·
[`idl/dds/sils/VehicleControl.idl`](../../idl/dds/sils/VehicleControl.idl)

---

## 9. 답변 후 우리가 할 일 (내부용)

| 답 | 후속 | 문서 |
|---|---|---|
| **Q1-1=(a)/(b)** | Sync Mode 로 **A-det → L3 목표**. 어댑터가 `SyncModeCtrlCmd` → `WaitForTick` → `EgoState` 를 tick 마다 중계 (§9.1) | [simulator-loops §7.2](sils-simulator-loops.md) |
| Q1-1=(c) | ROS 1 브리지를 별도 컨테이너로. 우리 어댑터는 어차피 SiLS 밖이라 **ROS 1/2 혼재 자체는 문제 아님** — 배포·유지 비용만 평가 | — |
| Q1-1=(d) | free-run 수용. **결정론 포기, 폐루프 동작 검증까지** 명시 보고. ⚠ 시간 종속은 `lockstep` 이 아닌 **별도 mode** 로 — "가짜 lockstep" 방지 | [§7.2 함정](sils-simulator-loops.md) |
| Q1-3 = "직전 값 사용" | 제어가 프레임에 결합되지 않음 → 스텝 제어가 되더라도 **det 성립 안 함**. 루프 B 와 같은 det/full 분리 적용 | 동상 |
| Q1-4 = "자유 실행 병존" | 어댑터는 **`WaitForTickResponse` 만 신뢰**하고 자유 실행 토픽은 버릴 것. 두 경로 혼용 금지를 채널맵 주석에 명시 | BR-M-1 |
| Q2-1=(b) | 차량 파라미터 확보 후 정규화 매핑 + **손실 명시** | BR-M-2 |
| Q4-1=(c) | 루프 A 는 `requirements.yaml` **별도 저작**. 루프 B 의 "한 소스" 성질 없음 | [§4 주의](sils-simulator-loops.md) |
| Q5-0 = "포팅 예정" | 시기에 따라 **ROS 1 경유(임시) → ROS 2 전환** 2단계 계획 | — |
| Q5-1 ≠ Fast DDS | [BR-C-2a](sils-real-bridge-requirements.md) 벤더 상호운용 매트릭스 적용 | — |

기존 작업 항목 대응: **A1-1**=Q1 · **A1-2**=Q2-1 · **A1-3**=Q4-1 · **A1-4**=Q2-2 ·
**신규**=Q5-0(ROS 2 로드맵).

### 9.1 Sync Mode 가 쓸 수 있다면 — 어댑터가 할 일 (예상)

```
  ClockMaster ──TickGrant{tick N}──▶ MORAI 어댑터
                                        │
                                        ├─ SyncModeCtrlCmd{frame=N, cmd}   제어 전달
                                        ├─ WaitForTick{frame=N}            블로킹
                                        │      ◀── WaitForTickResponse{vehicle_status, time}
                                        ├─ EgoState 발행 (단위 변환)
                                        └─ TickComplete{participant_id="morai-adapter"}
```

**우리 IDL·barrier·vECU 변경은 0** 이다 — SS_v2 와 같은 결론이지만, 이쪽은 **시뮬레이터 patch 도
불필요**해 보인다(SS_v2 는 W3 fork 가 필요하다). 역할만 뒤집혀 있다: 우리가 호출하고 MORAI 가 응답한다.

> ⚠ 단, 이 그림은 **Q1-2·Q1-3 이 예상대로 답이 왔을 때만** 성립한다. `time_step` 이 벽시계 목표
> 주기이거나 제어가 프레임에 결합되지 않으면 무너진다. 답이 오기 전에 이 배선을 구현하지 말 것.

---
*작성 2026-08-06 · 개정 2026-08-07.*

*개정 이유: 공개 `morai_msgs` 저장소 조사로 **Sync Mode 의 존재와 형태를 확인**해, Q1 을 "지원
합니까"에서 "ROS 2 에서 쓸 수 있습니까 + 어떻게 동작합니까"로 내렸다. Q5-0(ROS 2 포팅 로드맵)
신설. 근거는 §2.*

*질문 구성에는 루프 B 실측이 반영돼 있다 — Q1-3(제어·프레임 결합)은 SS_v2 에서 결정론을 깬 실제
원인이고, Q1-4(자유 실행 병존)는 SS_v2 `/clock` 이 BEST_EFFORT 자유 실행이던 실측에서 나왔다.*
