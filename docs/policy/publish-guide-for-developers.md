# 오픈소스 공개 절차 (팀원용)

> 대상 저장소: 사내 `obigo-gov/oss_multisw_framework` → 공개 `OSS-TEST-GROUP/oss_multisw_framework`
> 방식: 사내 `develop` 통째 전달 (별도 tracking 브랜치 없음)
> 사내/공개 기본 브랜치: 둘 다 `develop`

---

## 한눈에 보기

```
[개발자] 사내 develop 에서 작업 브랜치 → 개발 → 사내 develop 로 머지   ← 지금 하던 그대로
                          │
                          │ (공개 시점) 담당자가 Publish 워크플로 실행
                          ▼
[봇] 사내 develop 을 공개 저장소 임시 브랜치로 push → PR 자동 생성
                          │
                          ▼
[공개] 리뷰 후 공개 develop 로 머지
```

**개발자는 평소 개발 방식을 전혀 바꾸지 않습니다.** 공개는 담당자가 별도로 실행하는 단계예요.

---

## 1. 개발 (지금과 동일)

사내 `develop`에서 작업 브랜치를 만들어 개발하고, 사내 `develop`으로 머지한다.

```bash
git checkout develop
git pull
git checkout -b feature/my-work    # 작업 브랜치
# ... 개발, 커밋 ...
git push origin feature/my-work
```

그리고 **사내 저장소 안에서** `feature/my-work → develop` PR을 만들어 팀 리뷰 후 머지한다. (지금 방식 그대로)

> 여기까지는 전부 사내에만 있다. 공개 저장소로는 아무것도 나가지 않는다.

## 2. 공개 (담당자가 실행)

사내 `develop`에 공개할 내용이 모이면, 담당자가 공개를 실행한다.

- 사내 저장소 → **Actions** 탭 → **"Publish develop to Public"** → **Run workflow**

그러면 자동으로:
1. 내부 CI(build/test/security-scan) 실행
2. `publish-approval`에서 승인 대기 → 승인자 승인 (보안심의·기관 공개승인)
3. 봇이 사내 `develop`을 공개 저장소의 임시 브랜치 `publish/YYYYMMDD-<sha>`로 push
4. 공개 저장소에 `publish/... → develop` **PR 자동 생성**
5. CODEOWNERS 리뷰 후 담당자가 공개 `develop`로 **수동 머지**

**개발자가 공개 저장소에 직접 PR을 만들 필요가 없다.** 담당자가 Publish 워크플로만 돌리면 봇이 브랜치와 PR을 만들어준다.

---

## 자주 묻는 것

**Q. 사내 develop에서 작업 브랜치 → develop 머지, 그대로 해도 되나?**
네. 그게 기준입니다. 별도 `tracking` 브랜치는 쓰지 않습니다. 사내 `develop`이 곧 공개의 원본이에요.

**Q. 내 작업 브랜치를 공개 저장소로 바로 PR할 수 있나?**
아니요. 사내와 공개는 fork 관계가 아니라 저장소를 넘나드는 PR이 만들어지지 않습니다. 반드시 Publish 워크플로를 거쳐야 하고, 그러면 봇이 공개 저장소 안에서 PR을 만들어줍니다.

**Q. 사내에서 PR해도 되나?**
됩니다. `feature/... → develop` **사내 PR은 팀 리뷰용으로 자유롭게** 쓰세요. 공개와 무관하게 평소대로 하시면 됩니다.

**Q. 공개는 얼마나 자주 하나?**
정해진 주기(예: 스프린트 종료 시)나 릴리스 시점에 담당자가 실행합니다. 매 머지마다 공개할 필요는 없습니다.

---

## 담당자용: 외부 기여를 받는 경우

공개 저장소에 외부 개발자 PR이 올라오면, **사내 develop과 공개 develop이 갈라지지 않도록** 아래 순서를 지킨다.

1. 공개 PR을 리뷰한다. 채택할 것만 진행.
2. 채택 시, 그 변경을 **사내 develop로 먼저 반영**한다.
   (외부 PR 브랜치를 사내로 가져와 사내 develop에 머지하거나, 동일 내용을 사내에서 재작성)
3. 다시 **Publish 워크플로를 실행**해 사내 develop을 공개로 내보낸다.
4. 공개의 원래 외부 PR은 반영 완료로 정리(close)한다.

> 핵심 원칙: **머지는 사내 develop 한 곳에서만.** 공개 develop은 항상 사내 develop의 공개 미러 상태를 유지한다. 이렇게 하면 양쪽이 갈라져 생기는 충돌을 원천 차단한다.

> 만약 운영상 "공개 develop에서 직접 머지"를 택한다면, 공개 develop이 사내보다 앞설 수 있으므로 `sync-from-public` 워크플로로 사내 develop을 fast-forward 수신해야 한다. 단, 이 경우 사내 develop에 아직 공개하지 않은 커밋이 있으면 ff가 실패하므로, "사내 변경은 항상 먼저 공개로 보내고, 공개 머지 후 되받는" 순서를 지켜야 한다. 관리가 더 까다로우므로 위 1~4 방식을 권장한다.

---

## 요약

| 역할 | 하는 일 |
|---|---|
| 개발자 | 사내 develop에서 작업 브랜치 → 개발 → 사내 develop 머지 (평소대로) |
| 담당자 | 공개 시점에 Publish 워크플로 실행, 공개 PR 승인·머지 |
| 봇 | 사내 develop을 공개로 push하고 PR 자동 생성 |
