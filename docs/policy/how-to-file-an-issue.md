# 이슈 작성 가이드 · How to File an Issue

> 대상: 공개 저장소 `OSS-TEST-GROUP/oss_multisw_framework`, `OSS-TEST-GROUP/oss_hmi_app` 에
> 이슈를 등록하려는 사용자·기여자.
> Audience: anyone filing an issue on the public repos above.
>
> 관련 문서: 템플릿 자체를 추가·수정하는 방법은 메인테이너용 [issue-template-guide.md](./issue-template-guide.md) 를 보세요.
> This is a *reporter* guide. To add/edit the templates themselves, see the maintainer guide.

---

## 0. 한눈에 · Quick start

1. 저장소 상단 **Issues → New issue** 를 누른다.
2. 뜨는 선택 화면에서 상황에 맞는 항목을 고른다 (아래 §1 표).
3. 폼의 **필수 항목(＊)** 을 채우고 제출한다. 나머지도 최대한 채우면 처리가 빨라진다.
4. 한국어·영어 아무거나 좋다. 외부 공개 저장소이므로 **한/영 병기**를 권장한다.

> Click **Issues → New issue**, pick the matching template, fill the required fields, submit.
> Korean or English is fine; bilingual is preferred on the public repos.

---

## 1. 무엇을 어디에 · Where does it go?

New issue 화면에는 **빈 이슈가 막혀 있고**, 아래 항목만 보인다. 상황에 맞게 고른다.

| 상황 · Situation | 고를 항목 · Choose | 비고 |
|---|---|---|
| 동작이 깨졌거나 예상과 다름 · Something broken/unexpected | 🐛 **Bug Report** (폼) | §3 |
| 새 기능·개선 제안 · New feature / improvement | ✨ **Feature Request** (폼) | §4 |
| 문서 오류·누락·개선 · Doc error/gap | 📖 **Documentation** (폼) | §4 |
| 사용법·설치 등 질문 · Usage / how-to question | ❓ **Discussions Q&A** (링크) | 이슈 아님 |
| 방향이 열린 아이디어·토론 · Open-ended idea | 💡 **Discussions Ideas** (링크) | 이슈 아님 |
| 보안 취약점 · Security vulnerability | 🔒 **Security advisory** (링크) | §5 — 공개 이슈 금지 |

> 질문/토론은 **Issue 가 아니라 Discussions** 로 간다. 잘못 고르면 담당자가 Discussion 으로 옮겨달라고 안내할 수 있다.
> Questions and open discussions belong in **Discussions**, not Issues.

---

## 2. 등록 전 체크리스트 · Before you file

- [ ] **중복 검색** — 같은 문제가 이미 있는지 Issues 를 검색했다. / Searched existing issues for duplicates.
- [ ] **최신에서 재현** — 최신 릴리스(또는 `develop`)에서도 재현되는지 확인했다. / Reproduces on the latest release or `develop`.
- [ ] **최소 재현** — 문제를 재현하는 최소 절차/입력을 정리했다. / Have minimal steps/input to reproduce.
- [ ] **민감정보 제거** — 로그·스크린샷에서 토큰·비밀번호·내부 URL 등을 지웠다. / Scrubbed secrets from logs/screenshots.

> 중복·재현 정보가 갖춰질수록 처리(triage)가 빨라진다. 확신이 없으면 그냥 등록해도 된다 — 담당자가 도와준다.

---

## 3. 좋은 버그 리포트 · A good bug report

Bug Report 폼의 필드와 채우는 요령. **＊ 는 필수.**

| 필드 · Field | 요령 · Tip |
|---|---|
| 사전 확인 · Preflight ＊ | 중복 검색 체크는 필수. 최신 재현 여부도 표시. |
| 관련 컴포넌트 · Component ＊ | 문제가 난 영역을 고른다 (repo별 목록 아래). 모르면 "기타·모름". |
| 예상 동작 · Expected ＊ | 무엇이 일어나야 했나. |
| 실제 동작 · Actual ＊ | 실제로 무엇이 일어났나 (에러 메시지 포함). |
| 재현 절차 · Steps ＊ | 1·2·3 순서로. 남이 그대로 따라 재현할 수 있게. |
| 버전 · Version ＊ | 릴리스 버전 또는 커밋 SHA (예: `v0.0.1`, `abc1234`). |
| 환경 · Environment | repo별로 다름 (아래). |
| 로그 · Logs | 관련 로그/스택트레이스. 코드블록으로 자동 표시됨. |

