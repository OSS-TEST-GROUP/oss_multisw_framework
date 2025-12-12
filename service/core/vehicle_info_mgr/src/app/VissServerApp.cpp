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
#include "grpc_transport.h"
#ifndef BUILD_FOR_ANDROID
#include "mqtt_transport.h"
#include "dds_transport.h"
#include "DDSProvider.h"
#endif
#include "CANProvider.h"
#include "NetworkProvider.h"
#include "GrpcProvider.h"
#include "VssTree.h"
#include "Utils.h"
#include "JsonSchemaValidator.h"

// ################### Class Implementations ###################

// VissServerApp
VissServerApp::VissServerApp(const std::string& himPath, const std::string& grpc_provider_addr) 
    : m_grpc_provider_addr(grpc_provider_addr) {
    vss_tree = std::make_shared<viss::VSSTree>(himPath);
    if (!vss_tree->isValid()) {
        LOG_ERROR << "Error: Failed to load or parse HIM file from " << himPath;
        exit(1);
    }

    request_pool_ = std::make_shared<ObjectPool<RequestData>>();
    requestQueue = std::make_shared<ThreadSafeQueue<std::shared_ptr<RequestData>>>();
    service_manager = std::make_shared<ServiceManager>(requestQueue, vss_tree);

    // Add Providers
#ifndef BUILD_FOR_ANDROID
    service_manager->addProvider(std::make_unique<DDSProvider>());
    service_manager->addProvider(std::make_unique<CANProvider>());
#endif
    //service_manager->addProvider(std::make_unique<NetworkProvider>());
    service_manager->addProvider(std::make_unique<GrpcProvider>(m_grpc_provider_addr));

    // Add Transports
    transports.push_back(std::make_unique<GrpcTransport>(request_pool_, requestQueue));
#ifndef BUILD_FOR_ANDROID
    //transports.push_back(std::make_unique<MqttTransport>(request_pool_, requestQueue));
    transports.push_back(std::make_unique<DDSTransport>(request_pool_, requestQueue));
#endif
}

VissServerApp::~VissServerApp() {
    stop(); // Ensure stop is always called.
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

    // Start all transports.
    for (auto& transport : transports) {
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

    // Stop all transports to unblock their threads. These are blocking calls.
    for (auto& transport : transports) {
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
