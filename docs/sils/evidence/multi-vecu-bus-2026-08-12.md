# 증거 — 정준 버스 하나에 vECU 다섯이 동시에 돈다

> 측정 2026-08-12 · **도메인 42 단일** (파티션 Cyclone / SiLS Fast DDS) · headless
> 플랜트는 **SS_v2**(kashiwanoha, `isEgo` 없음), 상류 구성은
> [`autoware-return-path-2026-08-11.md` §11](autoware-return-path-2026-08-11.md) 그대로
> 설계: [`sils-vecu-arbitration.md`](../sils-vecu-arbitration.md)

지금까지 vECU 는 **한 번에 하나**만 돌렸다. 예제 다섯이 있는데 같이 올린 적이 없었고,
그래서 "여러 ECU 가 한 버스에서 각자 판정받는다" 는 이 프레임워크의 전제가 미검증이었다.
오늘 다섯을 동시에 올렸다.

---

## 1. 결과

```
SS_v2 시나리오 ──▶ ss2_ego_bridge  received=3601 published=3600
                          │
                    sils/ego/state ─────────────┬──────┬──────┬──────┐
                          │                     ▼      ▼      ▼      ▼
Autoware ─control_cmd─▶ aw_control_target_bridge  steer adaptive classic plainc
          received=37505 published=37505           │      │      │      │
                          │                        │      │      │      │
                 autoware/control_target ──▶ VCU   │      │      │      │
                                             │     │      │      │      │
                                    sils/ego/cmd  steer_cmd  cmd_adaptive  cmd_classic  cmd_plainc
```

| ECU | behavior | state | target | published | 마지막 출력 |
|---|---|---|---|---|---|
| steer | `steering-controller` | 1850 | 0 | 1851 | steering **0.08** |
| adaptive | `adaptive-drivecontroller` | 1848 | 0 | 1851 | accel **8** |
| classic | `fmu-behavior` | 1769 | 0 | 1772 | accel **8** |
| plainc | `plainc-speedctl` | 1848 | 0 | 1851 | accel **8** |
| **VCU** | `vehicle-interface` | 1849 | **1849** | 1850 | accel **−1.5** |

**마지막 두 줄이 요점이다.** VCU 만 `target_received` 가 오르고, 그 값은 Autoware 가 실제로
낸 판단이다(정지 유지 −1.5). 나머지 넷은 상태만 보고 자기 로직을 돈다.

`classic` 의 카운터가 낮은 이유는 §3 ②.

---

## 2. 재현

