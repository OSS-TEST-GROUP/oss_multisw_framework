#!/bin/sh
# 컨테이너 기동 시 `oss-domain-apply` 를 **호스트에** 설치한다.
#
# 왜 컨테이너가 호스트에 설치하는가
# ----------------------------------
# `oss-domain-apply` 는 도메인 변경을 반영하려면 **호스트에서 돌아야** 한다
# (재생성 대상 안에 실행 주체가 들어 있다 — 스크립트 헤더 참조). 그런데 그 파일을
# Yocto 이미지에만 넣으면 두 가지가 막힌다:
#
#   · 반영이 **재플래싱** 때까지 미뤄진다 — 컨테이너 업데이트로는 안 온다
#   · 151(컴퓨트 보드)은 **Ubuntu** 라 Yocto 패키징 범위 밖이다 (실측 22.04.5)
#
# 그래서 이미지에 스크립트를 담고, 기동할 때 호스트로 복사한다. 통로는 이미 있는
# `docker.sock` 이다 — `domain_agent` 가 호스트 `domain.env` 를 쓰는 것과 같은 방식
# (짧게 뜨는 컨테이너에 호스트 디렉토리를 붙이고, **본문은 stdin 으로만** 넘겨
# 셸 주입 여지를 없앤다. 헬퍼 이미지는 자기 이미지라 보드에 반드시 존재한다).
#
# 무엇을 어디에
# -------------
#   /opt/mmf/bin/oss-domain-apply                 실행 스크립트 (0755)
#   /etc/systemd/system/oss-domain-apply.service  실행 유닛
#   /etc/systemd/system/oss-domain-apply.path     domain.env 감시 트리거
#   /etc/systemd/system/multi-user.target.wants/oss-domain-apply.path  → enable
#
# 호스트 `/usr/bin` 은 건드리지 않는다 — 배포판 소유 디렉토리이고, 전용 경로면
# 충돌·추적 문제가 없다. 유닛의 ExecStart 가 이 경로를 가리킨다.
#
# ⚠️ 유닛은 **다음 재부팅부터** 활성화된다. systemd 는 새 유닛 파일을 `daemon-reload`
#    없이 반영하지 않고, 컨테이너에서 호스트 systemd 를 건드릴 수 없다 (실측 2026-09-08:
#    `--privileged --pid=host` 로도 /proc/1/ns/pid 접근 거부, 이미지에 systemctl 없음).
#    설치 순서를 바꿔도 마찬가지다. 그 전까지는 `/opt/mmf/bin/oss-domain-apply` 를
#    직접 실행하면 된다.
#
# 실패는 조용히 넘긴다 — 이 설치는 **편의 기능**이고, 실패해도 컨테이너 본체(adsw_manager
# 등)는 정상 기동해야 한다. docker.sock 이 없는 배포(개발 PC 등)가 흔하다.
set -e

SRC_DIR="${1:-/oss/host-install}"
SELF_CONTAINER="${MMF_SELF_CONTAINER:-}"
HOST_BIN_DIR=/opt/mmf/bin
HOST_UNIT_DIR=/etc/systemd/system

log() { echo "[install-domain-apply] $*"; }

[ -f "$SRC_DIR/oss-domain-apply.sh" ] || { log "소스 없음: $SRC_DIR — 건너뛴다"; exit 0; }
[ -S /var/run/docker.sock ] || { log "docker.sock 없음 — 건너뛴다"; exit 0; }
command -v docker >/dev/null 2>&1 || { log "docker CLI 없음 — 건너뛴다"; exit 0; }

# 자기 이미지를 찾는다. 잘 알려진 컨테이너 이름 → 이미지.
# (domain_agent::DiscoverSelfImage 와 같은 규칙)
find_self_image() {
    for n in $SELF_CONTAINER oss-mmf-ext-agl oss-mmf-ext-ros2; do
        [ -n "$n" ] || continue
        img=$(docker inspect "$n" --format '{{.Config.Image}}' 2>/dev/null) || continue
        [ -n "$img" ] && { echo "$img"; return 0; }
    done
    return 1
}

IMG=$(find_self_image) || { log "자기 이미지를 못 찾음 — 건너뛴다"; exit 0; }

# 호스트 경로에 파일 하나를 쓴다. 본문은 stdin.
#   $1 호스트 디렉토리 / $2 파일명 / $3 모드
put() {
    dir="$1"; base="$2"; mode="$3"
    docker run --rm -i --user 0:0 -v "$dir:/t" --entrypoint sh "$IMG" -c \
        "mkdir -p /t && cat > /t/$base.tmp && chmod $mode /t/$base.tmp && mv /t/$base.tmp /t/$base"
}

if put "$HOST_BIN_DIR" oss-domain-apply 0755 < "$SRC_DIR/oss-domain-apply.sh"; then
    log "설치: $HOST_BIN_DIR/oss-domain-apply"
else
    log "설치 실패(무시): $HOST_BIN_DIR/oss-domain-apply"; exit 0
fi

for u in oss-domain-apply.service oss-domain-apply.path; do
    [ -f "$SRC_DIR/$u" ] || continue
    if put "$HOST_UNIT_DIR" "$u" 0644 < "$SRC_DIR/$u"; then
        log "설치: $HOST_UNIT_DIR/$u"
    else
        log "설치 실패(무시): $u"
    fi
done

# enable 상당 — systemctl 없이 wants 심볼릭 링크를 만든다(이미지 빌드가 쓰는 방식).
# 다음 재부팅에 systemd 가 이 링크를 보고 .path 를 띄운다.
docker run --rm --user 0:0 -v "$HOST_UNIT_DIR:/t" --entrypoint sh "$IMG" -c \
    'mkdir -p /t/multi-user.target.wants \
     && ln -sf /etc/systemd/system/oss-domain-apply.path \
               /t/multi-user.target.wants/oss-domain-apply.path' \
    >/dev/null 2>&1 \
    && log "enable: multi-user.target.wants/oss-domain-apply.path (다음 재부팅부터)" \
    || log "enable 실패(무시)"

exit 0
