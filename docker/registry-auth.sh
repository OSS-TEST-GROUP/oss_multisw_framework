#!/bin/bash
# =============================================================================
# registry-auth.sh — 레지스트리(Harbor) 로그인 헬퍼 (source 해서 사용)
#
# ossuser 단일 계정으로 pull·push 를 모두 수행한다. builder 등 관리자 계정은 쓰지 않는다.
#   자격증명: REGISTRY_ID / REGISTRY_PWD (기본 ossuser)
# =============================================================================
REGISTRY_ADDRESS="${REGISTRY_ADDRESS:-registry.obigo.com}"
REGISTRY_ID="${REGISTRY_ID:-ossuser}"
REGISTRY_PWD="${REGISTRY_PWD:-Ossuser1234}"

# ossuser 로그인 (pull·push 공용).
registry_login() {
  echo "${REGISTRY_PWD}" | docker login "${REGISTRY_ADDRESS}" -u "${REGISTRY_ID}" --password-stdin
}
