#include "otam_actor_service.h"
#include "otam_server_service/otam_server_service.h"
#include "otam_client_service/otam_client_service.h"
#include "otam_vehicle_service/otam_vehicle_service.h"
#include "otam_diagnostic_service/otam_diagnostic_service.h"

#include <iostream>
#include <memory>
#include <stdexcept>
#include <thread>
#include <chrono>
#include <nlohmann/json.hpp>

OtamActorService::OtamActorService() {
  std::cout << "OtamActorService: Constructor called" << std::endl;
}

OtamActorService::~OtamActorService() {
  std::cout << "OtamActorService: Destructor called" << std::endl;
  dds_registry_.clear();
}

void OtamActorService::SetOtamServerService(OtamServerService* server_service) {
  otam_server_service_ = server_service;
  std::cout << "OtamActorService: OtamServerService connected" << std::endl;
}

void OtamActorService::SetOtamClientService(OtamClientService* client_service) {
  otam_client_service_ = client_service;
  std::cout << "OtamActorService: OtamClientService connected" << std::endl;
}

void OtamActorService::SetOtamVehicleService(OtamVehicleService* vehicle_service) {
  otam_vehicle_service_ = vehicle_service;
  std::cout << "OtamActorService: OtamVehicleService connected" << std::endl;

  // Register callback for vehicle info responses
  if (otam_vehicle_service_) {
    otam_vehicle_service_->RegisterCallback(
        [this](int32_t request_id, const std::string& name, const std::string& value) {
          this->OnVehicleInfoReceived(request_id, name, value);
        });
  }
}

void OtamActorService::SetOtamDiagnosticService(OtamDiagnosticService* diagnostic_service) {
  otam_diagnostic_service_ = diagnostic_service;
  std::cout << "OtamActorService: OtamDiagnosticService connected" << std::endl;
}

void OtamActorService::Initialize() {
    if (state_ != ServiceState::UNINITIALIZED) {
      std::cout << "OtamActorService already initialized"
                << std::endl;
      return;
    }
    name_ = "OtamActorService";

    try {

    // Initialize GetAppUpdateSettings (Request) - raw DDS + ROS2/rosbridge
    auto get_settings_type =
        std::make_shared<OtaManager::GetAppUpdateSettingsPubSubType>();
    get_app_update_settings_listener_ = std::make_shared<GetAppUpdateSettingsListener>(this);
    CreateDdsForTopic<OtaManager::GetAppUpdateSettings>(kTopicGetAppUpdateSettings,
                      get_settings_type, get_app_update_settings_listener_);
    CreateDdsForTopic<OtaManager::GetAppUpdateSettings>(kTopicRosGetAppUpdateSettings,
                      get_settings_type, get_app_update_settings_listener_);

    // Initialize GetAppUpdateSettingsResponse (Response) - raw DDS + ROS2/rosbridge
    auto get_settings_resp_type =
        std::make_shared<OtaManager::GetAppUpdateSettingsResponsePubSubType>();
    CreateDdsForTopic<OtaManager::GetAppUpdateSettingsResponse>(kTopicGetAppUpdateSettingsResponse,
                      get_settings_resp_type);
    CreateDdsForTopic<OtaManager::GetAppUpdateSettingsResponse>(kTopicRosGetAppUpdateSettingsResponse,
                      get_settings_resp_type);

    // Initialize SetAppUpdateSettings (Request) - raw DDS + ROS2/rosbridge
    auto set_settings_type =
        std::make_shared<OtaManager::SetAppUpdateSettingsPubSubType>();
    set_app_update_settings_listener_ = std::make_shared<SetAppUpdateSettingsListener>(this);
    CreateDdsForTopic<OtaManager::SetAppUpdateSettings>(kTopicSetAppUpdateSettings,
                      set_settings_type, set_app_update_settings_listener_);
    CreateDdsForTopic<OtaManager::SetAppUpdateSettings>(kTopicRosSetAppUpdateSettings,
                      set_settings_type, set_app_update_settings_listener_);

    // Initialize SetAppUpdateSettingsResponse (Response) - raw DDS + ROS2/rosbridge
    auto set_settings_resp_type =
        std::make_shared<OtaManager::SetAppUpdateSettingsResponsePubSubType>();
    CreateDdsForTopic<OtaManager::SetAppUpdateSettingsResponse>(kTopicSetAppUpdateSettingsResponse,
                      set_settings_resp_type);
    CreateDdsForTopic<OtaManager::SetAppUpdateSettingsResponse>(kTopicRosSetAppUpdateSettingsResponse,
                      set_settings_resp_type);

    // Initialize GetAppUpdateState (Request) - raw DDS + ROS2/rosbridge
    auto get_state_type =
        std::make_shared<OtaManager::GetAppUpdateStatePubSubType>();
    get_app_update_state_listener_ = std::make_shared<GetAppUpdateStateListener>(this);
    CreateDdsForTopic<OtaManager::GetAppUpdateState>(kTopicGetAppUpdateState, get_state_type,
                      get_app_update_state_listener_);
    CreateDdsForTopic<OtaManager::GetAppUpdateState>(kTopicRosGetAppUpdateState, get_state_type,
                      get_app_update_state_listener_);

    // Initialize GetAppUpdateStateResponse (Response) - raw DDS + ROS2/rosbridge
    auto get_state_resp_type =
        std::make_shared<OtaManager::GetAppUpdateStateResponsePubSubType>();
    CreateDdsForTopic<OtaManager::GetAppUpdateStateResponse>(kTopicGetAppUpdateStateResponse,
                      get_state_resp_type);
    CreateDdsForTopic<OtaManager::GetAppUpdateStateResponse>(kTopicRosGetAppUpdateStateResponse,
                      get_state_resp_type);

    // Initialize SetAppUpdate (Request) - raw DDS + ROS2/rosbridge
    auto set_update_type =
        std::make_shared<OtaManager::SetAppUpdatePubSubType>();
    set_app_update_listener_ = std::make_shared<SetAppUpdateListener>(this);
    CreateDdsForTopic<OtaManager::SetAppUpdate>(kTopicSetAppUpdate, set_update_type, set_app_update_listener_);
    CreateDdsForTopic<OtaManager::SetAppUpdate>(kTopicRosSetAppUpdate, set_update_type, set_app_update_listener_);

    // Initialize SetAppUpdateResponse (Response) - raw DDS + ROS2/rosbridge
    auto set_update_resp_type =
        std::make_shared<OtaManager::SetAppUpdateResponsePubSubType>();
    CreateDdsForTopic<OtaManager::SetAppUpdateResponse>(kTopicSetAppUpdateResponse,
                      set_update_resp_type);
    CreateDdsForTopic<OtaManager::SetAppUpdateResponse>(kTopicRosSetAppUpdateResponse,
                      set_update_resp_type);

    // Initialize SetServiceModeRequest (Publisher for lifecycle management)
    auto set_service_mode_type =
        std::make_shared<lifecyclemgr::SetServiceModeRequestPubSubType>();
    CreateDdsForTopic<lifecyclemgr::SetServiceModeRequest>(kTopicSetServiceModeRequest,
                      set_service_mode_type);

    // Initialize SetServiceModeResponse (Subscriber for lifecycle management)
    auto set_service_mode_resp_type =
        std::make_shared<lifecyclemgr::SetServiceModeResponsePubSubType>();
    set_service_mode_response_listener_ = std::make_shared<SetServiceModeResponseListener>(this);
    CreateDdsForTopic<lifecyclemgr::SetServiceModeResponse>(kTopicSetServiceModeResponse,
                      set_service_mode_resp_type,
                      set_service_mode_response_listener_);

    std::cout << "OtamActorService: All DDS topics initialized"
              << std::endl;
  } catch (const std::exception& e) {
    std::string err_msg = "OtamActorService: Initialization failed: " + std::string(e.what());
    std::cerr << err_msg << std::endl;
    if (otam_diagnostic_service_) {
      otam_diagnostic_service_->PublishOtaMgrInfo(static_cast<int32_t>(OtamError::FAILED), err_msg);
    }
    throw;
  }
  state_ = ServiceState::INITIALIZED;
}

