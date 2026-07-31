# SiLS 에이전트 자동화 설계 (Agent Automation)

SiLS 검증 파이프라인을 **LLM 에이전트로 자동화**하는 설계 문서. "에이전트가 SiLS를 돌리고 리포트를
받는다"를 **어디까지 에이전트가 하고, 어디부터는 절대 하지 않는가**의 경계로 정의한다.

> **스코프 — 이 문서가 주장하는 것 / 하지 않는 것 (먼저 읽을 것)**
>
> - **주장한다**: SiLS는 판정이 이미 결정론적 코드(`gate.py`/`evaluate.py`)에 있고 산출물이 전부
>   기계판독이므로, 그 **앞단(입력 생성)과 뒷단(실패 해석·리포트)** 은 에이전트로 자동화할 수 있다.
> - **주장하지 않는다**: 에이전트가 conformance 판정에 참여할 수 있다고 주장하지 **않는다**.
>   판정 권위는 `evaluate.py`/`parity.py`에만 있다([sils-tool-confidence.md](sils-tool-confidence.md) §1).
>   에이전트 출력은 **증거가 아니라 사람이 읽는 해석**이며, 안전 케이스에 인용될 수 없다.
> - **현재 상태**: 이 문서는 **설계(제안)** 다. 아래 §7의 "현재" 열이 실제 구현 상태다.
>   §3의 파이프라인은 이미 존재하는 것, §5의 에이전트 계층은 아직 없는 것이다.

---

## 1. 왜 SiLS가 에이전트 자동화에 유리한가

에이전트 자동화가 실패하는 전형적 원인은 **"결과가 좋은지 나쁜지를 LLM이 판단해야 하는"** 구조다.
판정이 확률적이면 CI로 쓸 수 없고, 회귀 방어선이 되지 못한다.

SiLS는 그 반대로 설계돼 있다:

| 이미 갖춰진 것 | 근거 | 자동화에 주는 의미 |
|---|---|---|
| **판정 = 결정론 코드** | `gate.py` (SCHEMA→FIDELITY→PARITY, fail-closed, 단일 exit code) | 에이전트가 채점자가 될 필요가 **없다** |
| **판정 경계 = 트레이스** | "실행이 남긴 JSONL이 공개 conformance 경계"([sils-extension-model.md](sils-extension-model.md) §2) | 에이전트가 소비할 입력이 **명세된 구조체**(`config/sils/trace-schema/v1.json`) |
| **구조화된 실패 증거** | `verdict.json` 의 `violations[]` + `clock`/`staging`/`plant`/... 섹션 | 실패 해석을 **트레이스 전문 없이** 시작할 수 있다 |
| **재현성** | lockstep tick·`recv_seq` staging·tick_seq 고정 결함 스케줄·FMU 순수함수 ([sils-tool-confidence.md](sils-tool-confidence.md) §2) | 에이전트가 "다시 돌려서 확인"이 **의미 있는** 행위가 된다 |
| **단일 진입점 + 단계 skip** | `sils-ci.sh` (`--traces-dir`/`--dry-run`/`--resolved`) | 에이전트가 **바이너리 없이도** 전 구간을 다룰 수 있다 |
| **병렬 격리** | `domain_lease.py` (유한 pool·TTL·owner 임대) | 에이전트 fan-out이 **DDS domain 충돌 없이** 가능 |

한 줄 요약: **채점기가 이미 있으므로, 에이전트는 채점을 하지 않고 채점의 앞뒤를 맡는다.**

---

## 2. 근본 경계 — 에이전트는 판정에 들어가지 않는다

