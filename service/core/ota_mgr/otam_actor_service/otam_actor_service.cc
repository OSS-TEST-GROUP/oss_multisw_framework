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

    // Signal background notification threads to stop, then join them
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

void OtamActorService::WriteServiceModeRequestInternal(
    int32_t request_id, lifecyclemgr::ServiceMode mode) {
  lifecycle_request_count_.fetch_add(1);

  auto it = dds_registry_.find(kTopicSetServiceModeRequest);
  if (it == dds_registry_.end()) {
    std::string err_msg = std::string("Failed: DDS not found for topic: ") + kTopicSetServiceModeRequest;
    std::cerr << "OtamActorService: " << err_msg << std::endl;
    if (otam_diagnostic_service_) {
      otam_diagnostic_service_->PublishOtaMgrInfo(static_cast<int32_t>(OtamError::FAILED), err_msg);
    }
    return;
  }

  auto request = std::make_shared<lifecyclemgr::SetServiceModeRequest>();
  request->os_type(lifecyclemgr::OsType::ROS2);
  request->service_list(std::vector<std::string>{});
  request->is_group(false);
  request->service_mode(mode);

  try {
    it->second->Write(std::static_pointer_cast<void>(request));
  } catch (const std::exception& e) {
    std::cerr << "OtamActorService: Failed to write SetServiceModeRequest: " << e.what() << std::endl;
    if (otam_diagnostic_service_) {
      otam_diagnostic_service_->PublishOtaMgrInfo(
          static_cast<int32_t>(OtamError::FAILED),
          std::string("Failed to write SetServiceModeRequest: ") + e.what());
    }
  }
}

void OtamActorService::StartLifecycleTimeoutWatcher(int32_t request_id) {
  if (!otam_server_service_) return;
  const int timeout_sec = otam_server_service_->GetLifecycleTimeoutSec();
  if (timeout_sec <= 0) return;

  std::lock_guard<std::mutex> lk(notify_threads_mutex_);
  notify_threads_.emplace_back([this, request_id, timeout_sec]() {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(timeout_sec);
    while (!stopping_.load() && std::chrono::steady_clock::now() < deadline) {
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
      std::lock_guard<std::mutex> l(pending_updates_mutex_);
      auto it = pending_updates_.find(request_id);
      if (it == pending_updates_.end() || !it->second.service_mode_pending) {
        return;  // response already handled; nothing to do
      }
    }
    if (stopping_.load()) return;

    // Timeout fired. Remove pending, notify FAIL, then request NORMAL_MODE restore.
    int32_t msg_id = -1;
    std::string app_name;
    {
      std::lock_guard<std::mutex> l(pending_updates_mutex_);
      auto it = pending_updates_.find(request_id);
      if (it == pending_updates_.end() || !it->second.service_mode_pending) return;
      msg_id = it->second.msg_id;
      app_name = it->second.app_name;
      pending_updates_.erase(it);
    }

    std::cerr << "OtamActorService: Lifecycle Manager response timeout (request_id: "
              << request_id << ")" << std::endl;
    if (otam_diagnostic_service_) {
      otam_diagnostic_service_->PublishOtaMgrInfo(
          static_cast<int32_t>(OtamError::FAILED),
          "Failed: Lifecycle Manager response timeout");
    }
    // PublishSetAppUpdateResponse(FAIL) triggers the F013 NORMAL-mode restore centrally.
    try {
      PublishSetAppUpdateResponse(msg_id, app_name, ReturnValue::FAIL);
    } catch (const std::exception& e) {
      std::cerr << "OtamActorService: Failed to publish timeout FAIL: " << e.what() << std::endl;
    }
  });
}

void OtamActorService::SendSetServiceModeRequest(int32_t request_id) {
  std::cout << "OtamActorService: Sending SetServiceModeRequest (request_id: "
            << request_id << ") to change mode to OTA_MODE" << std::endl;

  WriteServiceModeRequestInternal(request_id, lifecyclemgr::ServiceMode::OTA_MODE);

  if (otam_diagnostic_service_) {
    otam_diagnostic_service_->PublishOtaMgrInfo(
        static_cast<int32_t>(OtamError::SUCCEEDED),
        "Success: SetServiceModeRequest sent to switch to OTA_MODE");
  }

  StartLifecycleTimeoutWatcher(request_id);
}