void OtamActorService::Start() {
  state_ = ServiceState::STARTED;
  std::cout << "OtamActorService: Starting OtamActorService." << std::endl;
}

void OtamActorService::Stop() {
    std::cout << "OtamActorService: Stopping OtamActorService." << std::endl;

    // 백그라운드 알림 스레드들이 종료되도록 플래그 설정 후 join
    stopping_.store(true);
    {
        std::lock_guard<std::mutex> lk(notify_threads_mutex_);
        for (auto& t : notify_threads_) {
            if (t.joinable()) t.join();
        }
        notify_threads_.clear();
    }

    // Destroy all DDS instances
    for (auto& pair : dds_registry_) {
        if (pair.second) {
            std::cout << "OtamActorService: Destroying DDS for topic: " << pair.first << std::endl;
            pair.second->Destroy();
        }
    }
    state_ = ServiceState::UNINITIALIZED;
}

// CreateDdsForTopic is now a template function defined in the header

// ========== Request Handlers ==========

void OtamActorService::OnAppUpdateSettingsRequested(
    const OtaManager::GetAppUpdateSettings& msg) {
  std::cout << "OtamActorService: GetAppUpdateSettings received"
            << " - id: " << msg.id()
            << ", app_name: " << msg.app_name() << std::endl;

  // Check if OtamServerService is available
  if (!otam_server_service_) {
    std::cerr << "OtamActorService: OtamServerService not available" << std::endl;
    PublishAppUpdateSettingsResponse(
        msg.id(),
        msg.app_name(),
        "",
        ReturnValue::FAIL);

    // Publish diagnostic info
    if (otam_diagnostic_service_) {
      otam_diagnostic_service_->PublishOtaMgrInfo(static_cast<int32_t>(OtamError::FAILED), "Failed: OtamServerService not available");
    }
    return;
  }

  // Call OtamServerService to get OTA settings
  AppSettingsResult result = otam_server_service_->GetOtaSettings(msg.app_name());

  // Convert ReturnValue to ReturnValue
  ReturnValue ret_code;
  int32_t err_code;
  std::string err_msg;

  switch (result.ret_code) {
    case ReturnValue::SUCCESS:
      ret_code = ReturnValue::SUCCESS;
      err_code = 1;
      err_msg = "Success: GetAppUpdateSettings completed";
      break;
    case ReturnValue::FAIL:
      ret_code = ReturnValue::FAIL;
      err_code = 2;
      err_msg = "Failed: GetAppUpdateSettings failed for " + msg.app_name();
      break;
    default:
      ret_code = ReturnValue::UNKNOWN;
      err_code = 0;
      err_msg = "Unknown: GetAppUpdateSettings returned unknown result";
      break;
  }

  // Publish diagnostic info
  if (otam_diagnostic_service_) {
    otam_diagnostic_service_->PublishOtaMgrInfo(err_code, err_msg);
  }

  // Publish the response with settings from OtamServerService
  PublishAppUpdateSettingsResponse(
      msg.id(),
      msg.app_name(),
      result.app_settings,
      ret_code);
}