```
 ┌── 에이전트 영역 ──┐   ┌──── 결정론 영역 (에이전트 진입 금지) ────┐   ┌── 에이전트 영역 ──┐
 │                  │   │                                        │   │                  │
 │  시나리오/토폴로지 │──▶│  sils-ci.sh                            │──▶│  실패 트리아지     │
 │  생성 (§5.3)      │   │   lint→compile→run→evaluate→junit      │   │  (§5.1)          │
 │                  │   │   →compare→parity                      │   │                  │
 │  커버리지 탐색     │   │                                        │   │  리포트 합성       │
 │                  │   │  gate.py: verdict = f(trace)            │   │  (§5.2)          │
 └──────────────────┘   │  ↑ 이 f 를 에이전트가 대체·보정·재해석  │   └──────────────────┘
                        │    하는 순간 검증 체계가 무너진다        │
                        └────────────────────────────────────────┘
```

### 2.1 왜 이 경계인가

`evaluate.py`의 `conformant`는 `verdict == "PASSED" and not violations` — **단일 불변식**이다.
에이전트가 여기에 "그런데 이 violation은 사소해 보입니다"를 끼워 넣으면:

1. **재현성 상실** — 같은 트레이스가 실행마다 다른 결론을 낼 수 있다.
   [sils-tool-confidence.md](sils-tool-confidence.md) §2의 "같은 입력 → 동일 판정" 논거가 깨진다.
2. **제3자 재생산 불가** — OEM/안전팀이 같은 트레이스로 같은 결론을 얻는다는 보장이 사라진다.
3. **녹색 편향** — 에이전트는 태스크를 "완료"하려는 압력을 받는다. 판정 권한을 주면
   실패를 통과로 만드는 방향(§6.1)으로 흐른다.

### 2.2 파생 규칙: 에이전트 쓰기 금지 경로

에이전트가 **판정 기준 자체를 수정**하는 것은 판정에 참여하는 것과 같다. 단 대상을 두 종류로
나눠야 한다 — 전부 금지하면 정당한 개발까지 막힌다.

**(a) `deny` — 판정 기준 *데이터*.** 에이전트가 손댈 정당한 이유가 사실상 없다.

| 경로 | 이유 |
|---|---|
| `config/sils/trace-schema/` | 스키마 완화 = 위반을 위반이 아니게 만듦 |
| `config/sils/parity/` | tolerance 완화 = L4 판정 무력화 |
| `config/sils/regression/` | 회귀 규칙 완화 |
| `config/sils/fmu/allowlist.yaml` | **보안 게이트.** `.fmu`는 네이티브 `.so`/`.dll`을 싣고 로드 = **in-process 코드 실행**이라 `fmu_gate.py`가 sha256 대조로 fail-closed 방어한다. allowlist 추가 = 코드 실행 승인. §4.2 Phase 2에서는 **필수** |
| golden 트레이스 디렉터리 (경로는 P0에서 확정) | 잘못된 baseline은 이후 **모든** 회귀 판정을 오염 (§6.2) |

**(b) `ask` — 판정 *로직* 코드.**

| 경로 | 이유 |
|---|---|
| `tools/sils/conformance/*.py` | 판정 로직 본체. 새 analyzer 추가·버그 수정 같은 **정당한 개발이 계속 일어나므로** 전면 금지는 과하다. 대신 항상 사람이 본다 |

에이전트가 (a) 수정을 **제안**하는 것은 유효하다 — 커밋하는 것이 금지다.

### 2.3 강제는 2층 — 규칙을 어디에 두는가가 내용보다 중요하다

이 규칙은 **개발자별 설정이 아니다.** 각자 설정하면 보증이 사라진다 — 한 명이 설정하지 않은 채로
tolerance 완화를 커밋하면 그걸로 끝이다. `.clang-tidy`를 리포에 커밋하는 것과 같은 범주다.

| 파일 | 범위 | git | 무엇을 담나 |
|---|---|---|---|
| `.claude/settings.json` | 프로젝트 **공유** | **커밋** | **§2.2 규칙.** clone한 모든 개발자에게 적용되므로 복사 배포가 필요 없다 |
| `.claude/settings.local.json` | 개발자별 | **gitignore 필요** | 개인 오버라이드(모델·개인 allow 목록). 현재 `.gitignore`에 claude 항목이 없어 **추가해야 한다** |
| `~/.claude/settings.json` | 개인 전역 | — | 개인 취향. §2.2를 여기 두면 안 된다 |

