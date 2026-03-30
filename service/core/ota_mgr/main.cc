#include <iostream>
#include <thread>
#include <atomic>
#include <csignal>
#include <pthread.h>

#include "otam_main.h"
#include "otam_common/otam_logger.h"

static std::atomic<bool> g_shutdown_requested{false};

// FastDDS(boost::asio)가 내부적으로 SIGINT/SIGTERM signal_set을 등록하여
// std::signal() 핸들러를 무효화하는 문제를 방지.
// main 스레드에서 시그널을 블로킹한 뒤 전용 스레드에서 sigwait()으로 수신.
static void SignalThreadFn() {
    sigset_t wait_set;
    sigemptyset(&wait_set);
    sigaddset(&wait_set, SIGINT);
    sigaddset(&wait_set, SIGTERM);

    int sig = 0;
    sigwait(&wait_set, &sig);
    std::cout << "OtaManager: signal " << sig << " received, shutting down." << std::endl;
    g_shutdown_requested.store(true);
}

int main(int argc, char* argv[]) {
    std::cout << "Starting OtaManager" << std::endl;

    // SIGINT/SIGTERM을 모든 스레드에서 블로킹한 뒤 전용 스레드에서만 수신
    {
        sigset_t block_set;
        sigemptyset(&block_set);
        sigaddset(&block_set, SIGINT);
        sigaddset(&block_set, SIGTERM);
        pthread_sigmask(SIG_BLOCK, &block_set, nullptr);
    }
    std::thread sig_thread(SignalThreadFn);

    OtamLogger::Init();  // log to OTAM_LOG_FILE or logs/otamanager.log
    OtamMain manager;

    try {
        manager.Initialize();
        manager.Start();
        while (!g_shutdown_requested.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        std::cout << "OtaManager: shutdown signal received." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "OtaManager: fatal error: " << e.what() << std::endl;
    }
    manager.Stop();
    sig_thread.join();
    OtamLogger::Shutdown();
    return 0;
}
