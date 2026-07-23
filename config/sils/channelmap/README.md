# SiLS Channel Map — MORAI / AWSIM plant coupling

> Option 2 (버스 결합 외부 세계). 설계: [../../../docs/sils-morai-plant-adapter-design.md](../../../docs/sils-morai-plant-adapter-design.md)
> 파서: [service/sils/vecu/src/channel_map.hpp](../../../service/sils/vecu/src/channel_map.hpp)

MORAI/AWSIM 토폴로지에서 plant는 in-process `IPlant`가 아니라 **DDS 버스 위의 공유 세계**다(authority=morai). vECU는 로컬 모델을 `Step()` 하지 않고 — 시뮬레이터의 **STATE 신호를 구독**하고 자신의 **ACTUATION 신호를 pub** 한다. 채널맵은 그 vECU가 관심 있는 MIMO 신호를 **선언적으로** 나열한 것이다.

## 방향

| `dir` | 의미 | vECU 동작 |
|-------|------|-----------|
| `in`  | STATE (시뮬레이터가 broadcast) | **구독** (staged 입력) |
| `out` | ACTUATION (vECU가 되돌림)      | **발행** |

## 계약 타입 — 정규(canonical) 구조체 (Option b)

버스는 **평탄화 스칼라가 아니라 SI-정규 구조체**를 실어나른다. 시뮬레이터(MORAI/AWSIM/rosbag)가 자신의 네이티브 메시지를 이 정규 타입으로 **번역**(anti-corruption layer)하므로, 시뮬레이터가 바뀌어도 버스 계약은 불변이고 P1/P2(IMU·PointCloud2…)로 확장되며 Autoware 브리지와 타입을 공유한다.

| `type` | IDL | 방향 | 내용 (SI) |
|--------|-----|------|-----------|
| `canonical/ego-state` | [EgoState.idl](../../../idl/dds/sils/EgoState.idl) | in (STATE) | position(m)·orientation(rad)·velocity(m/s)·accel(m/s²)·yaw_rate(rad/s)·steering(rad)·gear |
| `canonical/vehicle-control` | [VehicleControl.idl](../../../idl/dds/sils/VehicleControl.idl) | out (ACTUATION) | steering(rad)·acceleration(m/s², signed)·gear |

> ⚠️ MORAI 어댑터는 `front_steer` deg→rad, angular vel deg/s→rad/s, velocity km/h→m/s 변환 필수(빠뜨리면 57.3× 오차). 변환은 어댑터에만 존재, 소비자는 정규만 읽는다.

## 스키마 (제약된 YAML 서브셋)

`channel_map.hpp`의 `ChannelMap::Load`가 파싱한다. yaml-cpp 의존성 없이 손으로 파싱하므로(SDK는 header-only + Fast-DDS 전용, `json_min.hpp`와 같은 이유) **문법이 좁고 고정**이다. 벗어나면 `file:line`과 함께 fail-closed(§14).

```yaml
channels:
  - name: Vehicle.EgoState        # VSS 브랜치(필수) — 필드↔VSS sub-path 매핑은 코덱 측(S2)
    dir: in                       # in | out (필수, 기본값 없음)
    topic: sils/ego/state         # DDS 토픽 (필수)
    type: canonical/ego-state     # 코덱이 (de)serialize 할 정규 struct 키 (필수, 여기선 불투명)
```

규칙:
- 2-space 들여쓰기, 한 줄에 `key: value` 하나. 리스트 항목은 `- `로 시작.
- `#`는 주석. 값의 감싸는 따옴표(`"`/`'`)는 벗겨진다.
- 필수 4개 키: `name` · `dir` · `topic` · `type`. 누락/오타/`dir` 오값은 에러.
- `(name, dir)` 쌍 중복은 에러.
- `type`은 파서에게 **불투명**하다 — 스칼라든 구조체든 코덱이 해석. 그래서 스키마 변경 없이 새 타입을 연다.

## 사용

`sils_vecu_vehicle_probe`(기본 probe)가 채널맵을 읽어 `in`→EgoState reader, `out`→
VehicleControl writer를 바인딩하고, 기준 `LongitudinalBehavior`로 free-run tick 루프를 돈다.

```bash
SILS_CHANNELMAP=config/sils/channelmap/ego-min.yaml \
SILS_CTRL_KP=0.8 SILS_CTRL_SETPOINT=10 \
  bin/sils_vecu_vehicle_probe
```

`SILS_CHANNELMAP`이 비어 있으면(기본) 채널맵은 비활성 — 기존 단일 엔드포인트 동작 그대로다(비침습, 가산).

커스텀 제어 ECU는 `IVehicleBehavior`([vehicle_behavior.hpp](../../../service/sils/vecu/src/vehicle_behavior.hpp))를
구현해 `RunVehicleProbe`로 주입한다 — 자기 채널맵을 동봉한다. 참조:
[examples/steer-ecu](../../../service/sils/examples/steer-ecu/)(조향 ECU).

## 예제

- [ego-min.yaml](ego-min.yaml) — 기본 probe(longitudinal)용 최소 2채널(STATE in 1 + ACTUATION out 1).

## 단계 (설계 문서 §5)

| 단계 | 이 디렉토리와의 관계 |
|------|----------------------|
| **S1 free-run** | 채널맵 로드·검증·trace (현재) |
| **S2 MIMO staged** | 다중 `dir: in` 토픽을 tick 경계에서 freeze |
| **S3 lockstep** | `TickControl.idl` 확장으로 시뮬레이터와 tick 동기 |
| **S4 conformance** | 채널 완전성·왕복 지연·lockstep 정합 판정 |
