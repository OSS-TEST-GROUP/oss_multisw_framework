#!/bin/bash
# =============================================================================
# registry-auth.sh — 레지스트리(Harbor) 로그인 헬퍼 (source 해서 사용)
#
# 자격증명은 이 파일에 두지 않는다 (공개 저장소로 전달되는 파일).
#   · CI(Jenkins/Actions): Harbor 로봇 계정(oss-ci) 을 credentials/secret 으로 주입
#                          → REGISTRY_ID / REGISTRY_PWD 환경변수
#   · 개발자 로컬        : 미리 `docker login registry.obigo.com` 한 번만 해두면 된다
#                          (이미 로그인돼 있으면 이 헬퍼는 아무것도 하지 않는다)
#
# pull 은 인증이 필요 없다 — oss 프로젝트는 public(익명 pull 허용) 이다.
# 인증이 필요한 경우는 push 뿐이다.
# =============================================================================
REGISTRY_ADDRESS="${REGISTRY_ADDRESS:-registry.obigo.com}"

# push 용 로그인. 이미 로그인 상태면 건너뛴다.
registry_login() {
  if [ -f "${HOME}/.docker/config.json" ] \
     && grep -q "${REGISTRY_ADDRESS}" "${HOME}/.docker/config.json" 2>/dev/null; then
    return 0
  fi
  if [ -z "${REGISTRY_ID}" ] || [ -z "${REGISTRY_PWD}" ]; then
    echo "ERROR: ${REGISTRY_ADDRESS} 로그인 정보가 없습니다." >&2
    echo "       CI 는 로봇 계정(oss-ci) 자격증명을 REGISTRY_ID/REGISTRY_PWD 로 주입하고," >&2
    echo "       로컬은 'docker login ${REGISTRY_ADDRESS}' 를 먼저 실행하세요." >&2
    return 1
  fi
  printf '%s' "${REGISTRY_PWD}" | docker login "${REGISTRY_ADDRESS}" -u "${REGISTRY_ID}" --password-stdin
}
