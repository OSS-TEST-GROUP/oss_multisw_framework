# SiLS 기능 검증 & 툴 상호운용 설계

"우리 SiLS로 개발하면 **로직 검증완료**가 되는가?"에 정직하게 답하고, **되게 만드는 경로**(기능 판정 엔진 + 표준 기반 상호운용)를 설계로 확정한다. 후속 구현 — (a) 기능 판정 엔진 → (b) OpenSCENARIO 수용 — 의 근거 문서.

> **한 줄 결론**: 지금 우리 SiLS는 **"루프가 건전하고 재현 가능한가"까지 자동 판정**한다(= 검증의 *토대* L3). **"내 로직이 내 요구를 만족하는가"**(= 검증완료)는 그 위에 **① 현실적 플랜트 ② 시나리오 ③ 기능 판정 기준**을 얹어야 성립하며, ③의 **판정 엔진은 지금 만들 수 있다**. 채택의 관건은 **다른 툴 사용자가 자기 산출물을 재작성 없이 가져오게** 하는 것 — 열린 표준(FMI·OpenSCENARIO·MF4·ReqIF)에서 만난다.

관련: [tool-confidence dossier](sils-tool-confidence.md) · [플랜트결합 설계](sils-morai-plant-adapter-design.md) · [확장 모델](sils-extension-model.md)

---

## 1. 문제 정의 — conformance ≠ verification

현재 conformance kit(`evaluate.py`/`gate.py`)이 자동 판정하는 것:

- 루프가 **닫혔나**(state 소비 → 로직 → control), 명령이 **well-formed**(유한·유효 gear), **결정론**인가, **안전 저하**(결함 주입 시 NaN 전파 안 함), **회귀**(골든 대비) 없나.

이것이 답하는 질문은 **"루프가 건전하고 재현 가능한가"**이다. 이것은 **"로직이 기능 요구를 만족하는가"와 다르다.** 우리 fidelity **L3 = "폐루프가 결정론적으로 돈다"**이지 "로직이 검증됐다"가 아니다 — L3는 검증의 *토대*이고, 검증완료는 그 위에 쌓는 층이다.

## 2. 두 층 — 우리가 주는 것 vs 개발자가 얹는 것

| | 지금 우리 SiLS가 자동으로 주는 것 | "검증완료"에 개발자가 얹어야 하는 것 |
|---|---|---|
| 실행 | 결정론·재현 폐루프(lockstep) | — |
| 판정 | conformance(루프 건전성)·결함 강건성·회귀 | **기능 요구 pass/fail**(③) |
| 플랜트 | fixture(단순 램프 = 배관용) / FMU 플랜트 **경로** | **현실적 플랜트**(MORAI·검증 FMU)(①) |
| 자극 | fixture 소스 + 결함 스케줄 | **의미 있는 시나리오**(②) |

우리는 **"검증의 엔진과 심판"**을 만들었다. **"무엇을 검증할지(플랜트·시나리오·기준)"**가 남은 층이며, 이 문서는 그중 **③ 기능 판정을 자동화**하고 **①②③ 입력을 표준에서 수용**하는 설계다.

## 3. 기능 판정 엔진 (구현 a)

### 3.1 형태 — 선언형 트레이스 assertion

개발자가 **요구 spec을 한 번 선언**하면, 새 analyzer(`analyze_requirements`)가 트레이스 시계열에서 메트릭을 계산해 **요구별 pass/fail**을 gate에 낸다. 자동화되는 것은 **판정 엔진**이고, 자동화 안 되는 유일한 것은 **기준을 1회 명세**하는 것(요구 자체 — 매 실행 수작업 아님).

```yaml
# requirements.yaml — 개발자가 한 번 작성. 요구 ID로 추적성 확보.
requirements:
  - id: SR-BRK-014                         # 요구 추적 ID (안전 케이스 연결)
    name: comfort-decel-bound
    signal: control_published.acceleration # 트레이스 이벤트.필드
    assert: {in_range: [-8.0, 4.0]}        # 항상(G) 범위 내, m/s²
    scenario: [urban, highway]             # 시나리오별 적용(②와 결합)
  - id: SR-BRK-021
    name: emergency-stop-settle
    signal: state_received.vx
    assert: {settle_to: {target: 0.0, tol: 0.2, within_tick: 40}}   # F[.,40]
  - id: SR-STE-007
    name: steering-actuator-limit
    signal: control_published.steering_angle
    assert: {max_abs: 0.6}                  # rad
```

