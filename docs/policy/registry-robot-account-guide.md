# 컨테이너 레지스트리 로봇 계정 가이드 (Harbor)

> **대상**: 레지스트리 담당자 · CI 관리자 · 공동 개발 기관
> **레지스트리**: `registry.obigo.com` (Harbor, 인증 방식 `db_auth`)
> **프로젝트**: `oss` — **public** (익명 pull 허용)
>
> 관련 문서: [tagging-rules-guide.md](./tagging-rules-guide.md) (이미지 태그 규칙),
> [build-install-run-guide.md](../guide/build-install-run-guide.md) (빌드·배포 흐름)

---

## 0. 한눈에 · Quick start

| 무엇을 | 누가 | 자격증명 |
|---|---|---|
| **pull** (이용자·개발자·보드) | 누구나 | **불필요** — 프로젝트가 public |
| **push** (CI) | Jenkins / Actions | **로봇 계정** `oss-ci` → credential store |
| **push** (사람, 수동 릴리스) | 담당자 | 개인 계정으로 `docker login` 1회 |

핵심 3원칙:
1. **pull 과 push 를 같은 계정으로 쓰지 않는다.** 공개 문서에 적은 pull 계정이 곧 push 권한이 되면 안 된다.
2. **자격증명을 저장소에 두지 않는다.** 평문 비밀번호는 공개 저장소로 전달되는 순간 노출된다.
3. **CI 는 사람 계정이 아니라 로봇 계정을 쓴다.** 담당자 이동·비밀번호 변경에 파이프라인이 흔들리지 않는다.

---

## 1. 왜 로봇 계정인가

Harbor 의 로봇 계정(robot account)은 자동화 전용 자격증명이다. 사람 계정을 CI 에 넣는 방식과 비교하면:

| 항목 | 사람 계정 공유 | **로봇 계정** |
|---|---|---|
| 권한 범위 | 그 사람의 모든 프로젝트 권한 | **프로젝트·동작 단위로 제한** (`oss` 에 pull+push 만) |
| 유출 시 영향 | 사람 계정 전체 폐기 → 본인 작업도 마비 | **해당 로봇만 폐기** — 다른 것 영향 없음 |
| 만료 관리 | 비밀번호 정책에 종속 | **만료일 지정** 가능 |
| 담당자 이동 | 파이프라인 중단 | 무관 |
| 감사(audit) | 사람 활동과 섞임 | 로봇 이름으로 분리 추적 |

---

## 2. 로봇 계정 생성 절차

Harbor UI 는 한글/영문이 섞여 표시될 수 있다. 아래 괄호는 영문 UI 표기다.

1. `https://registry.obigo.com` 로그인 (프로젝트 관리자 이상)
2. **프로젝트(Projects)** → **`oss`** 선택
3. **로봇 계정(Robot Accounts)** 탭 → **NEW ROBOT ACCOUNT**
4. 입력:

   | 항목 | 값 | 비고 |
   |---|---|---|
   | Name | `oss-ci` | 용도가 드러나는 이름 |
   | Description | `CI image push (Jenkins/Actions)` | 나중에 누가 봐도 알 수 있게 |
   | Expiration | `90` days | 무기한(`never`) 비권장 |
   | Permissions → Repository | ✅ **Pull**, ✅ **Push** | **Delete 는 체크하지 않는다** (최소 권한) |

5. **ADD**
6. 🔴 **토큰은 이 화면에서 한 번만 표시된다** → **COPY** 또는 **EXPORT TO FILE** 로 즉시 보관
7. 생성된 **로그인 이름**을 확인한다. 프로젝트 접두가 붙어 다음 형태가 된다:

   ```
   robot$oss+oss-ci
   ```

> **이름에 `$` 가 포함된다는 점이 중요하다.** 셸에서 리터럴로 쓰면 `$oss` 가 변수로 확장되어
> 사용자명이 깨진다. 항상 **작은따옴표** 또는 **환경변수 참조**로 전달한다 (§5 참고).

### 권한을 왜 Delete 없이 두는가
이미지 삭제는 되돌릴 수 없고 CI 에 필요하지도 않다. 삭제는 사람이 UI 에서 판단해 수행한다.
(임시 검증 태그 정리도 사람이 한다 — [tagging-rules-guide.md](./tagging-rules-guide.md) §2-6)

---

## 3. 검증

### 3-1. 로그인
```bash
docker login registry.obigo.com -u 'robot$oss+oss-ci'
# 토큰을 붙여넣는다 (입력값은 화면·셸 히스토리에 남지 않는다)
```
> `-u` 값은 **작은따옴표**로 감싼다. 큰따옴표를 쓰면 `$oss` 가 확장된다.

### 3-2. push 권한 확인 (임시 태그 사용)
```bash
docker pull alpine:3.20
docker tag  alpine:3.20 registry.obigo.com/oss/credential-test:credential-test
docker push registry.obigo.com/oss/credential-test:credential-test   # 성공해야 한다
```

