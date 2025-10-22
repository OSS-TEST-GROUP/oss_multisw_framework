#include "otam_diagnostic_service.h"

#include <iostream>
#include <utility>

constexpr const char* OtamDiagnosticService::kOtaMgrInfoTopic;

OtamDiagnosticService::OtamDiagnosticService()
    : is_initialized_(false) {}

void OtamDiagnosticService::Initialize() {
  if (is_initialized_) {
    std::cout << "OtamDiagnosticService already initialized"
              << std::endl;
    return;
  }

  std::cout << "Initializing OtamDiagnosticService..." << std::endl;

  // Create DDS implementation for NotifyOtaMgrInfo
  auto dds_impl = std::make_shared<OtamDdsImpl>();

  // Create DDS entities
  notify_ota_mgr_info_ = std::make_shared<OtaManager::NotifyOtaMgrInfoPubSubType>();
  dds_impl->Create(kOtaMgrInfoTopic, std::static_pointer_cast<void>(notify_ota_mgr_info_), nullptr);

  RegisterDds(kOtaMgrInfoTopic, dds_impl);

  is_initialized_ = true;
  std::cout << "OtamDiagnosticService initialized successfully"
            << std::endl;
}

void OtamDiagnosticService::Start() {
    std::cout << "OtamDiagnosticService: Starting OtamDiagnosticService." << std::endl;
}

void OtamDiagnosticService::Stop() {
    std::cout << "OtamDiagnosticService: Stopping OtamDiagnosticService." << std::endl;
}

void OtamDiagnosticService::PublishOtaMgrInfo(int32_t err_code, const std::string& err_msg) {
  if (!is_initialized_) {
    std::cerr << "Service not initialized. Call Initialize() first"
              << std::endl;
    return;
  }

  auto dds = GetDds(kOtaMgrInfoTopic);
  if (!dds) {
    std::cerr << "DDS not found for topic: " << kOtaMgrInfoTopic
              << std::endl;
    return;
  }

  // Create message
  auto msg = std::make_shared<OtaManager::NotifyOtaMgrInfo>();
  msg->err_code(err_code);
  msg->err_msg(err_msg);

  std::cout << "OtamDiagnosticService: Publishing OtaMgrInfo - err_code: "
            << err_code << ", err_msg: " << err_msg << std::endl;

  // Write through DDS
  dds->Write(msg);
}

void OtamDiagnosticService::RegisterDds(
    const std::string& topic_name,
    std::shared_ptr<OtamDdsImpl> dds) {
  dds_registry_[topic_name] = dds;
  std::cout << "Registered DDS for topic: " << topic_name
            << std::endl;
}

std::shared_ptr<OtamDdsImpl> OtamDiagnosticService::GetDds(
    const std::string& topic_name) {
  auto it = dds_registry_.find(topic_name);
  if (it != dds_registry_.end()) {
    return it->second;
  }
  return nullptr;
}