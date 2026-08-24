# 태깅 룰 가이드 (Git 태그 · 컨테이너 레지스트리 태그)

> **대상**: 공동 개발 기관 · 내부 개발자 · 릴리스 담당자
> **범위**: ① 공개 저장소(GitHub)에서 찍는 **Git 릴리스 태그** — `OSS-TEST-GROUP` 의
> **5 저장소 공통 정책**(§1-3), ② 컨테이너 레지스트리 `registry.obigo.com`(Harbor)의
> **이미지 태그**.
> 두 태그 모두 **공개 대상**이다 — 이미지는 공동 기관·외부 이용자가 pull 한다.
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
| 컨테이너 **불변 스냅샷 태그** (필수) | `registry.obigo.com` | `<branch>-<date>-<shortsha>` | `develop-20260731-9e4a5b6` |
| 컨테이너 **이동식 포인터** (필수) | `registry.obigo.com` | `latest` (+ 변종 `touch` 등) | `latest`, `touch` |

핵심 3원칙:
1. **Git 릴리스 태그는 공개 저장소에서만 찍는다** (사내는 동기화로 받는다).
2. **태그는 불변(immutable)** — 한번 배포된 `vX.Y.Z` 는 절대 옮기지 않는다.
3. **컨테이너 이미지는 `latest` 와 `<branch>-<date>-<shortsha>` 스냅샷 태그를 항상 함께 push**
   한다 (둘 다 필수 — 빌드 스크립트가 동시에 부여). `latest` 는 최신 스냅샷을 가리키는
   포인터이고, 배포·재현·롤백은 **스냅샷 태그로** 한다.

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

- **초기 개발 단계는 `0.y.z`** — 공개 API 가 안정되면 `v1.0.0` 으로 올린다.
  이 프로젝트의 **공통 기준선은 `v0.1.0`** 이며, 저장소별 수정은 `v0.1.1`, `v0.1.2` … (§1-3)
- **사전 릴리스**: `v1.2.0-rc1`, `v1.2.0-beta1` — GitHub Release 에서 **pre-release 체크**.
  정식 릴리스만 내부 배포 대상이며, `sync-release` 가 pre-release 를 자동으로 걸러낸다.

### 1-3. 프로젝트 공통 버전 정책 (5 저장소)

**MINOR 기준선은 프로젝트 전체가 공유하고, PATCH 는 저장소별로 올린다.**
공개 저장소가 여러 개이므로, 번호만 보고 "어느 세트끼리 맞는지" 알 수 있게 한 규칙이다.

대상 저장소 (`OSS-TEST-GROUP/`):

| 저장소 | 사내 대응 | 비고 |
|---|---|---|
| `oss_multisw_framework` | `obigo-gov/oss_multisw_framework` | 태그 사내 동기화 대상 |
| `oss_hmi_app` | `obigo-gov/oss_hmi_app` | 태그 사내 동기화 대상 |
| `oss-ids` | 없음 | 공개 단독 |
| `katech-autoware` | 없음 | 공개 단독 |
| `katech-autoware-universe` | 없음 | 공개 단독 |

#### 규칙

1. **공통 기준선(MINOR)** — 릴리스 마일스톤에서는 **모든 저장소에 같은 `vX.Y.0`** 을 부여한다.
   현재 기준선은 **`v0.1.0`** 이다.
2. **개별 PATCH** — 특정 저장소에서 이슈가 발생해 수정하면 **그 저장소만** PATCH 를 올린다.
   변경이 없는 저장소는 태그를 새로 찍지 않는다.
   ```
   예) MMF 에서 버그 수정 → oss_multisw_framework v0.1.1
       나머지 4 저장소는 v0.1.0 유지 (재태깅 X)
   ```
3. **다음 기준선** — 기능 추가 마일스톤에 도달하면 MINOR 를 올려 **다시 전체를 정렬**한다
   (`v0.2.0`). 이때 PATCH 로 어긋나 있던 번호도 함께 맞춰진다.
4. **조합 명시** — PATCH 단계에서는 저장소 간 번호가 일시적으로 어긋난다. 배포·검증 시에는
   Release 노트에 **실제 검증 조합**을 적는다.
   ```
   예) oss_hmi_app v0.1.0 Release 노트
       - 검증 조합 / Verified with: oss_multisw_framework v0.1.1, oss-ids v0.1.0
   ```