상류(파티션·브리지·SS_v2)는 [§11](autoware-return-path-2026-08-11.md#11-ss_v2-를-플랜트로-두고-자율주행-명령까지-갔다-2026-08-11-180x1821) 그대로. 그 위에:

```bash
# 컨테이너 하나에 vECU 다섯 (호스트 빌드를 read-only 마운트)
docker run -d --name sils-multi-vecu --net=host --ipc=host \
  -v "$PWD:/work:ro" -v "$PWD/runs/multi-vecu:/out" \
  -e LD_LIBRARY_PATH=/work/output/x86_64/lib \
  -e VISS_DDS_DOMAIN_ID=42 -e SILS_RUN_ID=multi-vecu -e SILS_DURABILITY=transient-local \
  --entrypoint /work/docker/sils/multi-vecu-entrypoint.sh \
  oss-mmf-sils-vecu:latest

# 호스트 프로세스로 띄우는 형태도 있다 (컨테이너를 안 쓸 때)
tools/sils/ss2/run_multi_vecu.sh --duration 60
```

신규: [`docker/sils/multi-vecu-entrypoint.sh`](../../../docker/sils/multi-vecu-entrypoint.sh) ·
[`tools/sils/ss2/run_multi_vecu.sh`](../../../tools/sils/ss2/run_multi_vecu.sh) ·
`config/sils/channelmap/multi/{adaptive,classic,plainc}-multi.yaml`

---

## 3. 함정 둘 (둘 다 오늘 밟았다)

### ① 예제 넷이 **전부 `sils/ego/cmd` 에 쓴다** ★

`adaptive-min` · `classic-min` · `plainc-min` · `vehicle-interface` 의 out 채널이 전부 같다.
그것들은 **동시에 도는 구성이 아니라 서로의 대안**이기 때문이다. 그대로 올리면:

> 한 토픽에 라이터가 넷 → 플랜트가 받는 값은 **발행 순서**가 정한다. **아무도 오류를 안 낸다.**

`config/sils/channelmap/multi/` 의 사본으로 out 토픽만 갈라서 풀었다. **코드는 안 바뀐다** —
토픽 바인딩이 config 인 것이 이 SDK 의 설계다(`SILS_CHANNELMAP`).

`steer-ecu` 가 원래부터 `sils/ego/steer_cmd` 로 갈라져 있는 것이 이 규칙의 선례다:
**액추에이터가 다르면 채널도 다르다.**

| ECU | out 토픽 |
|---|---|
| steer | `sils/ego/steer_cmd` (원래 그랬다) |
| adaptive | `sils/ego/cmd_adaptive` (신규) |
| classic | `sils/ego/cmd_classic` (신규) |
| plainc | `sils/ego/cmd_plainc` (신규) |
| **VCU** | **`sils/ego/cmd`** — 종축 액추에이션 채널. VCU 만 쓴다 |

### ② `classic_vecu` 는 그 컨테이너에서 못 돈다 — `unzip` 부재

```
Failed to unzip FMU: status = 32512      ← 127<<8 = command not found
classic_vecu: fmi4c_loadFmu failed
```

`oss-mmf-sils-vecu:latest` 에 `unzip` 이 없다. FMU 는 zip 이고 `fmi4c` 가 외부 `unzip` 을
부른다. **호스트에서는 정상**(`state=1769 published=1772`)이므로 FMU 자체 문제가 아니다.

이미지에 `unzip` 한 줄 추가하면 5/5 가 한 컨테이너에 들어간다. 그리고 이 사건 자체가
**ECU 마다 런타임 의존이 다르다**는 증거다 — 배치를 컨테이너로 나눌 근거이기도 하다.

`vecu.fmu` 는 gitignore 라 저장소에 없다. 먼저 빌드해야 한다:
```bash
service/sils/examples/classic-vecu/models/vecu-fmu/build.sh   # -> vecu.fmu (linux64)
```

---

## 4. 이 시험이 증명하는 것과 아닌 것

**증명한 것**

- 서로 다른 런타임의 vECU 다섯이 **한 정준 버스에서 동시에** 각자 입력을 구독하고 각자
  명령을 낸다. 같은 `EgoState` 에 서로 다른 답이 나온다(steering 0.08 / accel 8 / accel 8 …)
- **컨테이너 경계는 버스에 안 보인다** — 넷은 컨테이너 안, `classic` 은 호스트에서 돌았고
  같은 도메인에서 똑같이 동작했다. 배치는 배치일 뿐이다
- VCU 만 Autoware 판단(`ControlTarget`)을 받는다. 나머지는 상태만 본다

**증명하지 않은 것**

- **통합(아비트레이션)이 없다.** VCU 는 다른 넷의 출력을 **입력으로 받지 않는다** —
  `VehicleBehaviorContext` 가 `state` + `target` 두 개로 고정돼 있다. 즉 지금은
  "다섯이 각자 돈다" 이지 "다섯이 협조해 한 명령을 만든다" 가 아니다
- **예제 로직은 데모다.** P 제어 수준이라 차이 비교에서 나올 것이 별로 없다. 값은 실제
  공급사 로직이 들어왔을 때 나온다
- **차는 여전히 안 움직인다** — §8·§11 그대로. EgoEntity 가 없어 차량 모델이 없다

---

## 5. 다음

| 할 일 | 왜 |
|---|---|
| **아비트레이션** — 축 소유권 + 단조 안전 규칙 | [`sils-vecu-arbitration.md`](../sils-vecu-arbitration.md). VCU 가 이름값을 하게 만든다 |
| 불변식 검증을 verify 도구에 | 스펙 없이도 "명백히 틀림" 을 잡는 그물 |
| pass-through differential 자동 판정 | VCU 통과 모드에서 Autoware 값과 **같아야** 한다. 오늘은 눈으로만 확인(−1.5) |
| `oss-mmf-sils-vecu` 에 `unzip` | 5/5 를 한 컨테이너에 |
