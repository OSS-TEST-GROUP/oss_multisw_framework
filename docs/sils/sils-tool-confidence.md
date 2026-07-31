# SiLS vECU Tool Confidence & Evidence Dossier

이 문서는 SiLS vECU **conformance kit**(`tools/sils/conformance/`)이 내리는 판정을 **왜 신뢰할 수 있는가**를 근거와 함께 정리한 기술 문서다. ECU 개발사·OEM의 **기능안전(ISO 26262) 담당이 자기 안전 케이스에 얹을 수 있는 증거 자료**로 쓰이는 것을 목표로 한다.

> **스코프 — 이 문서가 주장하는 것 / 하지 않는 것 (먼저 읽을 것)**
>
> - **주장한다**: 이 kit은 SiLS 실행이 남긴 트레이스를 **결정론적·재현 가능하게** 판정하며, 그 판정의 근거(검사 항목·보증 수준·한계)가 공개되어 있다.
> - **주장하지 않는다**: 이 문서는 **ISO 26262 Tool Qualification(도구 자격, 26262-8 §11)이 아니다.** Tool Confidence Level(TCL) 분류와 자격은 **사용 맥락에 종속**되므로 도구 공급자가 단독으로 완결할 수 없다 — 그건 이 도구를 안전 활동에 쓰는 **조직의 안전 케이스**가 한다. 이 문서는 그 활동에 필요한 **도구 출력 근거·결정론·알려진 한계**를 제공하는 입력물이다.
> - 이 kit은 **SiLS(소프트웨어) 수준** 검증 도구다. 실시간 거동·전기적 I/O·하드웨어 결함은 HILS 몫이며 이 도구의 판정 범위 밖이다(§6 한계 참고).

---

## 1. 증거 체인 — 트레이스가 유일한 판정 경계

이 kit의 근본 설계는 **"실행이 남긴 JSONL 트레이스가 공개 conformance 경계"**(docs/sils/sils-extension-model.md §2)라는 것이다. 판정 도구는 오직 이 트레이스만 소비하며, vECU 내부 상태나 소스에 접근하지 않는다. 따라서 판정은 **관찰 가능한 증거에만 근거**하고, 제3자가 같은 트레이스로 같은 결론을 재생산할 수 있다.

```
   실행(vECU + 하네스) ──emit──▶  트레이스(JSONL)  ◀── 유일한 판정 입력
                                      │
      ┌───────────────────────────────┼────────────────────────────────┐
      ▼                                ▼                                ▼
 validate_trace.py                evaluate.py                     compare_golden.py
 (스키마 well-formed)         (fidelity·verdict·conformant)      (골든 회귀, §12.4)
      │                                │                                │
      └──────────────┬─────────────────┘                                │
                     ▼                                                   │
                  gate.py  ──(SCHEMA → FIDELITY → PARITY, fail-closed)──┐ │
                     │                                                  ▼ ▼
                     │                                              parity.py
                     │                                        (L4, 실타깃 레퍼런스 대조)
                     ▼
              단일 exit code + JUnit(--junit)
```

| 도구 | 답하는 질문 | 근거 |
|---|---|---|
| **validate_trace.py** | 트레이스가 스키마에 맞게 well-formed한가 | `config/sils/trace-schema/v1.json`의 이벤트별 `required` 필드 존재 검사. 미지 이벤트=위반(옵트아웃 가능), 여분 필드=허용(forward-compat). fail-closed |
| **evaluate.py** | 이 실행이 자체 기준으로 통과했는가·어느 fidelity인가 | terminal 이벤트 → verdict, 검사기(analyzer)들 → violations, ladder → fidelity |
| **compare_golden.py** | 이 실행이 known-good 골든과 여전히 일치하는가(회귀) | tolerance 규칙 기반 비교(byte 일치 아님, §12.4) |
| **parity.py** | SiLS vECU가 **실 ECU처럼** 거동하는가(L4) | 실타깃 레퍼런스 트레이스 대조 + 환경 attestation |
| **gate.py** | 위를 하나로 — 통과/실패 | 세 검사기를 라이브러리로 체이닝, 어느 단계든 실패면 non-zero |

