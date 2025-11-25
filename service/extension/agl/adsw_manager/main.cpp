#include "ADSWManager.hpp"
#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>

using namespace adsw_manager::agl;

std::atomic<bool> running(true);

void SignalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\n[Main] Termination signal received. Shutting down..." << std::endl;
        running = false;
    }
}

int main(int argc, char* argv[]) {
    // 종료 신호 핸들러 등록
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);

    ADSWManager adsw_manager;

    if (!adsw_manager.Initialize()) {
        std::cerr << "[Main] ADSWManager initialization failed." << std::endl;
        return 1;
    }

    if (!adsw_manager.StartService()) {
        std::cerr << "[Main] Failed to start ADSWManager service." << std::endl;
        return 1;
    }

    std::cout << "[Main] ADSWManager running. Press Ctrl+C to exit." << std::endl;

    // 프로그램 유지 (1초 간격으로 대기)
    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // 종료 처리
    adsw_manager.StopService();

    std::cout << "[Main] ADSWManager stopped. Exiting." << std::endl;

    return 0;
}