void OtamActorService::OnSetAppUpdateSettingsReceived(
    const OtaManager::SetAppUpdateSettings& msg) {
  std::cout << "OtamActorService: SetAppUpdateSettings received"
            << " - id: " << msg.id()
            << ", app_name: " << msg.app_name()
            << ", settings: " << msg.app_settings() << std::endl;

  // Check if OtamServerService is available
  if (!otam_server_service_) {
    std::cerr << "OtamActorService: OtamServerService not available" << std::endl;
    PublishSetAppUpdateSettingsResponse(
        msg.id(),
        ReturnValue::FAIL);

    // Publish diagnostic info
    if (otam_diagnostic_service_) {
      otam_diagnostic_service_->PublishOtaMgrInfo(static_cast<int32_t>(OtamError::FAILED), "Failed: OtamServerService not available");
    }
    return;
  }

  // Call OtamServerService to set OTA settings
  ReturnValue result = otam_server_service_->SetOtaSettings(
      msg.app_name(), msg.app_settings());

  // Convert ReturnValue to ReturnValue
  ReturnValue ret_code;
  int32_t err_code;
  std::string err_msg;

  switch (result) {
    case ReturnValue::SUCCESS:
      ret_code = ReturnValue::SUCCESS;
      err_code = 1;
      err_msg = "Success: SetAppUpdateSettings completed for " + msg.app_name();
      break;
    case ReturnValue::FAIL:
      ret_code = ReturnValue::FAIL;
      err_code = 2;
      err_msg = "Failed: SetAppUpdateSettings failed for " + msg.app_name();
      break;
    default:
      ret_code = ReturnValue::UNKNOWN;
      err_code = 0;
      err_msg = "Unknown: SetAppUpdateSettings returned unknown result";
      break;
  }

  // Publish diagnostic info
  if (otam_diagnostic_service_) {
    otam_diagnostic_service_->PublishOtaMgrInfo(err_code, err_msg);
  }

  // Publish the response with result from OtamServerService
  PublishSetAppUpdateSettingsResponse(msg.id(), ret_code);
}

void OtamActorService::OnAppUpdateStateRequested(
    const OtaManager::GetAppUpdateState& msg) {
  std::cout << "OtamActorService: GetAppUpdateState received"
            << " - id: " << msg.id()
            << ", app_name: " << msg.app_name() << std::endl;

  // Check if OtamServerService is available
  if (!otam_server_service_) {
    std::cerr << "OtamActorService: OtamServerService not available" << std::endl;
    PublishAppUpdateStateResponse(
        msg.id(),
        msg.app_name(),
        "",
        ReturnValue::FAIL);

    // Publish diagnostic info
    if (otam_diagnostic_service_) {
      otam_diagnostic_service_->PublishOtaMgrInfo(static_cast<int32_t>(OtamError::FAILED), "Failed: OtamServerService not available");
    }
    return;
  }

  // Call OtamServerService to get OTA update status
  AppUpdateStateResult result = otam_server_service_->GetOtaUpdateState(msg.app_name());

  // Parse the app_state_info JSON to extract revision and status
  std::string revision;
  std::string app_state_info;

  if (result.ret_code == ReturnValue::SUCCESS && !result.app_state_info.empty()) {
    app_state_info = result.app_state_info;
  }

  // Convert ReturnValue to ReturnValue
  ReturnValue ret_code;
  int32_t err_code;
  std::string err_msg;

  switch (result.ret_code) {
    case ReturnValue::SUCCESS:
      ret_code = ReturnValue::SUCCESS;
      err_code = 1;
      err_msg = "Success: GetAppUpdateState completed for " + msg.app_name();
      break;
    case ReturnValue::FAIL:
      ret_code = ReturnValue::FAIL;
      err_code = 2;
      err_msg = "Failed: GetAppUpdateState failed for " + msg.app_name();
      break;
    default:
      ret_code = ReturnValue::UNKNOWN;
      err_code = 0;
      err_msg = "Unknown: GetAppUpdateState returned unknown result";
      break;
  }

  // Publish diagnostic info
  if (otam_diagnostic_service_) {
    otam_diagnostic_service_->PublishOtaMgrInfo(err_code, err_msg);
  }

  // Publish the response with status info from OtamServerService
  PublishAppUpdateStateResponse(
      msg.id(),
      msg.app_name(),
      app_state_info,
      ret_code);
}