---

## 2. 결정론 논거 — 판정을 신뢰할 수 있는 핵심 이유

conformance 판정이 신뢰 가능한 1차 근거는 **판정 대상 실행이 결정론적으로 재현된다**는 것이다. kit이 판정하는 경로에는 **wall-clock/RNG가 관여하지 않는다**:

1. **클럭** — lockstep 모드(`SILS_CLOCK_MODE=lockstep`)에서 tick은 ClockMaster의 `TickGrant`로만 전진한다. `analyze_lockstep`이 tick_seq가 1..N +1 연속, sim_time이 상수 dt, 모든 barrier 닫힘, timeout 없음을 확인한다. → 시간 진행이 wall clock 독립.
2. **입력 순서** — tick 경계 staging(§7.3)이 각 도착에 `recv_seq`를 부여하고 tick 경계에서 freeze한다. `analyze_staging`이 "tick 순서로 이어붙인 recv_seq가 채널별 strictly increasing"(중복·역전·유령 없음) 불변식을 확인한다. → 어느 입력이 어느 tick에 소비되는지가 고정.
3. **결함 주입** — `SILS_INPUT_FAULTS`의 결함은 **tick_seq 고정 스케줄**에서 나온다(wall/RNG 아님). → 결함이 있는 실행조차 재현 가능.
4. **FMU 코시뮬** — `doStep(t, h)`는 입력의 순수 함수. → FMU 기반 제어/플랜트도 결정론.

**따라서 "같은 입력 → bit-동일 트레이스 → 동일 판정"이 성립한다.** 이는 §8 결론에서 두 번 실행해 `control_published` 시퀀스가 동일함으로 실증되었다(로드맵/설계문서 §5.2). 결정론은 회귀 비교(compare_golden)와 parity(L4)가 성립하기 위한 전제이기도 하다.

> **핵심 함의**: 판정이 재현 가능하다는 것은, 리뷰어/CI/고객 안전팀이 **독립적으로 같은 트레이스를 다시 판정해 동일 결론**을 얻을 수 있다는 뜻이다(§7 재현 레시피).

---

## 3. Verdict — 판정은 terminal 이벤트에 근거한다

`evaluate.py`는 트레이스의 **terminal 이벤트**로 verdict를 정한다(임의 추정 아님):

| terminal 이벤트 | verdict | 출처 |
|---|---|---|
| `run_pass` / `run_fail` | PASSED / FAILED | Run Coordinator (권위) |
| `ready_timeout` / `start_failed` / `aborted_orphaned` | ABORTED | Run Coordinator |
| `verify_pass` / `verify_fail` | PASSED / FAILED | 독립 verifier (coordinator 없는 standalone 런의 fallback; `terminal_source=verifier`로 기록) |
| (없음) | INCOMPLETE | 트레이스 절단 또는 진행 중 |

- verifier fallback은 **절단이 아니다**: verifier는 `received_count >= min_samples`일 때만 pass를 내므로 완결 신호다.
- **CONFORMANT의 정의는 엄격하다**: `verdict == PASSED` **그리고** violations가 하나도 없을 때만 conformant. PASSED-with-violations, INCOMPLETE, FAILED, ABORTED는 모두 non-conformant.

---

## 4. Fidelity ladder — 각 레벨이 보증하는 것 / 하지 않는 것

`fidelity_evidenced`는 트레이스가 **증거로 보인 최고 레벨**이다(`FIDELITY_RANK = {none:0, L0:1, L1:2, L2:3, L3:4, L4:5}`). 레벨은 **누적적 보증**이며, 각 레벨의 정확한 의미는:

| 레벨 | 보증한다 | 보증하지 **않는다** |
|---|---|---|
| **L0** | 엔드포인트 매칭 + 샘플 수신(VISS) 또는 버스 working loop(state in·control out). "연결되고 well-formed 데이터가 흐른다" | 프로토콜 정합·결정론·폐루프 |
| **L1** | 요청/응답 상관·QoS 정합(RELIABLE)·에러 처리 = 프로토콜 충실(reqresp conformant) | 폐루프·재현성 |
| **L2** | (제어 입력 open-loop 구동; live feedback 없음) | 폐루프 피드백 |
| **L3** | **폐루프 + 결정론.** in-process: live IPlant + feedback 연속성 + 결정론 궤적. 버스결합: 폐루프(state 소비) + **lockstep 결정론**(`clock.deterministic` 필수) | **실 타깃과의 일치**(그건 L4). SiLS는 여기까지가 상한 |
| **L4** | **실 ECU와의 parity** — 동일 자극에 실타깃 레퍼런스와 관찰 인터페이스가 계약 tolerance 내 일치 **AND** 환경 attestation(ARM64 + 실 gateway/protocol) 충족 | (단일 트레이스로 증거 불가 — parity.py가 cross-trace로 판정) |

**정직한 경계**: 
- **SiLS 도구가 자체적으로 도달 가능한 최고는 L3**이다. L3는 "폐루프가 결정론적으로 돈다"이지 "실차와 같다"가 아니다.
- **L4는 이 kit 단독으로 못 준다** — `parity.py`가 실타깃 레퍼런스 트레이스를 **권위**로 두고 대조하며, 값/타이밍 parity + attestation **둘 다** 필요하다. 값이 우연히 맞아도 x86·stub gateway면 L4가 아니라 최대 L3다.

---

## 5. 검사 → 보증 매핑 (analyzer가 실제로 확인하는 것)

verdict가 PASSED여도, 아래 검사 중 하나라도 위반이면 **conformant가 아니다**. 각 검사가 트레이스에서 확인하는 사실:

| analyzer | 확인하는 사실 | 위반이 뜻하는 것 |
|---|---|---|
| `analyze_lockstep` | tick_seq 연속·상수 dt·모든 barrier 닫힘·no timeout | 클럭 비결정론 |
| `analyze_staging` | §7.3 채널별 recv_seq strictly increasing(중복·역전·유령·유실 없음) | 입력 순서 비결정론 또는 꼬리 유실 |
| `analyze_behavior` | behavior_output에 수치 value 존재 | 제어 출력 누락 |
| `analyze_plant` | (in-process) live·폐루프 피드백 연속성·상수 dt·유한 출력 | 플랜트 개루프/비결정론/NaN |
| `analyze_vehicle` | (버스결합) tick당 1명령·순증·유한 steering/accel·유효 gear·폐루프(state 소비) | 버스 루프 개루프/기형 명령 |
| `analyze_fault` | **결함 완전성**(선언 수==주입 수) + **강건성**(각 결함 tick의 명령이 유한·유효) | 결함 미주입(거짓 PASS) 또는 **손상 상태가 액추에이션으로 전파(안전 저하 실패)** |
| `analyze_scenario` | (VISS 아웃바운드) 선언 결함 수==발화 수 | 결함 미주입 |
| `analyze_protocol` | 요청/응답 상관·QoS RELIABLE·에러 | 프로토콜 위반 |
| `analyze_attestation` | 실타깃 구성요소(arch/gateway/protocol) 기록 | (점수 아님 — L4 parity 입력) |

각 검사는 **결함이 조용히 통과하지 못하도록 fail-fast**(§14) 설계다 — 예: `analyze_fault`는 결함이 실제로 주입됐는지(완전성)와 그 결과 제어가 안전한지(강건성)를 분리해, "결함 런인데 결함이 안 들어가서 통과"하는 거짓 양성을 막는다.

---

## 6. 알려진 한계·가정 (안전팀이 반드시 읽을 것)

이 도구를 안전 활동에 쓸 때 **명시적으로 고려해야 하는** 한계:

