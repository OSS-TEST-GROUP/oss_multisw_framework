#include <iostream>
#include <thread>
#include <csignal>
#include <atomic>

#include "otam_main.h"
#include "otam_common/otam_logger.h"

static std::atomic<bool> g_shutdown_requested{false};

static void SignalHandler(int /*sig*/) {
    g_shutdown_requested.store(true);
}

int main(int argc, char* argv[]) {
    std::cout << "Starting OtaManager" << std::endl;

    std::signal(SIGINT,  SignalHandler);
    std::signal(SIGTERM, SignalHandler);

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
    OtamLogger::Shutdown();
    return 0;
}
