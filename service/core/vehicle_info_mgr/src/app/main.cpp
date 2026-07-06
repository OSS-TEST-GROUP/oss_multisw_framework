#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <thread>
#include <chrono>
#include <map>
#include <any>
#include <algorithm>
#include <typeinfo>
#include <filesystem>
#include <csignal>
#include <optional>

#include "VissServerApp.h"
#include "Logger.h"
#include "JsonSchemaValidator.h"

// ################### Signal Handling ###################
static VissServerApp* g_app_ptr = nullptr;

void signal_handler(int signum) {
    LOG_WARN << "Interrupt signal (" << signum << ") received. Shutting down..." << g_app_ptr;
    if (g_app_ptr) {
        g_app_ptr->stop();
    }
}

// ################### Main Entry Point (for non-Android builds) ###################
#ifndef BUILD_FOR_ANDROID
std::optional<std::string> getEnv(const std::string& key) {
    const char* val = std::getenv(key.c_str());
    return val ? std::optional<std::string>(val) : std::nullopt;
}

int main(int argc, char** argv) {
    // Default paths relative to the executable for a typical installation structure
    std::string configPath = getEnv("VEHICLE_CONFIG_PATH").value_or("config");
    std::string himPath = configPath + "/viss.him"; 
    std::string schemaPath = configPath + "/vissv3.0-schema.json";
    //std::string grpcProviderAddr = getEnv("GRPC_PROVIDER_ADDR").value_or("127.0.0.1:50052");

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--him" && i + 1 < argc) {
            himPath = argv[++i];
        } else if (arg == "--schema" && i + 1 < argc) {
            schemaPath = argv[++i];
        } else {
            LOG_ERROR << "Error: Unknown or invalid argument: " << arg;
            exit(1);
        }
    }

    // Initialize the schema validator before creating the app instance
    JsonSchemaValidator::init(schemaPath);

    VissServerApp app(himPath);
    g_app_ptr = &app;
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    LOG_INFO << "Starting VISS Server. Press Ctrl+C to exit.";
    app.run();
    
    LOG_INFO << "VISS Server has shut down gracefully.";
    g_app_ptr = nullptr;
    return 0;
}
#endif
