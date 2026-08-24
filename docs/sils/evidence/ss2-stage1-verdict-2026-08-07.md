# 루프 B 1계단(배선) 판정 — V2 / V1 / V3 실측

> **실측 증거 (2026-08-07).** Ubuntu x86_64. 이 파일은 **원측정값과 그 해석**이다.
> 도구: [`tools/sils/ss2/run_vecu_wire.sh`](../../../tools/sils/ss2/run_vecu_wire.sh)
> + [`analyze_wire.py`](../../../tools/sils/ss2/analyze_wire.py).
> 절차는 [ss2-bringup §6.2](../sils-ss2-bringup.md), 요구사항 반영은
> [BR-C-2a·BR-C-3](../sils-real-bridge-requirements.md) 에 있다.
>
> **상류 `scenario_simulator_v2` commit `3370c817efc5458fbe69cc3c02a9bc8956cb8a81`**
> ([`docker/sils/ss2-upstream.pin`](../../../docker/sils/ss2-upstream.pin), 2026-08-07 커밋).
> 아래 결론은 **이 커밋에 대한 사실**이다 — 상류를 옮기면 재검증할 것.

---

## 0. 한 장 요약

| | 질문 | 결과 | 다음에 미치는 영향 |
|---|---|---|---|
| **V2** | Fast DDS **2.6**(어댑터)이 쓴 것을 **3.x**(vECU)가 읽는가 | ✅ **PASS** | 1계단 게이트 통과. 2·3계단 진행 가능 |
| **V1** | `EgoState.velocity` 가 차량 프레임인가 지도 프레임인가 | ✅ **VEHICLE** | 정규 계약대로. **어댑터 변경 없음** |
| **V3** | `stamp_ns` 가 시뮬 시각인가 wall clock 인가 | ✅ **SIM · 50 ms 고정 스텝** | **W1/W2 난이도 하락** — 아래 §4 |

**1계단은 이것으로 닫힌다.**

---

## 1. 무엇을 왜 재는가 — 배경

우리 vECU 는 **순수 DDS** 로 듣고, `scenario_simulator_v2`(이하 SS_v2)는 **ROS 2** 로 말한다.
ROS 2 가 이름을 바꾸기 때문에(`/entity/status` → `rt/entity/status`) 같은 네트워크에 있어도
서로 보이지 않는다. 그래서 그 사이에 번역기를 두었다 —
[`adapters/ss2`](../../../adapters/ss2/README.md), 작업항목 B2-1.

```
   SS_v2          [ss2_ego_bridge]           vECU
   ROS 2    ──▶   Fast DDS 2.6      ──▶   Fast DDS 3.x
                  (rclcpp 와 한 프로세스)      (우리 빌드)
                        ▲                        ▲
                   B2-1 에서 확인            여기가 미확인이었다
                   "썼다"                     "읽었다"
```

어댑터가 rclcpp 와 한 프로세스를 쓰므로 **ROS 2 Humble 이 동봉한 Fast DDS 2.6 에 묶인다**
(한 프로세스에 메이저 버전 둘은 불가). vECU 는 3.x 다. RTPS 상 호환이어야 하지만
**아무도 확인한 적이 없었고**, 여기서 막히면 2·3계단이 전부 무의미해진다. 그것이 V2 다.

V1·V3 는 게이트가 아니라 **측정**이다. 와이어가 통해도 값의 의미가 틀리면 조용히 잘못된
결과가 나오기 때문에, 같은 런에서 함께 확인한다.

---

## 2. 측정 구성

| 항목 | 값 |
|---|---|
| 시뮬레이터 | `cpp_mock_scenarios` (**Autoware 없음**) |
| 시나리오 | `traffic_simulation_demo` (런 A) / `lanechange_left` (런 B) |
| RMW | `rmw_fastrtps_cpp` (상류 기본값 Cyclone 을 우리가 재정의) |
| DDS 도메인 | 42 (어댑터 출력 = vECU 입력) |
| ego 엔티티 | `SS2_EGO_NAME=ego` (Autoware 없으면 타입이 EGO 가 아니므로 이름 지정 필수) |
| vECU | `sils_vecu_vehicle_probe`, 채널맵 `ego-min.yaml`, free-run 50 ms 틱 |
| QoS | RELIABLE · KEEP_LAST · **TRANSIENT_LOCAL** (양쪽 일치) |

