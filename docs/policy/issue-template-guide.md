# 이슈 템플릿 작성 가이드 (Issue Template Authoring Guide)

> 대상: `obigo-gov/oss_multisw_framework`, `obigo-gov/oss_hmi_app` (및 공개 미러 `OSS-TEST-GROUP/*`)
> 목적: GitHub 이슈 폼(Issue Forms)을 **일관된 규칙**으로 추가·수정·유지보수하기 위한 안내.
> 방식: 템플릿은 **사내 develop** 에서 편집 → Publish 워크플로로 공개 저장소에 전달된다 (직접 공개 편집 금지).

---

## 1. 위치와 구성

이슈 템플릿은 각 저장소의 **`.github/ISSUE_TEMPLATE/`** 에 둔다. 현재 4개 파일:

| 파일 | 역할 | 표시 이름 |
|---|---|---|
| `config.yml` | 이슈 선택 화면 설정 (빈 이슈 차단 + Discussions/보안 링크) | — (폼 아님) |
| `bug_report.yml` | 버그 제보 폼 | 🐛 버그 제보 · Bug Report |
| `feature_request.yml` | 기능/개선 제안 폼 | ✨ 기능 제안 · Feature Request |
| `documentation.yml` | 문서 오류·개선 폼 | 📖 문서 오류 · Documentation |

> `config.yml` 은 폼이 아니라 **선택 화면 메타 설정**이다. 나머지는 `body:` 를 가진 **이슈 폼(Issue Form)** 이다.

---

## 2. 이 프로젝트의 작성 규칙 (컨벤션)

1. **한/영 병기.** 공개 저장소는 외부 기여자 대상이므로 `label`·`description`·`options` 를 `한국어 / English` 형식으로 병기한다.
   - 예: `label: 예상 동작 · Expected behavior`, `description: 무엇이 일어나야 했나요? / What did you expect to happen?`
2. **빈 이슈 차단.** `config.yml` 에 `blank_issues_enabled: false`.
3. **라벨 규칙.** 각 폼의 `labels:` 에 유형 라벨 + `needs-triage` 를 함께 단다.
   - 버그 → `["bug", "needs-triage"]`, 기능 → `["enhancement", "needs-triage"]`, 문서 → `["documentation", "needs-triage"]`
4. **제목 접두어.** `title:` 로 `[Bug] ` / `[Feature] ` / `[Docs] ` 프리픽스를 준다.
5. **컴포넌트 드롭다운 필수.** 트리아지를 위해 첫 필드로 컴포넌트를 고르게 한다 (repo별 목록은 §4).
6. **기여 유도.** 문서·기능 폼에는 "직접 PR 제출 의향" 체크박스를 둔다.
7. **YAML 안전.** `:` 나 특수문자가 들어가는 옵션·문자열은 반드시 따옴표로 감싼다. (예: `- "ota_mgr (OtaManager)"`)

---

## 3. GitHub 이슈 폼 문법 요점

`body:` 는 필드 리스트다. 자주 쓰는 타입:

| type | 용도 | 주요 attributes |
|---|---|---|
| `markdown` | 안내 문구(입력 아님) | `value` |
| `input` | 한 줄 입력 | `label`, `description`, `placeholder` |
| `textarea` | 여러 줄 입력 | `label`, `description`, `placeholder`, `render`(예: `shell` → 코드블록) |
| `dropdown` | 선택 | `label`, `options`(리스트), `multiple`(true/false) |
| `checkboxes` | 체크박스 | `label`, `options`(각 `label`, `required`) |

- 각 필드는 **고유한 `id`** 를 가진다(동일 파일 내 중복 금지).
- 필수 입력은 `validations: { required: true }`.
- `placeholder` 는 예시만 보여주고 제출값에는 포함되지 않는다.