로드 순서는 user → project → local(뒤가 앞을 덮음)이지만, 권한 규칙은 목록이 **누적**되고 `deny`가
`allow`를 이기므로 개인 설정으로 공유 `deny`를 되돌릴 수는 없다.

**그리고 `settings.json` 하나로는 보증이 되지 않는다.** `deny`는 `Edit`/`Write`를 막지만
**Bash 우회가 가능**하다(`sed -i`, `> file`, `python -c`). `.claude/settings.json` 자체도 리포
파일이라 편집할 수 있다.

| 층 | 수단 | 성질 |
|---|---|---|
| **1층** | `.claude/settings.json` (커밋) | **가드레일.** 실수·무심한 편집을 막고, 우회하려면 **git diff에 보이게** 만든다. 즉시·무료 |
| **2층** | `.github/CODEOWNERS` + 해당 경로 리뷰 필수 | **진짜 보증.** Bash 우회까지 막는 유일한 지점 |

현재 CODEOWNERS가 없다. 다만 [publish-develop.yml](../../.github/workflows/publish-develop.yml)에
"공개 develop 직접 push 금지(정책 §6), PR 필수" 규약이 이미 있어 문화는 존재한다 — SiLS 판정
경로만 지정하면 붙는다.

---

## 3. 현재 파이프라인 (에이전트가 감싸는 대상)

```
config/sils/{contracts,qos,topologies}  +  service/sils/vecu/descriptors/
        │
        ▼  lint_catalog.py  →  compile.py  →  resolved-run.yaml
        ▼  run_phase1.sh (local | docker | fleet)   ← domain_lease.py
        │
     trace.jsonl  ◀── 유일한 판정 입력
        │
        ├─ validate_trace.py ─ 스키마 well-formed
        ├─ evaluate.py ────── verdict.json (fidelity L0~L4, violations)
        ├─ compare_golden.py ─ 회귀 (golden 대비)
        └─ parity.py ──────── L4 (실타깃 레퍼런스 대조)
        │
     gate.py (위 3개 fail-closed 체이닝) → 단일 exit code + junit.xml
```

상세는 [tools/sils/README.md](../../tools/sils/README.md), [sils-tool-confidence.md](sils-tool-confidence.md) §1.

### 3.1 에이전트가 소비하는 계약면

에이전트는 **`verdict.json` + `junit.xml` + `violations[]`** 를 1차 입력으로 쓴다.
`trace.jsonl` 전문은 2차 — **violation이 지목한 구간만** 읽는다(§6.4).

`evaluate.py`가 run당 내보내는 필드(에이전트 입력 스키마로 그대로 사용):

| 필드 | 트리아지에서의 역할 |
|---|---|
| `verdict` / `terminal_source` | PASSED·FAILED·ABORTED·INCOMPLETE + 누가 그 판정을 냈는지(coordinator/verifier) |
| `conformant` | 최종 게이트 |
| `fidelity_evidenced` | 어느 충실도까지 증거가 있었나 (L0~L4) |
| `violations[]` | **트리아지 시작점.** 각 항목이 어느 analyzer에서 왔는지 문자열에 담김 |
| `clock` / `staging` / `behavior` / `plant` / `vehicle` / `scenario` / `fault` | analyzer별 요약 — 어느 축이 깨졌는지 |
| `attestation` | L4 게이트 (arch/gateway/protocol) |
| `instance_ids` / `domains` | fleet 완전성·격리 확인 |
| `evidence.{endpoints_matched,samples_received,verify_pass}` | L0 증거 |

---

## 4. 전제 조건 — 자동화의 첫 조각은 에이전트가 아니다

