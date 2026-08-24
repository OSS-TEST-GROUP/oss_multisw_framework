# 증거 — MMF 의 TypeObject 전파, 그리고 **내가 원인을 잘못 짚은 기록**

> 측정 2026-08-12 · 도메인 42 단일 · kashiwanoha · headless
>
> ⚠ **이 문서는 한 번 크게 틀렸다.** 초판은 "MMF 를 붙이면 Autoware 파티션 3종이
> 4.2초 만에 전멸한다" 고 단정했다. **그 인과는 성립하지 않는다.** 실제 사망 원인은
> DDS 와 무관한 것이었고(§4), 오진 과정을 지우지 않고 남긴다 — 같은 함정이 이 rig 에
> 구조적으로 깔려 있기 때문이다.

---

## 1. 사실로 확정된 것

미패치 MMF 를 도메인 42 에 넣으면 Autoware 파티션 3종이 이 로그를 쏟는다:

```
[42] dq.builtin: type [INVALID 0000000000000000000000000000]:
                 ddsi_xt_type_init_impl with invalid type object
```

**★ 순서가 결과를 바꾼다.** 에러는 리더-라이터 매칭 시점의 타입 교환에서 나므로,
**MMF 가 파티션보다 먼저 떠 있으면 미패치라도 0건**이다. 그래서 정직한 시험은
**파티션을 먼저 세우고 MMF 를 나중에 넣는 것**이다. 같은 미패치 이미지로 0건과
540건이 둘 다 나왔다 — 이 함정 때문에 판정 스크립트에 `--restart-mmf` 를 뒀다.

아래는 전부 **파티션 먼저 → MMF 나중** 조건이다:

| 도메인 42 의 Fast DDS 참가자 | 파티션 | `invalid type object` |
|---|---|---|
| 어댑터 + vECU (**MMF 없음**) | 생존 | **0** |
| **+ 미패치 MMF** | **생존** | **1260** (perception 252 · decision 735 · control 273) |
| 〃 (다른 런) | 생존 | 540 |
| **+ 패치 MMF** (`oss-mmf-core:local`) | 생존 | **0** (기준선 0 → 투입 후 0, 증가 0) |

- **MMF 가 원인이다** — 없으면 0, 넣으면 1260, 패치하면 다시 0.
- **그러나 죽지는 않는다** — 160초·120초 두 번 관찰, 노드 127개, 로그 계속 흐름.
- 버스트로 한 번 나고 멈춘다(1260에서 정지). discovery 시점의 타입 교환이다.

우리 컴포넌트는 이 규칙을 이미 지킨다. MMF 만 안 지켰다:

| 코드 | Fast DDS | 규칙 |
|---|---|---|
| [`sils_participant_qos.hpp`](../../../service/sils/vecu/src/sils_participant_qos.hpp) | 3.2 | `fastdds.type_propagation = disabled` |
| [`aw_type_propagation.hpp`](../../../adapters/ss2/include/aw_type_propagation.hpp) | 2.6 | `auto_fill_type_object(false)` + `auto_fill_type_information(false)` |
| `ota_mgr/otam_common/otam_dds_impl.h` | 3.2 | ✅ 있었다 (한 곳만) |
| `DDSProvider.cpp` · `dds_transport.cpp` · `DDSProtocol.cpp` | 3.2 | ❌ 없었다 → 고침 |

---

## 2. 고침 — 왜 XML 로는 안 되고 소스여야 했나

`oss-mmf-core` 이미지는 **이미** `FASTDDS_DEFAULT_PROFILES_FILE=/oss/fastdds-profile.xml`
을 읽는다. 그 파일에 속성을 넣어 봤고, **적용되지 않았다.**

이유는 [`DDSProvider.cpp:566`](../../../service/core/vehicle_info_mgr/src/providers/DDSProvider.cpp):

```cpp
DomainParticipantQos pqos = PARTICIPANT_QOS_DEFAULT;   // ← 복사
```

Fast DDS 3.2.2 소스(`DomainParticipantFactory.cpp`, MMF 가 링크한 바로 그 버전):

| 행 | 코드 |
|---|---|
| **160** | `const DomainParticipantQos& pqos = (&qos == &PARTICIPANT_QOS_DEFAULT) ? default_participant_qos_ : qos;` |
| 493 | `load_profiles()` → `reset_default_participant_qos()` — XML 이 여기로 들어온다 |
| 306 | `get_default_participant_qos()` — 그 값을 꺼내는 공개 API |

**주소 비교라서, 상수를 복사하는 순간 XML 경로를 잃는다.** 그래서 이 참가자에는
`type_propagation` 뿐 아니라 이미지가 담아 배포하는 **UDP 전용 전송·data sharing OFF 도
한 번도 안 걸려 있었다.** 경고는 없다.

고침은 두 줄:

```cpp
DomainParticipantQos pqos = DomainParticipantFactory::get_instance()
                              ->get_default_participant_qos();          // XML 이 산다
pqos.properties().properties().emplace_back("fastdds.type_propagation", "disabled");  // 바닥
```

XML 만으로 두면 배포 파일 하나에 의존해 조용히 풀릴 수 있어서 코드에 바닥을 깐다.

> ★ **교훈: "프로파일 파일에 들어갔다" 는 "적용됐다" 가 아니다.**

---

## 3. 그래서 이 패치의 값어치는