void OtamActorService::SendSetServiceModeRequest(int32_t request_id) {
  std::cout << "OtamActorService: Sending SetServiceModeRequest (request_id: "
            << request_id << ") to change mode to OTA_MODE" << std::endl;

  auto it = dds_registry_.find(kTopicSetServiceModeRequest);
  if (it == dds_registry_.end()) {
    std::string err_msg = std::string("Failed: DDS not found for topic: ") + kTopicSetServiceModeRequest;
    std::cerr << "OtamActorService: " << err_msg << std::endl;
    if (otam_diagnostic_service_) {
      otam_diagnostic_service_->PublishOtaMgrInfo(static_cast<int32_t>(OtamError::FAILED), err_msg);
    }
    return;
  }

  // Create SetServiceModeRequest message
  auto request = std::make_shared<lifecyclemgr::SetServiceModeRequest>();

  // Set OsType to ROS2 (adjust if needed)
  request->os_type(lifecyclemgr::OsType::ROS2);

  // Set empty service list for now (apply to all services)
  std::vector<std::string> service_list;
  request->service_list(service_list);

  // Set is_group to false
  request->is_group(false);

  // Set service_mode to OTA_MODE
  request->service_mode(lifecyclemgr::ServiceMode::OTA_MODE);

  // Publish the request
  it->second->Write(std::static_pointer_cast<void>(request));

  std::cout << "OtamActorService: SetServiceModeRequest sent successfully (request_id: "
            << request_id << ")" << std::endl;

  // Publish diagnostic info
  if (otam_diagnostic_service_) {
    otam_diagnostic_service_->PublishOtaMgrInfo(static_cast<int32_t>(OtamError::SUCCEEDED), "Success: SetServiceModeRequest sent to switch to OTA_MODE");
  }
}

void OtamActorService::OnSetServiceModeResponseReceived(
    const lifecyclemgr::SetServiceModeResponse& msg) {
  std::cout << "OtamActorService: SetServiceModeResponse received - ret_code: "
            << static_cast<int>(msg.ret_code()) << std::endl;

  // Find pending request waiting for SetServiceModeResponse
  // unique_lock 사용: erase 후 안전하게 unlock하고 Publish 호출 가능
  std::unique_lock<std::mutex> lock(pending_updates_mutex_);

  int32_t target_key = -1;
  int32_t msg_id = -1;
  std::string app_name;
  bool success = false;

  for (auto& pair : pending_updates_) {
    if (pair.second.service_mode_pending) {
      target_key = pair.first;
      msg_id     = pair.second.msg_id;
      app_name   = pair.second.app_name;
      pair.second.service_mode_pending = false;
      success = (msg.ret_code() == lifecyclemgr::ReturnCode::SUCCESS);
      break;
    }
  }

  if (target_key < 0) return;  // 대기 중인 요청 없음

  if (success) {
    std::cout << "OtamActorService: SetServiceModeResponse SUCCESS (request_id: "
              << target_key << ")" << std::endl;
    if (otam_diagnostic_service_) {
      otam_diagnostic_service_->PublishOtaMgrInfo(
          static_cast<int32_t>(OtamError::SUCCEEDED), "Success: Service mode changed to OTA_MODE");
    }

    if (otam_vehicle_service_) {
      // Vehicle 정보 요청 — lock 유지 상태로 호출 (map은 아직 유효)
      otam_vehicle_service_->RequestVehicleInfo(target_key, "Vehicle.LowVoltageSystemState");
      otam_vehicle_service_->RequestVehicleInfo(target_key, "Vehicle.Speed");
    } else {
      std::cerr << "OtamActorService: OtamVehicleService not available" << std::endl;
      pending_updates_.erase(target_key);
      lock.unlock();
      if (otam_diagnostic_service_) {
        otam_diagnostic_service_->PublishOtaMgrInfo(
            static_cast<int32_t>(OtamError::FAILED), "Failed: OtamVehicleService not available");
      }
      PublishSetAppUpdateResponse(msg_id, app_name, ReturnValue::FAIL);
    }
  } else {
    std::cerr << "OtamActorService: SetServiceModeResponse FAIL (request_id: "
              << target_key << ")" << std::endl;
    pending_updates_.erase(target_key);
    lock.unlock();
    if (otam_diagnostic_service_) {
      otam_diagnostic_service_->PublishOtaMgrInfo(
          static_cast<int32_t>(OtamError::FAILED), "Failed: Service mode change to OTA_MODE failed");
    }
    PublishSetAppUpdateResponse(msg_id, app_name, ReturnValue::FAIL);
  }
}