> **왜 이렇게 하나**: 저장소마다 완전히 독립적으로 버전을 매기면 기관 간 "무엇끼리 맞는
> 조합인가"를 매번 표로 확인해야 한다. 반대로 PATCH 까지 전부 일괄로 올리면 변경 없는
> 저장소에 의미 없는 태그가 쌓인다. **MINOR 는 공유, PATCH 는 개별**이 두 문제를 함께 피한다.

> **초기 기준선이 `v0.1.0` 인 이유**: `0.y.z` 는 공개 API 가 아직 안정화 전임을 뜻하고
> (§1-2), `0.1.0` 은 "첫 공개 마일스톤"으로 읽힌다. 그 전에 찍힌 `v0.0.1`·`v0.0.2` 태그는
> 이력으로 남겨 둔다 (§1-5 — 공유된 태그는 삭제·이동하지 않는다).

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

> **이 레지스트리의 이미지는 공개 대상이다.** 공개 저장소의 README·가이드가 외부 이용자에게
> `docker pull registry.obigo.com/oss/…` 를 안내하며, 공동 개발 기관도 같은 태그로 받는다.
> pull 은 **read-only 계정**으로 하고, push 는 담당자·CI 만 수행한다.
>
> 따라서 **이미지명·태그명은 외부에 노출되는 공개 식별자**다. 내부 정보가 드러나는 이름을
> 쓰지 않는다 (정상 경로에서는 `develop-…` 스냅샷만 push 되므로 문제되지 않는다 — §2-2).

### 2-1. 이미지 이름 규칙
```
registry.obigo.com/oss/<컴포넌트명>:<태그>
```
- 프로젝트 네임스페이스는 **`oss/`** 로 고정.
- 컴포넌트명은 **소문자 + 하이픈**. 예: `oss-mmf-core`, `oss-mmf-ext-ros2`,
  `oss-mmf-ext-agl`, `oss-hmi-launcher`, `oss-adsw-monitor`, `rviz2-agl`.
- 이름은 공개 문서에 그대로 실리므로 **한번 정하면 바꾸지 않는다** (외부 pull 경로가 깨진다).

### 2-2. 태그 체계 — 불변 스냅샷 + 이동식 포인터

이미지는 **`latest` 와 불변 스냅샷 태그를 항상 함께 push** 한다 (**둘 다 필수**).
스냅샷 태그가 정본이고, `latest` 는 최신 스냅샷을 가리키는 별칭이다.
빌드 스크립트가 두 태그를 **한 번에 부여**하므로 따로 챙길 일은 없다 (§2-4).

| 태그 유형 | 형식 | 성격 | 용도 |
|---|---|---|---|
| **불변 스냅샷** (필수) | `<branch>-<date>-<shortsha>` | **불변** — 절대 덮어쓰지 않음 | 배포·재현·롤백·이슈 추적 |
| **`latest`** (필수) | `latest` | **이동식** 포인터 | 최신 개발본 편의 pull |
| **변종(variant)** | `<변종명>` 또는 `<변종>-<date>-<shortsha>` | 이동식 / 불변 | `touch`, `autoware` 등 기능 변종 |
| **릴리스 연동** (선택) | `vX.Y.Z` | 불변 | 공개 Git 릴리스 태그와 대응 (§3) |

#### 스냅샷 태그 형식
```
[<prefix>-]<branch>-<date>-<shortsha>
     │         │        │         └─ 소스 커밋 short SHA (git rev-parse --short HEAD)
     │         │        └─ 빌드 날짜 YYYYMMDD (UTC)
     │         └─ 빌드한 소스 브랜치명 (Docker 태그 비허용 문자 → 하이픈 치환)
     └─ (선택) 변종 구분 prefix — 아래 "변종 prefix" 참고
```