최소 예시:
```yaml
name: ✨ 기능 제안 · Feature Request
description: 새 기능을 제안합니다. / Suggest a feature.
title: "[Feature] "
labels: ["enhancement", "needs-triage"]
body:
  - type: textarea
    id: proposal
    attributes:
      label: 제안 내용 · Proposal
      description: 원하는 동작 / Desired behavior
    validations:
      required: true
```

---

## 4. 저장소별 차이 (중요)

대부분 동일하되 **아래 3가지는 repo마다 다르게** 유지한다.

| 항목 | oss_multisw_framework (MMF) | oss_hmi_app (HMI) |
|---|---|---|
| `config.yml` 보안 URL | `…/OSS-TEST-GROUP/oss_multisw_framework/security/advisories/new` | `…/OSS-TEST-GROUP/oss_hmi_app/security/advisories/new` |
| 컴포넌트 드롭다운 | vehicle_info_mgr / ota_mgr / lifecycle_mgr / diagnostics / adsw_manager(ROS2) / adsw_manager(AGL) / ota_client / 빌드·CI / DDS·IDL / 기타 | adsw_monitor / hmi_launcher / lfc_monitor / ota_monitor / oss_launcher_bot / vehicle_data_test / 공통·인프라 / 기타 |
| 환경 필드 | 빌드 타깃(x86_64/aarch64) · 확장(ROS2/AGL) | Flutter 버전 · 실행 타깃(linux/android/web) · 보드/디바이스 |

> Discussions 링크(`…/orgs/OSS-TEST-GROUP/discussions/categories/{q-a,ideas}`)는 **조직 단위**라 두 repo 공통이다.

---

## 5. 새 템플릿 / 필드 추가 절차

1. **사내 develop** 에서 작업 브랜치 생성 (`chore/issue-templates-…`).
2. `.github/ISSUE_TEMPLATE/` 에 `.yml` 추가/수정. §2 규칙 준수.
   - 새 유형이면 새 파일(예: `question.yml`) — 단 질문류는 Discussions 로 유도하므로 신중히.
3. **YAML 검증** (제출 전 필수):
   ```bash
   python3 - <<'PY'
   import yaml, glob
   for f in glob.glob(".github/ISSUE_TEMPLATE/*.yml"):
       yaml.safe_load(open(f, encoding="utf-8")); print("OK", f)
   PY
   ```
4. 사내 PR → 리뷰 → develop 머지.
5. **공개 반영**: Publish 워크플로("Publish develop to Public")로 사내 develop → 공개 develop 전달. (이슈 폼은 공개 저장소의 **기본 브랜치**에 있어야 New issue 화면에 뜬다.)
6. **동작 확인**: 공개(또는 사내) repo → **Issues → New issue** → 템플릿이 뜨고 필드가 의도대로 나오는지 확인.

---

## 6. 흔한 함정

- **YAML 파싱 실패** → GitHub이 해당 템플릿을 숨기거나 폼 전체가 안 뜬다. `:` 포함 옵션은 따옴표 필수. 제출 전 §5-3 검증.
- **`id` 중복** → 폼 로드 실패. 파일 내 유일하게.
- **`config.yml` 링크가 사내 기준** → 사내에선 Discussions/Advisories가 없어 링크가 404일 수 있다. 링크는 **공개 저장소/조직 기준**이며, 공개 전달 후 정상 동작한다.
- **Discussions 카테고리 URL** → 해당 카테고리(Q&A, Ideas)가 공개 저장소/조직 Discussions에 실제로 존재해야 링크가 유효하다.
- **공개에서 직접 수정 금지** → 반드시 사내 develop에서 편집 후 Publish. 두 저장소가 갈라지지 않게.

---

## 7. 참고

- GitHub 공식 문서: "Syntax for issue forms" / "Configuring issue template chooser"
- 관련 정책: [open-source-operation-policy.md](./open-source-operation-policy.md) (공개 전달 흐름), [external-contribution-guide.md](./external-contribution-guide.md) (외부 기여 처리)