void OtamActorService::OnAppUpdateRequested(
    const OtaManager::SetAppUpdate& msg) {
  std::cout << "OtamActorService: SetAppUpdate received"
            << " - id: " << msg.id()
            << ", app_name: " << msg.app_name() << std::endl;

  // Check if OtamServerService is available
  if (!otam_server_service_) {
    std::cerr << "OtamActorService: OtamServerService not available" << std::endl;

    // Publish diagnostic info
    if (otam_diagnostic_service_) {
      otam_diagnostic_service_->PublishOtaMgrInfo(static_cast<int32_t>(OtamError::FAILED), "Failed: OtamServerService not available");
    }

    PublishSetAppUpdateResponse(
        msg.id(),
        msg.app_name(),
        ReturnValue::FAIL);
    return;
  }

  // Check if OtamVehicleService is available
  if (!otam_vehicle_service_) {
    std::cerr << "OtamActorService: OtamVehicleService not available" << std::endl;

    // Publish diagnostic info
    if (otam_diagnostic_service_) {
      otam_diagnostic_service_->PublishOtaMgrInfo(static_cast<int32_t>(OtamError::FAILED), "Failed: OtamVehicleService not available");
    }

    PublishSetAppUpdateResponse(
        msg.id(),
        msg.app_name(),
        ReturnValue::FAIL);
    return;
  }

  // Generate unique request ID and create pending request
  int32_t request_id;
  {
    std::lock_guard<std::mutex> lock(pending_updates_mutex_);
    request_id = next_request_id_++;
    pending_updates_.emplace(request_id, PendingUpdateRequest(msg.id(), msg.app_name()));
  }

  std::cout << "OtamActorService: Created pending request (request_id: "
            << request_id << "), sending SetServiceModeRequest..." << std::endl;

  // First, send SetServiceModeRequest to switch to OTA_MODE
  // Vehicle info will be requested after receiving SetServiceModeResponse with SUCCESS
  SendSetServiceModeRequest(request_id);
}

void OtamActorService::OnVehicleInfoReceived(
    int32_t request_id, const std::string& name, const std::string& value) {
  std::lock_guard<std::mutex> lock(pending_updates_mutex_);

  // Find the pending request
  auto it = pending_updates_.find(request_id);
  if (it == pending_updates_.end()) {
    // No pending request for this ID, ignore
    return;
  }

  PendingUpdateRequest& request = it->second;

  std::cout << "OtamActorService: Vehicle info received for request "
            << request_id << ": " << name << " = " << value << std::endl;

  // Store the received info
  request.vehicle_info[name] = value;
  request.pending_info.erase(name);

  // Check if all required info has been received
  if (request.pending_info.empty()) {
    std::cout << "OtamActorService: All vehicle info collected for request "
              << request_id << ", processing update..." << std::endl;

    // Copy request data before erasing (iterator 무효화 방지)
    PendingUpdateRequest request_copy = request;
    pending_updates_.erase(it);

    // mutex를 해제한 상태로 ProcessPendingUpdate 호출 (deadlock 방지)
    // unique_lock의 스코프를 명시적으로 제어
    {
      std::unique_lock<std::mutex> release_lock(pending_updates_mutex_, std::adopt_lock);
      release_lock.unlock();
      ProcessPendingUpdate(request_copy);
      // lock_guard가 소멸될 때 다시 잠금 필요 없음 — 함수 종료 직전이므로 return
      return;
    }
  } else {
    std::cout << "OtamActorService: Still waiting for vehicle info: ";
    for (const auto& pending : request.pending_info) {
      std::cout << pending << " ";
    }
    std::cout << std::endl;
  }
}

void OtamActorService::ProcessPendingUpdate(const PendingUpdateRequest& request) {
  std::cout << "OtamActorService: Processing pending update for app: "
            << request.app_name << std::endl;

  // Get vehicle state and speed from collected info
  std::string vehicle_state;
  std::string vehicle_speed;

  auto state_it = request.vehicle_info.find("Vehicle.LowVoltageSystemState");
  if (state_it != request.vehicle_info.end()) {
    vehicle_state = state_it->second;
  }

  auto speed_it = request.vehicle_info.find("Vehicle.Speed");
  if (speed_it != request.vehicle_info.end()) {
    vehicle_speed = speed_it->second;
  }

  std::cout << "OtamActorService: Vehicle state: " << vehicle_state
            << ", Vehicle speed: " << vehicle_speed << std::endl;

  // Check vehicle condition before running OTA update
  // Condition: vehicle_state == "acc" || (vehicle_state == "ignition" && vehicle_speed == "0")
  if (vehicle_state == "acc") {
    std::cout << "OtamActorService: Vehicle is in ACC state - OTA update allowed" << std::endl;
  } else if (vehicle_state == "ignition" && vehicle_speed == "0") {
    std::cout << "OtamActorService: Vehicle is in IGNITION state with speed 0 - OTA update allowed" << std::endl;
  } else {
    std::cerr << "OtamActorService: Vehicle condition not met for OTA update"
              << " (state: " << vehicle_state << ", speed: " << vehicle_speed << ")" << std::endl;

    // Publish diagnostic info
    if (otam_diagnostic_service_) {
      std::string err_msg = "Failed: Vehicle condition not met for OTA update (state: "
                            + vehicle_state + ", speed: " + vehicle_speed + ")";
      otam_diagnostic_service_->PublishOtaMgrInfo(static_cast<int32_t>(OtamError::FAILED), err_msg);
    }

    PublishSetAppUpdateResponse(
        request.msg_id,
        request.app_name,
        ReturnValue::FAIL);
    return;
  }

  // Call OtamServerService to run OTA update
  ReturnValue result = otam_server_service_->RunOtaUpdate(request.app_name);

  // Convert ReturnValue to ReturnValue
  ReturnValue ret_code;
  int32_t err_code;
  std::string err_msg;

  switch (result) {
    case ReturnValue::SUCCESS:
      ret_code = ReturnValue::SUCCESS;
      err_code = 1;
      err_msg = "Success: OTA update started for " + request.app_name;
      break;
    case ReturnValue::FAIL:
      ret_code = ReturnValue::FAIL;
      err_code = 2;
      err_msg = "Failed: OTA update failed for " + request.app_name;
      break;
    default:
      ret_code = ReturnValue::UNKNOWN;
      err_code = 0;
      err_msg = "Unknown: OTA update returned unknown result for " + request.app_name;
      break;
  }

  // Publish diagnostic info
  if (otam_diagnostic_service_) {
    otam_diagnostic_service_->PublishOtaMgrInfo(err_code, err_msg);
  }

  // Publish the response with result from OtamServerService
  PublishSetAppUpdateResponse(
      request.msg_id,
      request.app_name,
      ret_code);
}

