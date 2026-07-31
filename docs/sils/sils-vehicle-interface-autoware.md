# SiLS — Vehicle Interface (VCU) ↔ Autoware 연동 설계

> 상태: **설계 검토 단계 (구현 전)**
> 대상: `service/sils` — 새 vECU "Vehicle Interface(VCU)"
> 관련: [sils-morai-plant-adapter-design.md](sils-morai-plant-adapter-design.md)(Option 2 버스 결합), [sils-morai-autoware-topology.html](sils-morai-autoware-topology.html) ①, [sils-morai-simulator-integration-review.md](sils-morai-simulator-integration-review.md)
> Autoware 소스 기준: `/Volumes/DATAPart/AUTOWARE/source/autoware_msgs`, `.../autoware.universe/control/autoware_vehicle_cmd_gate`
> 작성: 2026-07-23

---

## 0. 왜 필요한가

MORAI는 **단일 통합 차량 명령**(`morai_msgs/CtrlCmd`: 조향·가속·브레이크·기어 한 덩어리)만 받는다. Autoware의 제어 출력을 MORAI에 넣으려면 그 사이에서 **받아 아비트·안전제한·통합·변환**하는 컴포넌트가 필요하다. 이것이 **Vehicle Interface(VCU)** — 개별 액추에이터 ECU가 MORAI로 직행하는 게 아니라, VCU가 통합 명령을 만들어 넘긴다.

```
Autoware ──Control + GearCommand──▶ VCU (Vehicle Interface) ──통합 VehicleControl──▶ [MORAI 어댑터] ──▶ MORAI
   ▲                                     │
   └──── SteeringReport / VelocityReport ─┘   (EgoState 에서 만들어 되돌림)
```

> 개별 액추에이터 ECU(brake/steer)는 이 폐루프의 일부가 아니라 **그 ECU를 단독 시험**하는 별도 시나리오다. 전체 AD 루프는 **Autoware → VCU → MORAI**.

---

## 1. Autoware 제어 인터페이스 (실측)

`autoware_vehicle_cmd_gate`가 아비트 후 최종 명령을 아래 토픽으로 낸다. VCU는 이 경계를 소비/생산한다(실차의 `vehicle_interface` 노드와 같은 자리).

### VCU 가 구독 (Autoware → 차량)
| 토픽 | 타입 | 핵심 필드 (단위) |
|---|---|---|
| `/control/command/control_cmd` | `autoware_control_msgs/Control` | `lateral.steering_tire_angle`(**rad**), `lateral.steering_tire_rotation_rate`(rad/s), `longitudinal.velocity`(m/s), `longitudinal.acceleration`(**m/s²**), `longitudinal.jerk`(m/s³) + is_defined 플래그 |
| `/control/command/gear_cmd` | `autoware_vehicle_msgs/GearCommand` | `command`(enum: NEUTRAL=1, DRIVE=2, REVERSE=20, PARK=22, …) |
| (선택) `turn_indicators_cmd` / `hazard_lights_cmd` / `emergency_cmd` | 각 Command | 방향지시·비상 |

### VCU 가 발행 (차량 → Autoware, 피드백)
| 토픽 | 타입 | 필드 |
|---|---|---|
| `/vehicle/status/steering_status` | `SteeringReport` | `steering_tire_angle`(rad) |
| `/vehicle/status/velocity_status` | `VelocityReport` | `longitudinal_velocity`, `lateral_velocity`, `heading_rate`(rad/s) |
| `/vehicle/status/gear_status` | `GearReport` | `report`(gear enum) |
| `/vehicle/status/control_mode` | `ControlModeReport` | 실제 수락 모드 |

> **단위가 우리 정규 계약과 거의 일치**한다: `steering_tire_angle`=rad(=우리 `steering_angle`), `longitudinal.acceleration`=m/s²(=우리 `acceleration`). 좌표/단위 변환 부담이 MORAI(deg/km-h) 쪽보다 훨씬 작다 — gear enum 매핑 정도만.

---

## 2. 결정 — Autoware 타입을 버스에서 "정규 미러"로 (Option i)

Autoware는 ROS2(=DDS)라 제어 타입이 이미 DDS 타입이지만, 우리 FastDDS vECU가 그걸 **직접** 구독하려면 ROS2 type name/hash·CDR·QoS·`ROS_DOMAIN_ID`·rmw 호환을 모두 정확히 맞춰야 한다(integration-review §3.4 경고). 대신:

- **정규 미러 계약** `idl/dds/sils/ControlTarget.idl`(SI 단위)을 두고, **Autoware↔DDS 브리지(외부 컴포넌트)**가 `autoware_control_msgs/Control`→`ControlTarget`로 번역한다.
- VCU는 정규 `ControlTarget`만 본다 → **MORAI를 EgoState 정규로 받은 것과 완전히 대칭**. ROS2 상호운용 난관은 브리지(외부, anti-corruption layer)로 밀어낸다.

> 브리지는 SiLS 소유가 아니다(MORAI 어댑터와 동일). 실제로는 ROS2 topic bridge 또는 얇은 relay node. SiLS는 계약(ControlTarget)만 소유.

---

## 3. 정규 계약 + 매핑

