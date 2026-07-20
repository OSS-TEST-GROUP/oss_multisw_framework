# 봇 / 서비스 계정 준비 상세 가이드

> 목적: 사내 → Public 코드 전달·동기화 워크플로가 공개 저장소에 접근할 때 사용할
>       전용 계정(봇)을 만들고 권한·토큰을 설정한다.
> 적용: `OSS-TEST-GROUP/oss_multisw_framework`, `OSS-TEST-GROUP/oss_hmi_app` (공개 저장소 공통)
> 봇은 **하나만 만들어 두 저장소에 재사용**한다. (아직 만든 봇이 없다면 이 문서대로 새로 생성)

---

## 0. 봇 계정이란? 왜 필요한가

GitHub에는 "봇 전용 계정 타입"이 따로 있는 게 아니라, **일반 사용자 계정을 사람이 아닌 자동화 용도로 전용하는 것**을 봇/서비스 계정(machine user)이라 부른다. 워크플로가 공개 저장소에 push하거나 PR을 만들려면 누군가의 자격으로 접근해야 하는데, 이를 개인 계정으로 하면:

- 담당자가 퇴사·이동하면 파이프라인이 멈춘다
- 누가 공개했는지 추적이 개인 활동과 섞인다
- 개인 계정 권한을 회수하기 어렵다

그래서 **전용 봇 계정 하나로 공개 push를 일원화**한다. (오픈소스 운영정책 §5)

---

## 1. 봇 계정 생성

1. 봇 전용 이메일 주소를 준비한다.
   - GitHub 계정 하나당 이메일 하나가 필요하다. 개인 이메일 말고 **팀 공용/그룹 메일**을 쓴다. (예: `oss-bot@회사도메인`)
   - 그룹 메일이 어려우면 IT에 별칭(alias) 메일을 요청한다.
2. 로그아웃(또는 시크릿 창) 상태에서 https://github.com/signup 접속
3. 위 봇 이메일로 새 계정을 만든다.
   - 사용자명 예: `obigo-oss-bot` (조직·용도가 드러나는 이름 권장)
4. 이메일 인증을 완료한다.

> 참고: GitHub 정책상 이런 machine user 계정은 허용된다. 단, 한 사람이 여러 개의 개인 무료 계정을 만드는 것은 금지이므로, 봇은 "개인의 부계정"이 아니라 **팀이 공동 관리하는 자동화 계정**으로 취급한다.

## 2. 봇 계정 보안 설정

봇 계정도 실제 권한을 갖는 계정이므로 보안을 갖춘다.

1. **2단계 인증(2FA) 활성화**: Settings → Password and authentication → Two-factor authentication
   - 인증 앱(TOTP) 사용 권장. 복구 코드는 팀 비밀 저장소(사내 vault 등)에 보관.
   - ⚠️ 봇의 2FA를 개인 휴대폰에만 걸면 그 사람 부재 시 잠긴다. 복구 수단을 팀이 공유 보관할 것.
2. 로그인 자격(이메일/비밀번호/복구코드)을 **팀 공용 비밀번호 관리도구**에 저장한다.

## 3. 공개 조직(OSS-TEST-GROUP)에 봇 초대

봇이 공개 저장소에 push하려면 조직 멤버여야 한다.

1. `OSS-TEST-GROUP` 조직 소유자 계정으로 로그인
2. 조직 → People → **Invite member** → 봇 사용자명(`obigo-oss-bot`) 초대
3. 봇 계정으로 로그인해 초대 수락

## 4. 저장소 권한 부여 (두 저장소 각각)

봇에게 각 공개 저장소에 대한 **Write** 역할을 준다. (feature 브랜치 push + PR 생성에 필요)

`OSS-TEST-GROUP/oss_multisw_framework` → Settings → Collaborators and teams → **Add people** → 봇 → Role: **Write**

`OSS-TEST-GROUP/oss_hmi_app` → 동일하게 **Write** 부여