// ========== Response Publishers ==========

void OtamActorService::PublishAppUpdateSettingsResponse(
    int32_t id,
    const std::string& app_name,
    const std::string& app_settings,
    ReturnValue code) {
  auto it = dds_registry_.find(kTopicGetAppUpdateSettingsResponse);
  if (it == dds_registry_.end()) {
    std::string err_msg = std::string("Failed: DDS not found for topic: ") + kTopicGetAppUpdateSettingsResponse;
    std::cerr << "OtamActorService: " << err_msg << std::endl;
    if (otam_diagnostic_service_) {
      otam_diagnostic_service_->PublishOtaMgrInfo(static_cast<int32_t>(OtamError::FAILED), err_msg);
    }
    return;
  }

  auto response = std::make_shared<OtaManager::GetAppUpdateSettingsResponse>();
  response->id(id);
  response->app_name(app_name);
  response->app_settings(app_settings);
  response->ret_code(code);

  it->second->Write(std::static_pointer_cast<void>(response));
  std::cout << "OtamActorService: Published "
            << "GetAppUpdateSettingsResponse - id: " << id
            << std::endl;

  // ROS2/rosbridge 채널에도 동일 응답 발행
  auto ros_it = dds_registry_.find(kTopicRosGetAppUpdateSettingsResponse);
  if (ros_it != dds_registry_.end()) {
    ros_it->second->Write(std::static_pointer_cast<void>(response));
  }
}

void OtamActorService::PublishSetAppUpdateSettingsResponse(
    int32_t id,
    ReturnValue code) {
  auto it = dds_registry_.find(
      kTopicSetAppUpdateSettingsResponse);
  if (it == dds_registry_.end()) {
    std::string err_msg = std::string("Failed: DDS not found for topic: ") + kTopicSetAppUpdateSettingsResponse;
    std::cerr << "OtamActorService: " << err_msg << std::endl;
    if (otam_diagnostic_service_) {
      otam_diagnostic_service_->PublishOtaMgrInfo(static_cast<int32_t>(OtamError::FAILED), err_msg);
    }
    return;
  }

  auto response =
      std::make_shared<OtaManager::SetAppUpdateSettingsResponse>();
  response->id(id);
  response->ret_code(code);

  it->second->Write(std::static_pointer_cast<void>(response));
  std::cout << "OtamActorService: Published "
            << "SetAppUpdateSettingsResponse - id: " << id
            << std::endl;

  // ROS2/rosbridge 채널에도 동일 응답 발행
  auto ros_it = dds_registry_.find(kTopicRosSetAppUpdateSettingsResponse);
  if (ros_it != dds_registry_.end()) {
    ros_it->second->Write(std::static_pointer_cast<void>(response));
  }
}

void OtamActorService::PublishAppUpdateStateResponse(
    int32_t id,
    const std::string& app_name,
    const std::string& app_state_info,
    ReturnValue code) {
  auto it = dds_registry_.find(kTopicGetAppUpdateStateResponse);
  if (it == dds_registry_.end()) {
    std::string err_msg = std::string("Failed: DDS not found for topic: ") + kTopicGetAppUpdateStateResponse;
    std::cerr << "OtamActorService: " << err_msg << std::endl;
    if (otam_diagnostic_service_) {
      otam_diagnostic_service_->PublishOtaMgrInfo(static_cast<int32_t>(OtamError::FAILED), err_msg);
    }
    return;
  }

  auto response = std::make_shared<OtaManager::GetAppUpdateStateResponse>();
  response->id(id);
  response->app_name(app_name);
  response->app_state_info(app_state_info);
  response->ret_code(code);

  it->second->Write(std::static_pointer_cast<void>(response));
  std::cout << "OtamActorService: Published "
            << "GetAppUpdateStateResponse - id: " << id
            << std::endl;

  // ROS2/rosbridge 채널에도 동일 응답 발행
  auto ros_it = dds_registry_.find(kTopicRosGetAppUpdateStateResponse);
  if (ros_it != dds_registry_.end()) {
    ros_it->second->Write(std::static_pointer_cast<void>(response));
  }
}

