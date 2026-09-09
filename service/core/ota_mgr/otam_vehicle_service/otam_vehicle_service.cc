#include "otam_vehicle_service.h"
#include "otam_diagnostic_service/otam_diagnostic_service.h"

#include <iostream>
#include <utility>
#include <nlohmann/json.hpp>

constexpr const char*
    OtamVehicleService::kVissRequestTopic;
constexpr const char*
    OtamVehicleService::kVehicleInfoResponseTopic;

OtamVehicleService::OtamVehicleService() {}

void OtamVehicleService::SetOtamDiagnosticService(OtamDiagnosticService* svc) {
  diagnostic_service_ = svc;
}

// LCOV_EXCL_START — 프레임워크 DDS 경계(Initialize/Start/Stop/RequestVehicleInfo):
// 실 DomainParticipant/DataWriter/Reader 를 직접 구동. 유닛 불가 → 통합 레인이 검증.
void OtamVehicleService::Initialize() {
  if (state_ != ServiceState::UNINITIALIZED) {
    std::cout << "OtamVehicleService already initialized"
              << std::endl;
    return;
  }
  name_ = "OtamVehicleService";

  std::cout << "Initializing OtamVehicleService..." << std::endl;

    // Create DDS implementation for VissRequest (publisher)
  auto viss_request_dds = std::make_shared<OtamDdsImpl<VissRequestTopic>>();
  viss_request_ = std::make_shared<VissRequestTopicPubSubType>();

  viss_request_dds->Create(kVissRequestTopic,
                           std::static_pointer_cast<void>(viss_request_),
                           nullptr);
  RegisterDds(kVissRequestTopic, viss_request_dds);

  // Create DDS implementation for VISS response (subscriber)
  auto response_dds = std::make_shared<OtamDdsImpl<VissRequestTopic>>();
  viss_response_ = std::make_shared<VissRequestTopicPubSubType>();
  response_listener_ = std::make_shared<ResponseListener>(this);

  response_dds->Create(kVehicleInfoResponseTopic,
                       std::static_pointer_cast<void>(viss_response_),
                       std::static_pointer_cast<void>(response_listener_));

  RegisterDds(kVehicleInfoResponseTopic, response_dds);

  state_ = ServiceState::INITIALIZED;
  std::cout << "OtamVehicleService initialized successfully"
            << std::endl;
}

void OtamVehicleService::Start() {
  state_ = ServiceState::STARTED;
  std::cout << "OtamVehicleService: Starting OtamVehicleService." << std::endl;
}

void OtamVehicleService::Stop() {
    std::cout << "OtamVehicleService: Stopping OtamVehicleService." << std::endl;

    // Destroy all DDS instances
    for (auto& pair : dds_registry_) {
        if (pair.second) {
            std::cout << "OtamVehicleService: Destroying DDS for topic: " << pair.first << std::endl;
            pair.second->Destroy();
        }
    }
    state_ = ServiceState::UNINITIALIZED;
}

void OtamVehicleService::RequestVehicleInfo(
    int32_t request_id,
    const std::string& name) {
  if (state_ != ServiceState::STARTED) {
    std::string err_msg = "Failed: RequestVehicleInfo called before service is started";
    std::cerr << "OtamVehicleService: " << err_msg << std::endl;
    if (diagnostic_service_) {
      diagnostic_service_->PublishOtaMgrInfo(static_cast<int32_t>(OtamError::FAILED), err_msg);
    }
    return;
  }

  auto dds = GetDds(kVissRequestTopic);
  if (!dds) {
    std::string err_msg = std::string("Failed: DDS not found for topic: ") + kVissRequestTopic;
    std::cerr << "OtamVehicleService: " << err_msg << std::endl;
    if (diagnostic_service_) {
      diagnostic_service_->PublishOtaMgrInfo(static_cast<int32_t>(OtamError::FAILED), err_msg);
    }
    return;
  }

  // Generate unique request ID string
  std::string request_id_str = "otam-" + std::to_string(request_id);

  // Create response topic name (using kVehicleInfoResponseTopic as response channel)
  std::string response_topic_name = kVehicleInfoResponseTopic;

  // Create inner request JSON
  nlohmann::json request_inner = {
      {"action", "get"},
      {"path", name},
      {"requestId", request_id_str}
  };

  // Create outer request JSON with topic and request payload
  nlohmann::json request_outer = {
      {"topic", response_topic_name},
      {"request", request_inner.dump()}
  };

  // Create VissRequestTopic message and set payload
  auto viss_msg = std::make_shared<VissRequestTopic>();
  viss_msg->payload(request_outer.dump());

  std::cout << "Sending VissRequest to " << kVissRequestTopic << std::endl;
  std::cout << "Payload: " << request_outer.dump(2) << std::endl;

  // Send the message via DDS
  dds->Write(std::static_pointer_cast<void>(viss_msg));
}

