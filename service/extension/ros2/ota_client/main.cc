#include <thread>
#include <cstdlib>
#include <string>
#include <iostream>

#include "otac_main/OtacMain.hpp"

using namespace ros2::ota_client;

static std::string resolve_config_path(int argc, char* argv[]) {
    // Priority: --config <path> > OTA_CLIENT_CONFIG env > default path
    for (int i = 1; i < argc - 1; ++i) {
        if (std::string(argv[i]) == "--config") {
            return std::string(argv[i + 1]);
        }
    }

    const char* env = std::getenv("OTA_CLIENT_CONFIG");
    if (env && env[0] != '\0') {
        return std::string(env);
    }

    return "config/ota_client_config.json";
}

int main(int argc, char* argv[]) {

    std::string config_path = resolve_config_path(argc, argv);
    std::cout << "[ota_client] config path: " << config_path << "\n";

    OtacMain otac_main_;
    if (otac_main_.Initialize(config_path)) {
        otac_main_.Start();
    } else {
        otac_main_.Stop();
    }

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
