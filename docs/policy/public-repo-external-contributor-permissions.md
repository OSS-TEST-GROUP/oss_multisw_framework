# Public 저장소 외부 개발자 권한 가이드

> 질문: 저장소를 public으로 열면 외부 개발자가 fork + PR 제출까지만 가능한가?
> 답: **네.** 쓰기 권한이 없는 외부인은 fork·PR 제출까지만 가능하고, 직접 push·머지는 불가능하다.
> 단, "머지는 담당자만"을 확실히 하려면 권한 관리 + branch protection을 함께 설정해야 한다.
> 대상: `OSS-TEST-GROUP/oss_multisw_framework`, `OSS-TEST-GROUP/oss_hmi_app`

---

## 핵심: 가시성과 쓰기 권한은 별개

"public으로 바꾸면 아무나 수정할 수 있다"는 흔한 오해다. 그렇지 않다.

- **public(가시성)** = 누구나 코드를 **볼 수 있고 fork 할 수 있다**
- **쓰기 권한** = 명시적으로 부여받은 사람만 push·머지할 수 있다

public으로 열어도 외부인이 코드를 직접 바꾸는 일은 없다. fork해서 PR로 **제안**만 할 수 있다.

---

## GitHub 기본 동작 (설정 없이도 보장)

저장소에 **쓰기 권한이 없는 사람**(= 외부 개발자)이 public 저장소에서 할 수 있는 것:

| 동작 | 가능 여부 |
|---|---|
| 저장소 fork | ✅ 가능 |
| 자기 fork에서 개발 후 원본으로 PR 제출 | ✅ 가능 |
| 이슈 생성·코멘트 | ✅ 가능 |
| 원본 저장소에 직접 push (브랜치 생성/커밋) | ❌ 불가 |
| PR 머지 버튼 사용 | ❌ 불가 |
| 남의 PR에 리뷰 코멘트 | ✅ 가능하나 구속력 없음 |

즉 쓰기 권한이 없는 이상 머지는 원래 못 한다. 이것이 "fork + PR까지만" 상태다.

---

## 그래도 두 겹으로 잠그는 이유

기본 동작만 믿으면 안 되는 건 **권한을 가진 사람** 때문이다. 조직 멤버·팀·Collaborator 중
실수로 Write 이상을 받은 사람은 머지할 수 있다. 그래서 두 겹으로 막는다.

### 1겹: 외부인에게 권한을 주지 않기
- 외부 개발자를 저장소 **Collaborator로 추가하지 않는다** (fork + PR 모델 유지)
- 조직 기본 멤버 권한(Base permissions)을 **Read**로 둔다

### 2겹: Branch protection
- 권한 있는 내부인조차 `develop`에 직접 머지하지 못하게 한다
- PR + CODEOWNERS 리뷰 필수, 지정된 담당자만 병합, 관리자도 예외 없음

이 두 겹으로 "외부는 fork+PR까지만, 머지는 내부 담당자만"이 확실해진다.

---

## 설정 체크리스트

공개 저장소가 "fork + PR 제출까지만" 상태인지 아래 4가지로 확인한다.

- [ ] **Collaborators 확인**
  Settings → Collaborators and teams → 외부 개발자가 목록에 **없는지**
  (있다면 제거하거나 Read 권한으로 조정)

- [ ] **조직 기본 권한 확인**
  Organization → Settings → Member privileges → **Base permissions = Read** (또는 None)
  (Write이면 조직원 전체가 저장소에 쓸 수 있으니 주의)

- [ ] **Branch protection (`develop`)**
  Settings → Branches → `develop` 규칙:
  - Require a pull request before merging
  - Require approvals (1~2인) + Require review from Code Owners
  - Do not allow bypassing (관리자·봇 포함)
  - Restrict who can push/merge → 내부 담당자(팀)만
  - (선택) Require status checks to pass

- [ ] **Fork PR 보안**
  Settings → Actions → General → Fork pull request workflows →
  **Require approval for first-time contributors** 활성화
  (`pull_request_target` 트리거는 사용하지 않음 — 시크릿 노출 위험)

이 4가지가 맞으면 외부는 fork+PR까지만, 머지는 내부 담당자만으로 잠긴다.

---

## 자주 묻는 것

**Q. public으로 바꾸는 것 자체는 안전한가?**
안전하다. 가시성만 열릴 뿐 쓰기 권한은 그대로다. 외부인은 fork+PR까지만 가능하다.

**Q. 외부인이 fork한 코드는 어떻게 되나?**
fork는 외부인 자기 계정의 복사본이다. 원본과 별개이며, 그들이 원본에 반영하려면 PR을
제출하는 수밖에 없다. 그 PR을 받을지 말지는 내부 담당자가 결정한다.

**Q. 외부 PR을 받아들이는 절차는?**
공개에서 바로 머지하지 않는다. "외부기여-처리가이드"의 절차대로, 사내 develop로 가져와
반영한 뒤 Publish 워크플로로 다시 공개에 전달한다. (머지는 사내 develop 한 곳에서만)

---

## 요약

- public 전환은 **가시성만** 여는 것 — 쓰기 권한은 별개
- 외부 개발자는 기본적으로 **fork + PR 제출까지만** 가능 (직접 push·머지 불가)
- "머지는 담당자만"을 확실히 하려면 **권한 관리(외부인 Read) + branch protection**을 함께 설정
- 위 4개 체크리스트로 상태 확인