| 규칙 | 내용 |
|---|---|
| 문자 | 영숫자 + `-` + `.` + `_` 만 (Docker 태그 제약). **슬래시(`/`) 금지** |
| 브랜치 치환 | 비허용 문자를 모두 `-` 로: `sed 's/[^A-Za-z0-9._-]/-/g'`<br>예) `feature/rviz2-ext-agl` → `feature-rviz2-ext-agl` |
| 날짜 | `YYYYMMDD` **UTC** (`date -u +%Y%m%d`). 하루 여러 번 빌드해도 SHA 로 구분된다 |
| SHA | `git rev-parse --short HEAD` (기본 7자). 앞에 `g` 같은 접두어를 붙이지 않는다 |
| prefix | 선택. 브랜치와 같은 규칙으로 치환된다. 지정하지 않으면 붙지 않는다 |
| 길이 | 128자 제한 — 브랜치명이 길면 앞 40자로 잘라 쓴다 |

> 형식은 **빌드 스크립트가 자동 계산**한다 (§2-4). 손으로 조립하지 않는다.

#### ⚠️ 변종 prefix — 같은 소스로 다른 이미지를 만들 때

**같은 커밋에서 여러 변종 이미지를 만들어 같은 리포지토리에 push 하면 스냅샷 태그가 서로
덮어쓴다.** 브랜치·날짜·SHA 가 모두 같아 태그가 동일해지기 때문이다.

```
# prefix 없이 두 변종을 빌드하면 — 나중 것이 앞의 것을 덮어쓴다
oss/rviz2-agl:develop-20260801-e0399cc   ← 표준 빌드
oss/rviz2-agl:develop-20260801-e0399cc   ← autoware 변종 (같은 태그! 덮어씀)
```

이를 막기 위해 **`SNAP_PREFIX` 환경변수로 변종을 구분**한다.

```bash
SNAP_PREFIX=touch    ./docker/script/build_rviz2_multiarch.sh
# → oss/rviz2-agl:touch-develop-20260801-e0399cc

SNAP_PREFIX=debug BUILD_TYPE=Debug ./docker/build_multiarch.sh
# → oss/oss-mmf-core:debug-develop-20260801-abc1234
```

| 언제 쓰나 | prefix 예 |
|---|---|
| 기능 변종 (터치본·Autoware 연동 등) | `touch`, `autoware`, `base` |
| 빌드 타입이 다른 이미지 (Debug/Release 를 같은 repo 에) | `debug` |
| 그 외 같은 소스·같은 repo 로 나뉘는 산출물 | 구분되는 짧은 이름 |

- **변종이 서로 다른 리포지토리로 가면 prefix 가 필요 없다** (예: `oss-mmf-core` /
  `oss-mmf-ext-ros2` / `oss-mmf-ext-agl` 는 repo 가 달라 충돌하지 않는다).
#### 스크립트별 기본 prefix

`SNAP_PREFIX` 를 명시하면 항상 그 값이 우선한다. 명시하지 않았을 때의 기본값은 다음과 같다.

| 스크립트 | 기본 prefix | 결과 |
|---|---|---|
| MMF `docker/build_multiarch.sh` | 없음 | `develop-20260801-abc1234` |
| HMI `build_multiarch.sh` | 없음 | `develop-20260801-abc1234` |
| HMI `build_rviz2_multiarch.sh` | **`TAG` 값** (단 `latest` 면 없음) | `TAG=touch` → `touch-develop-…` |
| HMI `build_rviz2_autoware.sh` | **`autoware`** | `autoware-develop-…` |

- rviz2 계열은 **한 리포지토리(`oss/rviz2-agl`)를 여러 변종이 공유**하므로 기본값을 두어
  깜빡해도 덮어쓰지 않게 했다. 기본값은 레지스트리의 기존 관행(`touch-…`, `base-…`)과 같다.
- 강제로 prefix 를 없애려면 `SNAP_PREFIX=""` 를 명시한다.
- prefix 는 **접두**로 붙인다. 아키별 중간 태그가 `-arm64`/`-amd64` **접미**를 쓰므로,
  접미로 붙이면 둘이 섞여 읽기 어려워진다.

**예시**
```
develop-20260731-9e4a5b6                 ← develop 표준 빌드
touch-develop-20260801-e0399cc           ← 터치 변종
autoware-develop-20260801-e0399cc        ← Autoware 변종
debug-develop-20260801-abc1234           ← Debug 빌드
touch-develop-20260801-e0399cc-arm64     ← 변종 + 아키별 중간 태그
```

