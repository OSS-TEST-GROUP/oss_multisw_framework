# 태깅 룰 가이드 (Git 태그 · 컨테이너 레지스트리 태그)

> **대상**: 공동 개발 기관 · 내부 개발자 · 릴리스 담당자
> **범위**: ① 공개 저장소(GitHub)에서 찍는 **Git 릴리스 태그**, ② 사내 컨테이너
> 레지스트리(`registry.obigo.com`)의 **이미지 태그**.
> **원칙**: 두 태그는 **다른 체계**다. 혼동하지 않도록 §3 대비표를 먼저 본다.
>
> 관련 문서: [release-deployment-guide.md](./release-deployment-guide.md) (태그→배포 흐름),
> [open-source-operation-policy.md](./open-source-operation-policy.md) (공개 운영 정책).

---

## 0. 한눈에 · Quick start

| 무엇을 | 어디서 | 형식 | 예 |
|---|---|---|---|
| 릴리스 **Git 태그** | **공개** 저장소 (GitHub Releases) | `vMAJOR.MINOR.PATCH` | `v0.0.1`, `v1.2.0` |
| 사전 릴리스 Git 태그 | 공개 저장소 (pre-release 체크) | `vX.Y.Z-rcN` | `v1.2.0-rc1` |
| 컨테이너 **이미지 태그** | 사내 레지스트리 | `<의미있는 버전>` + `latest` | `3.32.12`, `latest` |
| 변종(variant) 이미지 태그 | 사내 레지스트리 | `<변종명>` | `touch`, `autoware` |

핵심 3원칙:
1. **Git 릴리스 태그는 공개 저장소에서만 찍는다** (사내는 동기화로 받는다).
2. **태그는 불변(immutable)** — 한번 배포된 `vX.Y.Z` 는 절대 옮기지 않는다.
3. **컨테이너 `latest` 는 이동식 포인터** — 재현성이 필요한 곳엔 고정 태그를 쓴다.

---

## 1. Git 릴리스 태그 (공개 저장소)

### 1-1. 어디서 찍는가 — 공개 저장소
정본이 공개 저장소이므로 **릴리스 태그도 공개에서 찍는다.** 사내는 태그를 만들지 않고,
`sync-release` 워크플로가 공개 태그를 받아 사내 `deploy` 브랜치로 반영한다.

```
사내 develop ──(publish PR)──▶ 공개 develop
                                   │  공개에서 태그 vX.Y.Z + Release 생성
                                   ▼
                              공개 태그 vX.Y.Z
                                   │  (sync-release 가 fetch)
                                   ▼
                          사내 deploy 브랜치 = vX.Y.Z ──▶ 내부 배포
```

> **태그는 PR 로 전달되지 않는다.** 브랜치는 PR·리뷰를 타지만, 태그는 이미 확정된 커밋에
> 붙는 라벨이라 `push`/`fetch` 로 직접 이동한다.

### 1-2. 버전 형식 — SemVer
`vMAJOR.MINOR.PATCH` (앞에 `v` 를 붙인다)

| 자리 | 올리는 기준 | 예 |
|---|---|---|
| MAJOR | 호환성이 깨지는 변경 (API·IDL·설정 비호환) | `v1.0.0` → `v2.0.0` |
| MINOR | 하위호환 기능 추가 | `v1.0.0` → `v1.1.0` |
| PATCH | 하위호환 버그 수정 | `v1.0.0` → `v1.0.1` |

- **초기 개발 단계는 `0.y.z`** — `v0.0.1` 처럼 시작하고, 공개 API 가 안정되면 `v1.0.0`.
- **사전 릴리스**: `v1.2.0-rc1`, `v1.2.0-beta1` — GitHub Release 에서 **pre-release 체크**.
  정식 릴리스만 내부 배포 대상이며, `sync-release` 가 pre-release 를 자동으로 걸러낸다.

### 1-3. 두 저장소(MMF · HMI) 버전 정책
`oss_multisw_framework` 와 `oss_hmi_app` 은 **각각 독립적으로 버전을 매긴다**
(저장소가 분리되어 있고 릴리스 주기가 다르므로). 다만 **함께 검증한 조합**은
Release 노트에 상호 참조로 명시한다.

```
예) oss_hmi_app v0.2.0 Release 노트
    - 검증 조합 / Verified with: oss_multisw_framework v0.1.3
```

### 1-4. 태그 생성 절차 (담당자)

**권장 — GitHub UI**
1. 공개 저장소 → **Releases → Draft a new release**
2. **Choose a tag** → 새 태그 `vX.Y.Z` 입력 → Target `develop`
3. Release 제목·노트 작성 ([release-notes-template.md](./release-notes-template.md) 사용)
4. 정식이면 **Set as the latest release**, 사전 릴리스면 **pre-release** 체크
5. **Publish release**

