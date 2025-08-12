#include "DiagnosticManager.hpp"

#include "logger/Log.hpp"

namespace diagnostic_manager {

DiagnosticManager::DiagnosticManager() {}
DiagnosticManager::~DiagnosticManager() {}

bool DiagnosticManager::Initialize() {
    service_executor_ = std::make_shared<ServiceExecutor>();
    if (!service_executor_->Initialize()) {
        LOG_ERROR("Failed to initialize the ServiceExecutor.");
        return false;
    }

    message_transmitter_ = std::make_shared<MessageTransmitter>(service_executor_);
    if (!message_transmitter_->Initialize()) {
        LOG_ERROR("Failed to initialize the MessageTransmitter.");
        return false;
    }

    diag_info_registry_ = std::make_shared<DiagInfoRegistry>();

    diagnostic_analyzer_ = std::make_shared<DiagnosticAnalyzer>(service_executor_, message_transmitter_, diag_info_registry_);
    if (!diagnostic_analyzer_->Initialize()) {
        LOG_ERROR("Failed to initialize the DiagnosticAnalyzer.");
    }

    diagnostic_publisher_ = std::make_shared<DiagnosticPublisher>(service_executor_, message_transmitter_, diag_info_registry_);
    if (!diagnostic_publisher_->Initialize()) {
        LOG_ERROR("Failed to initialize the DiagnosticPublisher.");
    }

    return true;
}

bool DiagnosticManager::Start() {
    if (!service_executor_->Start()) {
        LOG_ERROR("Failed to start the ServiceExecutor.");
        return false;
    }
    return true;
}
void DiagnosticManager::Stop() {}

} // namespace diagnostic_manager