| | |
|---|---|
| **한다** | 파티션 로그의 에러 1260건 → 0. 배포 프로파일이 그 참가자에 **처음으로** 실제 적용 |
| **안 한다** | 크래시 방지. **애초에 크래시하지 않았다** |

**긴급도는 낮다.** 로그 위생과 설정 정합성의 문제이지 가용성 문제가 아니다.

---

## 4. ★ 진짜 사망 원인 — DDS 가 아니었다

파티션 3종은 세 번 사라졌고, 매번 **`docker ps -a` 에서도** 없어졌다. 원인:

```bash
# oss_adsw/partition/run_partitions.sh
trap cleanup INT TERM EXIT     # cleanup() { docker stop <파티션 3종>; }
tail -n +1 -F <로그 3개>        # 영원히 안 끝난다
```

파티션은 `--rm` 이라 **stop 되는 순간 컨테이너까지 사라진다.**

그리고 이 러너가 **8개나 동시에 살아 있었다**(이전 세션 것 포함). 각자 트랩을 들고
같은 세 컨테이너를 겨눈다. 그중 아무 하나의 프로세스 그룹만 정리돼도 전멸한다.

**실증** — PGID 하나를 그룹째 TERM:

```
파티션: 0/3   docker ps -a: 0     ← 흔적 없이 사라짐
partitions.log: "Stopping partition containers..."
```

관측된 사망 징후와 정확히 일치한다.

### 왜 이렇게 오래 못 찾았나 — 두 가지가 겹쳤다

**(1) `kill <pid>` 로는 트랩이 안 돈다.** bash 는 포그라운드 명령(`tail -F`)이 끝나야
트랩을 처리하는데 그게 영원히 안 끝나므로 시그널이 pending 으로 남는다.
**프로세스 그룹째** 죽어야 `tail` 이 죽고 그제서야 cleanup 이 돈다.
→ 특정 툴 호출에서만 터지고, **산발적으로** 보인다.

**(2) 로그 마지막 줄이 `invalid type object` 였다.** 그런데 —

```
decision_log.txt: 전체 2532줄 중 735줄이 그 에러 = 29.0%
```

**로그가 아무 이유로 끊겨도 마지막 줄이 그것일 확률이 높다.**
빈발하는 로그를 "마지막이라서" 원인으로 삼은 것이 오진의 핵심이다.

여기에 대조 실험(MMF 없음 → 0건)이 겹치면서 인과처럼 보였다. 그 대조 실험은
**"MMF 가 에러를 만든다"만 증명하고 "MMF 가 죽인다"는 증명하지 않는다.** 나는
그 둘을 구분하지 않았다.

> ★ **반증한 것은 실측이 아니라 사용자였다.** "다른 장비에서 MMF + 파티션 3종이
> 도메인 0 으로 잘 돈다, 수정 없이" — 그 한마디로 격리 실험(파티션 + MMF 단독)을
> 돌렸고, 살아남았다. **내 rig 에서만 죽었고, 그건 내 rig 에 러너가 8개 떠 있어서였다.**

### 재발 방지

[`tools/sils/ss2/run_partitions_detached.sh`](../../../tools/sils/ss2/run_partitions_detached.sh)
— `docker run -d` 만 하고 **트랩도 tail 도 없다.** 기동 전에 상류 러너가 살아 있으면
거부한다. 에이전트/스크립트가 띄울 때는 이쪽을 쓴다(사람이 Ctrl-C 로 내릴 거면 상류가 맞다).

---

## 5. 다음 사람에게

| 증상 | 먼저 의심할 것 |
|---|---|
| 파티션 3종이 동시에, `docker ps -a` 에도 없이 사라짐 | **`run_partitions.sh` 의 EXIT 트랩.** `pgrep -cf '[r]un_partitions.sh'` 로 몇 개 떠 있는지 세라 |
| 로그 마지막 줄이 `invalid type object` | **원인 아니다.** 그 줄은 로그의 29% 다 |
| MMF 를 붙였더니 뭔가 이상 | 에러 로그는 진짜다(§1). 크래시는 아니다 |
| `invalid type object` 가 났는데 MMF 도 우리 것도 안 띄웠다 | **그럴 수 있다.** 참가자 0개에서도 224건이 난다(08-13). 이 조합의 풍토병이다 |

> ★ **후속 (2026-08-13).** 같은 대조군 설계를 **우리 정준 타입**에 적용했고 결론이 같다 —
> 오염은 참, 사망은 미지지 → [`our-typeobject-2026-08-13.md`](our-typeobject-2026-08-13.md).
> 다만 그 문서가 위 §1 의 구도 하나를 정정한다: "우리 컴포넌트는 규칙을 지키고 MMF 만
> 안 지켰다" 는 **오염원이 그 둘뿐일 때만** 성립하는 그림인데, 아무도 없어도 나는 에러가
> 있다. 그리고 판정은 줄 수가 아니라 **해시(TypeIdentifier)** 로 해야 갈린다.

> **빈발하는 로그 줄은 "마지막에 있다"는 이유로 원인이 되지 못한다.**
> 원인을 세우려면 그것 하나만 바꾼 대조군이 필요하고, 그 대조군이 증명하는 명제가
> 정확히 무엇인지 적어야 한다.

---

*관련: [`c1-with-mmf-2026-08-12.md`](c1-with-mmf-2026-08-12.md) ·
[`sils-docker-runbook.md`](../sils-docker-runbook.md) ·
[`sils-integration-topology.md` §8-(2)](../sils-integration-topology.md)*
