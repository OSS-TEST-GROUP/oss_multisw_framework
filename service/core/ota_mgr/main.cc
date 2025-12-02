#include <iostream>
#include <thread>

#include "otam_main.h"

int main(int argc, char* argv[]) {
    std::cout << "Starting OtaManager" << std::endl;

    OtamMain manager;

    try {
        if (argc > 1) {
            int sleep_duration = std::stoi(argv[1]);
            std::cout << "Sleeping for " << sleep_duration << " seconds before initialization." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(sleep_duration));
        }

        manager.Initialize();
        manager.Start();
        while (true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    } catch (const std::exception& e) {
        std::string error_msg = "Failed to publish manager ready notification: " + std::string(e.what());
    }
    manager.Stop();
    return 0;
}