void OtamActorService::StartVissTimeoutWatcher(int32_t request_id) {
  if (!otam_server_service_) return;
  const int timeout_sec = otam_server_service_->GetVissTimeoutSec();
  if (timeout_sec <= 0) return;

  std::lock_guard<std::mutex> lk(notify_threads_mutex_);
  notify_threads_.emplace_back([this, request_id, timeout_sec]() {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(timeout_sec);
    while (!stopping_.load() && std::chrono::steady_clock::now() < deadline) {
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
      std::lock_guard<std::mutex> l(pending_updates_mutex_);
      auto it = pending_updates_.find(request_id);
      if (it == pending_updates_.end() || it->second.pending_info.empty()) {
        return;  // all info arrived (or request handled); no timeout
      }
    }
    if (stopping_.load()) return;

    int32_t msg_id = -1;
    std::string app_name;
    std::string missing;
    {
      std::lock_guard<std::mutex> l(pending_updates_mutex_);
      auto it = pending_updates_.find(request_id);
      if (it == pending_updates_.end() || it->second.pending_info.empty()) return;
      msg_id = it->second.msg_id;
      app_name = it->second.app_name;
      for (const auto& sig : it->second.pending_info) {
        if (!missing.empty()) missing += ", ";
        missing += sig;
      }
      pending_updates_.erase(it);
    }

    std::cerr << "OtamActorService: VISS response timeout (request_id: " << request_id
              << ", missing: " << missing << ")" << std::endl;
    if (otam_diagnostic_service_) {
      otam_diagnostic_service_->PublishOtaMgrInfo(
          static_cast<int32_t>(OtamError::FAILED),
          "Failed: VISS response timeout (missing: " + missing + ")");
    }
    try {
      PublishSetAppUpdateResponse(msg_id, app_name, ReturnValue::FAIL);
    } catch (const std::exception& e) {
      std::cerr << "OtamActorService: Failed to publish VISS timeout FAIL: "
                << e.what() << std::endl;
    }
  });
}