두 런이 필요했던 이유: `traffic_simulation_demo` 의 ego 는 **완전 직선**이라
V1 을 판정할 수 없었다(§3.2).

---

## 3. 결과

### 3.1 V2 — interop ✅ PASS

**런 A (`traffic_simulation_demo`, 30 초)**

| | 값 |
|---|---|
| trace 행 | 596 |
| **distinct src_seq** | **513** |
| src_seq 역행 | **0** |
| ZOH 유지 틱 | 83 |

**런 B (`lanechange_left`)**

| | 값 |
|---|---|
| trace 행 | 526 |
| **distinct src_seq** | **28** (시나리오 전체 길이가 sim 1.4 초) |
| src_seq 역행 | **0** |

> **판정**: Fast DDS 2.6 writer 가 쓴 `EgoState` 를 Fast DDS 3.x reader 가
> **정상 수신**한다. 타입 이름·CDR 레이아웃·QoS 매칭·도메인 discovery 가
> 전부 맞물린다. **1계단 최대 미지수 해소.**

**`distinct` 와 `행` 을 구분해서 읽어야 한다.** vECU 틱 루프는 zero-order hold 다 —
새 샘플이 없는 틱은 직전 값을 그대로 다시 보고한다
([`vehicle_run.hpp`](../../../service/sils/vecu/src/vehicle_run.hpp)). 따라서 **행 수는
와이어가 실어나른 양이 아니라 vECU 가 틱한 횟수**다. §5 에 이것 때문에 생긴 오판정 2건.

### 3.2 V1 — 프레임 ✅ VEHICLE

`lanechange_left` 런에서:

| | 값 |
|---|---|
| 요각 총 이동 | 0.204 rad |
| 판정 지점 헤딩 편차 | **0.166 rad** |
| 속력 | 10.0 m/s |
| **`\|vy\|/\|v\|`** | **0.0000** |

**해석.** 지도 프레임이라면 그 지점에서 `vy = 10 × sin(0.166) = 1.65 m/s`,
비율 0.165 가 나왔어야 한다. **0 이다.**

> **판정**: `action_status.twist` 는 **차량(entity) 프레임**이다.
> `idl/dds/sils/EgoState.idl` 의 `// vehicle frame, m/s` 주석이 실측으로 확인됐다.
> **어댑터에 회전 변환은 필요 없다.**

**허상 가능성을 배제했다.** `vy` 가 항상 0 인 것이 "지도 프레임이 아니어서"가 아니라
"어댑터가 y 를 안 채워서"일 수 있다. 전 구간을 확인했다:

| 단계 | 코드 |
|---|---|
| bridge | `e.twist_linear.y = d.status.action_status.twist.linear.y` |
| mapper | `s.velocity = ego.twist_linear` (1:1 통과) |
| writer | `msg.velocity(MakeVec3(s.velocity))` |
| vECU | `e.vy = s.velocity().y()` |

그리고 **같은 벡터의 `vx = 10.0` 이 실려 왔다** — 채널은 살아 있고 `y` 만 0 이다.

> ⚠ **직선 런으로는 이 질문에 답할 수 없다.** 두 가설 모두 `vy ≈ 0` 을 예측한다.
> 런 A 는 `yaw_travel_rad = 0.0` 으로 **INCONCLUSIVE** 였고, 그것이 정직한 답이다.
> 도구는 이 경우를 통과로도 실패로도 집계하지 않는다.

### 3.3 V3 — 클록 ✅ SIM · 50 ms 고정 스텝 ★

| | 런 A | 런 B |
|---|---|---|
| 첫 `stamp_ns` | 34,050,000,000 (34.05 s) | 50,000,000 (0.05 s) |
| 마지막 `stamp_ns` | 60,050,000,000 | 1,400,000,000 |
| **고정 스텝** | **50 ms** | **50 ms** |
| sim/wall (신규 샘플 구간) | ~0.9 | ~1.0 |

**두 가지가 나왔다.**