### 3.1 새 타입 — `ControlTarget` (Autoware 제어 미러, sim→VCU 아님, 상위→VCU)
```
struct ControlTarget {
    string             run_id;
    unsigned long long seq;
    long long          stamp_ns;
    double             steering_angle;   // rad  (= Control.lateral.steering_tire_angle)
    double             acceleration;     // m/s² (= Control.longitudinal.acceleration, 단일 모드 고정)
    octet              gear;             // 0=P 1=R 2=N 3=D
};
```
> 우리 `VehicleControl`과 필드가 사실상 동일하다 — 의미가 다르다: `ControlTarget`=Autoware가 **원하는 목표**, `VehicleControl`=VCU가 아비트·안전 후 **plant에 내리는 명령**. 평시엔 근사 pass-through, 이상시(타임아웃·비상) VCU가 개입.

### 3.2 매핑 체인
| 단계 | 변환 |
|---|---|
| Autoware `Control` → `ControlTarget` | (브리지, 외부) steering_tire_angle→steering_angle, longitudinal.acceleration→acceleration |
| `GearCommand` → `ControlTarget.gear` | DRIVE(2)→3, REVERSE(20)→1, PARK(22)→0, NEUTRAL(1)→2 |
| `ControlTarget` + `EgoState` → `VehicleControl` | (VCU) 아비트·안전 클램프·통합 |
| `VehicleControl` → `morai_msgs/CtrlCmd` | (MORAI 어댑터, 외부) rad·m/s² 유지, longl_cmd_type=acceleration |
| `EgoState` → `SteeringReport`/`VelocityReport` | (VCU) steering_angle→steering_tire_angle, velocity→longitudinal_velocity, yaw_rate→heading_rate |

---

## 4. VCU 책임 (제어 로직 = 아비트·안전, 궤적 제어 아님)

1. **매핑/통합** — ControlTarget → 단일 VehicleControl.
2. **안전** — control command **타임아웃 시 safe stop**(리뷰 성공기준): 최근 ControlTarget이 N ms 이상 없으면 감속/제동 명령으로 대체.
3. **클램프** — 조향각/가속 물리 한계·rate 제한.
4. **리포트** — EgoState → SteeringReport/VelocityReport 되돌림(Autoware 폐루프).
5. **모드** — engage/emergency 반영(후속).

> 즉 VCU의 "control logic"은 **아비트레이션·안전 계층**이지 경로 추종이 아니다(그건 Autoware). steer-ecu의 P 컨트롤러와는 성격이 다르다.

---

## 5. 프로파일 B — 입력 2개로 확장

현재 `IVehicleBehavior`의 `VehicleBehaviorContext`는 **상태(EgoState)만** 담는다. VCU는 **상태 + 상위 제어명령(ControlTarget)** 둘을 받으므로 컨텍스트 확장이 필요하다:
```cpp
struct VehicleBehaviorContext {
    uint64_t tick_seq; int64_t sim_time_ns;
    EgoStateSample state;         bool have_state;
    ControlTargetSample target;   bool have_target;   // ← 추가 (프로파일 B)
};
```
가산·비침습 — 기존 longitudinal/steering(상태만 쓰는) behavior는 그대로 동작.

---

## 6. 단계화 (VCU 기준)

- **S0** — 이 문서 확정 + ControlTarget 스키마.
- **S1** — ControlTarget in + EgoState in → VehicleControl out (매핑·pass-through, free-run). 테스트 fixture로 ControlTarget 발행(sils_ego_source의 짝).
- **S2** — 리포트(SteeringReport/VelocityReport) 되돌림 + gear enum 매핑.
- **S3** — 안전(타임아웃→safe stop)·클램프·아비트레이션.
- **S4** — lockstep + conformance(왕복·타임아웃 판정).
- **외부(SiLS 밖)** — Autoware↔DDS 브리지, MORAI 어댑터.

---

## 7. 구현 계획 (합의된 순서)

1. **`idl/dds/sils/ControlTarget.idl`** 추가 (+ CMake: sils_vehicle_idl 에 합류).
2. **`VehicleBehaviorContext` 확장** — `ControlTargetSample target` + `have_target`(프로파일 B). `vehicle_wire.hpp`에 ControlTarget 디코더, `vehicle_run.hpp`가 target 채널도 구독.
3. **VCU 예제** `service/sils/examples/vehicle-interface` — `IVehicleBehavior` 구현(매핑+아비트), main, 채널맵(in: ControlTarget + EgoState / out: VehicleControl [+ reports]), README. brake/steer-ecu와 동일 패턴.

---

## 8. 열린 질문

1. **리포트 타입** — SteeringReport/VelocityReport도 정규 미러(`VehicleStatus.idl`)로 둘지, 브리지가 EgoState에서 직접 만들지.
2. **longitudinal 모드** — acceleration 고정 vs velocity 병행(Autoware는 둘 다 채울 수 있음, is_defined 플래그). 계약은 하나로 고정 권장(acceleration).
3. **VCU 위치** — examples/(개발자 ECU) vs core(기본 제공 인터페이스)? brake/steer 선례 따라 **examples 권장**.
4. **타임아웃/safe stop 정책** — 감속률·제동 강도 기본값.
5. **다중 액추에이터 분해** — VCU가 통합 명령만 낼지, 부분 액추에이터 ECU로 다시 분해하는 시나리오까지 볼지(별도).

---

*확정되면 §7 순서로 ControlTarget.idl → 컨텍스트 확장 → VCU 예제 착수. Autoware 소스는 `/Volumes/DATAPart/AUTOWARE`(로컬) 참조.*