void OtamActorService::OnSetServiceModeResponseReceived(
    const lifecyclemgr::SetServiceModeResponse& msg) {
  std::cout << "OtamActorService: SetServiceModeResponse received - ret_code: "
            << static_cast<int>(msg.ret_code()) << std::endl;

  // Find pending request waiting for SetServiceModeResponse
  // Use unique_lock so we can safely unlock before calling Publish after erase
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

  if (target_key < 0) return;  // no pending request waiting for this response

  if (success) {
    std::cout << "OtamActorService: SetServiceModeResponse SUCCESS (request_id: "
              << target_key << ")" << std::endl;
    if (otam_diagnostic_service_) {
      otam_diagnostic_service_->PublishOtaMgrInfo(
          static_cast<int32_t>(OtamError::SUCCEEDED), "Success: Service mode changed to OTA_MODE");
    }

    const bool vehicle_info_enabled =
        otam_server_service_ && otam_server_service_->IsVehicleInfoEnabled();

    if (vehicle_info_enabled && otam_vehicle_service_) {
      // Request vehicle info — called while holding the lock (map still valid)
      std::cout << "OtamActorService: Requesting vehicle info (vehicle_info_enabled=true)" << std::endl;
      otam_vehicle_service_->RequestVehicleInfo(target_key, "Vehicle.LowVoltageSystemState");
      otam_vehicle_service_->RequestVehicleInfo(target_key, "Vehicle.Speed");
      otam_vehicle_service_->RequestVehicleInfo(
          target_key, "Vehicle.Powertrain.TractionBattery.StateOfCharge.Current");
      StartVissTimeoutWatcher(target_key);
    } else if (vehicle_info_enabled && !otam_vehicle_service_) {
      std::cerr << "OtamActorService: OtamVehicleService not available" << std::endl;
      pending_updates_.erase(target_key);
      lock.unlock();
      if (otam_diagnostic_service_) {
        otam_diagnostic_service_->PublishOtaMgrInfo(
            static_cast<int32_t>(OtamError::FAILED), "Failed: OtamVehicleService not available");
      }
      try {
        PublishSetAppUpdateResponse(msg_id, app_name, ReturnValue::FAIL);
      } catch (const std::exception& e) {
        std::cerr << "OtamActorService: Failed to publish FAIL response: " << e.what() << std::endl;
      }
    } else {
      // vehicle_info_enabled == false: skip vehicle state query, proceed directly to update
      std::cout << "OtamActorService: Vehicle info check disabled, skipping to update (request_id: "
                << target_key << ")" << std::endl;
      PendingUpdateRequest request_copy = pending_updates_.at(target_key);
      pending_updates_.erase(target_key);
      lock.unlock();
      ProcessPendingUpdate(request_copy);
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
    try {
      PublishSetAppUpdateResponse(msg_id, app_name, ReturnValue::FAIL);
    } catch (const std::exception& e) {
      std::cerr << "OtamActorService: Failed to publish FAIL response: " << e.what() << std::endl;
    }
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

  // Check if OtamVehicleService is available (required only when vehicle_info_enabled)
  const bool vehicle_info_enabled =
      otam_server_service_ && otam_server_service_->IsVehicleInfoEnabled();
  if (vehicle_info_enabled && !otam_vehicle_service_) {
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

  const bool lifecycle_enabled = otam_server_service_->IsLifecycleEnabled();

  if (lifecycle_enabled) {
    std::cout << "OtamActorService: Created pending request (request_id: "
              << request_id << "), sending SetServiceModeRequest..." << std::endl;
    SendSetServiceModeRequest(request_id);
  } else {
    std::cout << "OtamActorService: Lifecycle check disabled, bypassing SetServiceModeRequest"
              << " (request_id: " << request_id << ")" << std::endl;
    // Synthesize a SUCCESS response to trigger the post-service-mode flow
    lifecyclemgr::SetServiceModeResponse synthetic;
    synthetic.ret_code(lifecyclemgr::ReturnCode::SUCCESS);
    OnSetServiceModeResponseReceived(synthetic);
  }
}

void OtamActorService::OnVehicleInfoReceived(
    int32_t request_id, const std::string& name, const std::string& value) {
  std::unique_lock<std::mutex> lock(pending_updates_mutex_);

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

    PendingUpdateRequest request_copy = request;
    pending_updates_.erase(it);
    lock.unlock();
    ProcessPendingUpdate(request_copy);
    return;
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

  // Skip vehicle condition check when vehicle_info_enabled == false
  const bool vehicle_info_enabled =
      otam_server_service_ && otam_server_service_->IsVehicleInfoEnabled();

  if (vehicle_info_enabled) {
    // Check vehicle condition: acc, or ignition with speed 0
    if (vehicle_state == "acc") {
      std::cout << "OtamActorService: Vehicle is in ACC state - OTA update allowed" << std::endl;
    } else if (vehicle_state == "ignition" && vehicle_speed == "0") {
      std::cout << "OtamActorService: Vehicle is in IGNITION state with speed 0 - OTA update allowed" << std::endl;
    } else {
      std::cerr << "OtamActorService: Vehicle condition not met for OTA update"
                << " (state: " << vehicle_state << ", speed: " << vehicle_speed << ")" << std::endl;

      if (otam_diagnostic_service_) {
        std::string err_msg = "Failed: Vehicle condition not met for OTA update (state: "
                              + vehicle_state + ", speed: " + vehicle_speed + ")";
        otam_diagnostic_service_->PublishOtaMgrInfo(static_cast<int32_t>(OtamError::FAILED), err_msg);
      }

      try {
        PublishSetAppUpdateResponse(request.msg_id, request.app_name, ReturnValue::FAIL);
      } catch (const std::exception& e) {
        std::cerr << "OtamActorService: Failed to publish FAIL response: " << e.what() << std::endl;
      }
      return;
    }

    // Battery State-of-Charge check. Reject update when battery below threshold
    // to prevent mid-update battery drain.
    const double threshold = otam_server_service_->GetBatteryThresholdPercent();
    auto soc_it = request.vehicle_info.find("Vehicle.Powertrain.TractionBattery.StateOfCharge.Current");
    const std::string soc_str = (soc_it != request.vehicle_info.end()) ? soc_it->second : "";
    double soc = -1.0;
    try {
      if (!soc_str.empty()) soc = std::stod(soc_str);
    } catch (const std::exception&) {
      soc = -1.0;  // unparseable → treated as unknown, rejected below
    }

    if (soc < threshold) {
      std::cerr << "OtamActorService: Battery SoC below threshold"
                << " (soc: " << soc_str << ", threshold: " << threshold << ")" << std::endl;

      if (otam_diagnostic_service_) {
        std::string err_msg = "Failed: Battery SoC below threshold (soc: " + soc_str
                              + ", threshold: " + std::to_string(threshold) + ")";
        otam_diagnostic_service_->PublishOtaMgrInfo(static_cast<int32_t>(OtamError::FAILED), err_msg);
      }

      try {
        PublishSetAppUpdateResponse(request.msg_id, request.app_name, ReturnValue::FAIL);
      } catch (const std::exception& e) {
        std::cerr << "OtamActorService: Failed to publish FAIL response: " << e.what() << std::endl;
      }
      return;
    }
  } else {
    std::cout << "OtamActorService: Vehicle info check disabled - OTA update allowed without vehicle state check" << std::endl;
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
  try {
    PublishSetAppUpdateResponse(
        request.msg_id,
        request.app_name,
        ret_code);
  } catch (const std::exception& e) {
    std::cerr << "OtamActorService: Failed to publish update response: " << e.what() << std::endl;
  }
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

  // Publish the same response on the ROS2/rosbridge channel
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

  // Publish the same response on the ROS2/rosbridge channel
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

  // Publish the same response on the ROS2/rosbridge channel
  auto ros_it = dds_registry_.find(kTopicRosGetAppUpdateStateResponse);
  if (ros_it != dds_registry_.end()) {
    ros_it->second->Write(std::static_pointer_cast<void>(response));
  }
}

void OtamActorService::PublishSetAppUpdateResponse(
    int32_t id,
    const std::string& app_name,
    ReturnValue code) {
  // Notify registered clients on failure first, so clients are informed even if the
  // DDS path is unavailable (e.g., topic not registered, write throws, no subscribers).
  if (code == ReturnValue::FAIL && otam_client_service_) {
    try {
      otam_client_service_->NotifyAppUpdateStatus(app_name, "FAILED");
    } catch (const std::exception& e) {
      std::cerr << "OtamActorService: Failed to notify client of FAILED status: "
                << e.what() << std::endl;
    }
  }

  // F013: Restore Lifecycle to NORMAL on any OTA failure so other services can resume.
  // Skipped when lifecycle integration is disabled.
  if (code == ReturnValue::FAIL && otam_server_service_
      && otam_server_service_->IsLifecycleEnabled()) {
    WriteServiceModeRequestInternal(0, lifecyclemgr::ServiceMode::NORMAL);
  }

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

  // Publish the same response on the ROS2/rosbridge channel
  auto ros_it = dds_registry_.find(kTopicRosSetAppUpdateResponse);
  if (ros_it != dds_registry_.end()) {
    ros_it->second->Write(std::static_pointer_cast<void>(response));
  }

  // If the update started successfully, poll status in a background thread and notify clients.
  // Store the thread in notify_threads_ instead of detaching, so Stop() can safely join it.
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
              // Sleep in 50ms slices to react to the stopping_ flag promptly
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
    lifecyclemgr::SetServiceModeResponse message;
    eprosima::fastdds::dds::SampleInfo info;

    try {
        if (reader->take_next_sample(&message, &info) == eprosima::fastdds::dds::RETCODE_OK) {
            if (info.valid_data) {
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