> **왜 스냅샷 태그인가**: `latest` 만으로는 "그때 보드에 올라간 이미지"를 다시 만들 수 없다.
> 스냅샷 태그는 **어느 브랜치의 어느 커밋을 언제 빌드했는지**가 태그 자체에 남아, 이슈 재현·
> 롤백·기관 간 검증 조합 공유가 가능해진다.

#### 🔒 레지스트리 push 는 default 브랜치(`develop`)에서만

**기능 브랜치에서는 레지스트리 push 자체가 차단된다.** `latest` 뿐 아니라 스냅샷 태그도
push 하지 않는다 — 빌드 스크립트가 현재 브랜치를 확인해 `develop` 이 아니면 **중단**한다.

```
ERROR: 현재 브랜치 'feature-xxx' — :latest + 스냅샷 push 는
       default 브랜치 'develop' 에서만 허용됩니다.
       'develop' 로 머지 후 그 브랜치에서 빌드하세요.
```

| 왜 | 내용 |
|---|---|
| `latest` 오염 방지 | 기능 브랜치 빌드가 정본 `latest` 를 덮어쓰는 사고를 원천 차단 |
| 레지스트리 위생 | 머지되지 않은(사라질) 브랜치의 이미지가 쌓이지 않는다 |
| 추적성 | 레지스트리의 모든 이미지가 `develop` 히스토리에 존재하는 커밋을 가리킨다 |

- **기능 브랜치를 검증**해야 하면 로컬 빌드(`build_local.sh` 등)를 쓰거나, 정책 조정이
  필요한 예외 상황에서만 `DEFAULT_BRANCH` 환경변수로 우회한다 (권장하지 않음).
- **커밋되지 않은 변경(dirty)** 이 있으면 역시 push 가 차단된다 — SHA 가 실제 내용을
  대표하지 못하므로 재현성이 깨진다.

#### 태그 선택 기준

**push 는 둘 다 필수**이고, 아래는 **pull·참조할 때 어느 태그를 쓰는지**의 기준이다.

| 상황 | 사용할 태그 |
|---|---|
| 보드·운영 배포 | **스냅샷 태그** (필수) |
| 이슈 재현·회귀 확인 | **스냅샷 태그** |
| 릴리스 검증 | **스냅샷 태그** (+ `vX.Y.Z` 대응) |
| 개발 PC 에서 최신 편의 pull | `latest` |
| 기관 간 "무엇으로 검증했나" 공유 | **스냅샷 태그** (문서·이슈에 그대로 기재) |

### 2-3. 멀티아키텍처(amd64 + arm64) 규칙
하나의 태그에 두 아키텍처를 묶은 **manifest list** 를 만든다. 그러면 x86 개발 PC 는 amd64 를,
Orin/AGL 보드(aarch64)는 arm64 를 **자동 선택**해 받는다.
**스냅샷 태그와 `latest` 둘 다 manifest list 로 만든다.**

```bash
# MMF (repo 루트에서) — 스냅샷 태그는 스크립트가 자동 계산한다
BUILD_TYPE=Release ./docker/build_multiarch.sh

# HMI (apps/ 에서)
./docker/script/build_multiarch.sh
APP=hmi_launcher ./docker/script/build_multiarch.sh
```
- **스냅샷 태그를 손으로 넘기지 않는다.** 스크립트가 브랜치·UTC 날짜·short SHA 로 계산해
  `:latest` 와 `:<snap>` 두 manifest list 를 함께 push 한다.
- 아키별로 각각 빌드·push 후 manifest list 로 결합한다 (런타임 이미지는 아키별 사전 빌드
  번들·Dockerfile 을 쓰므로 단일 `--platform=a,b` 빌드가 불가능하다).
- 중간 산출물인 아키별 태그는 스크립트가 `:<snap>-amd64` / `:<snap>-arm64` 로 push 한 뒤
  manifest list 로 묶는다.
- 이미지에는 OCI 라벨이 baked 된다: `image.revision`(전체 SHA), `image.ref.name`(브랜치),
  `image.created`(UTC), `image.version`(스냅샷 태그). → `docker inspect` 로 출처 확인 가능.