**SiLS는 현재 CI에 연결돼 있지 않다.** `Jenkinsfile`, `Jenkinsfile.integration`,
`Jenkinsfile.unittest`, `.github/workflows/*` 전체에 `sils` 참조가 없다.

즉 **artifact를 정기적으로 생산하는 주체가 없다.** 에이전트는 소비자이므로, 소비할 것이 먼저 있어야
한다. 순서를 뒤집으면(에이전트가 매번 직접 빌드·실행) 느리고 불안정하고 재현되지 않는다.

| 전제 | 왜 필요한가 | 형태 |
|---|---|---|
| **P0-a. CI 스테이지** | 정기·재현 가능한 artifact 생산 | Jenkins에 `sils-ci.sh` 스테이지 1개 (`--traces-dir` 경로부터 시작 가능) |
| **P0-b. artifact 보존 규약** | 에이전트가 "지난번"과 비교할 수 있어야 함 | `runs/<run_id>/{resolved-run.yaml,traces/,verdict.json,junit.xml,compare/,parity/}` 를 N일 보존 |

### 4.1 플랫폼 제약 (실측)

| 환경 | vECU 본체(C++) | 도구 계층(py/sh) | 에이전트 루프 가능 경로 |
|---|---|---|---|
| Linux x86_64 + FastDDS | ✅ 실런타임 검증 (L0·L2·L3) | ✅ | 전 구간 (`run` 포함) |
| docker 백엔드 | ✅ (`--ipc=host` 필수 — SHM) | ✅ | 전 구간 |
| macOS (현 개발 데스크톱) | ❌ 툴체인 없음 | ✅ | `--traces-dir`(사전 캡처) · `--dry-run` **만** |

→ **로컬 에이전트 작업은 사전 캡처 트레이스 기반으로 설계해야 한다.** `sils-ci.sh`가 이 경로를
이미 1급으로 지원한다(주석: "샌드박스 테스트 가능 경로"). 트리아지·리포트·드리프트 감시는 전부
이 경로로 개발·검증 가능하다.

### 4.2 실행 주체 — Phase 1(사내) → Phase 2(국내 공개·연구)

"개발자가 자기 ECU를 올려 검증받는다"가 이 플랫폼의 방향이다. 다만 **누가 올리는가**에 따라 요구가
완전히 달라지고, 그건 2단계로 계획돼 있다.

| | **Phase 1 — 사내 (현재)** | **Phase 2 — 국내 공개 (대학·연구)** |
|---|---|---|
| 트리거 | 사내 git PR/push | 공개 git PR (fork 포함) |
| 컨트리뷰터 신뢰 | **신뢰 가능** (자기 조직) | fork PR = 임의 코드. 다만 악의보다 **실수**가 압도적 |
| ECU 소스 공개 | 문제 없음 | **문제 없음** — 연구자는 재현성·논문 때문에 공개를 원한다 |
| 실행 환경 제공자 | **사내 Jenkins** (이미 있음, self-hosted `OSS-Agent`) | **GitHub-hosted runner** (공개 리포 무료) → 조직 부담 0 |
| 목적 | 검증 환경 **자체를 검증** | 검증 환경을 **연구·교육에 개방** |

**핵심: 실행 환경 제공자는 부수적 인프라가 아니라 판정 권위의 일부다.** 누가 어디서 돌렸는지가
신뢰 수준을 정한다 — `parity.py`의 attestation 게이트(arch/gateway/protocol)가 정확히 그것을
검사한다.

**Phase 1에서는 실행/판정 분리가 필요 없다.** 사내는 신뢰 문제가 없으므로 CI가 그냥 빌드·실행·판정을
다 한다. self-hosted Jenkins가 오히려 정답이다. 따라서 §7의 0단계 → P0 → P1 순서가 그대로 유효하다.