**assert 라이브러리**(트레이스에서 계산): `in_range`(경계)·`max_abs`·`max_rate`(jerk 등 차분)·`settle_to`(정착)·`overshoot_lt`·`never`(안전 불변식). — STL의 G(always)/F(eventually)를 **데이터로** 표현한 간이형.

### 3.2 현업 대비 위치

이 접근은 **간이 STL / Simulink Test Assessment를 코드가 아니라 데이터(YAML)로** 쓴 것이다. **선언형 → tool-neutral·diff 가능·CI 친화**이며 "트레이스가 경계"라는 우리 철학과 일관. CAPL/Python 같은 실행 코드가 아니라 스펙이라 리뷰·추적·재현이 쉽다.

### 3.3 정직한 의존

**엔진은 지금 만들 수 있지만(순수 Python·트레이스만·sandbox 완결, S5 결함주입과 동류), 판정의 값어치는 ① 현실적 플랜트에 걸린다.** stub fixture 상대로 "제동거리"를 판정하면 무의미하다 — 엔진은 지금, 의미는 실 플랜트 붙은 뒤.

## 4. 상호운용 / 마이그레이션 — 재작성 최소화

**채택의 관건**: 다른 툴을 쓰던 사람이 **자기 산출물을 재사용하거나 최소 변경으로** 우리 SiLS를 쓰게 해야 한다. 재작성을 강요하면 안 옮겨온다.

### 4.1 원칙 — 표준에서 만난다

산출물은 두 부류: **표준 포맷**(importer만 만들면 들어옴) vs **독점 로직**(툴 락인, 재표현 불가피). 그들의 **모델·시나리오·요구·데이터**는 대부분 열린 표준으로 뽑을 수 있으므로, 그것을 consume하면 재작성이 "판정 assertion 몇 줄"로 줄어든다.

| 표준 | 무엇 | 우리 활용 | 상태 |
|---|---|---|---|
| **FMI / FMU** | 모델·제어기·V-ECU export | 플랜트 **및** ECU로 로드 | ✅ **이미 있음** |
| **OpenSCENARIO (.xosc)** | 주행 시나리오 + 종료·성공 조건 | 시나리오(②) **+ StopTrigger→요구(③)** | ⭐ 최대 레버리지 신규 |
| **ASAM MF4** | 측정 트레이스 | 우리 트레이스 ↔ MF4 in/out | 신규 |
| **ReqIF** | 요구 항목(DOORS/Polarion/Simulink Req) | 요구 ID·추적성 링크 | 신규 |
| **STL 수식** | 형식 시간 요구(Breach/S-TaLiRo) | assert로 직접 수용 | 신규(선택) |

### 4.2 출발 툴별 "최소 변경"

| 출발점 | 그대로 재사용 | 재표현 필요 |
|---|---|---|
| **Simulink(MBD)** | 모델→**FMU**, 요구→**ReqIF** | Test Assessment 로직 → YAML(소량) |
| **CarMaker/MORAI/Autoware** | **OpenSCENARIO .xosc** | **거의 없음**(성공조건이 xosc에) |
| **Vector CANoe** | 측정 **MF4** | CAPL 테스트 → YAML |
| **dSPACE** | **MF4**, 모델 | AutomationDesk Python 평가 → YAML |
| **형식기법** | **STL 수식** | 거의 없음 |

**정직한 한계**: Simulink `verify()`·CAPL·AutomationDesk 스크립트 같은 **독점 판정 로직은 그대로 못 옮긴다**(락인). 옮겨오는 건 표준 계층(모델=FMU·시나리오=xosc·요구=ReqIF·데이터=MF4)이며, pass/fail은 한 번 재표현한다 — 단 OpenSCENARIO StopTrigger는 그 자체가 pass/fail이라 재표현마저 최소.

### 4.3 설계 원칙 — 정규 IR + 얇은 importer

MORAI 어댑터와 **동일 패턴**: **중립 정규 표현 + 얇은 importer(anti-corruption 번역)**. 우리 내부 표현(requirements / 시나리오 IR)을 표준들이 매핑돼 들어오는 superset으로 두고, `.xosc→시나리오+요구`·`ReqIF→요구ID`·`MF4↔트레이스` importer는 **실 고객이 그 포맷을 들고 올 때** 만든다(earned-abstraction). 엔진은 하나, 입구는 여러 표준.