**push 결과 검증 (pull 없이)**
```bash
docker buildx imagetools inspect registry.obigo.com/oss/oss-hmi-launcher:develop-20260731-9e4a5b6
docker manifest inspect  registry.obigo.com/oss/oss-hmi-launcher:develop-20260731-9e4a5b6  # arm64 포함 확인
```

### 2-4. push 절차 — 빌드 스크립트를 쓴다

태그 부여·push 는 **빌드 스크립트가 수행한다.** `docker tag`/`docker push` 를 손으로 조합하지
않는다 (스냅샷 규칙·가드가 스크립트에 구현되어 있다).

```bash
# 0) 전제: develop 브랜치 + 커밋 완료(clean) 상태
git checkout develop && git pull && git status   # clean 확인

# 1) 빌드 + push  (:latest + :<snap> 을 함께 push)
#    MMF
BUILD_TYPE=Release ./docker/build_multiarch.sh
#    HMI (apps/ 에서)
./docker/script/build_multiarch.sh
```

스크립트가 자동으로 하는 일:

| 단계 | 내용 |
|---|---|
| 가드 ① | **default 브랜치(`develop`) 아니면 중단** — 레지스트리 push 불가 |
| 가드 ② | **커밋되지 않은 tracked 변경 있으면 중단** (untracked 빌드 산출물은 무시) |
| 태그 계산 | `<branch>-<date(UTC)>-<shortsha>` |
| push | 아키별 이미지 → manifest list `:latest` + `:<snap>` |
| 라벨 | OCI `revision`/`ref.name`/`created`/`version` baked |

> **CI 자격증명**: Jenkins/Actions 에서는 계정·비밀번호를 **credentials/secret 으로 주입**한다.
> 스크립트·Jenkinsfile 에 평문으로 적지 않는다.

> **검증 목적의 임시 태그**가 필요하면 `:latest` 를 건드리지 않는 이름을 쓴다
> (예: HMI `TAG=multiarch-verify ./docker/script/build_multiarch.sh`). 검증 후 정리한다.

### 2-5. 배포 · 재현 (스냅샷 태그 사용)

```bash
# 보드에서 스냅샷 태그로 pull (arm64 자동 선택)
docker pull registry.obigo.com/oss/oss-hmi-launcher:develop-20260731-9e4a5b6

# run 스크립트에 스냅샷 태그를 명시
IMAGE=registry.obigo.com/oss/oss-hmi-launcher:develop-20260731-9e4a5b6 \
  APP_VER=3.32.5 sh run-on-board-arm64.sh
```

**역추적**: 태그에서 소스 커밋을 바로 찾을 수 있다.
```bash
git show 9e4a5b6            # develop-20260731-9e4a5b6 의 소스 커밋
git log --oneline -1 9e4a5b6
```

### 2-6. 금지 · 주의

| ❌ 금지 | 이유 |
|---|---|
| **스냅샷 태그를 다른 이미지로 덮어쓰기** | 불변성 위반 — 재현·롤백 불가 |
| 보드·운영 배포에 `latest` 사용 | 언제 무엇이 바뀌는지 추적 불가 |
| 스냅샷 태그 없이 `latest` 만 push | 그 시점 이미지를 다시 지정할 수 없다 |
| `latest` 없이 스냅샷 태그만 push | 두 태그는 함께 push 한다 (§2-2) — 최신 포인터가 끊긴다 |
| **변종 이미지를 prefix 없이 같은 repo 에 push** | 스냅샷 태그가 서로 덮어써 재현이 깨진다 (§2-2 변종 prefix) |
| **기능 브랜치에서 레지스트리 push** | 스크립트가 차단 (`latest` 오염·레지스트리 위생) |
| 가드 우회 (`DEFAULT_BRANCH` 임의 변경) | 정본 `latest` 와 레지스트리 추적성이 깨진다 |
| `docker tag`/`push` 를 손으로 조합 | 가드·태그 규칙을 건너뛴다 — 스크립트를 쓴다 |
| 태그에 슬래시(`/`)·공백 등 비허용 문자 | Docker 태그 규칙 위반 → push 실패 |
| 커밋 없이(dirty) 빌드해 push | SHA 가 실제 내용과 불일치 (스크립트가 차단) |
| 아키 확인 없이 보드에 pull | amd64 이미지가 arm64 보드에서 기동 실패 |
| 임시 검증 태그 방치 | 레지스트리 오염 — 검증 후 정리 |

