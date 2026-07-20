# 외부 기여 처리 가이드 (담당자용)

> 대상: 공개 저장소 `OSS-TEST-GROUP/oss_multisw_framework`, `OSS-TEST-GROUP/oss_hmi_app`
> 방식: 사내 `develop` 통째 전달. **머지는 사내 develop 한 곳에서만** 일어난다.
> 원칙: 공개 develop은 사내 develop의 미러. 외부 기여도 사내를 거쳐 다시 공개로 나간다.

---

## 왜 공개에서 직접 머지를 막아야 하나

공개 저장소와 사내 저장소 양쪽에서 각각 `develop`에 머지하면 두 히스토리가 갈라진다.
갈라지면 이후 동기화가 충돌하고(“머지 지옥”), 어느 쪽이 정본인지 모호해진다.

이를 막는 규칙은 하나다: **머지는 사내 develop 에서만 한다.** 공개 저장소의 `develop`에는
아무도(외부 개발자도, 관리자도, 봇도) 직접 머지하지 않는다. 외부 기여는 "사내로 가져와 →
사내 develop에 반영 → 다시 공개로 전달" 경로로만 반영된다.

---

## 1. 공개 저장소에서 외부 머지 차단 설정

외부 개발자는 공개 저장소에서 **fork 후 PR 제출까지만** 가능하고, 머지는 불가능해야 한다.
`OSS-TEST-GROUP/oss_multisw_framework` (및 `oss_hmi_app`) → Settings에서:

### Branch protection (`develop`)
Settings → Branches → Add branch ruleset (또는 Add rule) → 대상 `develop`
- ✅ **Require a pull request before merging** (PR 없이 직접 push 금지)
- ✅ **Require approvals** (최소 1~2인)
- ✅ **Require review from Code Owners** (CODEOWNERS 지정자 리뷰 필수)
- ✅ **Do not allow bypassing the above settings** (관리자·봇도 예외 없음)
- ✅ **Restrict who can push / merge** → 병합 권한을 **내부 담당자(또는 팀)만** 허용
- ✅ (선택) **Require status checks to pass** (CI 통과 필수)

### 외부 개발자 권한
- 외부 기여자는 저장소 Collaborator로 추가하지 않는다. (fork + PR 모델)
- 조직 기본 멤버 권한을 **Read**로 두어, 외부인이 실수로 머지 버튼을 쓸 수 없게 한다.

### Fork PR 보안
- Settings → Actions → General → Fork pull request workflows →
  **Require approval for first-time contributors** 활성화
- `pull_request_target` 트리거는 사용하지 않는다. (시크릿 노출 위험)

이 설정으로 외부 PR은 "제출 + 리뷰 대상"까지만 되고, 실제 머지는 내부 담당자만,
그것도 정책상 공개 develop에는 하지 않는다.

---

## 2. 외부 개발자의 작업 브랜치를 사내로 가져오기

외부 개발자가 공개 저장소를 fork하여 자기 계정 브랜치에서 개발하고, 공개 저장소로 PR을 올렸다고 하자.
(예: `외부계정:feature/their-work` → `OSS-TEST-GROUP/oss_multisw_framework:develop`)

이 변경을 **사내로 가져와** 검토·반영한다. 사내 저장소 클론에서:

```bash
# 사내 저장소 클론에서 (origin = 사내 obigo-gov)
# 공개 저장소를 remote 로 추가 (봇 토큰 or 개인 권한)
git remote add public https://github.com/OSS-TEST-GROUP/oss_multisw_framework.git

# 방법 A) PR 번호로 바로 가져오기 (가장 간단). 예: PR #42
git fetch public pull/42/head:review/pr-42
git checkout review/pr-42
# → review/pr-42 브랜치에 외부 기여 내용이 담긴다. 여기서 빌드·테스트·검토.

# 방법 B) 특정 브랜치명을 알 때
# git fetch public feature/their-work:review/their-work
```

> `pull/<번호>/head`는 그 PR의 최신 커밋을 가리키는 GitHub 표준 참조다.
> fork한 외부 계정에 직접 접근할 필요 없이 공개 저장소를 통해 가져올 수 있다.

## 3. 사내 develop에 반영

검토가 끝나 채택하기로 하면, 사내 `develop`에 병합한다.

```bash
git checkout develop
git pull origin develop            # 사내 develop 최신화

# 채택 방식 택1:
# (권장) squash 로 한 커밋으로 정리해 반영
git merge --squash review/pr-42
git commit -m "외부 기여 반영: <설명> (원 PR: OSS-TEST-GROUP#42)"

# 또는 일반 머지
# git merge review/pr-42

git push origin develop            # 사내 develop 갱신
```

> 기여자 표기(attribution)를 남기려면 일반 머지를 쓰거나, squash 커밋 메시지에
> 원 작성자와 원 PR 링크를 적는다. 라이선스/CLA 정책에 맞춰 처리한다.

## 4. 공개 저장소로 다시 전달

사내 `develop`에 반영됐으니, Publish 워크플로로 공개에 내보낸다.

- 사내 저장소 → Actions → **"Publish develop to Public"** → Run workflow

봇이 사내 develop을 공개 저장소 임시 브랜치로 push하고 `→ develop` PR을 만든다.
CODEOWNERS 리뷰 후 담당자가 공개 develop로 머지하면, 외부 기여가 정식으로 공개 develop에 들어간다.

## 5. 원래 외부 PR 정리

외부 기여가 위 경로로 공개 develop에 반영됐으므로, 공개 저장소의 **원래 외부 PR을 닫는다(close).**
닫을 때 기여자에게 감사와 함께 반영 사실을 남긴다.

예시 코멘트:
> Thanks for the contribution! This has been merged into `develop` via our internal
> integration process (see <머지 커밋/PR 링크>). Closing this PR as the changes are now included.

---

## 전체 흐름 요약

```
[외부 개발자] fork → 개발 → 공개 저장소로 PR 제출 (머지는 불가)
                                   │
[담당자] 리뷰 → 채택 결정
                                   │
        git fetch public pull/<PR번호>/head  (사내로 가져오기)
                                   │
        사내 develop 에 머지 → git push origin develop
                                   │
        Publish 워크플로 실행 → 봇이 공개로 전달 → 공개 develop 머지
                                   │
        원래 외부 PR close (반영 완료 안내)
```

핵심: **외부 개발자는 공개에서 "제안"만 하고, 실제 머지는 사내 develop 한 곳에서만.**
공개 develop은 항상 사내 develop의 공개 미러로 유지되어 두 저장소가 갈라지지 않는다.

---

## oss_hmi_app 적용

동일하다. 위 명령·설정에서 저장소 이름만 `oss_hmi_app`으로 바꾼다.
- 공개 remote: `https://github.com/OSS-TEST-GROUP/oss_hmi_app.git`
- Publish 워크플로: 사내 `obigo-gov/oss_hmi_app`의 "Publish develop to Public"
- Branch protection·Fork PR 보안 설정도 `oss_hmi_app` 공개 저장소에 동일하게 적용