void OtamActorService::PublishSetAppUpdateResponse(
    int32_t id,
    const std::string& app_name,
    ReturnValue code) {
  auto it = dds_registry_.find(kTopicSetAppUpdateResponse);
  if (it == dds_registry_.end()) {
    std::string err_msg = std::string("Failed: DDS not found for topic: ") + kTopicSetAppUpdateResponse;
    std::cerr << "OtamActorService: " << err_msg << std::endl;
    if (otam_diagnostic_service_) {
      otam_diagnostic_service_->PublishOtaMgrInfo(static_cast<int32_t>(OtamError::FAILED), err_msg);
    }
    return;
  }

  auto response = std::make_shared<OtaManager::SetAppUpdateResponse>();
  response->id(id);
  response->app_name(app_name);
  response->ret_code(code);

  it->second->Write(std::static_pointer_cast<void>(response));
  std::cout << "OtamActorService: Published "
            << "SetAppUpdateResponse - id: " << id << std::endl;

  // ROS2/rosbridge 채널에도 동일 응답 발행
  auto ros_it = dds_registry_.find(kTopicRosSetAppUpdateResponse);
  if (ros_it != dds_registry_.end()) {
    ros_it->second->Write(std::static_pointer_cast<void>(response));
  }

  // 업데이트가 성공적으로 시작된 경우 백그라운드 스레드에서 상태를 폴링하여 클라이언트에 알림
  // detach() 대신 notify_threads_에 보관하여 Stop() 시 안전하게 join
  if (code == ReturnValue::SUCCESS && otam_client_service_ && otam_server_service_) {
    std::lock_guard<std::mutex> lk(notify_threads_mutex_);
    notify_threads_.emplace_back([this, app_name]() {
      try {
        std::cout << "OtamActorService: Starting background notification thread for "
                  << app_name << std::endl;

        auto start_time = std::chrono::steady_clock::now();
        const auto timeout_duration = std::chrono::seconds(10);

        bool keep_polling = true;
        while (keep_polling && !stopping_.load()) {
          AppUpdateStateResult result = otam_server_service_->GetOtaUpdateState(app_name);

          std::string operation_state = "Unknown";
          if (result.ret_code == ReturnValue::SUCCESS && !result.app_state_info.empty()) {
            try {
              nlohmann::json state_json = nlohmann::json::parse(result.app_state_info);
              if (state_json.contains("operationState")) {
                operation_state = state_json["operationState"].get<std::string>();
              }
            } catch (const nlohmann::json::exception& e) {
              std::string err_msg = "Failed to parse operationState: " + std::string(e.what());
              std::cerr << "OtamActorService: " << err_msg << std::endl;
              if (otam_diagnostic_service_) {
                otam_diagnostic_service_->PublishOtaMgrInfo(
                    static_cast<int32_t>(OtamError::FAILED), err_msg);
              }
            }
          }

          otam_client_service_->NotifyAppUpdateStatus(app_name, operation_state);
          std::cout << "OtamActorService: Notified client for " << app_name
                    << " with state: " << operation_state << std::endl;

          if (operation_state == "Running") {
            auto elapsed = std::chrono::steady_clock::now() - start_time;
            if (elapsed >= timeout_duration) {
              std::cerr << "OtamActorService: Timeout (10s) for " << app_name << std::endl;
              keep_polling = false;
            } else {
              // 50ms 단위로 쪼개어 stopping_ 플래그에 즉시 반응
              for (int i = 0; i < 10 && !stopping_.load(); ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
              }
            }
          } else {
            keep_polling = false;
            std::cout << "OtamActorService: Background notification completed for "
                      << app_name << " final state: " << operation_state << std::endl;
          }
        }
      } catch (const std::exception& e) {
        std::string err_msg = "Background notification failed: " + std::string(e.what());
        std::cerr << "OtamActorService: " << err_msg << std::endl;
        if (otam_diagnostic_service_) {
          otam_diagnostic_service_->PublishOtaMgrInfo(
              static_cast<int32_t>(OtamError::FAILED), err_msg);
        }
      }
    });
  }
}