### 3-3. 정리
검증 후 **임시 리포지토리를 삭제**한다. 로봇에는 Delete 권한이 없으므로 UI 에서 수행한다:
> 프로젝트 `oss` → **저장소(Repositories)** → `oss/credential-test` → 삭제

### 3-4. 실패 시 확인 순서
| 증상 | 확인할 것 |
|---|---|
| `unauthorized ... action: push` | 로봇 권한에 **Push** 체크 여부 |
| `unauthorized: unauthorized to access repository` (로그인 상태에서) | 로봇이 **해당 프로젝트** 소속인지 |
| 로그인 자체 실패 | 토큰 오타 / **만료일** 경과 |
| 사용자명이 `robot` 로만 잘림 | `$` 가 셸에서 확장됨 → 작은따옴표로 감쌀 것 |

---

## 4. 자격증명 등록 (credential store)

**자격증명은 credential store 에만 둔다.** 저장소·스크립트·문서에 평문으로 적지 않는다.

### 4-1. Jenkins
1. **Manage Jenkins → Credentials → System → Global credentials → Add Credentials**
2. 입력:
   - Kind: **Username with password**
   - Username: `robot$oss+oss-ci`
   - Password: 로봇 토큰
   - **ID: `harbor-oss-ci`** ← Jenkinsfile 이 참조하는 이름 (정확히 일치해야 한다)
3. **Create**

Jenkinsfile 사용 예 (실제 적용된 형태):
```groovy
environment {
    IMAGE_REGISTRY = "registry.obigo.com"
    // REGISTRY_USR / REGISTRY_PSW 가 자동 생성되고 로그에서 마스킹된다
    REGISTRY = credentials('harbor-oss-ci')
}
...
sh '''
    printf '%s' "${REGISTRY_PSW}" | docker login "${IMAGE_REGISTRY}" -u "${REGISTRY_USR}" --password-stdin
'''
```
> `sh '''...'''` (단일 인용) 안에서 `${...}` 를 쓰면 Groovy 가 비밀을 스크립트 본문에 끼워넣지
> 않아 Jenkins 의 마스킹이 유지된다.

### 4-2. GitHub Actions
UI: 저장소 → Settings → Secrets and variables → Actions → **New repository secret**

또는 CLI (값이 셸 히스토리에 남지 않는다):
```bash
gh secret set HARBOR_ROBOT_USER  -R obigo-gov/oss_multisw_framework --body 'robot$oss+oss-ci'
gh secret set HARBOR_ROBOT_TOKEN -R obigo-gov/oss_multisw_framework   # 실행 후 붙여넣기
```

### 4-3. 개발자 로컬
```bash
docker login registry.obigo.com        # 최초 1회 — ~/.docker/config.json 에 저장되어 유지된다
```
- pull 만 할 거면 **로그인이 필요 없다** (프로젝트가 public).
- 빌드 스크립트(`docker/registry-auth.sh`)는 **이미 로그인돼 있으면 아무것도 하지 않고**,
  자격증명이 없으면 안내 후 중단한다. 하드코딩 fallback 은 없다.

---

## 5. 하지 말아야 할 것

| ❌ | 이유 |
|---|---|
| 저장소·스크립트·문서에 비밀번호 평문 기재 | 공개 저장소로 전달되는 순간 노출. git 히스토리에도 남는다 |
| pull 계정과 push 계정을 하나로 운영 | 공개 문서에 적은 pull 자격증명이 곧 push 권한이 된다 |
| 로봇 이름을 큰따옴표/무인용으로 셸에 기재 | `$oss` 가 확장되어 사용자명이 깨진다 |
| 로봇에 Delete·관리자 권한 부여 | 최소 권한 위반. 삭제는 사람이 UI 에서 |
| 만료일 `never` | 유출 시 무기한 유효 |
| 관리자 계정(`builder` 등)을 CI 에 사용 | 권한 과다 + 유출 영향 범위가 넓다 |

---

## 6. 운영 (만료·교체·폐기)

**만료 전 교체**
1. 새 로봇(`oss-ci-2`) 발급 → Jenkins Credentials 값 갱신 → CI 1회 실행해 확인
2. 확인 후 기존 로봇 폐기 (로봇 계정 탭 → 삭제)
> 순서를 지키면 CI 중단 없이 교체된다. 반대로 하면 파이프라인이 멈춘다.

**유출 의심 시**
1. 해당 로봇 **즉시 삭제(revoke)** — 다른 계정·CI 는 영향 없다
2. 새 로봇 발급 → credential store 갱신
3. Harbor **로그(Logs)** 탭에서 해당 로봇의 push 이력 점검

**만료 관리**: 만료 1~2주 전 알림을 캘린더에 등록한다.

---

## 7. 참고

- 이미지 태그 규칙(스냅샷 태그·`latest`·멀티아키): [tagging-rules-guide.md](./tagging-rules-guide.md)
- 빌드·배포 흐름: [build-install-run-guide.md](../guide/build-install-run-guide.md)
- 공개 저장소 push 용 GitHub 봇 계정(별개 주제): [bot-account-setup-guide.md](./bot-account-setup-guide.md)
- Harbor 공식 문서: "Robot Accounts", "Configure Project Access Level"
