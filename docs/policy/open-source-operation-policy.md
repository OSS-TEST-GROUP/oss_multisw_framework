# 오픈소스 개발자 포털 운영 정책

> 연구개발 과제 산출물의 오픈소스 공개 및 사내–공개 저장소 연계 운영 정책
> 최종 수정일: 2026-07-18

## 저장소 정보

| 구분 | 값 |
|---|---|
| 사내(비공개) 원본 저장소 | https://github.com/obigo-gov/oss_multisw_framework |
| 사내 원본 기본 브랜치 | `develop` |
| 공개(오픈) 조직 | https://github.com/OSS-TEST-GROUP |
| 공개 배포 저장소(정본) | https://github.com/OSS-TEST-GROUP/oss_multisw_framework |
| 공개 저장소 기본 브랜치(정본) | `develop` |

> 배포 방향: **사내 `obigo-gov/oss_multisw_framework`(origin) → 공개 `OSS-TEST-GROUP/oss_multisw_framework`(public)**
> 본 문서의 명령·예시에서 `origin`은 사내(obigo-gov), `public`은 공개(OSS-TEST-GROUP) 조직을 가리킨다.
> 공개 저장소명은 사내 원본과 동일하게 `oss_multisw_framework`로 가정했으며, 다르게 쓸 경우 URL만 교체한다.

---

## 1. 목적

연구개발 과제에서 개발한 산출물을 공개(Public) GitHub 저장소로 오픈하여 개발자 포털 및 오픈소스 생태계를 운영한다. 사내(Private) 저장소에서 개발한 기능을 공개 저장소로 전달하고, 외부 개발자의 기여를 선별 수용하며, 그 결과를 다시 사내로 반영하는 전체 흐름의 운영 규칙을 정의한다.

---

## 2. 핵심 원칙

1. **머지는 사내 `develop` 한 곳에서만 한다.**
   개발 머지는 사내 `develop`에서 이뤄지고, 외부 기여도 사내 `develop`을 거쳐 반영된다. 공개 저장소에는 사내 `develop`을 전달만 한다. 두 곳에서 각각 머지해 히스토리가 갈라지는 것을 원천 차단한다.

2. **공개 저장소는 사내 `develop`의 공개 미러다.**
   사내 `develop`을 정기적으로 공개 `develop`에 전달한다. 공개 `develop`은 항상 사내 `develop`의 반영본 상태를 유지한다.

3. **공개 저장소 `develop`에는 누구도 직접 머지하지 않는다.**
   외부 개발자·관리자·봇 모두 공개 `develop` 직접 push·머지 불가. 전달은 임시 브랜치 + PR을 거쳐 담당자가 수행한다.

4. **외부 기여는 사내로 가져와 반영 후 다시 공개로 전달한다.**
   공개 PR을 개별 cherry-pick 하지 않고, 사내 `develop`에 반영한 뒤 Publish로 재전달한다.

5. **내부 배포는 릴리스 태그 기준의 안정 버전만 사용한다.**
   릴리스 태그는 공개 저장소에서 생성하고, 사내는 그 태그를 `deploy` 브랜치로 받아 배포한다.

---

## 3. 저장소 구조

```
[사내 저장소]  obigo-gov/oss_multisw_framework   ← 개발 trunk
   develop ──(Publish 워크플로)──▶ [공개 저장소] OSS-TEST-GROUP/oss_multisw_framework
                                        develop  ← 사내 develop의 공개 미러(정본 공개면)
                                           │  릴리스 시점에 태그 생성
                                           ▼
                                     태그 v1.0.0
                                           │ (sync-release 로 fetch)
                                           ▼
[사내]  deploy 브랜치 = v1.0.0  ──▶ 내부 배포
```

- **사내 전용 코드는 없음** → 저장소 분리 불필요, 전체 공개 가능
- 사내 저장소와 공개 저장소는 fork 관계가 아니며, `git remote` 연결로 코드·태그를 주고받는다.
- 별도 `tracking` 브랜치를 두지 않는다. 사내 `develop`이 곧 개발 기준선이자 공개 전달 원본이다.

---

## 4. 브랜치 전략

### 사내 저장소 (obigo-gov)
| 브랜치 | 용도 | 규칙 |
|---|---|---|
| `develop` | 개발 trunk (기본 브랜치). 작업 브랜치가 여기로 머지 | 팀 현행 방식 유지 |
| `feature/*` | 기능 개발 | `develop`에서 분기, 사내 PR로 `develop`에 머지 |
| `deploy` | 릴리스 태그 기준 안정 버전 | sync-release 워크플로만 갱신, 내부 배포가 참조 |