---

## 3. Git 태그 ↔ 컨테이너 태그 대비 (혼동 방지)

| 항목 | Git 릴리스 태그 | 컨테이너 이미지 태그 |
|---|---|---|
| 대상 | **소스 커밋** | **빌드된 이미지** |
| 위치 | 공개 GitHub 저장소 | `registry.obigo.com` (Harbor) |
| 형식 | `vX.Y.Z` (SemVer, `v` 접두) | `<branch>-<date>-<shortsha>` (+ `latest`) |
| 누가 | 릴리스 담당자 | 빌드 담당자 · CI |
| 공개 여부 | **공개** (외부 기여자가 봄) | **공개** (공동 기관·외부에서 pull) |
| 불변성 | 항상 불변 | **스냅샷 태그는 불변**, `latest` 는 이동 |
| 개수 | 릴리스당 1개 | 빌드마다 스냅샷 1개 (+ 포인터) |

**연결 규칙 (권장)**: 릴리스 `vX.Y.Z` 를 찍을 때, **그 커밋으로 빌드한 스냅샷 이미지에
`vX.Y.Z` 태그를 추가로 부여**한다. 스냅샷 태그는 그대로 남겨 추적성을 유지한다.

```
공개 소스 태그      v0.2.0                            (커밋 9e4a5b6)
사내 이미지 태그    oss-hmi-launcher:develop-20260731-9e4a5b6   ← 정본 스냅샷
                   oss-hmi-launcher:v0.2.0                     ← 릴리스 별칭 (같은 이미지)
                   oss-hmi-launcher:latest                     ← 포인터
```

> 스냅샷 태그의 `<shortsha>` 가 곧 Git 커밋이므로, **이미지 → 소스 역추적이 태그만으로 가능**하다.
> 릴리스 노트에는 스냅샷 태그를 함께 적어 기관 간 검증 조합을 명확히 한다.

---

## 4. 릴리스 체크리스트

**Git 릴리스 (공개 저장소)**
- [ ] 사내 develop → 공개 develop publish 완료 (공개할 내용이 모두 반영됨)
- [ ] 버전 자리 결정 (MAJOR/MINOR/PATCH 기준 §1-2)
- [ ] **범위 결정** — 공통 기준선(MINOR)이면 **5 저장소 전체**, 개별 수정이면 **해당 저장소만** (§1-3)
- [ ] `vX.Y.Z` annotated 태그 + GitHub Release 생성 (노트 포함)
- [ ] 정식/pre-release 구분 체크
- [ ] 다른 저장소와의 **검증 조합** 노트에 명시 (§1-3 — PATCH 로 번호가 어긋난 경우 특히)
- [ ] `sync-release` 로 사내 `deploy` 반영 확인

**컨테이너 이미지**
- [ ] **`develop` 브랜치**에서 빌드 (기능 브랜치는 스크립트가 차단)
- [ ] 커밋 완료 상태(clean)에서 빌드 — dirty 트리 아님 (스크립트가 차단)
- [ ] 빌드 스크립트로 push — `:latest` + **불변 스냅샷 태그** 동시 생성
- [ ] 같은 소스로 만든 **변종 이미지면 `SNAP_PREFIX` 지정** (touch/autoware/debug 등)
- [ ] (릴리스면) 같은 이미지에 `vX.Y.Z` 태그 추가 부여
- [ ] 멀티아키(manifest list) push 및 `imagetools inspect` 로 arm64 포함 확인
- [ ] 보드에서 **스냅샷 태그로** pull·기동 검증 (arm64 자동 선택 확인)
- [ ] 배포·검증에 사용한 **스냅샷 태그를 문서·이슈·릴리스 노트에 기재**
- [ ] 자격증명이 secret 으로 주입되었는지 (평문 없음)
- [ ] 임시 검증 태그 정리

---

## 5. 자주 묻는 질문