**Phase 2에서 실행/판정을 분리할 수 있다는 것이 이 아키텍처의 이점이다.** `gate.py`는 trace만
소비하고 **`trace.jsonl`은 데이터이지 코드가 아니다.** 그래서 남의 코드를 실행하지 않고도 판정할 수
있다 — `--traces-dir`가 정확히 그 경로다.

| | Tier A — trace 제출 판정 | Tier B — 실제 실행 |
|---|---|---|
| PR이 담는 것 | ECU 소스 + **자기가 돌린 `trace.jsonl`** | 소스만 |
| CI가 하는 일 | `validate_trace` + `evaluate` | 빌드 + run + 전체 게이트 |
| 코드 실행 | **없음** | 있음 (신뢰 필요) |
| 필요 환경 | **python3만** | 격리 러너 + 공개 builder 이미지 |
| 적용 | 모든 PR 즉시 (fork 포함) | 메인테이너 승인/라벨 후 |
| 권위 | 컨트리뷰터 **자기신고** = 주장 | **authority** |

Tier A만으로도 실수의 대부분(스키마 위반·계약 위반·lockstep 깨짐·fleet 불완전)이 잡힌다. 그리고
자기신고 trace의 신뢰 수준을 표현할 어휘가 **이미 있다**: `parity.py`의 `PARITY_UNATTESTED`(=L3
상한). 새 개념을 만들 필요가 없다.

> **Phase 2의 최우선 가치는 재현성이다.** 연구자에게 핵심 질문은 "내 결과를 남이 재현할 수
> 있는가"이고, SiLS는 이미 그것을 갖고 있다 — 같은 입력 → bit-동일 trace → 동일 판정
> ([sils-tool-confidence.md](sils-tool-confidence.md) §2). `capture_golden`/`compare_golden`이
> 그대로 **연구 재현 도구**가 된다. 나중에 덧붙일 기능이 아니라 이미 결정된 강점이다.
>
> 공개 대상이 **국내**이므로 한국어 문서는 그대로 자산이다(문서 영어화는 Phase 2 범위 밖).

---

## 5. 에이전트 계층 설계

우선순위는 **효용 ÷ 위험** 순이다. 번호가 곧 구축 순서다.

### 5.1 A. 실패 트리아지 (우선순위 1)

FAIL 하나에 사람이 수천 줄 JSONL을 훑는 일을 대체한다. **효용 최대, 위험 최소(읽기 전용).**

| 항목 | 내용 |
|---|---|
| **트리거** | `gate.py`/`sils-ci.sh` exit ≠ 0 |
| **입력** | `verdict.json`, `junit.xml`, 지목된 `trace.jsonl` 구간, `resolved-run.yaml`, 최근 커밋 범위 |
| **출력** | 트리아지 노트(마크다운): ① 어느 analyzer의 어느 불변식이 깨졌나 ② 그 직전 tick의 상태·입력 ③ 의심 변경 지점 ④ 재현 명령 한 줄 |
| **권한** | **읽기 전용.** 코드·설정 수정 없음 |
| **성공 기준** | 재현 명령이 그대로 실패를 재현한다 (결정론이므로 검증 가능) |
| **실패 모드** | 그럴듯한 오진. → 완화: 재현 명령을 **필수 출력**으로 두고, 재현되지 않으면 "원인 미확정"으로 보고하게 강제 |

트리아지가 다루는 대표 실패 축 (analyzer별):

- `analyze_lockstep` — tick_seq 불연속 / barrier 미닫힘 / timeout → ClockMaster 경계 조사
- `analyze_staging` — 채널별 `recv_seq` 단조성 위반(중복·역전·유령) → 입력 스테이징 경계
- `compare_golden` MISMATCH — **어느 신호가 어느 tick에서 tolerance를 벗어났는가**가 곧 답
- `parity.py` DIVERGENCE / PARITY_UNATTESTED — 신호 발산 vs attestation 누락 구분
- fleet 완전성 — `missing instance` / `duplicate instance` → 격리·lease 문제