### 공개 저장소 (OSS-TEST-GROUP)
| 브랜치 | 용도 | 보호 |
|---|---|---|
| `develop` | 공개 정본 (기본 브랜치). 사내 develop의 미러 | 직접 push·머지 금지, PR+CODEOWNERS 리뷰 필수 |
| `publish/*` | Publish 워크플로가 만드는 전달용 임시 브랜치 | PR 후 정리 |
| 릴리스 태그 `vX.Y.Z` | 안정 버전 배포 기준 (SemVer) | 공개에서 생성 |

---

## 5. 사내 develop → Public 전달 (Publish 워크플로)

팀은 사내 `develop`에서 평소대로 개발한다(작업 브랜치 → 사내 PR → 사내 `develop` 머지). 공개는 별도 단계로, 담당자가 **Publish 워크플로**를 실행해 사내 `develop`을 공개 저장소로 전달한다.

### 동작

담당자가 사내 저장소 Actions에서 "Publish develop to Public"을 실행하면:

1. 내부 CI(build/test/security-scan) 실행
2. `publish-approval` 게이트에서 승인 대기 → 승인자 승인 (보안심의·기관 공개승인)
3. 봇이 사내 `develop`을 공개 저장소의 임시 브랜치 `publish/YYYYMMDD-<sha>`로 push
4. 공개 저장소에 `publish/... → develop` PR 자동 생성
5. CODEOWNERS 리뷰 후 담당자가 공개 `develop`로 수동 머지

공개 `develop`에는 임시 브랜치를 통한 PR로만 반영되며, 직접 push는 하지 않는다(봇 포함). 상세 워크플로는 `publish-develop.yml` 참조.

### 운영 규칙
- **push 전용 봇/서비스 계정**을 1개 만들어 공개 push를 일원화한다. 개인 계정 금지. → 이력 추적·권한 회수 용이.
- **봇은 공개 `develop`에 직접 push·머지할 수 없다.** branch protection이 봇에도 적용되어, 봇은 임시 브랜치 전달과 PR 생성까지만. 머지는 담당자가 한다.
- **공개 승인 게이트는 push 직전에 배치한다.** 보안심의·기관 공개승인을 통과해야만 공개 push가 실행된다.
- **PR 머지는 자동화하지 않는다.** (자동 머지 금지)
- 봇 인증은 `oauth2:<PAT>` HTTPS 형식을 쓰고, `actions/checkout`이 심는 extraheader를 해당 git 명령에서 무력화한다. (실전 이슈로 확인된 사항)

---

## 6. 외부 기여 처리 (Public)

- 외부 개발자는 표준 fork & pull 모델로 공개 저장소에 PR을 올린다. (public 저장소에서 외부인은 fork+PR까지만 가능, 직접 push·머지 불가)
- **공개 `develop`에는 외부 PR을 직접 머지하지 않는다.** 채택할 기여는 사내 `develop`으로 가져와 반영한 뒤 Publish로 재전달한다. (상세: `external-contribution-guide.md`)
  - `git fetch public pull/<PR번호>/head:review/pr-<번호>`로 사내에 가져옴
  - 검토 후 사내 `develop`에 머지(squash 권장) → Publish 워크플로 실행
  - 원래 외부 PR은 반영 완료로 close
- 이렇게 머지를 사내 `develop` 한 곳에 모아 두 저장소가 갈라지지 않게 한다.

### Branch protection (공개 `develop`)
- `develop` 직접 push·머지 금지, PR + CODEOWNERS 리뷰 필수, status check 통과 필수
- 관리자·봇에게도 규칙 적용, 병합 권한은 내부 담당자만
- 외부인은 Collaborator로 추가하지 않고 조직 기본 권한 Read (상세: `public-repo-external-contributor-permissions.md`)

### Fork PR CI 보안
- `pull_request` 트리거만 사용. **`pull_request_target` 금지**(secrets 노출 위험)
- "require approval for first-time contributors" 활성화

---

## 7. Public → 사내 동기화

develop 통째 방식에서는 **머지가 사내 `develop` 한 곳에서만** 일어나고 공개는 그 미러이므로, 정기적인 develop 역방향 동기화는 원칙적으로 불필요하다(사내가 항상 최신). 외부 기여도 §6처럼 사내를 거쳐 반영한다.

