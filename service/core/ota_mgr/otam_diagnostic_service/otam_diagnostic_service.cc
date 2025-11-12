#include "otam_diagnostic_service.h"

#include <iostream>
#include <utility>

constexpr const char* OtamDiagnosticService::kOtaMgrInfoTopic;

OtamDiagnosticService::OtamDiagnosticService() {}

void OtamDiagnosticService::Initialize() {
  if (state_ != ServiceState::UNINITIALIZED) {
    std::cout << "OtamDiagnosticService already initialized"
              << std::endl;
    return;
  }
  name_ = "OtamDiagnosticService";

  std::cout << "Initializing OtamDiagnosticService..." << std::endl;

  // Create DDS implementation for NotifyOtaMgrInfo
  auto dds_impl = std::make_shared<OtamDdsImpl<OtaManager::NotifyOtaMgrInfo>>();

  // Create DDS entities
  notify_ota_mgr_info_ = std::make_shared<OtaManager::NotifyOtaMgrInfoPubSubType>();
  dds_impl->Create(kOtaMgrInfoTopic, std::static_pointer_cast<void>(notify_ota_mgr_info_), nullptr);

  RegisterDds(kOtaMgrInfoTopic, dds_impl);

  state_ = ServiceState::INITIALIZED;
  std::cout << "OtamDiagnosticService initialized successfully"
            << std::endl;
}

void OtamDiagnosticService::Start() {
  state_ = ServiceState::STARTED;
  std::cout << "OtamDiagnosticService: Starting OtamDiagnosticService." << std::endl;
}

void OtamDiagnosticService::Stop() {
    std::cout << "OtamDiagnosticService: Stopping OtamDiagnosticService." << std::endl;

    // Destroy all DDS instances
    for (auto& pair : dds_registry_) {
        if (pair.second) {
            std::cout << "OtamDiagnosticService: Destroying DDS for topic: " << pair.first << std::endl;
            pair.second->Destroy();
        }
    }
    state_ = ServiceState::UNINITIALIZED;
}

void OtamDiagnosticService::PublishOtaMgrInfo(int32_t err_code, const std::string& err_msg) {
  if (state_ != ServiceState::STARTED) {
    std::cout << "Service not initialized. Call Initialize() first"
              << std::endl;
    return;
  }

  auto dds = GetDds(kOtaMgrInfoTopic);
  if (!dds) {
    std::cout << "DDS not found for topic: " << kOtaMgrInfoTopic
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
  dds->Write(std::static_pointer_cast<void>(msg));
}

void OtamDiagnosticService::RegisterDds(
    const std::string& topic_name,
    std::shared_ptr<OtamDds> dds) {
  dds_registry_[topic_name] = dds;
  std::cout << "Registered DDS for topic: " << topic_name
            << std::endl;
}

std::shared_ptr<OtamDds> OtamDiagnosticService::GetDds(
    const std::string& topic_name) {
  auto it = dds_registry_.find(topic_name);
  if (it != dds_registry_.end()) {
    return it->second;
  }
  return nullptr;
}