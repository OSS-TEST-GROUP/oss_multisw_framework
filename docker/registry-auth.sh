#!/bin/bash
# =============================================================================
# registry-auth.sh — 레지스트리(Harbor) 인증 헬퍼 (source 해서 사용)
#
# 정책: 최소권한. 평소엔 pull 전용 계정(ossuser)으로 로그인하고, push 는 push
#       권한 계정으로 push 시점에만 잠깐 로그인(별도 루틴) 후 pull 계정으로 복귀.
#
# pull  계정: REGISTRY_ID / REGISTRY_PWD        (기본 ossuser — 저위험, pull 전용)
# push  계정: REGISTRY_PUSH_ID / REGISTRY_PUSH_PWD  ← env/secret 로 주입 (리포에 X)
#   당분간 builder(관리자)를 여기에 넣어 쓰되, Harbor 에 oss 프로젝트 push 전용
#   robot/Developer 계정을 만들면 그 값으로 교체(코드 변경 불필요).
#   예) export REGISTRY_PUSH_ID='robot$oss+pusher' REGISTRY_PUSH_PWD='<token>'
#       export REGISTRY_PUSH_ID=builder            REGISTRY_PUSH_PWD='<pwd>'   # 임시
# =============================================================================
REGISTRY_ADDRESS="${REGISTRY_ADDRESS:-registry.obigo.com}"
REGISTRY_ID="${REGISTRY_ID:-ossuser}"
REGISTRY_PWD="${REGISTRY_PWD:-Ossuser1234}"

# pull 계정(ossuser)으로 로그인 — 상시 기본. push 후 복귀에도 사용.
registry_pull_login() {
  echo "${REGISTRY_PWD}" | docker login "${REGISTRY_ADDRESS}" -u "${REGISTRY_ID}" --password-stdin
}

# push 계정으로 로그인 — push 직전에만 호출. env 미설정이면 실패(가드).
registry_push_login() {
  if [ -z "${REGISTRY_PUSH_ID:-}" ] || [ -z "${REGISTRY_PUSH_PWD:-}" ]; then
    echo "[registry-auth] ERROR: push 하려면 REGISTRY_PUSH_ID / REGISTRY_PUSH_PWD env 가 필요합니다." >&2
    echo "[registry-auth]   예) export REGISTRY_PUSH_ID=builder REGISTRY_PUSH_PWD='<pwd>'" >&2
    return 1
  fi
  echo "[registry-auth] push 계정으로 전환: ${REGISTRY_PUSH_ID}" >&2
  echo "${REGISTRY_PUSH_PWD}" | docker login "${REGISTRY_ADDRESS}" -u "${REGISTRY_PUSH_ID}" --password-stdin
}