1. **시뮬 시각이다.** wall clock 이면 epoch 기준 ns(~1.7e18)여야 하는데 34 초·0.05 초다.
2. **모든 stamp 가 50 ms 의 배수다.** 값이 아니라 **차분의 GCD** 로 검출했으므로
   임의의 시작 오프셋(34.05 초)에 영향받지 않는다.

> **판정**: SS_v2 는 **50 ms 고정 스텝 적분기**이며, 그 스텝을 실시간에 맞춰 굴린다.
> "wall clock 으로 돈다"(B1-1)는 절반만 맞다 — **내부에 "한 스텝"이라는 개념이 이미 있다.**

---

## 4. 이것이 2계단에 미치는 영향 ★

V3 가 가장 값진 발견이다. W1/W2 의 성격이 달라진다:

| | V3 이전 가정 | 실측 이후 |
|---|---|---|
| SS_v2 의 시간 | wall clock, 스텝 개념 없음 | **50 ms 고정 스텝이 이미 존재** |
| W1/W2 가 할 일 | 스텝을 **새로 만들어** 주입 | 기존 스텝을 **외부에서 구동** |
| 패치 규모 | 큼 | **작음** |

V1 이 VEHICLE 로 나온 것도 (덜 극적이지만) 중요하다 — 어댑터에 좌표 변환을 넣을
필요가 없으므로 **2계단 되먹임 경로가 지금 코드 위에 그대로 얹힌다.**

---

## 5. 측정 과정에서 드러난 것 — 신뢰도를 위해 함께 남긴다

숫자만 남기면 다음 사람이 같은 함정에 빠진다.

### 5.1 운용 함정 2가지

| 함정 | 증상 | 대응 |
|---|---|---|
| **짧은 mock 시나리오** | `lanechange_left` 는 sim **1.4 초**에 끝난다. vECU 를 나중에 띄우면 TRANSIENT_LOCAL 이 보관한 **샘플 1개**만 받는다 | **vECU 를 먼저 띄우고 시나리오를 나중에** 투입 |
| **호스트 환경변수 누수** | 이전 세션의 `SILS_RUN_ID`·`VISS_DDS_DOMAIN_ID` 가 새어 들어와 도메인/식별자가 어긋난다 | `run_vecu_wire.sh` 가 `SILS_*`/`VISS_*` 를 **전부 wipe** 후 재설정 |

### 5.2 분석기 오판정 3건 — 전부 ZOH 를 못 읽어서

| # | 오판정 | 원인 | 수정 |
|---|---|---|---|
| 1 | 정상 596 샘플 런을 **FAIL** | `src_seq` **반복**(`1200→1200`)을 재정렬로 읽음. ZOH 의 정상 신호다 | 역행(`b<a`)만 FAIL, 반복은 계수만 |
| 2 | 정체된 스트림이 **PASS** | `min_samples` 가 **행 수**를 셈. distinct 는 1이었다 | distinct src_seq 기준으로 판정 |
| 3 | `sim_per_wall = 0.05` | 시나리오 종료 후 vECU 가 30 초를 더 틱한 **죽은 꼬리**가 wall 구간에 포함 | 신규 샘플 구간에서만 측정 |

세 건 모두 회귀 시험으로 고정했다(`test_analyze_wire.py`).
**공통 교훈: ZOH 아래에서는 "행 수"가 아니라 "distinct 시퀀스"가 전달량이다.**

---

## 6. 아직 열려 있는 것

| | 상태 |
|---|---|
| ~~상류 commit SHA~~ | ✅ **고정** `3370c817…` — `build-ss2.sh` 기본값도 이 SHA |
| Cyclone DDS 열 (BR-C-2a) | ⬜ 시뮬레이터·프로브 동시 전환 필요 |
| XCDR 버전 | ⬜ RTPS 캡처 필요 |
| ego 가 실제로 도는 런에서 V1 재확인 | 🟡 2계단에서 자연히 확보됨 |
| `VehicleControl`/`ControlTarget` 의 2.6↔3.x | 🟡 미확인이나 위험 낮음(같은 IDL 원시타입·같은 생성기) |

---

*작성 2026-08-07. 원측정 로그: `runs/ss2-wire/{ss2-wire,ss2-lanechange}/`.*
