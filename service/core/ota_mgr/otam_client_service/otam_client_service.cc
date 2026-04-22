  #include "otam_client_service.h"
  #include "otam_diagnostic_service/otam_diagnostic_service.h"
  #include <iostream>

  /**
   * @file otam_client_service.cc
   * @brief Implementation of OtamClientService class
   */

  OtamClientService::OtamClientService()
      : is_session_active_(false) {
      // Constructor: Initialize service state
  }

  void OtamClientService::SetOtamDiagnosticService(OtamDiagnosticService* svc) {
      diagnostic_service_ = svc;
  }

  void OtamClientService::ReportErrorToDiagnosticService(int32_t err_code, const std::string& error_message) {
      std::cerr << "OtamClientService: " << error_message << std::endl;
      if (diagnostic_service_) {
          diagnostic_service_->PublishOtaMgrInfo(err_code, error_message);
      }
  }

  OtamClientService::~OtamClientService() {
      std::cout << "OtamClientService: Destructor called." << std::endl;
      // Destructor: DDS instances are automatically cleaned up by unique_ptr
      registration_dds_.reset();
      registration_response_dds_.reset();
      manager_ready_dds_.reset();
      app_status_dds_.reset();
      app_status_dds_ros_.reset();
  }

  void OtamClientService::Initialize() {

  if (state_ != ServiceState::UNINITIALIZED) {
    std::cout << "OtamClientService already initialized"
              << std::endl;
    return;
  }
  name_ = "OtamClientService";

    std::cout << "OtamClientService: Initializing with DDS setup." << std::endl;

      try {
          // Initialize DDS infrastructure in proper order
          InitializeDDS();
          SetupPublishers();
          SetupSubscribers();

          std::cout << "OtamClientService: DDS initialization completed successfully." << std::endl;
      }
      catch (const std::exception& e) {
          std::cerr << "OtamClientService: Failed to initialize DDS: " << e.what() << std::endl;
          throw;
      }
      state_ = ServiceState::INITIALIZED;
  }

  void OtamClientService::Start() {
      std::cout << "OtamClientService: Starting OtamClientService." << std::endl;
      NotifyManagerReady();
      state_ = ServiceState::STARTED;
  }

  void OtamClientService::Stop() {
      std::cout << "OtamClientService: Stopping OtamClientService." << std::endl;

      // Destroy all DDS instances
      if (registration_dds_) {
          std::cout << "OtamClientService: Destroying registration_dds_" << std::endl;
          registration_dds_->Destroy();
      }
      if (registration_response_dds_) {
          std::cout << "OtamClientService: Destroying registration_response_dds_" << std::endl;
          registration_response_dds_->Destroy();
      }
      if (manager_ready_dds_) {
          std::cout << "OtamClientService: Destroying manager_ready_dds_" << std::endl;
          manager_ready_dds_->Destroy();
      }
      if (app_status_dds_) {
          std::cout << "OtamClientService: Destroying app_status_dds_" << std::endl;
          app_status_dds_->Destroy();
      }
      if (app_status_dds_ros_) {
          std::cout << "OtamClientService: Destroying app_status_dds_ros_" << std::endl;
          app_status_dds_ros_->Destroy();
      }
      if (get_app_list_dds_) {
          std::cout << "OtamClientService: Destroying get_app_list_dds_" << std::endl;
          get_app_list_dds_->Destroy();
      }
      if (get_app_list_response_dds_) {
          std::cout << "OtamClientService: Destroying get_app_list_response_dds_" << std::endl;
          get_app_list_response_dds_->Destroy();
      }
      state_ = ServiceState::UNINITIALIZED;
  }

  void OtamClientService::InitializeDDS() {
      // Create DDS implementation instances for each message type
      // Each instance manages a separate DDS communication channel

      registration_dds_ = std::make_unique<OtamDdsImpl<OtaManager::SetOtaClientRegistration>>();
      registration_response_dds_ = std::make_unique<OtamDdsImpl<OtaManager::SetOtaClientRegistrationResponse>>();
      manager_ready_dds_ = std::make_unique<OtamDdsImpl<OtaManager::NotifyOtaManagerReady>>();
      app_status_dds_     = std::make_unique<OtamDdsImpl<OtaManager::NotifyAppUpdateStatus>>();
      app_status_dds_ros_ = std::make_unique<OtamDdsImpl<OtaManager::NotifyAppUpdateStatus>>();

      // Create listener for incoming registration messages
      registration_listener_ = std::make_shared<RegistrationListener>(this);

      // SetOtaClientRegistration
      set_ota_client_reg_ = std::make_shared<OtaManager::SetOtaClientRegistrationPubSubType>();
      registration_dds_->Create(TOPIC_REGISTRATION,
                               std::static_pointer_cast<void>(set_ota_client_reg_),
                               std::static_pointer_cast<void>(registration_listener_));

      // SetOtaClientRegistrationResponse (TRANSIENT_LOCAL: late-joining clients can receive it)
      set_ota_client_reg_res_ = std::make_shared<OtaManager::SetOtaClientRegistrationResponsePubSubType>();
      registration_response_dds_->Create(TOPIC_REGISTRATION_RESPONSE,
                               std::static_pointer_cast<void>(set_ota_client_reg_res_),
                               nullptr, true);

      // NotifyOtaManagerReady (TRANSIENT_LOCAL: late-joining clients can receive it)
      notify_ota_mgr_ready_ = std::make_shared<OtaManager::NotifyOtaManagerReadyPubSubType>();
      manager_ready_dds_->Create(TOPIC_MANAGER_READY,
                               std::static_pointer_cast<void>(notify_ota_mgr_ready_),
                               nullptr, true);

      // NotifyAppUpdateStatus (raw DDS + ROS2/rosbridge)
      notify_app_update_status_ = std::make_shared<OtaManager::NotifyAppUpdateStatusPubSubType>();
      app_status_dds_->Create(TOPIC_APP_STATUS,
                               std::static_pointer_cast<void>(notify_app_update_status_),
                               nullptr);
      app_status_dds_ros_->Create(TOPIC_ROS_APP_STATUS,
                               std::static_pointer_cast<void>(notify_app_update_status_),
                               nullptr);

      // F022 — GetAppList request topic (subscribe) + Response (publish).
      // Response uses TRANSIENT_LOCAL so a late subscriber on the requester
      // side still gets the most recent answer.
      get_app_list_dds_ =
          std::make_unique<OtamDdsImpl<OtaManager::GetAppList>>();
      get_app_list_response_dds_ =
          std::make_unique<OtamDdsImpl<OtaManager::GetAppListResponse>>();
      get_app_list_listener_ = std::make_shared<GetAppListListener>(this);
      get_app_list_ = std::make_shared<OtaManager::GetAppListPubSubType>();
      get_app_list_response_ =
          std::make_shared<OtaManager::GetAppListResponsePubSubType>();
      get_app_list_dds_->Create(
          TOPIC_GET_APP_LIST,
          std::static_pointer_cast<void>(get_app_list_),
          std::static_pointer_cast<void>(get_app_list_listener_));
      get_app_list_response_dds_->Create(
          TOPIC_GET_APP_LIST_RESPONSE,
          std::static_pointer_cast<void>(get_app_list_response_),
          nullptr, true);

      std::cout << "OtamClientService: DDS instances created." << std::endl;
  }

  void OtamClientService::SetupPublishers() {

  }

    void OtamClientService::SetupSubscribers() {

  }

    int32_t OtamClientService::HandleClientRegistration(OtaClientInfo& app_list) {
        static int32_t cur_client_id = 0;
        int32_t circle = 0;

        auto found = FindClient(app_list.client_name);
        if( found != ota_client_info_list_.end() ) {
            return found->first;
        }

        while(1) {
            auto it = ota_client_info_list_.find(cur_client_id);
            if( it == ota_client_info_list_.end() ) {
            return cur_client_id;
            }

            ++cur_client_id;
            if(cur_client_id == std::numeric_limits<int32_t>::max()) {
            if(circle > 0) {
                break;
            }
            cur_client_id = 0;
            ++circle;
            }          
        }
        return -1;
    }

    std::map<int32_t, OtaClientInfo>::iterator OtamClientService::FindClient(const std::string& client_name) {
        for(auto it = ota_client_info_list_.begin(); it != ota_client_info_list_.end(); it++) {
            if(it->second.client_name.compare(client_name) == 0) {
                std::cout << "OtamClientService: Found client name=" << client_name << std::endl;
                return it;
            }
        }
        return ota_client_info_list_.end();
    }

    int32_t OtamClientService::FindClientIdWithApp(const std::string& app_name) {
        for(const auto &it : ota_client_info_list_) {
            for(const auto &sub_it : it.second.app_names) {
                if(sub_it.compare(app_name) == 0){
                std::cout << "OtamClientService: Found app name=" << app_name << std::endl;
                }
                return it.first;
            }
        }
        return -1;
    }

  void OtamClientService::NotifyManagerReady() {
      std::cout << "OtamClientService: Notifying Manager Ready via DDS." << std::endl;

      try {
          // Create and populate the manager ready message
          auto message = std::make_shared<OtaManager::NotifyOtaManagerReady>();
          // Note: Current IDL defines this as an empty struct
          // Add timestamp or other fields if IDL is updated

          // Publish the message to all subscribers
          manager_ready_dds_->Write(std::static_pointer_cast<void>(message));
          std::cout << "OtamClientService: Manager ready notification sent successfully." << std::endl;
      }
      catch (const std::exception& e) {
          std::string error_msg = "Failed to publish manager ready notification: " + std::string(e.what());
          ReportErrorToDiagnosticService(static_cast<int32_t>(OtamError::FAILED), error_msg);
          throw std::runtime_error("DDS Exception during NotifyManagerReady: " + error_msg);
      }
  }

  void OtamClientService::OnMessageReceived(const OtaManager::SetOtaClientRegistration& registration_data) {
      // Build comma-separated app list string for logging
      std::string app_list_str;
      for (const auto& app : registration_data.app_list()) {
          if (!app_list_str.empty()) app_list_str += ", ";
          app_list_str += app;
      }

      std::cout << "[OtamClientService] Received registration from client: "
                << registration_data.client_name() << ", apps: " << app_list_str << std::endl;

      try {
          // Convert IDL message format to internal format
          OtaClientInfo client_info = {registration_data.client_name(), registration_data.app_list()};

          // Process the registration internally
          int32_t client_id = HandleClientRegistration(client_info);

          ota_client_info_list_[client_id] = client_info;

          // Send registration response back to the client
          ReturnValue ret_code = (client_id >= 0) ? ReturnValue::SUCCESS : ReturnValue::FAIL;
          if (client_id < 0) {
              std::cout << "[OtamClientService] Registration failed for " << registration_data.client_name()
                        << ": Client ID allocation exhausted" << std::endl;
              ReportErrorToDiagnosticService(static_cast<int32_t>(OtamError::FAILED),
                  "OtaClient Registration Failed: " + registration_data.client_name());
          } else {
              std::cout << "[OtamClientService] Assigned client_id=" << client_id
                        << " to " << registration_data.client_name() << std::endl;
          }

          PublishRegistrationResponse(client_id, ret_code);
          std::cout << "[OtamClientService] Published registration response: client_id=" << client_id
                    << ", result=" << ((ret_code == ReturnValue::SUCCESS) ? "SUCCESS" : "FAIL") << std::endl;

          if (client_id >= 0 && diagnostic_service_) {
              diagnostic_service_->PublishOtaMgrInfo(0,
                  "OtaClient Registered: " + registration_data.client_name() +
                  " (id=" + std::to_string(client_id) + ")");
          }
      }
      catch (const std::exception& e) {
          std::string reason = std::string(e.what());
          std::cout << "[OtamClientService] Registration failed for " << registration_data.client_name()
                    << ": " << reason << std::endl;
          ReportErrorToDiagnosticService(static_cast<int32_t>(OtamError::FAILED),
              "OtaClient Registration Failed: " + registration_data.client_name());

          // Send failure response
          try {
              PublishRegistrationResponse(0, ReturnValue::FAIL);
          }
          catch (...) {
              std::cerr << "[OtamClientService] Failed to send error response" << std::endl;
          }
      }
  }

  void OtamClientService::PublishRegistrationResponse(long client_id, ReturnValue ret_code) {
      try {
          // Create SetOtaClientRegistrationResponse message
          auto response_message = std::make_shared<OtaManager::SetOtaClientRegistrationResponse>();
          response_message->client_id(client_id);
          response_message->ret_code(ret_code);

          // Publish the response using registration_response_dds_
          if (registration_response_dds_) {
              registration_response_dds_->Write(std::static_pointer_cast<void>(response_message));
              
              std::cout << "OtamClientService: Published registration response - Client ID: " 
                        << client_id << ", Result: " 
                        << ((ret_code == ReturnValue::SUCCESS) ? "SUCCESS" : "FAIL") << std::endl;
          } else {
              throw std::runtime_error("Registration response DDS publisher not initialized");
          }
      }
      catch (const std::exception& e) {
          std::string error_msg = "Failed to publish registration response: " + std::string(e.what());
          ReportErrorToDiagnosticService(static_cast<int32_t>(OtamError::FAILED), error_msg);
          throw std::runtime_error(error_msg);
      }
  }

  void OtamClientService::NotifyAppUpdateStatus(const std::string& app_name, const std::string& status) {
      try {
          // Create NotifyAppUpdateStatus message
          auto status_message = std::make_shared<OtaManager::NotifyAppUpdateStatus>();
          
          // Set client_id (currently hardcoded as mentioned in header comment)
          // TODO: Make client_id configurable or retrieve from context

          auto client_id = FindClientIdWithApp(app_name);
          if(client_id >= 0){
            status_message->client_id(client_id);
            status_message->app_name(app_name);
            status_message->status(status);
          } else {
              ReportErrorToDiagnosticService(static_cast<int32_t>(OtamError::FAILED),
                  "Failed: No registered client found for app '" + app_name + "'");
          }

          // Publish the status (raw DDS + ROS2/rosbridge)
          if (app_status_dds_) {
              app_status_dds_->Write(std::static_pointer_cast<void>(status_message));
              std::cout << "OtamClientService: Notified app update status - id: " << client_id << ", App: " 
                        << app_name << ", Status: " << status << std::endl;
          } else {
              throw std::runtime_error("App status DDS publisher not initialized");
          }
          // Publish the same message on the ROS2/rosbridge channel
          if (app_status_dds_ros_) {
              app_status_dds_ros_->Write(std::static_pointer_cast<void>(status_message));
          }
      }
      catch (const std::exception& e) {
          std::string error_msg = "Failed to notify app update status: " + std::string(e.what());
          ReportErrorToDiagnosticService(static_cast<int32_t>(OtamError::FAILED), error_msg);
          throw std::runtime_error(error_msg);
      }
  }

  void OtamClientService::RegistrationListener::on_data_available(eprosima::fastdds::dds::DataReader* reader) {
      // This callback is invoked by FastDDS when new registration data arrives
      OtaManager::SetOtaClientRegistration message;
      eprosima::fastdds::dds::SampleInfo info;

      try {
          // Attempt to read the next available message
          if (reader->take_next_sample(&message, &info) == eprosima::fastdds::dds::RETCODE_OK) {
              if (info.valid_data) {
                  // Forward the message to the service for processing
                  service_->OnMessageReceived(message);
              } else {
                  std::cout << "RegistrationListener: Received invalid data sample" << std::endl;
              }
          } else {
              std::cout << "RegistrationListener: Failed to read message sample" << std::endl;
          }
      }
      catch (const std::exception& e) {
          service_->ReportErrorToDiagnosticService(static_cast<int32_t>(OtamError::FAILED),
              "RegistrationListener: Exception in data callback: " + std::string(e.what()));
      }
  }
  // ─────────────────────────────────────────────────────────────────────
  // F022 — GetAppList request listener.
  //
  // Mirrors the RegistrationListener pattern: pull the next sample,
  // forward to HandleGetAppListRequest. Any exception is funnelled
  // through diagnostic_service_ so it surfaces in the Manager logs and
  // doesn't crash the FastDDS reader thread.
  // ─────────────────────────────────────────────────────────────────────
  void OtamClientService::GetAppListListener::on_data_available(
      eprosima::fastdds::dds::DataReader* reader) {
      OtaManager::GetAppList request;
      eprosima::fastdds::dds::SampleInfo info;

      try {
          if (reader->take_next_sample(&request, &info) ==
              eprosima::fastdds::dds::RETCODE_OK) {
              if (info.valid_data) {
                  service_->HandleGetAppListRequest(request);
              } else {
                  std::cout << "GetAppListListener: invalid data sample"
                            << std::endl;
              }
          } else {
              std::cout << "GetAppListListener: failed to read sample"
                        << std::endl;
          }
      } catch (const std::exception& e) {
          service_->ReportErrorToDiagnosticService(
              static_cast<int32_t>(OtamError::FAILED),
              std::string("GetAppListListener: ") + e.what());
      }
  }

  void OtamClientService::HandleGetAppListRequest(
      const OtaManager::GetAppList& req) {
      auto resp =
          BuildGetAppListResponse(req.id(), ota_client_info_list_);
      if (!get_app_list_response_dds_) {
          std::cerr << "OtamClientService: GetAppListResponse DDS missing"
                    << std::endl;
          return;
      }
      auto out = std::make_shared<OtaManager::GetAppListResponse>(resp);
      get_app_list_response_dds_->Write(
          std::static_pointer_cast<void>(out));
      std::cout << "OtamClientService: GetAppListResponse id=" << resp.id()
                << " apps=" << resp.app_names().size() << std::endl;
  }

  // ─────────────────────────────────────────────────────────────────────
  // F022 — pure aggregator. Static so unit tests don't need a live
  // OtamClientService instance (and don't need to touch DDS at all).
  // ─────────────────────────────────────────────────────────────────────
  OtaManager::GetAppListResponse OtamClientService::BuildGetAppListResponse(
      int64_t request_id,
      const std::map<int32_t, OtaClientInfo>& clients) {
      // Step 1: dedupe — for each app name, remember the lowest client_id
      //         that registered it. std::map keeps insertion of first
      //         (lowest) value naturally because we iterate clients in
      //         key order.
      std::map<std::string, int32_t> app_to_owner;
      for (const auto& [cid, info] : clients) {
          for (const auto& app : info.app_names) {
              auto it = app_to_owner.find(app);
              if (it == app_to_owner.end() || cid < it->second) {
                  app_to_owner[app] = cid;
              }
          }
      }

      // Step 2: emit parallel arrays in app-name lexicographic order
      //         (std::map iteration order).
      std::vector<std::string> names;
      std::vector<int32_t>     owners;
      names.reserve(app_to_owner.size());
      owners.reserve(app_to_owner.size());
      for (const auto& [app, owner] : app_to_owner) {
          names.push_back(app);
          owners.push_back(owner);
      }

      OtaManager::GetAppListResponse resp;
      resp.id(request_id);
      resp.app_names(names);
      resp.owner_client_ids(owners);
      resp.ret_code(ReturnValue::SUCCESS);
      return resp;
  }