### 5.2 B. 리포트 합성 (우선순위 2)

**트리아지가 있어야 의미가 있다** — 해석 없는 리포트는 `junit.xml`을 다시 쓴 것에 불과하다.

| 항목 | 내용 |
|---|---|
| **입력** | N개 run의 `verdict.json` + `compare/` + `parity/` + (있으면) 트리아지 노트 |
| **출력** | 사람이 읽는 요약. 리포 관행에 맞춰 `docs/*.html` 형식 또는 공유 링크 |
| **필수 포함** | ① 새로 깨진 것(전회 대비 delta)을 **최상단** ② 충실도 분포(L0~L4) ③ 계속 깨져 있는 것 ④ 커버리지 공백 ⑤ **판정은 kit이 냈음을 명시** |
| **금지** | 요약이 verdict와 다른 인상을 주는 표현("대체로 양호"류). PASS/FAIL 수치를 먼저 쓴다 |
| **주기** | 야간 배치 1회 + 회귀 발생 시 즉시 |

### 5.3 C. 시나리오·토폴로지 확장 (우선순위 3)

현재 토폴로지는 **2개**뿐이다: `viss_probe_smoke.yaml`, `viss_scenario.yaml`.
조합 공간(behavior × plant × scenario 결함주입 × clock mode)은 훨씬 넓다.

| 항목 | 내용 |
|---|---|
| **에이전트가 하는 일** | 토폴로지/디스크립터 YAML **생성**, 커버되지 않은 조합 탐색 |
| **에이전트가 하지 않는 일** | 결과 판정. `evaluate.py`가 채점하고 `lint_catalog.py`/`compile.py`가 입력 위생을 본다 |
| **기존 자산 활용** | `import_openscenario.py` (OpenSCENARIO → 시나리오 입력) 가 이미 있음 — 생성보다 **가져오기**가 우선 검토 대상 |
| **종료 조건** | 새로 깨지는 조합이 K회 연속 나오지 않을 때 |
| **핵심 난점** | FAIL이 **진짜 결함**인지 **생성된 YAML이 틀린 것**인지 구분. → 완화: 신규 조합의 첫 FAIL은 항상 "시나리오 유효성 미확인"으로 분류하고, `lint`/`compile` 통과 + 사람 확인 후에만 결함으로 승격 |
| **병렬화** | `domain_lease.py`의 유한 pool이 상한. lease pool 크기를 넘는 fan-out은 대기하므로 안전하되 느려짐 |

### 5.4 D. 스키마·문서 드리프트 감시 (우선순위 4)

`docs/`에 SiLS 문서가 30개 이상, `.md`/`.html` 짝, 그리고 스키마가 3곳에 존재한다:

```
config/sils/trace-schema/v1.json   ← 기계 판독 source of truth
docs/sils/sils-trace-schema.md          ← 사람용 companion
evaluate.py 가 실제로 판정하는 이벤트 집합  ← 실효 구현
```

셋이 어긋나면 "문서엔 있는데 판정하지 않는 이벤트" 또는 그 역이 생긴다. 실제로 v1은 S4에서 버스
결합 이벤트를 편입했고, 문서에 그 전환 흔적이 남아 있다(스코프 노트).

| 항목 | 내용 |
|---|---|
| **검사** | ① 스키마 ↔ 문서 표 ↔ analyzer 지원 이벤트 3자 일치 ② `.md`/`.html` 짝 내용 동기 ③ `docs/sils/sils-reading-order.md` 인덱스 누락 |
| **출력** | 불일치 목록 + 제안 패치 (커밋은 사람) |
| **주기** | `config/sils/trace-schema/`·`tools/sils/conformance/` 변경 시 |

---

## 6. 위험과 안티패턴

### 6.1 가장 큰 위험: 녹색 편향 (green bias)