void OtamActorService::GetAppUpdateSettingsListener::on_data_available(eprosima::fastdds::dds::DataReader* reader) {
    // This callback is invoked by FastDDS when new registration data arrives
    OtaManager::GetAppUpdateSettings message;
    eprosima::fastdds::dds::SampleInfo info;

    try {
        // Attempt to read the next available message
        if (reader->take_next_sample(&message, &info) == eprosima::fastdds::dds::RETCODE_OK) {
            if (info.valid_data) {
                // Forward the message to the service for processing
                service_->OnAppUpdateSettingsRequested(message);
            } else {
                std::cout << "GetAppUpdateSettingsListener: Received invalid data sample" << std::endl;
            }
        } else {
            std::cout << "GetAppUpdateSettingsListener: Failed to read message sample" << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::string err_msg = "GetAppUpdateSettingsListener: Exception in data callback: " + std::string(e.what());
        std::cerr << err_msg << std::endl;
        if (service_->otam_diagnostic_service_) {
            service_->otam_diagnostic_service_->PublishOtaMgrInfo(static_cast<int32_t>(OtamError::FAILED), err_msg);
        }
    }
}

void OtamActorService::SetAppUpdateSettingsListener::on_data_available(eprosima::fastdds::dds::DataReader* reader) {
    // This callback is invoked by FastDDS when new registration data arrives
    OtaManager::SetAppUpdateSettings message;
    eprosima::fastdds::dds::SampleInfo info;

    try {
        // Attempt to read the next available message
        if (reader->take_next_sample(&message, &info) == eprosima::fastdds::dds::RETCODE_OK) {
            if (info.valid_data) {
                // Forward the message to the service for processing
                service_->OnSetAppUpdateSettingsReceived(message);
            } else {
                std::cout << "SetAppUpdateSettingsListener: Received invalid data sample" << std::endl;
            }
        } else {
            std::cout << "SetAppUpdateSettingsListener: Failed to read message sample" << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::string err_msg = "SetAppUpdateSettingsListener: Exception in data callback: " + std::string(e.what());
        std::cerr << err_msg << std::endl;
        if (service_->otam_diagnostic_service_) {
            service_->otam_diagnostic_service_->PublishOtaMgrInfo(static_cast<int32_t>(OtamError::FAILED), err_msg);
        }
    }
}

void OtamActorService::GetAppUpdateStateListener::on_data_available(eprosima::fastdds::dds::DataReader* reader) {
    // This callback is invoked by FastDDS when new registration data arrives
    OtaManager::GetAppUpdateState message;
    eprosima::fastdds::dds::SampleInfo info;

    try {
        // Attempt to read the next available message
        if (reader->take_next_sample(&message, &info) == eprosima::fastdds::dds::RETCODE_OK) {
            if (info.valid_data) {
                // Forward the message to the service for processing
                service_->OnAppUpdateStateRequested(message);
            } else {
                std::cout << "GetAppUpdateStateListener: Received invalid data sample" << std::endl;
            }
        } else {
            std::cout << "GetAppUpdateStateListener: Failed to read message sample" << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::string err_msg = "GetAppUpdateStateListener: Exception in data callback: " + std::string(e.what());
        std::cerr << err_msg << std::endl;
        if (service_->otam_diagnostic_service_) {
            service_->otam_diagnostic_service_->PublishOtaMgrInfo(static_cast<int32_t>(OtamError::FAILED), err_msg);
        }
    }
}

void OtamActorService::SetAppUpdateListener::on_data_available(eprosima::fastdds::dds::DataReader* reader) {
    // This callback is invoked by FastDDS when new registration data arrives
    OtaManager::SetAppUpdate message;
    eprosima::fastdds::dds::SampleInfo info;

    try {
        // Attempt to read the next available message
        if (reader->take_next_sample(&message, &info) == eprosima::fastdds::dds::RETCODE_OK) {
            if (info.valid_data) {
                // Forward the message to the service for processing
                service_->OnAppUpdateRequested(message);
            } else {
                std::cout << "SetAppUpdateListener: Received invalid data sample" << std::endl;
            }
        } else {
            std::cout << "SetAppUpdateListener: Failed to read message sample" << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::string err_msg = "SetAppUpdateListener: Exception in data callback: " + std::string(e.what());
        std::cerr << err_msg << std::endl;
        if (service_->otam_diagnostic_service_) {
            service_->otam_diagnostic_service_->PublishOtaMgrInfo(static_cast<int32_t>(OtamError::FAILED), err_msg);
        }
    }
}

void OtamActorService::SetServiceModeResponseListener::on_data_available(eprosima::fastdds::dds::DataReader* reader) {
    // This callback is invoked by FastDDS when SetServiceModeResponse arrives
    lifecyclemgr::SetServiceModeResponse message;
    eprosima::fastdds::dds::SampleInfo info;

    try {
        // Attempt to read the next available message
        if (reader->take_next_sample(&message, &info) == eprosima::fastdds::dds::RETCODE_OK) {
            if (info.valid_data) {
                // Forward the message to the service for processing
                service_->OnSetServiceModeResponseReceived(message);
            } else {
                std::cout << "SetServiceModeResponseListener: Received invalid data sample" << std::endl;
            }
        } else {
            std::cout << "SetServiceModeResponseListener: Failed to read message sample" << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::string err_msg = "SetServiceModeResponseListener: Exception in data callback: " + std::string(e.what());
        std::cerr << err_msg << std::endl;
        if (service_->otam_diagnostic_service_) {
            service_->otam_diagnostic_service_->PublishOtaMgrInfo(static_cast<int32_t>(OtamError::FAILED), err_msg);
        }
    }
}