### 컴포넌트·환경은 repo마다 다르다 · Repo-specific fields

| 항목 | MMF (`oss_multisw_framework`) | HMI (`oss_hmi_app`) |
|---|---|---|
| 컴포넌트 | vehicle_info_mgr / ota_mgr / lifecycle_mgr / diagnostics / adsw_manager(ROS2) / adsw_manager(AGL) / ota_client / 빌드·CI / DDS·IDL / 기타 | adsw_monitor / hmi_launcher / lfc_monitor / ota_monitor / oss_launcher_bot / vehicle_data_test / 공통·인프라 / 기타 |
| 환경 필드 | 빌드 타깃(x86_64/aarch64) · 확장(ROS2/AGL) | Flutter 버전 · 실행 타깃(linux/android/web) · 보드/디바이스 |

**좋은 예 · Good example**

```
Component: ota_mgr (OtaManager)
Expected: OTA 업데이트가 100%에서 정상 완료되어야 한다. / OTA should finish at 100%.
Actual:  99%에서 멈추고 "checksum mismatch" 로그 후 롤백됨. / Stalls at 99%, rolls back after "checksum mismatch".
Steps:
  1. ./build.sh --aarch64 --set-extension-platform=ROS2
  2. ota_mgr 실행 후 <이미지> 배포
  3. 진행률 99%에서 실패 / fails at 99%
Version: v0.0.1
Build target: aarch64  /  Extension: ROS2
Logs: (붙여넣기 — 코드블록으로 표시됨)
```

---

## 4. 기능 제안 · 문서 이슈 · Feature / Documentation

**✨ Feature Request** — "무엇을 원한다"보다 **왜 필요한가(문제)** 를 먼저 적으면 채택 판단이 쉽다.
- 해결하려는 문제 ＊ / 제안 내용 ＊ / 고려한 대안 / 직접 PR 제출 의향 체크박스.
- 아직 흐릿한 아이디어라면 폼 대신 **Discussions Ideas** 가 낫다.

**📖 Documentation** — 어느 문서의 어느 부분인지 위치를 정확히.
- 문서 위치(경로/URL) ＊ / 무엇이 문제인가 ＊ / 제안하는 수정 / 직접 PR 제출 의향 체크박스.

> 두 폼 모두 **"직접 PR 제출 의향"** 체크박스가 있다. 체크하면 담당자가 기여 절차를 안내한다
> (외부 기여는 fork+PR → 담당자가 사내로 반영하는 흐름 — [external-contribution-guide.md](./external-contribution-guide.md)).

---

## 5. 보안 취약점은 공개 이슈로 올리지 마세요 · Security

취약점을 공개 이슈로 올리면 악용될 수 있다. 반드시 **비공개 신고 채널**을 쓴다.

- New issue 화면의 **🔒 Security advisory** 링크 → 비공개로 신고.
  - MMF: `https://github.com/OSS-TEST-GROUP/oss_multisw_framework/security/advisories/new`
  - HMI: `https://github.com/OSS-TEST-GROUP/oss_hmi_app/security/advisories/new`
- 자세한 정책은 저장소의 `SECURITY.md` 참고.

> Do **not** open a public issue for vulnerabilities — use the private Security advisory link.

---

## 6. 제출 후 흐름 · After you submit

1. 제출 시 유형 라벨(`bug`/`enhancement`/`documentation`) + **`needs-triage`** 가 자동으로 붙는다.
2. 담당자가 확인(triage)하여 우선순위·담당·후속(재현 요청, 추가 정보 요청 등)을 정한다.
3. 추가 정보 요청이 오면 이슈 코멘트로 답해 주세요. / Please respond in comments if more info is requested.
4. 수정은 **사내 `develop`** 에서 이뤄지고, Publish 흐름을 거쳐 공개 저장소에 반영된다.
   외부 기여(직접 고쳐서 PR)도 담당자가 사내로 가져와 반영한다.

---

## 7. 참고 · References

- 이슈 폼을 추가·수정하는 방법 (메인테이너): [issue-template-guide.md](./issue-template-guide.md)
- 외부 기여 처리 흐름 (담당자): [external-contribution-guide.md](./external-contribution-guide.md)
- 전체 운영 정책: [open-source-operation-policy.md](./open-source-operation-policy.md)
- GitHub 공식: "Creating an issue", "About issue and pull request templates"