에이전트는 태스크 완료 압력을 받는다. 판정 기준에 손댈 수 있으면 **"실패를 고치는" 대신 "실패를
실패가 아니게 만드는"** 경로가 항상 더 싸다. 구체적으로는 tolerance 완화, 스키마 필드 required 해제,
golden 재캡처. 그래서 §2.2의 쓰기 금지 경로와 §2.3의 2층 강제가 이 설계의 **핵심 안전장치**다 —
편의가 아니다. 특히 이건 프롬프트 지침으로 막을 수 없다: 지침은 완료 압력과 같은 층위에서 경쟁하고,
권한·리뷰는 그 위에 있다.

### 6.2 golden 오염

`capture_golden.py`는 비-conformant·비-결정론 run을 **스스로 거부**한다. 하지만 "conformant하지만
의도와 다른" run은 거부하지 못한다. → **golden 캡처는 에이전트가 하지 않는다.** 제안까지만.

### 6.3 에이전트 출력을 증거로 오인

트리아지 노트는 **가설**이다. `verdict.json`은 **증거**다. 리포트에서 이 둘을 시각적으로 구분해야
한다(출처 표기). [sils-tool-confidence.md](sils-tool-confidence.md)가 안전 케이스 입력물로 쓰이는
문서인 만큼, 에이전트 산출물이 그 체인에 섞이면 문서의 주장 자체가 약해진다.

### 6.4 컨텍스트 낭비

`trace.jsonl`은 lockstep run에서 수천~수만 줄이다. 전문을 컨텍스트에 붓는 것은 비싸고 정확도도
떨어진다. **`verdict.json` → `violations[]` → 지목된 tick 구간** 순으로 좁혀 읽는다.

### 6.5 wall-clock 의존 판정

lockstep run에서 golden 비교 시 벽시계 latency는 이미 스킵 대상이다(실행에서 드러난 결함으로 수정된
항목). 에이전트가 새 검사를 제안할 때 **벽시계 기반 기준을 도입하지 않도록** 명시해야 한다.

---

## 7. 단계별 계획

| 단계 | 내용 | 현재 | 산출물 | 성공 기준 |
|---|---|---|---|---|
| **0** | 권한 방어선 (§2.3) — 어떤 에이전트가 SiLS를 만지기 전에 | ❌ `.claude/` 없음 | `.claude/settings.json` · `.gitignore` 한 줄 · `CODEOWNERS` | 판정 경로 수정이 리뷰 없이 머지되지 않는다 |
| **P0** | Jenkins에 `sils-ci.sh` 스테이지 + artifact 보존 | ❌ 없음 | `runs/<run_id>/` | 야간 1회 자동 생산 |
| **P1** | 트리아지 에이전트 (읽기 전용) | ❌ | 트리아지 노트 | 재현 명령이 실패를 재현 |
| **P2** | 리포트 합성 | ❌ | 일간 요약 | 새 회귀가 최상단에 뜬다 |
| **P3** | 시나리오·토폴로지 확장 | 토폴로지 2개 | 신규 토폴로지 | 새 결함 1건 이상 발견 |
| **P4** | 드리프트 감시 | ❌ | 불일치 목록 | 스키마 3자 일치 유지 |

P1은 **macOS에서 사전 캡처 트레이스로 개발·검증 가능**하므로 P0과 병행 가능하다
(`tools/sils/conformance/samples/*.trace.jsonl` 이 이미 있다).

### 7.1 하네스 배선

이 리포에는 아직 `.claude/` 디렉터리가 없다. 에이전트 계층을 두려면:

| 구성물 | 위치 | git | 역할 |
|---|---|---|---|
| 권한 방어선 | `.claude/settings.json` | **커밋** | §2.2의 `deny`/`ask` 목록. 1층 가드레일 |
| 개인 설정 제외 | `.gitignore` | 커밋 | `.claude/settings.local.json` 한 줄. 없으면 개인 설정이 커밋된다 |
| 리뷰 강제 | `.github/CODEOWNERS` | 커밋 | 2층. 판정 경로에 리뷰어 지정 — Bash 우회를 막는 유일한 지점 |
| `sils-triage` 에이전트 | `.claude/agents/` | 커밋 | §5.1. 읽기 전용 도구만 부여 |
| `sils-run` 스킬 | `.claude/skills/` | 커밋 | `sils-ci.sh` 호출 규약(경로 선택·artifact 위치)을 고정. P0의 부산물 |