## 5. 현업 SILS 방식 비교 (참고)

> 출처: 도메인 지식. 큰 그림(툴·표준)은 안정적이나 세부 버전은 리서치로 정밀화 가능.

| 진영 | 요구를 넣는 형식 | 판정 방식 | 자극 |
|---|---|---|---|
| **MathWorks** | Test Assessment/`verify()` + Simulink Requirements | 케이스별 verdict + 추적성 + Coverage(MC/DC) | Test Sequence |
| **Vector** | 테스트 테이블 + CAPL | 스텝 verdict → 리포트 | CANoe |
| **dSPACE** | AutomationDesk Python + 신호 평가 라이브러리 | 시퀀스 pass/fail | ModelDesk |
| **NI** | TestStand 스텝 + 한계 검사 | 스텝 pass/fail | VeriStand |
| **형식기법(AV/학계)** | **STL** 수식 | robustness + falsification | 시나리오 falsification |
| **AV/Autoware** | **OpenSCENARIO** StopTrigger 조건 | 시나리오 성공/실패 | .xosc |
| **우리(목표)** | **선언형 YAML**(간이 STL) + 표준 import | gate PASS/FAIL + 요구ID | .xosc(수용) |

**공통 패턴**: 전부 "**신호 시계열에 시간적 assertion을 걸어 자동 pass/fail**". 표현 수단만 다르다(코드/그래픽/형식수식/시나리오 내장). 우리는 그것을 **선언형 데이터**로 하고 **표준을 수용**하는 자리를 택한다. 표준 계층: 자극=OpenSCENARIO, 접근 API=ASAM XIL, 측정=ASAM MDF/MF4.

## 6. 로드맵

- **(a) `analyze_requirements` 엔진** — ✅ **구현 완료**(evaluate.py/gate.py). assert 5종(`in_range`/`max_abs`/`max_rate`/`settle_to`/`overshoot_lt`), `requirements.yaml`(요구 ID 추적성 + signal `event.field` + 선택 `scenario`), gate `--requirements`로 FIDELITY verdict에 융합. sandbox 자체검증(단위 5 + gate 2, 전체 155 passed). 샘플 `samples/vehicle-requirements.yaml`.
  ```bash
  gate.py run.jsonl --requirements requirements.yaml    # 요구 미충족이면 gate FAIL
  evaluate.py run.jsonl --requirements requirements.yaml  # report["requirements"]: total/passed/failed
  ```
- **(b) OpenSCENARIO 수용** — `.xosc → 시나리오(②) + StopTrigger를 요구(③)로` 추출하는 importer. **②와 ③을 한 번에** 때리고, **MORAI·Autoware·CarMaker·Simulink가 전부 이 포맷을 말함** → 어느 진영에서 와도 재사용. 나중에 **① 실 브리지(MORAI)와도 정합**(같은 .xosc로 MORAI 구동 + 우리 판정).

**순서 근거**: (a)가 판정 엔진(요구를 받는 내부 표현)을 세우고, (b)가 그 요구를 **표준에서 자동으로 채운다**. (a) 없이 (b)만 있으면 채울 대상이 없다.

**정직한 의존**: (a)(b) 둘 다 **메커니즘은 지금** 만들 수 있으나, **판정의 의미는 ① 실 플랜트** 붙은 뒤에 완성된다. ①(실 브리지)은 별도 개발자 확인 후 착수(현재 보류).

## 7. 스코프 / 한계 (정직)

1. **독점 판정 로직은 재사용 불가** — 표준 계층만 옮겨오고 assertion은 1회 재표현.
2. **완전한 시간 논리(STL 중첩 U/연산)는 나중** — 지금은 간이 assert.
3. **커버리지(MC/DC)·실시간 충실도는 범위 밖** — 전자는 SIL 코드커버리지 툴, 후자는 HILS.
4. **판정 값어치는 ① 실 플랜트 종속** — 엔진은 지금, 의미는 나중.
5. **현업 비교(§5)는 도메인 지식 기반** — 포맷 세부는 구현 시 리서치로 못박음.
