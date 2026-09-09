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
#include <set>

#include "VissServerApp.h"
#include "Logger.h"
#include "grpc_transport.h"
#ifndef BUILD_FOR_ANDROID
#include "mqtt_transport.h"
#include "dds_transport.h"
#include "DDSProvider.h"
#endif

#include "UdsProvider.h"
#include "GrpcProvider.h"
#include "VssTree.h"
#include "Utils.h"
#include "JsonSchemaValidator.h"
#include "viss_websocket_transport.h"
#include "ConfigManager.h"
#include "Utils.h"

// ################### Class Implementations ###################

// VissServerApp
VissServerApp::VissServerApp(const std::string& himPath) {
    vss_tree = std::make_shared<viss::VSSTree>(himPath);
    if (!vss_tree->isValid()) {
        LOG_ERROR << "Error: Failed to load or parse HIM file from " << himPath;
        exit(1);
    }

    request_pool_ = std::make_shared<ObjectPool<RequestData>>();
    requestQueue = std::make_shared<ThreadSafeQueue<std::shared_ptr<RequestData>>>();
    service_manager = std::make_shared<ServiceManager>(requestQueue, vss_tree);






    // Initialize ConfigManager from the loaded VSS tree
    auto& config = ConfigManager::getInstance();
    config.initializeFromTree(vss_tree);

    // Add Providers based on Config
    for (const auto& proto : config.provider.supportedProtocols) {
        if (proto == "Grpc") {
            service_manager->addProvider(std::make_unique<GrpcProvider>());
            LOG_INFO << "[VissServerApp] Added GrpcProvider";
        } else if (proto == "Uds") {
            service_manager->addProvider(std::make_unique<UdsProvider>());
            LOG_INFO << "[VissServerApp] Added UdsProvider";
        }
#ifndef BUILD_FOR_ANDROID
        else if (proto == "DDS") {
            service_manager->addProvider(std::make_unique<DDSProvider>());
            LOG_INFO << "[VissServerApp] Added DDSProvider";
        }

#endif
    }

    // Add Transports based on supported protocols in Config
    const auto& protocols = ConfigManager::getInstance().getSupportedProtocols();
    
    // Shared instance for transports that can handle multiple protocols
    std::shared_ptr<VissWebsocketTransport> ws_shared;

    for (const auto& proto : protocols) {
        if (proto == "gRPC") {
            transports[proto] = std::make_shared<GrpcTransport>(request_pool_, requestQueue);
            LOG_INFO << "[VissServerApp] Added GrpcTransport for protocol: " << proto;
        } else if (proto == "WSS" || proto == "UDS" || proto == "HTTPS" || proto == "MQTTS") {
            if (!ws_shared) {
                ws_shared = std::make_shared<VissWebsocketTransport>(request_pool_, requestQueue);
            }
            transports[proto] = ws_shared;
            LOG_INFO << "[VissServerApp] Mapping protocol " << proto << " to VissWebsocketTransport";
        } 
#ifndef BUILD_FOR_ANDROID
        else if (proto == "DDS") {
            transports[proto] = std::make_shared<DDSTransport>(request_pool_, requestQueue);
            LOG_INFO << "[VissServerApp] Added DDSTransport for protocol: " << proto;
        }
#endif
        else {
            LOG_WARN << "[VissServerApp] Protocol " << proto << " is requested but not yet implemented.";
        }
    }
}

VissServerApp::~VissServerApp() {
    stop(); // Ensure stop is always called.
    if (m_service_manager_thread.joinable()) {
        m_service_manager_thread.join();
    }
    LOG_INFO << "~VissServerApp() destructor finished.";
}

void VissServerApp::run() {
    bool already_running = false;
    // Atomically check if m_running is false, and if so, set it to true.
    if (!m_running.compare_exchange_strong(already_running, true)) {
        LOG_WARN << "VissServerApp::run() called while already running.";
        return;
    }

    if (!vss_tree->isValid()) {
        LOG_ERROR << "Failed to initialize VSS tree.";
        m_running = false;
        return;
    }
    LOG_INFO << "VSS tree initialized.";

    m_service_manager_thread = std::thread(&ServiceManager::run, service_manager);

    // Start all unique transports.
    std::set<std::shared_ptr<ITransport>> unique_transports;
    for (auto const& [key, val] : transports) {
        unique_transports.insert(val);
    }
    for (auto& transport : unique_transports) {
        transport->start();
    }
    LOG_INFO << "Server components initialized. Waiting for shutdown signal...";
    
    // Block here until stop() is called.
    int dummy;
    m_shutdown_queue.wait_and_pop(dummy);

    LOG_INFO << "VissServerApp::run() main loop finished.";
}

void VissServerApp::stop() {
    // Atomically check if m_running is true, and if so, set it to false.
    bool was_running = true;
    if (!m_running.compare_exchange_strong(was_running, false)) {
        // If it was already false, we are not running, so nothing to do.
        return;
    }

    LOG_INFO << "VissServerApp::stop() called.";

    // Stop all unique transports to unblock their threads.
    std::set<std::shared_ptr<ITransport>> unique_transports;
    for (auto const& [key, val] : transports) {
        unique_transports.insert(val);
    }
    for (auto& transport : unique_transports) {
        transport->stop();
    }

    // Stop the service manager to unblock its thread. This is a blocking call.
    if (service_manager) {
        service_manager->stop();
    }

    // Now that all components signaled by stop() are finished, join the main service thread.
    if (m_service_manager_thread.joinable()) {
        m_service_manager_thread.join();
    }

    // Unblock the run() method's main loop
    m_shutdown_queue.push(0);
    
    LOG_INFO << "VissServerApp::stop() completed.";
}