**CLI (동등)**
```bash
# annotated 태그 생성 (권장 — 작성자·날짜·메시지가 남는다)
git tag -a v0.0.1 -m "v0.0.1 — 최초 공개 릴리스"
git push public v0.0.1              # 공개 저장소로 태그 push

# GitHub Release 까지 한 번에 (gh 인증 필요)
gh release create v0.0.1 -R OSS-TEST-GROUP/oss_multisw_framework \
  --target develop --title "v0.0.1" --notes-file RELEASE_NOTES.md --latest
```

> **annotated 태그(`-a`)를 쓴다.** lightweight 태그는 메타데이터가 없어 릴리스 추적에 불리하다.

### 1-5. 금지 · 주의

| ❌ 금지 | 이유 |
|---|---|
| 이미 push 된 태그를 **다른 커밋으로 이동** (`-f`) | 태그를 받아간 쪽과 불일치 → 재현 불가·배포 사고 |
| 사내 저장소에서 릴리스 태그를 **먼저** 찍기 | 정본은 공개. 사내는 동기화로 받는다 |
| `v` 없는 태그(`1.0.0`), 대문자·공백 | 도구·워크플로가 `v*` 패턴을 기대 |
| 릴리스 노트 없는 태그 | 공동 기관·외부 기여자가 변경 내용을 알 수 없다 |

**잘못 찍었을 때**: 태그를 옮기지 말고 **다음 PATCH 로 올린다** (`v1.0.1`).
아직 아무도 받아가지 않은 직후라면 삭제 후 재생성이 가능하나, 공유된 뒤에는 금지한다.
```bash
# (공유 전에만) 삭제
git push public :refs/tags/v0.0.1   # 원격 태그 삭제
git tag -d v0.0.1                   # 로컬 태그 삭제
```

---

## 2. 컨테이너 레지스트리 태그 (`registry.obigo.com`)

### 2-1. 이미지 이름 규칙
```
registry.obigo.com/oss/<컴포넌트명>:<태그>
```
- 프로젝트 네임스페이스는 **`oss/`** 로 고정.
- 컴포넌트명은 **소문자 + 하이픈**. 예: `oss-mmf-core`, `oss-mmf-ext-ros2`,
  `oss-mmf-ext-agl`, `oss-hmi-launcher`, `oss-adsw-monitor`, `rviz2-agl`.

### 2-2. 태그 종류

| 태그 유형 | 형식 | 성격 | 용도 |
|---|---|---|---|
| **버전** | `3.32.12` (앱 버전) 또는 `vX.Y.Z` (릴리스 연동) | **불변 권장** | 재현 가능한 배포·롤백 |
| **`latest`** | `latest` | **이동식** | 최신 개발본 편의 pull |
| **변종(variant)** | `touch`, `autoware` | 이동식 | 기능 변종 이미지 |
| **검증용** | `multiarch-verify` 등 | 임시 | 실험·검증 후 폐기 |

> **`latest` 는 "최신"만 의미하며 특정 버전을 보장하지 않는다.** 보드 배포·이슈 재현·
> 릴리스 검증에는 **반드시 버전 태그**를 쓴다.

### 2-3. 멀티아키텍처(amd64 + arm64) 규칙
하나의 태그에 두 아키텍처를 묶은 **manifest list** 를 만든다. 그러면 x86 개발 PC 는 amd64 를,
Orin/AGL 보드(aarch64)는 arm64 를 **자동 선택**해 받는다.

```bash
# MMF
BUILD_TYPE=Release ./docker/build_multiarch.sh
# HMI
TAG=latest ./docker/script/build_multiarch.sh
```
- 아키별로 각각 빌드·push 후 manifest list 로 결합한다 (런타임 이미지는 아키별 사전 빌드
  번들·Dockerfile 을 쓰므로 단일 `--platform=a,b` 빌드가 불가능하다).
- 아키별 개별 태그가 필요하면 접미사를 쓴다: `:latest-arm64`, `:latest-amd64`.

**push 결과 검증 (pull 없이)**
```bash
docker buildx imagetools inspect registry.obigo.com/oss/oss-hmi-launcher:latest
docker manifest inspect  registry.obigo.com/oss/oss-hmi-launcher:3.32.12   # arm64 포함 확인
```