1. **1-tick I/O 지연** — 단일 배리어 lockstep에서 vECU는 sim state를 T-1에 소비한다. "로직이 도는가"엔 무해하나 **폐루프 동특성이 1-tick 밀린다** — 실차 제어 성능과 정확히 같지 않다. zero-lag은 phased barrier(미구현).
2. **폐루프는 fixture로 증명됨, 실 MORAI/Autoware 아님** — 현재 lockstep 결정론·폐루프는 SDK 동봉 fixture 소스(`sils_ego_source` 등)로 실증됐다. **실 MORAI는 실시간 Unity라 우리 배리어에 물려있지 않다** — MORAI를 tick에 물릴 수 있는지(step API)가 미확인이며, 안 되면 실 연동 시 결정론은 우리 참가자 사이에서만 성립한다.
3. **실시간 충실도 없음** — SiLS는 비실시간이 설계다. 스케줄링 지터·전기적 I/O 타이밍·CAN 지연은 모델링되지 않으며 HILS 영역이다.
4. **단일 vECU 위주** — "프로세스=인스턴스" 모델. 다중 ECU가 한 버스에서 상호작용하는 통합 시나리오는 배관만 되어 있고 대규모로 검증되지 않았다.
5. **결함 taxonomy는 시작 집합** — 인바운드 `drop/nan/oor`, 아웃바운드 `drop/corrupt/duplicate`. 실제 안전 케이스가 요구하는 결함(비트 플립·지연·부분 손상 등)은 확장 필요.
6. **정식 도구 자격 아님** — 위 §스코프. TCL 분류·validation은 사용 조직 몫.

이 한계들은 결함이 아니라 **적용 경계**다. 안전 케이스는 이 경계 안에서 이 도구의 출력을 근거로 사용해야 한다.

---

## 7. 독립 재현 레시피 (판정의 재검증)

제3자가 판정을 독립 재생산하는 절차 — 도구 신뢰의 실증:

```bash
# 1) 같은 실행을 두 번 (lockstep 결정론)
#    → control_published 시퀀스가 bit-동일해야 함 (결정론 확인)
diff <(grep control_published run1.jsonl) <(grep control_published run2.jsonl)   # 차이 없음

# 2) 트레이스를 게이트에 통과 (스키마 → fidelity → parity)
python3 tools/sils/conformance/gate.py run1.jsonl            # exit 0 = 통과
python3 tools/sils/conformance/gate.py run1.jsonl --junit out.xml   # CI 인제스트용

# 3) 판정 세부 (fidelity/conformant/각 analyzer)
python3 tools/sils/conformance/evaluate.py run1.jsonl        # fleet/fidelity/conformant + 검사별 결과

# 4) 골든 회귀 (known-good 대비)
python3 tools/sils/conformance/compare_golden.py --candidate run1.jsonl --golden golden.jsonl --rules rules.yaml

# 5) (해당 시) L4 parity — 실타깃 레퍼런스 대조
python3 tools/sils/conformance/parity.py --sils run1.jsonl --target ref.jsonl --contract parity.yaml
```

kit 자체의 정확성은 단위 테스트로 회귀 보호된다(`tools/sils/conformance/test_*.py`, 148+ 케이스): analyzer별 정상/위반 경로, fidelity bump, terminal 처리, MIMO staging 오탐 회귀, 결함 완전성/강건성/전파 등.

---

## 8. 요약

- 판정은 **관찰 가능한 트레이스**에만 근거하고, **결정론적으로 재현**되며, **검사 항목·보증 수준·한계가 공개**되어 있다 — 이것이 이 도구를 안전 케이스의 근거로 쓸 수 있게 하는 세 축이다.
- **SiLS 상한은 L3**(폐루프 결정론), **L4(실 타깃 parity)는 parity.py + 실타깃 레퍼런스 + attestation**으로만.
- 이 문서는 **정식 도구 자격이 아니라 그 입력**이다. TCL 분류·자격은 도구를 쓰는 조직이 자기 맥락에서 수행한다.
- 최대 미검증 가정은 **실 MORAI/Autoware와의 폐루프**(현재 fixture로만 실증) — 실 브리지 연동 시 재검증 대상.

관련: [conformance kit README](../../tools/sils/conformance/README.md) · [트레이스 스키마](../../config/sils/trace-schema/v1.json) · [플랜트결합 설계](sils-morai-plant-adapter-design.md) · [확장 모델](sils-extension-model.md)
