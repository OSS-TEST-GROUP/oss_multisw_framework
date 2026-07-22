# SiLS Channel Map — MORAI / AWSIM plant coupling

> Option 2 (버스 결합 외부 세계). 설계: [../../../docs/sils-morai-plant-adapter-design.md](../../../docs/sils-morai-plant-adapter-design.md)
> 파서: [service/sils/vecu/src/channel_map.hpp](../../../service/sils/vecu/src/channel_map.hpp)

MORAI/AWSIM 토폴로지에서 plant는 in-process `IPlant`가 아니라 **DDS 버스 위의 공유 세계**다(authority=morai). vECU는 로컬 모델을 `Step()` 하지 않고 — 시뮬레이터의 **STATE 신호를 구독**하고 자신의 **ACTUATION 신호를 pub** 한다. 채널맵은 그 vECU가 관심 있는 MIMO 신호를 **선언적으로** 나열한 것이다.

## 방향

| `dir` | 의미 | vECU 동작 |
|-------|------|-----------|
| `in`  | STATE (시뮬레이터가 broadcast) | **구독** (staged 입력) |
| `out` | ACTUATION (vECU가 되돌림)      | **발행** |

## 스키마 (제약된 YAML 서브셋)

`channel_map.hpp`의 `ChannelMap::Load`가 파싱한다. yaml-cpp 의존성 없이 손으로 파싱하므로(SDK는 header-only + Fast-DDS 전용, `json_min.hpp`와 같은 이유) **문법이 좁고 고정**이다. 벗어나면 `file:line`과 함께 fail-closed(§14).

```yaml
channels:
  - name: Vehicle.Speed        # VSS path (필수) — vehicle_info_mgr VSS 어휘 재사용
    dir: in                    # in | out (필수, 기본값 없음)
    topic: morai/ego/speed     # DDS 토픽 (필수)
    type: vss-scalar           # 코덱이 해석할 wire-type 키 (필수, 여기선 불투명)
```

규칙:
- 2-space 들여쓰기, 한 줄에 `key: value` 하나. 리스트 항목은 `- `로 시작.
- `#`는 주석. 값의 감싸는 따옴표(`"`/`'`)는 벗겨진다.
- 필수 4개 키: `name` · `dir` · `topic` · `type`. 누락/오타/`dir` 오값은 에러.
- `(name, dir)` 쌍 중복은 에러.

## 사용

```bash
export SILS_CHANNELMAP=config/sils/channelmap/ego-min.yaml
# (S1: 로드+검증+trace. 각 채널의 DDS reader/writer 바인딩은 후속 — VSS 신호 DDS 타입 확정 후)
```

`SILS_CHANNELMAP`이 비어 있으면(기본) 채널맵은 비활성 — 기존 단일 엔드포인트 동작 그대로다(비침습, 가산).

## 예제

- [ego-min.yaml](ego-min.yaml) — 최소 2채널(STATE in 1 + ACTUATION out 1). S1 브링업용.

## 단계 (설계 문서 §5)

| 단계 | 이 디렉토리와의 관계 |
|------|----------------------|
| **S1 free-run** | 채널맵 로드·검증·trace (현재) |
| **S2 MIMO staged** | 다중 `dir: in` 토픽을 tick 경계에서 freeze |
| **S3 lockstep** | `TickControl.idl` 확장으로 시뮬레이터와 tick 동기 |
| **S4 conformance** | 채널 완전성·왕복 지연·lockstep 정합 판정 |
