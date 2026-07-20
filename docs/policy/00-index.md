# 오픈소스 공개 운영 — 최종 산출물 인덱스

> 방식: 사내 `develop` 통째 전달 (tracking 없음)
> 저장소: 사내 `obigo-gov/{oss_multisw_framework, oss_hmi_app}` → 공개 `OSS-TEST-GROUP/{동일명}`
> 기본 브랜치: 사내·공개 모두 `develop`

---

## 1. 문서 (md) — 읽고 팀에 공유

| 파일 | 용도 | 주 대상 |
|---|---|---|
| `open-source-operation-policy.md` | 전체 운영 정책 (원칙·구조·브랜치·배포). **기준 문서** | 전체 |
| `publish-guide-for-developers.md` | 개발자 일상 흐름 (develop 개발 → 담당자 Publish) | 개발자 |
| `external-contribution-guide.md` | 외부 PR을 사내로 가져와 반영 → 재공개 | 담당자 |
| `public-repo-external-contributor-permissions.md` | 외부인이 fork+PR까지만 되는 원리·설정 | 담당자 |
| `release-deployment-guide.md` | 릴리스 태그(공개에서 생성) → 사내 deploy 배포 | 담당자 |
| `bot-account-setup-guide.md` | 공개 push용 봇 계정·토큰·secret 설정 | 담당자 |
| `release-notes-template.md` | 릴리스 노트 템플릿 (다기관 기여·사사 포함) | 담당자 |

---

## 2. 워크플로 (yaml) — 사내 저장소에 배치

배치 위치는 모두 **사내 저장소**의 `.github/workflows/`이며, 파일명은 꼬리표(`-oss_hmi_app`)를 떼고 넣는다.

### obigo-gov/oss_multisw_framework
| 원본 파일 | 배치 이름 | 역할 |
|---|---|---|
| `publish-develop.yml` | `.github/workflows/publish-develop.yml` | 사내 develop → 공개, PR 자동 생성 |
| `sync-release.yml` | `.github/workflows/sync-release.yml` | 공개 릴리스 태그 → 사내 deploy |

### obigo-gov/oss_hmi_app
| 원본 파일 | 배치 이름 | 역할 |
|---|---|---|
| `publish-develop-oss_hmi_app.yml` | `.github/workflows/publish-develop.yml` | 사내 develop → 공개, PR 자동 생성 |
| `sync-release-oss_hmi_app.yml` | `.github/workflows/sync-release.yml` | 공개 릴리스 태그 → 사내 deploy |

> 두 저장소용 워크플로 차이는 `env`의 `PUBLIC_REPO`뿐이다.
> 러너 `ubuntu-latest`, 인증 `oauth2:<PAT>` + extraheader 무력화가 이미 반영되어 있다.
> 공개 develop에서 직접 머지를 허용하는 운영으로 바꿀 경우에만 `sync-from-public`(ff-only) 워크플로를 추가로 둔다. 기본 방식에서는 불필요.

---

## 3. 저장소별 필수 설정 (문서 참고해 GitHub UI에서)

각 공개/사내 저장소 쌍마다 아래를 갖춘다.

**공개 저장소 (OSS-TEST-GROUP/…)**
- default branch `develop`
- Branch protection(`develop`): PR+CODEOWNERS 필수, 직접 push·머지 차단(봇 포함), 병합은 담당자만
- 외부인 Collaborator 미추가, 조직 기본 권한 Read
- Fork PR: first-time contributor 승인 필요, `pull_request_target` 미사용
- 비치 문서: `LICENSE`(Apache 2.0), `CONTRIBUTING.md`, `SECURITY.md`, `CODE_OF_CONDUCT.md`, PR 템플릿, `CODEOWNERS`

**사내 저장소 (obigo-gov/…)**
- 봇 계정에 공개 저장소 Write 권한
- secret `PUBLIC_BOT_TOKEN` 등록
- Environment `publish-approval` + 승인자 지정
- 워크플로 3종 배치 (위 표)
- 내부 배포 파이프라인이 `deploy` 브랜치 참조

---

## 4. 전체 흐름 한 장 요약

```
개발자: 사내 develop 에서 작업 → 사내 develop 머지 (평소대로)
          │
담당자: Publish 워크플로 실행 (승인 게이트 통과)
          │  봇이 사내 develop → 공개 임시 브랜치 push + PR 생성
          ▼
공개 develop 머지 (담당자, CODEOWNERS 리뷰)
          │
          │ 릴리스 시점: 공개 저장소에서 태그 vX.Y.Z 생성
          ▼
sync-release: 공개 태그 → 사내 deploy → 내부 배포

외부 기여: 공개에 fork+PR (머지 불가)
          → 담당자가 사내 develop 로 가져와 반영 → Publish 로 재전달 → 원 PR close
```
