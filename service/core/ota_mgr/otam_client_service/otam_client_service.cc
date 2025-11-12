  #include "otam_client_service.h"
  #include <iostream>

  /**
   * @file otam_client_service.cc
   * @brief Implementation of OtamClientService class
   */

  OtamClientService::OtamClientService()
      : is_session_active_(false) {
      // Constructor: Initialize service state
  }

  OtamClientService::~OtamClientService() {
      std::cout << "OtamClientService: Destructor called." << std::endl;
      // Destructor: DDS instances are automatically cleaned up by unique_ptr
      registration_dds_.reset();
      registration_response_dds_.reset();
      manager_ready_dds_.reset();
      app_status_dds_.reset();
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
      state_ = ServiceState::UNINITIALIZED;
  }

  void OtamClientService::InitializeDDS() {
      // Create DDS implementation instances for each message type
      // Each instance manages a separate DDS communication channel

      registration_dds_ = std::make_unique<OtamDdsImpl<OtaManager::SetOtaClientRegistration>>();
      registration_response_dds_ = std::make_unique<OtamDdsImpl<OtaManager::SetOtaClientRegistrationResponse>>();
      manager_ready_dds_ = std::make_unique<OtamDdsImpl<OtaManager::NotifyOtaManagerReady>>();
      app_status_dds_ = std::make_unique<OtamDdsImpl<OtaManager::NotifyAppUpdateStatus>>();

      // Create listener for incoming registration messages
      registration_listener_ = std::make_shared<RegistrationListener>(this);

      // SetOtaClientRegistration
      set_ota_client_reg_ = std::make_shared<OtaManager::SetOtaClientRegistrationPubSubType>();
      registration_dds_->Create(TOPIC_REGISTRATION,
                               std::static_pointer_cast<void>(set_ota_client_reg_),
                               std::static_pointer_cast<void>(registration_listener_));

      // SetOtaClientRegistrationResponse
      set_ota_client_reg_res_ = std::make_shared<OtaManager::SetOtaClientRegistrationResponsePubSubType>();
      registration_response_dds_->Create(TOPIC_REGISTRATION_RESPONSE,
                               std::static_pointer_cast<void>(set_ota_client_reg_res_),
                               nullptr);

      // NotifyOtaManagerReady
      notify_ota_mgr_ready_ = std::make_shared<OtaManager::NotifyOtaManagerReadyPubSubType>();
      manager_ready_dds_->Create(TOPIC_MANAGER_READY,
                               std::static_pointer_cast<void>(notify_ota_mgr_ready_),
                               nullptr);

      // NotifyAppUpdateStatus
      notify_app_update_status_ = std::make_shared<OtaManager::NotifyAppUpdateStatusPubSubType>();
      app_status_dds_->Create(TOPIC_APP_STATUS,
                               std::static_pointer_cast<void>(notify_app_update_status_),
                               nullptr);

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
          ReportErrorToDiagnosticService(error_msg);
          throw std::runtime_error("DDS Exception during NotifyManagerReady: " + error_msg);
      }
  }

  void OtamClientService::OnMessageReceived(const OtaManager::SetOtaClientRegistration& registration_data) {
      std::cout << "OtamClientService: Processing registration from client '"
                << registration_data.client_name() << "' with "
                << registration_data.app_list().size() << " applications." << std::endl;

      try {
          // Convert IDL message format to internal format
          OtaClientInfo client_info = {registration_data.client_name(), registration_data.app_list()};

          // Process the registration internally
          int32_t client_id = HandleClientRegistration(client_info);

          ota_client_info_list_[client_id] = client_info;

          // Send registration response back to the client
          ReturnValue ret_code = (client_id >= 0) ? ReturnValue::SUCCESS : ReturnValue::FAIL;
          PublishRegistrationResponse(client_id, ret_code);

          std::cout << "OtamClientService: Client registration processed. Result: "
                    << ((client_id >= 0) ? "SUCCESS" : "FAILURE") << std::endl;
      }
      catch (const std::exception& e) {
          std::string error_msg = "Error processing client registration: " + std::string(e.what());
          ReportErrorToDiagnosticService(error_msg);

          // Send failure response
          try {
              PublishRegistrationResponse(0, ReturnValue::FAIL);
          }
          catch (...) {
              std::cerr << "OtamClientService: Failed to send error response" << std::endl;
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
          std::cerr << "OtamClientService: " << error_msg << std::endl;
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
              std::cerr << "OtamClientService: No client has the app = " << app_name <<  std::endl;
          }

          // Publish the status using app_status_dds_
          if (app_status_dds_) {
              app_status_dds_->Write(std::static_pointer_cast<void>(status_message));
              
              std::cout << "OtamClientService: Notified app update status - id: " << client_id << ", App: " 
                        << app_name << ", Status: " << status << std::endl;
          } else {
              throw std::runtime_error("App status DDS publisher not initialized");
          }
      }
      catch (const std::exception& e) {
          std::string error_msg = "Failed to notify app update status: " + std::string(e.what());
          std::cerr << "OtamClientService: " << error_msg << std::endl;
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
          std::cerr << "RegistrationListener: Exception in data callback: " << e.what() << std::endl;
      }
  }