#include "otam_vehicle_service.h"

#include <iostream>
#include <utility>

constexpr const char*
    OtamVehicleService::kVehicleInfoRequestTopic;
constexpr const char*
    OtamVehicleService::kVehicleInfoResponseTopic;

OtamVehicleService::OtamVehicleService()
    : is_initialized_(false) {}

void OtamVehicleService::Initialize() {
  if (is_initialized_) {
    std::cout << "OtamVehicleService already initialized"
              << std::endl;
    return;
  }

  std::cout << "Initializing OtamVehicleService..." << std::endl;

  // Create DDS implementation for GetVehicleInfo (publisher)
  auto request_dds = std::make_shared<OtamDdsImpl>();
  get_vehicle_info_ = std::make_shared<OtaManager::GetVehicleInfoPubSubType>();

  request_dds->Create(kVehicleInfoRequestTopic, std::static_pointer_cast<void>(get_vehicle_info_), nullptr);
  RegisterDds(kVehicleInfoRequestTopic, request_dds);

  // Create DDS implementation for GetVehicleInfoResponse (subscriber)
  auto response_dds = std::make_shared<OtamDdsImpl>();
  get_vehicle_info_res_ = std::make_shared<OtaManager::GetVehicleInfoResponsePubSubType>();
  response_listener_ = std::make_shared<ResponseListener>(this);

  response_dds->Create(kVehicleInfoResponseTopic,
                       std::static_pointer_cast<void>(get_vehicle_info_res_),
                       std::static_pointer_cast<void>(response_listener_));

  RegisterDds(kVehicleInfoResponseTopic, response_dds);

  is_initialized_ = true;
  std::cout << "OtamVehicleService initialized successfully"
            << std::endl;
}

void OtamVehicleService::Start() {
    std::cout << "OtamVehicleService: Starting OtamVehicleService." << std::endl;
}

void OtamVehicleService::Stop() {
    std::cout << "OtamVehicleService: Stopping OtamVehicleService." << std::endl;
}

void OtamVehicleService::RequestVehicleInfo(
    int32_t request_id,
    const std::string& name) {
  if (!is_initialized_) {
    std::cerr << "Service not initialized. Call Initialize() first"
              << std::endl;
    return;
  }

  auto dds = GetDds(kVehicleInfoRequestTopic);
  if (!dds) {
    std::cerr << "DDS not found for topic: "
              << kVehicleInfoRequestTopic << std::endl;
    return;
  }

  // Create and publish request message
  auto request_msg = std::make_shared<OtaManager::GetVehicleInfo>();
  request_msg->id(request_id);
  request_msg->name(name);
  
  dds->Write(request_msg);
}

void OtamVehicleService::OnVehicleInfoResponseReceived(
    const OtaManager::GetVehicleInfoResponse& msg) {
  std::cout << "\n=== Vehicle Info Response Received ===" << "\n"
            << "  ID: " << msg.id() << "\n"
            << "  Name: " << msg.name() << "\n"
            << "  Value: " << msg.value() << "\n"
            << "  Return Code: "
            << static_cast<int>(msg.ret_code()) << "\n"
            << "======================================\n"
            << std::endl;

  // Process the response based on ret_code
  switch (msg.ret_code()) {
    case ReturnValue::SUCCESS:
      std::cout << "Successfully retrieved: " << msg.name()
                << " = " << msg.value() << std::endl;
      vehicle_info_[msg.name()] = msg.value();

      // Notify all registered callbacks
      for (const auto& callback : callbacks_) {
        if (callback) {
          callback(msg.id(), msg.name(), msg.value());
        }
      }
      break;
    case ReturnValue::FAIL:
      std::cerr << "Failed to retrieve: " << msg.name()
                << std::endl;
      break;
    case ReturnValue::UNKNOWN:
    default:
      std::cerr << "Unknown result for: " << msg.name() << std::endl;
      break;
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

void OtamVehicleService::RegisterDds(
    const std::string& topic_name,
    std::shared_ptr<OtamDdsImpl> dds) {
  dds_registry_[topic_name] = dds;
  std::cout << "Registered DDS for topic: " << topic_name
            << std::endl;
}

std::shared_ptr<OtamDdsImpl> OtamVehicleService::GetDds(
    const std::string& topic_name) {
  auto it = dds_registry_.find(topic_name);
  if (it != dds_registry_.end()) {
    return it->second;
  }
  return nullptr;
}

void OtamVehicleService::ResponseListener::on_data_available(eprosima::fastdds::dds::DataReader* reader) {
    // This callback is invoked by FastDDS when new registration data arrives
    OtaManager::GetVehicleInfoResponse message;
    eprosima::fastdds::dds::SampleInfo info;

    try {
        // Attempt to read the next available message
        if (reader->take_next_sample(&message, &info) == eprosima::fastdds::dds::RETCODE_OK) {
            if (info.valid_data) {
                // Forward the message to the service for processing
                service_->OnVehicleInfoResponseReceived(message);
            } else {
                std::cout << "ResponseListener: Received invalid data sample" << std::endl;
            }
        } else {
            std::cout << "ResponseListener: Failed to read message sample" << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "ResponseListener: Exception in data callback: " << e.what() << std::endl;
    }
}