### 2-4. 태그 부여 · push 절차
```bash
docker login registry.obigo.com -u <계정>          # 자격증명은 secret 으로 주입 (하드코딩 금지)

# 로컬 이미지 → 레지스트리 태그 2개(버전 + latest) 부여
docker tag oss-hmi-launcher:3.32.12 registry.obigo.com/oss/oss-hmi-launcher:3.32.12
docker tag oss-hmi-launcher:3.32.12 registry.obigo.com/oss/oss-hmi-launcher:latest

docker push --all-tags registry.obigo.com/oss/oss-hmi-launcher
```

> **CI 자격증명**: Jenkins/Actions 에서는 계정·비밀번호를 **credentials/secret 으로 주입**한다.
> 스크립트·Jenkinsfile 에 평문으로 적지 않는다.

### 2-5. 금지 · 주의

| ❌ 금지 | 이유 |
|---|---|
| 이미 배포된 **버전 태그를 다른 이미지로 덮어쓰기** | 같은 태그가 다른 내용 → 재현 불가 |
| 보드·운영 배포에 `latest` 사용 | 언제 무엇이 바뀌는지 추적 불가 |
| 아키 확인 없이 보드에 pull | amd64 이미지가 arm64 보드에서 기동 실패 |
| 임시 검증 태그 방치 | 레지스트리 오염 — 검증 후 정리 |

---

## 3. Git 태그 ↔ 컨테이너 태그 대비 (혼동 방지)

| 항목 | Git 릴리스 태그 | 컨테이너 이미지 태그 |
|---|---|---|
| 대상 | **소스 커밋** | **빌드된 이미지** |
| 위치 | 공개 GitHub 저장소 | 사내 `registry.obigo.com` |
| 형식 | `vX.Y.Z` (SemVer, `v` 접두) | `3.32.12` / `latest` / `touch` |
| 누가 | 릴리스 담당자 | 빌드 담당자 · CI |
| 공개 여부 | **공개** (외부 기여자가 봄) | **사내 전용** |
| 불변성 | 항상 불변 | 버전 태그는 불변, `latest` 는 이동 |
| 개수 | 릴리스당 1개 | 이미지당 여러 개 가능 |

**연결 규칙 (권장)**: 릴리스 `vX.Y.Z` 를 찍을 때, 그 커밋으로 빌드한 이미지에도
**대응하는 버전 태그**를 부여해 소스↔이미지 추적성을 확보한다.

```
공개 소스 태그      v0.2.0
사내 이미지 태그    registry.obigo.com/oss/oss-hmi-launcher:0.2.0   (+ :latest)
```

---

## 4. 릴리스 체크리스트

**Git 릴리스 (공개 저장소)**
- [ ] 사내 develop → 공개 develop publish 완료 (공개할 내용이 모두 반영됨)
- [ ] 버전 자리 결정 (MAJOR/MINOR/PATCH 기준 §1-2)
- [ ] `vX.Y.Z` annotated 태그 + GitHub Release 생성 (노트 포함)
- [ ] 정식/pre-release 구분 체크
- [ ] 상대 저장소와의 **검증 조합** 노트에 명시 (§1-3)
- [ ] `sync-release` 로 사내 `deploy` 반영 확인

**컨테이너 이미지**
- [ ] 버전 태그 + `latest` 두 개 부여
- [ ] 멀티아키(manifest list) push 및 `imagetools inspect` 로 arm64 포함 확인
- [ ] 보드 pull·기동 검증 (`latest` 가 arm64 를 자동 선택하는지)
- [ ] 자격증명이 secret 으로 주입되었는지 (평문 없음)
- [ ] 임시 검증 태그 정리

---

## 5. 자주 묻는 질문

**Q. 사내 저장소에도 같은 `vX.Y.Z` 태그를 둘 수 있나?**
가능하다. 단 **공개에서 먼저 찍고** 동일 커밋에 맞춰 사내에 push 하는 순서를 지킨다.
사내가 먼저 찍으면 정본(공개) 원칙이 깨진다.

**Q. `latest` 만 쓰면 안 되나?**
개발 편의로는 괜찮지만, **배포·재현·롤백에는 부적합**하다. 문제가 생겼을 때 "그때의 latest"를
복원할 수 없다. 버전 태그를 함께 부여한다.

**Q. Git 태그와 이미지 태그 버전을 꼭 같게 맞춰야 하나?**
필수는 아니다 (앱 자체 버전 체계가 있을 수 있다). 다만 §3 의 연결 규칙처럼 **대응 관계를
남기면** 소스↔이미지 추적이 쉬워진다.

**Q. 사전 릴리스(rc)도 이미지를 올리나?**
올려도 되지만 태그로 구분한다 (`1.2.0-rc1`). `latest` 는 **정식 릴리스에만** 옮긴다.
