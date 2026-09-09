// domain_agent — 원격(HMI 보드) Settings 가 정한 도메인을 이 호스트의 도메인
// 파일에 반영한다. 설계와 경계는 domain_agent.h 참조.
//
// 사용법:  domain_agent [<idl_root>]
//   idl_root  : std_msgs/String.idl 을 찾을 루트 (기본 "idl", 컨테이너는 /oss/bin/idl)
//   도메인 파일: MMF_DOMAIN_ENV_FILE (기본 /opt/mmf/config/domain.env)
#include "domain_agent.h"

#include <chrono>
#include <cstdio>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>

namespace {
volatile std::sig_atomic_t g_stop = 0;
void OnSignal(int) { g_stop = 1; }
}  // namespace

int main(int argc, char** argv) {
    std::string idl_root = "idl";
    if (argc > 1) idl_root = argv[1];

    std::string env_path = "/opt/mmf/config/domain.env";
    if (const char* e = std::getenv("MMF_DOMAIN_ENV_FILE")) {
        if (*e) env_path = e;
    }

    // 컨테이너 로그가 파이프라 stdout 이 블록 버퍼링된다 — 기동/반영 로그가 크래시나
    // 종료 전까지 안 보이면 보드에서 디버깅이 불가능하다(실측: 성공 로그가 안 나오고
    // 실패 로그만 stderr 로 즉시 보였다). 줄 단위로 흘린다.
    setvbuf(stdout, nullptr, _IOLBF, 0);

    std::signal(SIGINT, OnSignal);
    std::signal(SIGTERM, OnSignal);

    DomainAgent agent;
    if (!agent.Initialize(idl_root, env_path)) {
        std::cerr << "[domain_agent] 초기화 실패 — 종료\n";
        return 1;
    }

    // 수신은 리스너 콜백에서 처리된다. 주기적으로 상태를 다시 발행하지는 않는다 —
    // 상태 토픽은 TRANSIENT_LOCAL latch 라 늦게 붙는 구독자도 마지막 값을 받는다.
    while (!g_stop) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    std::cout << "[domain_agent] 종료 신호 — 정리\n";
    return 0;
}
