# 릴리스 태그 및 내부 배포 가이드

> 대상: 공개 `OSS-TEST-GROUP/oss_multisw_framework`, `oss_hmi_app` / 사내 `obigo-gov/...`
> 방식: **방식 A — 릴리스 태그는 공개 저장소에서 찍는다.** (정본 = 공개 원칙 유지)
> 배포: 사내는 공개의 정식 릴리스 태그를 `deploy` 브랜치로 받아 내부 배포한다.

---

## 결정: 태그는 공개 저장소에서 찍는다 (방식 A)

develop을 이미 "공개가 정본"으로 운영하므로, 릴리스 태그도 같은 원칙으로 **공개 저장소에서**
생성한다. 그래야 외부 사용자와 내부 배포가 **완전히 같은 릴리스**를 쓰게 되고, 정본이 한 곳으로 유지된다.

```
사내 develop ──(Publish 워크플로)──▶ 공개 develop
                                         │  릴리스 시점에
                                         ▼  공개에서 태그 + GitHub Release 생성
                                    공개 태그 v1.0.0
                                         │  (sync-release 가 받음)
                                         ▼
                                    사내 deploy 브랜치 = v1.0.0  ──▶ 내부 배포
```

---

## 핵심: 태그는 "PR로 전달"되지 않는다

브랜치(코드)와 태그는 전달 방식이 다르다. 이 구분이 중요하다.

| 대상 | 전달 방식 | 이유 |
|---|---|---|
| 브랜치/코드 | 임시 브랜치 push → **PR** → 리뷰·머지 | 사람이 검토할 내용이 있음 |
| **태그** | **PR 없이** push/fetch 로 직접 이동 | 이미 확정된 커밋에 붙는 라벨, 리뷰 대상 아님 |

즉 릴리스 태그는 `publish-develop` 같은 PR 흐름을 타지 않는다.
공개에서 찍힌 태그를 사내가 `git fetch` 로 받아오는 단방향 동기화다.

---

## 릴리스 프로세스 (공개 저장소)

1. 사내 develop의 공개할 상태를 **Publish 워크플로**로 공개 develop에 반영한다.
2. 공개 develop이 릴리스할 만큼 안정되면, **공개 저장소에서 릴리스를 만든다.**
   - GitHub → 공개 저장소 → Releases → **Draft a new release**
   - Tag: `vX.Y.Z` (SemVer: 호환성 깨짐=major, 기능추가=minor, 버그수정=patch)
   - Target: `develop` (또는 릴리스 대상 커밋)
   - Release notes 작성 후 **Publish release**
3. 태그 `vX.Y.Z`가 공개 저장소에 생성된다.

> 정식 릴리스만 배포 대상이다. `v1.2.0-rc1` 같은 pre-release 태그는 내부 배포에서 제외된다
> (sync-release 워크플로가 pre-release를 자동으로 걸러낸다).

---

## 사내 수신 및 배포 (sync-release 워크플로)

`sync-release.yml`(사내 저장소 배치)이 공개의 최신 **정식** 릴리스 태그를 사내 `deploy` 브랜치로 받는다.

동작:
1. 6시간마다(또는 수동) 공개 저장소 태그 fetch
2. 최신 정식 릴리스 태그 선택 (`v[0-9]*.[0-9]*.[0-9]*`, `-` 붙은 pre-release 제외)
3. 사내 `deploy` 브랜치를 그 태그로 이동시켜 push
4. 내부 배포 파이프라인은 **`deploy` 브랜치**를 바라보게 설정 → 항상 안정 버전만 배포

> `deploy` 브랜치는 이 워크플로만 갱신한다. 사람이 직접 건드리지 않는다.

---

## 방향 정리 (헷갈리기 쉬운 부분)

이 방식에서 각 흐름의 방향:

| 흐름 | 방향 | 수단 |
|---|---|---|
| 개발 코드 공개 | 사내 develop → 공개 develop | Publish 워크플로 (PR) |
| 외부 기여 반영 | 공개 → 사내 develop → 다시 공개 | fetch + 사내 머지 + Publish |
| **릴리스 태그** | **공개에서 생성 → 사내 deploy 수신** | **sync-release (fetch, PR 아님)** |

즉 태그는 **공개가 출발점**이고 사내가 받는 쪽이다. 사내에서 태그를 찍어 공개로 보내지 않는다.
(사내에서 릴리스를 확정하려는 "방식 B"를 택하지 않은 이유: develop 정본이 공개인데 태그 정본만
사내가 되면 관리가 이원화되기 때문. 방식 A는 정본을 공개 한 곳으로 일관되게 유지한다.)

---

## 체크리스트

- [ ] 공개 저장소에서 릴리스(태그+Release) 생성 권한자·절차 정하기
- [ ] SemVer 규칙 합의 (vX.Y.Z)
- [ ] pre-release(`-rc` 등) 태그는 내부 배포 제외됨을 인지
- [ ] 사내 저장소에 `sync-release.yml` 배치 (framework/hmi_app 각각)
- [ ] 사내 `deploy` 브랜치를 sync-release 워크플로만 갱신하도록 운영
- [ ] 내부 배포 파이프라인이 `deploy` 브랜치를 참조하도록 설정
- [ ] 첫 릴리스(`v1.0.0`) 생성 후 sync-release가 deploy를 갱신하는지 검증

---

## oss_hmi_app 적용

동일하다. `sync-release.yml`의 `PUBLIC_REPO`만 `oss_hmi_app`으로 설정된 버전을 사내
`obigo-gov/oss_hmi_app`에 배치하고, 공개 `OSS-TEST-GROUP/oss_hmi_app`에서 태그를 찍으면 된다.