// LCOV_EXCL_STOP
void OtamVehicleService::OnVehicleInfoResponseReceived(
    int32_t request_id,
    const std::string& name,
    const std::string& value) {
  std::cout << "\n=== VISS Vehicle Info Response Received ===" << "\n"
            << "  Request ID: " << request_id << "\n"
            << "  Path (Name): " << name << "\n"
            << "  Value: " << value << "\n"
            << "==========================================\n"
            << std::endl;

  // Store the vehicle info
  if (!name.empty() && !value.empty()) {
    std::cout << "Successfully retrieved: " << name
              << " = " << value << std::endl;
    vehicle_info_[name] = value;

    // Notify all registered callbacks
    for (const auto& callback : callbacks_) {
      if (callback) {
        callback(request_id, name, value);
      }
    }
  } else {
    std::string err_msg = "Failed: VISS response has empty name or value (request_id=" + std::to_string(request_id) + ")";
    std::cerr << "OtamVehicleService: " << err_msg << std::endl;
    if (diagnostic_service_) {
      diagnostic_service_->PublishOtaMgrInfo(static_cast<int32_t>(OtamError::FAILED), err_msg);
    }
  }
}

std::string OtamVehicleService::GetVehicleInfo(const std::string& name) {
  auto it = vehicle_info_.find(name);
  if (it != vehicle_info_.end()) {
    return it->second;
  }
  return "";
}

void OtamVehicleService::RegisterCallback(VehicleInfoCallback callback) {
  if (callback) {
    callbacks_.push_back(callback);
    std::cout << "OtamVehicleService: Callback registered" << std::endl;
  }
}

// LCOV_EXCL_START — DDS 레지스트리/리스너 경계(RegisterDds/GetDds/ResponseListener):
// 실 DataReader 콜백. 유닛 불가 → 통합 레인이 검증. (파일 끝까지 제외)
void OtamVehicleService::RegisterDds(
    const std::string& topic_name,
    std::shared_ptr<OtamDdsImpl<VissRequestTopic>> dds) {
  dds_registry_[topic_name] = dds;
  std::cout << "Registered DDS for topic: " << topic_name
            << std::endl;
}

std::shared_ptr<OtamDdsImpl<VissRequestTopic>> OtamVehicleService::GetDds(
    const std::string& topic_name) {
  auto it = dds_registry_.find(topic_name);
  if (it != dds_registry_.end()) {
    return it->second;
  }
  return nullptr;
}

void OtamVehicleService::ResponseListener::on_data_available(eprosima::fastdds::dds::DataReader* reader) {
    // This callback is invoked by FastDDS when new VISS response arrives
    VissRequestTopic message;
    eprosima::fastdds::dds::SampleInfo info;

    try {
        // Attempt to read the next available message
        if (reader->take_next_sample(&message, &info) == eprosima::fastdds::dds::RETCODE_OK) {
            if (info.valid_data) {
                std::cout << "ResponseListener: Received VISS response" << std::endl;
                std::cout << "Payload: " << message.payload() << std::endl;

                try {
                    // Parse the JSON payload
                    nlohmann::json response_json = nlohmann::json::parse(message.payload());

                    // Extract fields from VISS response
                    // Expected format: {"action":"get", "requestId":"...", "path":"...", "value":"...", ...}
                    std::string action = response_json.value("action", "");
                    std::string request_id = response_json.value("requestId", "");
                    std::string path = response_json.value("path", "");
                    std::string value = response_json.value("value", "");

                    // Extract numeric request ID from "otam-<id>" format
                    int32_t id = 0;
                    if (request_id.find("otam-") == 0) {
                        id = std::stoi(request_id.substr(5));
                    }

                    std::cout << "Parsed VISS response: action=" << action
                              << ", requestId=" << request_id
                              << ", path=" << path
                              << ", value=" << value << std::endl;

                    // Forward to service for processing
                    service_->OnVehicleInfoResponseReceived(id, path, value);

                } catch (const nlohmann::json::parse_error& e) {
                    std::string err_msg = "ResponseListener: JSON parse error: " + std::string(e.what());
                    std::cerr << err_msg << std::endl;
                    if (service_->diagnostic_service_) {
                        service_->diagnostic_service_->PublishOtaMgrInfo(static_cast<int32_t>(OtamError::FAILED), err_msg);
                    }
                } catch (const std::exception& e) {
                    std::string err_msg = "ResponseListener: Error processing response: " + std::string(e.what());
                    std::cerr << err_msg << std::endl;
                    if (service_->diagnostic_service_) {
                        service_->diagnostic_service_->PublishOtaMgrInfo(static_cast<int32_t>(OtamError::FAILED), err_msg);
                    }
                }
            } else {
                std::cout << "ResponseListener: Received invalid data sample" << std::endl;
            }
        } else {
            std::cout << "ResponseListener: Failed to read message sample" << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::string err_msg = "ResponseListener: Exception in data callback: " + std::string(e.what());
        std::cerr << err_msg << std::endl;
        if (service_->diagnostic_service_) {
            service_->diagnostic_service_->PublishOtaMgrInfo(static_cast<int32_t>(OtamError::FAILED), err_msg);
        }
    }
}