전부 **커밋 대상**이라는 점이 요점이다. 하나라도 개발자별 파일에 두면 그 사람만 보호되고 팀 보증은
사라진다(§2.3).

작성 예 (`deny`/`ask` 분리는 §2.2):

```json
{
  "permissions": {
    "deny": [
      "Edit(./config/sils/trace-schema/**)", "Write(./config/sils/trace-schema/**)",
      "Edit(./config/sils/parity/**)",       "Write(./config/sils/parity/**)",
      "Edit(./config/sils/regression/**)",   "Write(./config/sils/regression/**)"
    ],
    "ask": [
      "Edit(./tools/sils/conformance/**)",   "Write(./tools/sils/conformance/**)"
    ]
  }
}
```

golden 디렉터리는 P0에서 `--capture-golden-dir` 위치를 확정한 뒤 `deny`에 추가한다 — 지금 적으면
추측이 된다.

### 7.2 P0에서 지금 지킬 것 (Phase 2 대비)

Phase 1의 CI를 Jenkins에 붙이는 것은 맞지만, §4.2의 Phase 2(공개·GitHub Actions)를 아는 지금
**잘못 굳히면 나중에 비싸지는 결정**들이 있다.

| 지킬 것 | 안 지키면 |
|---|---|
| `sils-ci.sh`를 **감싸지 말고 그대로 호출** | Phase 2에서 GitHub Actions로 갈아탈 때 Jenkinsfile 안의 로직을 다시 쓴다. **단일 진입점**으로 설계된 성질을 버리지 말 것 |
| builder 이미지를 **공개 가능한 형태로** | 사내 registry(`IMAGE_REGISTRY` + 자격증명) 의존을 굳히면 Phase 2에서 전면 재작업. `BUILD_SILS` 기본값도 같이 정리 |
| artifact 레이아웃을 **CI 도구 중립으로** | Jenkins 관례(`output/integration-results/**`)에 묶이면 GitHub Actions에서 재사용 불가 |
| 토폴로지·예제를 늘릴 때 **문서와 함께** | Phase 2의 진입 장벽이 여기서 결정된다. 대학원생이 30분 안에 첫 vECU를 돌릴 수 있어야 한다 |

그리고 §2.2의 두 항목이 Phase 2에서 **선택이 아니라 필수**가 된다: `config/sils/fmu/`(외부인이 임의
FMU를 올림 = in-process 네이티브 코드 실행)와 `CODEOWNERS`(외부 컨트리뷰터가 판정 기준을 만짐).

---

## 8. 상태

- **설계 제안.** 구현된 에이전트 계층은 없다(§7 "현재" 열).
- 감싸는 대상(§3 파이프라인)은 존재하며 도구 계층은 실행·테스트 완료
  ([tools/sils/README.md](../../tools/sils/README.md) 검증 상태 참고).
- **Phase 1(사내)이 현재 단계.** 실행 주체는 사내 Jenkins로 정해졌다(§4.2) — Tier A/B 분리는
  Phase 2 사항이므로 지금 구현하지 않는다.
- 선행 결정 필요: artifact 보존 기간·위치, golden 디렉터리 경로(§7.1).

---

*관련: [sils-tool-confidence.md](sils-tool-confidence.md)(판정 신뢰 근거) ·
[sils-trace-schema.md](sils-trace-schema.md)(계약면) ·
[sils-extension-model.md](sils-extension-model.md) §2(경계) ·
[tools/sils/README.md](../../tools/sils/README.md)(도구 인덱스)*
