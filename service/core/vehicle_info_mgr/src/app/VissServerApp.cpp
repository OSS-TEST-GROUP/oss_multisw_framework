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
#include <optional>

#include "VissServerApp.h"
#include "grpc_transport.h"
#include "mqtt_transport.h"
#include "dds_transport.h"
#include "DDSProvider.h"
#include "CANProvider.h"
#include "NetworkProvider.h"
#include "VssTree.h"
#include "Utils.h"
#include "JsonSchemaValidator.h"

// ################### Class Implementations ###################

// VissServerApp
VissServerApp::VissServerApp(const std::string& himPath) {
    vss_tree = std::make_shared<viss::VSSTree>(himPath);
    if (!vss_tree->isValid()) {
        std::cerr << "Error: Failed to load or parse HIM file from " << himPath << std::endl;
        exit(1);
    }

    request_pool_ = std::make_shared<ObjectPool<RequestData>>();
    requestQueue = std::make_shared<ThreadSafeQueue<std::shared_ptr<RequestData>>>();
    service_manager = std::make_shared<ServiceManager>(requestQueue, vss_tree);

    // Add Providers
    service_manager->addProvider(std::make_unique<DDSProvider>());
    service_manager->addProvider(std::make_unique<CANProvider>());
    service_manager->addProvider(std::make_unique<NetworkProvider>());

    // Add Transports
    transports.push_back(std::make_unique<GrpcTransport>(request_pool_, requestQueue));
    transports.push_back(std::make_unique<MqttTransport>(request_pool_, requestQueue));
    transports.push_back(std::make_unique<DDSTransport>(request_pool_, requestQueue));
}

VissServerApp::~VissServerApp() {
    service_manager->stop();
    if (service_manager_thread.joinable()) {
        service_manager_thread.join();
    }
    for (auto& transport : transports) {
        transport->stop();
    }
}

void VissServerApp::run() {
    if (!vss_tree->isValid()) {
        std::cerr << "Failed to initialize VSS tree." << std::endl;
        return;
    }
    std::cout << "VSS tree initialized." << std::endl;

    service_manager_thread = std::thread(&ServiceManager::run, service_manager);

    std::vector<std::thread> transport_threads;
    for (auto& transport : transports) {
        transport_threads.emplace_back(&ITransport::start, transport.get());
    }
    std::cout << "Server components initialized." << std::endl;
    
    for (auto& th : transport_threads) {
        if (th.joinable()) {
            th.join();
        }
    }
}

// ################### Main Entry Point ###################
std::optional<std::string> getEnv(const std::string& key) {
    const char* val = std::getenv(key.c_str());
    return val ? std::optional<std::string>(val) : std::nullopt;
}

int main(int argc, char** argv) {
    // Default paths relative to the executable for a typical installation structure
    std::string configPath = getEnv("VEHICLE_CONFIG_PATH").value_or("config");
    std::string himPath = configPath.append("/viss.him"); 
    std::string schemaPath = configPath.append("/vissv3.0-schema.json");
    //std::string himPath = "config/viss.him"; 
    //std::string schemaPath = "config/vissv3.0-schema.json";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--him" && i + 1 < argc) {
            himPath = argv[++i];
        } else if (arg == "--schema" && i + 1 < argc) {
            schemaPath = argv[++i];
        }
    }

    // Initialize the schema validator before creating the app instance
    JsonSchemaValidator::init(schemaPath);

    VissServerApp app(himPath);
    app.run();
    return 0;
}