**Q. 한 저장소만 고쳤는데 5 개 전부 태그를 올려야 하나?**
아니다. **PATCH 는 해당 저장소만** 올린다 (§1-3 규칙 2). 나머지는 기준선 태그를 유지한다.
전체를 다시 정렬하는 시점은 **MINOR 기준선을 올릴 때**다.

**Q. 그러면 저장소 간 번호가 어긋나는데 괜찮은가?**
의도된 상태다. MINOR 자리(`v0.1.x`)가 같으면 **같은 기준선**이라는 뜻이므로 호환 조합을
바로 알 수 있다. 정확한 조합은 Release 노트의 "검증 조합"에 적는다.

**Q. 이미 찍힌 `v0.0.1`·`v0.0.2` 는 지워야 하나?**
지우지 않는다. 공유된 태그를 삭제·이동하지 않는 것이 원칙이다 (§1-5). `v0.1.0` 이 더 높은
버전이므로 최신으로 표시되어 혼란도 없다.

**Q. 사내 저장소에도 같은 `vX.Y.Z` 태그를 둘 수 있나?**
가능하다. 단 **공개에서 먼저 찍고** 동일 커밋에 맞춰 사내에 push 하는 순서를 지킨다.
사내가 먼저 찍으면 정본(공개) 원칙이 깨진다.

**Q. `latest` 만 쓰면 안 되나?**
개발 PC 편의로는 괜찮지만 **배포·재현·롤백에는 금지**다. 문제가 생겼을 때 "그때의 latest"를
복원할 수 없다. **스냅샷 태그를 항상 함께 push** 하고, 배포엔 스냅샷 태그를 쓴다.

**Q. 하루에 여러 번 빌드하면 태그가 겹치지 않나?**
겹치지 않는다. 날짜가 같아도 **커밋 SHA 가 다르면 다른 태그**다
(`develop-20260731-9e4a5b6` vs `develop-20260731-f450dc9`).
같은 커밋을 재빌드하면 같은 태그가 되는데, 이때는 **덮어쓰지 않는다**
(내용이 같아야 정상이며, 다르다면 빌드 환경 차이를 먼저 조사한다).

**Q. 기존 `3.32.12` 같은 앱 버전 태그는 어떻게 되나?**
앱 자체 버전 표기로는 계속 쓸 수 있으나, **배포 지정은 스냅샷 태그로 통일**한다.
앱 버전은 `APP_VER` 등 런타임 인자·릴리스 노트로 표현한다.

**Q. Git 태그와 이미지 태그를 꼭 맞춰야 하나?**
스냅샷 태그의 `<shortsha>` 가 이미 Git 커밋을 가리키므로 자동으로 연결된다.
릴리스 시에는 §3 처럼 같은 이미지에 `vX.Y.Z` 를 **추가** 부여한다.

**Q. 사전 릴리스(rc)도 이미지를 올리나?**
올린다. 스냅샷 태그로 push 하고, 필요하면 `v1.2.0-rc1` 별칭을 추가한다.
`latest` 는 **정식 릴리스(기본 브랜치)에만** 옮긴다.

**Q. 태그에 브랜치명을 넣으면 브랜치 삭제 후 의미가 없어지지 않나?**
브랜치명은 **"어느 흐름에서 나온 빌드인지"** 를 알려주는 라벨이고, 재현의 근거는 SHA 다.
브랜치가 삭제돼도 SHA 로 소스를 찾을 수 있으므로 추적성은 유지된다.
정상 경로에서는 `develop` 만 push 되므로 대부분 `develop-…` 형태다.

**Q. 기능 브랜치에서 만든 이미지를 보드에서 테스트하려면?**
레지스트리 push 는 막혀 있으므로 **로컬 빌드**(`docker/build_local.sh` 등)로 이미지를 만들어
`docker save`/`load` 로 보드에 옮기거나, 먼저 `develop` 에 머지한 뒤 정상 경로로 빌드한다.
`DEFAULT_BRANCH` 로 가드를 우회하는 것은 권장하지 않는다.

**Q. 스냅샷 태그가 실제로 붙었는지 어떻게 확인하나?**
```bash
docker buildx imagetools inspect registry.obigo.com/oss/oss-mmf-core:<snap>   # manifest·아키 확인
docker inspect --format '{{json .Config.Labels}}' <이미지> | jq   # OCI 라벨(revision/version)
```