- 다만 운영상 공개 `develop`에서 직접 머지가 발생하는 경우를 대비해 `sync-from-public.yml`(ff-only)을 보조로 둘 수 있다. 이때 사내 `develop`에 아직 공개하지 않은 커밋이 있으면 ff가 실패하므로, "사내 변경은 항상 먼저 공개로 보내고 공개 머지 후 되받는" 순서를 지켜야 한다.
- **릴리스 태그**는 공개에서 생성되어 사내 `deploy`로 수신된다(§8). 이것이 실질적인 Public → 사내 방향 동기화다.

---

## 8. 릴리스 및 내부 배포

- **SemVer** 준수: 호환성 깨짐=major, 기능추가=minor, 버그수정=patch (`vX.Y.Z`)
- **릴리스 태그는 공개 저장소에서 생성한다.** 공개 `develop`이 안정되면 공개 저장소에서 태그 + GitHub Release 작성 (정본=공개 원칙 유지, 외부 사용자와 내부 배포가 같은 릴리스 사용). 상세: `release-deployment-guide.md`
- 사내는 최신 **릴리스 태그**를 `deploy` 브랜치로 받아 내부 배포한다. (develop 추종 아님 → 배포 안정성 확보)
- **태그는 PR로 전달되지 않는다.** 확정된 커밋의 라벨이므로 `sync-release` 워크플로가 fetch로 직접 수신한다.

```yaml
# 사내 저장소 .github/workflows/sync-release.yml (요지)
- run: |
    git fetch public --tags
    # 정식 릴리스만 선택 (v1.2.0-rc1 같은 pre-release 제외)
    LATEST=$(git tag -l 'v[0-9]*.[0-9]*.[0-9]*' --sort=-v:refname | grep -v -- '-' | head -1)
    echo "최신 릴리스: $LATEST"
    git branch -f deploy "$LATEST"
    git push origin deploy --force-with-lease
```

- 초기에는 `develop` + 태그만으로 충분. 여러 버전을 동시 유지보수하게 되면 `release/1.x` 브랜치 도입 검토.

---

## 9. 초기 공개 절차 (Public 저장소 최초 개설)

사내 전용 코드는 없더라도, **커밋 히스토리에 남은 내부 흔적**(사내 URL, 테스트 credential, 내부 이메일 등)은 공개 전 반드시 점검한다. 첫 공개는 히스토리를 접고 새로 시작하는 방식을 권장한다.

```bash
# 사내 원본(obigo-gov)을 깨끗한 위치에 클론해 작업 (origin = 사내)
git clone https://github.com/obigo-gov/oss_multisw_framework.git oss-publish && cd oss-publish
# public = 공개 조직(OSS-TEST-GROUP)
git remote add public https://github.com/OSS-TEST-GROUP/oss_multisw_framework.git

# 공개할 소스 상태에서 히스토리를 접어 새 시작 (사내 develop 기준)
git checkout develop
git checkout --orphan public-develop
git add -A
git commit -m "Initial public release"

# 공개 조직의 기본 브랜치 develop 으로 push
git push public public-develop:develop
```

push 후 GitHub에서 `OSS-TEST-GROUP/oss_multisw_framework`의 default branch가 `develop`으로 지정돼 있는지 확인한다.
히스토리 보존이 꼭 필요하면 `git filter-repo`로 민감 정보만 제거한다.

### 최초 공개 후

develop 통째 방식에서는 별도 `tracking` 브랜치나 재기준화가 필요 없다. 사내 `develop`이 곧 개발 trunk이자 공개 전달 원본이므로, 최초 공개 push로 공개 `develop`을 만든 뒤에는 이후 Publish 워크플로로 계속 전달하면 된다.

- 최초 공개 시 orphan으로 히스토리를 접었다면, 그 시점의 사내 `develop`과 공개 `develop`은 내용이 동일하다. 이후 사내 `develop`에 쌓인 변경을 Publish로 전달한다.
- (참고) 최초 공개를 orphan으로 하면 공개 히스토리가 사내와 SHA가 달라진다. Publish 워크플로는 PR 방식으로 전달하므로 이 차이는 문제되지 않는다. 다만 사내 `develop`을 공개 히스토리에 맞춰 시작하고 싶다면 최초 1회 정렬이 필요할 수 있다 — 팀 상황에 맞게 판단한다.

---

## 10. 라이선스 및 기여자 관리