> Write 권한이어도, `develop` branch protection에서 직접 push를 전면 차단하면 봇은 **feature/* push까지만** 가능하다. `develop` 반영은 반드시 PR + 사람 리뷰를 거친다. (정책 §6)

## 5. 봇 토큰(Fine-grained PAT) 발급

워크플로가 봇 자격으로 인증할 토큰을 만든다. **봇 계정으로 로그인한 상태에서** 진행한다.

1. 봇 계정 → Settings → Developer settings → **Personal access tokens** → **Fine-grained tokens** → **Generate new token**
2. 설정:
   - **Token name**: `oss-publish-token` (용도가 드러나게)
   - **Expiration**: 90일 등 만료일 지정 (무기한 비권장)
   - **Resource owner**: `OSS-TEST-GROUP` 선택
     - 조직이 fine-grained PAT를 허용하도록 설정돼 있어야 하고, 조직 소유자의 승인이 필요할 수 있다. (아래 6번 참고)
   - **Repository access**: **Only select repositories** → `oss_multisw_framework`, `oss_hmi_app` 두 개 선택
   - **Permissions** → Repository permissions:
     - **Contents**: Read and write  (브랜치 push용)
     - **Pull requests**: Read and write  (PR 자동 생성용)
     - **Metadata**: Read-only  (자동 필수 항목)
3. **Generate token** → 표시된 토큰 문자열을 **즉시 복사**한다. (다시 볼 수 없음)

## 6. 조직의 Fine-grained PAT 정책 확인 (필요 시)

5번에서 Resource owner로 조직 선택이 안 되거나 토큰이 대기 상태이면, 조직 설정을 확인한다.

`OSS-TEST-GROUP` → Settings → Third-party Access → **Personal access tokens**
- fine-grained token 허용 여부 확인
- "Require approval" 설정 시, 발급된 토큰을 조직 소유자가 **승인**해야 활성화됨 (Pending requests에서 승인)

## 7. 사내 저장소에 토큰 등록 (secret)

발급받은 토큰을 **사내 저장소(obigo-gov)** 의 secret으로 등록한다. secret은 저장소마다 독립이므로 **두 사내 저장소에 각각** 넣는다.

`obigo-gov/oss_multisw_framework` → Settings → Secrets and variables → Actions → **New repository secret**
- Name: `PUBLIC_BOT_TOKEN`
- Value: 5번에서 복사한 토큰

`obigo-gov/oss_hmi_app` → 동일하게 `PUBLIC_BOT_TOKEN` 등록

> 하나의 토큰이 두 공개 저장소에 모두 접근 가능하도록 5번에서 두 저장소를 선택했으므로, 같은 토큰 값을 두 사내 저장소 secret에 넣으면 된다.

## 8. 동작 확인

1. 사내 저장소에서 테스트 브랜치로 공개 흐름을 한 번 돌려본다.
   ```bash
   git checkout tracking
   git checkout -b publish/test-bot
   git commit --allow-empty -m "bot test"
   git push origin publish/test-bot
   ```
2. 워크플로 실행 로그에서 "Push to public" 단계가 성공하는지 확인
3. 공개 저장소에 `feature/test-bot` 브랜치와 PR이 생성됐는지 확인
4. 확인 후 테스트 브랜치·PR 정리

---

## 토큰 만료 관리 (운영 팁)

Fine-grained PAT는 만료일이 있으므로, 만료 전 갱신이 필요하다. 운영 부담을 줄이려면:

- 만료 1~2주 전 알림을 캘린더에 등록하거나
- **GitHub App 방식으로 전환**하면 단기 토큰이 자동 발급·회전되어 만료 관리가 사라진다. (초기 설정은 더 복잡. 워크플로 본체는 그대로 두고 토큰 획득 단계만 교체) — 장기 운영 시 권장

---

## 체크리스트

- [ ] 봇 전용 그룹 이메일 확보
- [ ] 봇 GitHub 계정 생성 (`obigo-oss-bot` 등)
- [ ] 봇 2FA 설정 + 복구 수단 팀 공유 보관
- [ ] `OSS-TEST-GROUP` 조직에 봇 초대·수락
- [ ] 두 공개 저장소에 봇 Write 권한 부여
- [ ] Fine-grained PAT 발급 (두 저장소, Contents+PR write)
- [ ] (필요 시) 조직 토큰 정책 승인
- [ ] 두 사내 저장소에 `PUBLIC_BOT_TOKEN` secret 등록
- [ ] 테스트 브랜치로 end-to-end 동작 확인
- [ ] 토큰 만료일 관리 방안 결정 (갱신 알림 또는 GitHub App 전환)