- **오픈 라이선스를 채택한다.** 산출물을 오픈소스 라이선스로 공개하여 외부 개발자의 자유로운 사용·수정·재배포·기여를 허용한다.
- **권장 라이선스: Apache License 2.0.** 연구개발 과제 산출물에는 특허 실시권(patent grant)을 명시적으로 포함하는 Apache 2.0이 적합하다. 기여자·이용자 간 특허 분쟁 소지를 줄이고, 상업적 활용까지 폭넓게 허용한다. (단순·간결함을 우선하면 MIT, 파생물의 개방 강제가 필요하면 GPL 계열 등 대안도 검토 가능)
- **과제 협약 정합성 확인.** 최종 라이선스 확정 전, 연구개발(특히 국가 R&D) 과제 협약상 소유권·공개 조건과 선택 라이선스가 상충하지 않는지 검토한다.
- **CLA 또는 DCO** 봇을 연동하여 기여물의 IP 귀속을 명확히 한다. *(결정 필요: CLA는 기여자 서명·관리 체계가 필요하지만 권리 귀속이 명확하고, DCO는 커밋 sign-off만으로 가벼움. 운영 부담과 IP 요구 수준을 비교해 확정할 것.)*
- `LICENSE`(채택 라이선스 전문), `CONTRIBUTING.md`, PR 템플릿을 저장소에 비치한다. 필요 시 소스 파일 헤더에 라이선스 고지를 삽입한다.
- **`SECURITY.md`를 비치한다.** 보안 취약점의 **비공개 신고 채널**(GitHub Private Vulnerability Reporting 또는 전용 이메일)을 안내한다. 취약점이 공개 이슈로 올라와 패치 전에 노출되는 사고를 방지한다.
- **`CODE_OF_CONDUCT.md`를 비치한다.** 외부 개발자 커뮤니티를 운영하므로 행동 규범과 위반 시 조치 기준을 명시한다. (Contributor Covenant 등 표준 템플릿 활용 가능)

---

## 11. 준수 규칙 요약 (Do / Don't)

**반드시 지킬 것**
- 머지는 사내 `develop` 한 곳에서만 한다.
- 공개는 담당자가 Publish 워크플로로 실행한다. (사내 develop → 공개, PR 방식)
- 공개 승인 게이트(보안심의·기관 승인)를 통과해야만 공개 push가 실행된다.
- 외부 기여는 사내 `develop`으로 가져와 반영 후 다시 공개로 전달한다.
- 공개 push는 전용 봇/서비스 계정으로 일원화한다.
- 릴리스 태그는 공개 저장소에서 생성한다.

**하지 말 것**
- 공개 `develop`에 직접 push·머지 금지 (외부인·관리자·봇 모두).
- 외부 PR을 공개 develop에 바로 머지하거나 개별 cherry-pick 금지.
- 개인 계정으로 공개 push 금지.
- `pull_request_target` 사용 금지.
- 자동 머지 금지(머지는 담당자 판단).

---

## 12. 초기 구축 체크리스트

- [ ] 오픈 라이선스 확정 (Apache 2.0 권장, 과제 협약 정합성 확인) 및 `LICENSE` 비치
- [ ] CLA vs DCO 결정 및 봇 연동
- [ ] 공개 저장소(`OSS-TEST-GROUP/oss_multisw_framework`) 생성 및 히스토리 정리 후 최초 공개 push
- [ ] 공개 default branch가 `develop`으로 지정되어 있는지 확인
- [ ] 배포 방향 확인: 사내 `obigo-gov`(origin) → 공개 `OSS-TEST-GROUP`(public)
- [ ] Branch protection(공개 `develop`) + CODEOWNERS 설정 (봇 포함 직접 push·머지 차단)
- [ ] 외부인 권한 제한 (Collaborator 미추가, 조직 기본 권한 Read)
- [ ] `CONTRIBUTING.md` / PR 템플릿 / `SECURITY.md` / `CODE_OF_CONDUCT.md` 비치
- [ ] 공개 push 전용 봇/서비스 계정 발급 + 토큰(secret `PUBLIC_BOT_TOKEN`) 등록
- [ ] 공개 승인 게이트(`publish-approval` Environment + 승인자) 설정
- [ ] 사내에 `publish-develop.yml` 배치 (사내 develop → 공개, PR 자동 생성)
- [ ] 사내에 `sync-release.yml` 배치 (공개 릴리스 태그 → 사내 deploy)
- [ ] 내부 배포 파이프라인이 `deploy` 브랜치를 참조하도록 변경
- [ ] (선택) `sync-from-public.yml` 배치 — 공개 develop 직접 머지 운영 시에만
- [ ] oss_hmi_app도 동일 항목 반복 (저장소별 봇 권한·secret·Environment·워크플로)
