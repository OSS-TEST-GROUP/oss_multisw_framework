  #ifndef OTAM_CLIENT_SERVICE_H_
  #define OTAM_CLIENT_SERVICE_H_

  #include "otam_common/otam_service.h"
  #include "otam_common/otam_dds_impl.h"
  #include "otam_common/otam_dds_idl.h"

  #include <memory>
  #include <vector>
  #include <functional>

class OtamDiagnosticService;

  /**
   * @file otam_client_service.h
   * @brief OTA Manager Client Service implementation
   * @author OTA Manager Team
   * @date 2024
   */

  /**
   * @struct ApplicationInfo
   * @brief Structure to hold application information
   */
  struct OtaClientInfo {
      std::string client_name;
      std::vector<std::string> app_names;
  };



  /**
   * @class OtamClientService
   * @brief OTA Manager Client Service for handling OTA client communications
   * 
   * This service manages DDS-based communications with OTA clients, including
   * client registration, status notifications, and update management.
   * It uses multiple DDS instances to handle different message types defined
   * in the OtamClientServiceMsg.idl file.
   * 
   * The service implements the following DDS communication patterns:
   * - Subscribes to client registration requests
   * - Publishes registration responses
   * - Publishes manager ready notifications
   * - Publishes application update status notifications
   * 
   * @see Service
   */
  class OtamClientService : public OtamService {
  public:
      /**
       * @brief Default constructor
       * 
       * Initializes the service with default values.
       * DDS setup is performed in Initialize() method.
       */
      OtamClientService();

      /**
       * @brief Virtual destructor
       * 
       * Ensures proper cleanup of DDS resources and service state.
       */
      ~OtamClientService() override;

      /**
       * @brief Initializes the OTA Client Service
       * 
       * Sets up DDS communication infrastructure including:
       * - Creating DDS instances for different message types
       * - Setting up publishers for outgoing messages
       * - Setting up subscribers for incoming messages
       * - Registering message listeners
       * 
       * @throws std::runtime_error if DDS initialization fails
       */
      void Initialize() override;

      /**
       * @brief Starts the OTA Client Service
       * 
       * Activates the service and begins processing DDS messages.
       * After this call, the service is ready to handle client communications.
       */
      void Start() override;

      /**
       * @brief Stops the OTA Client Service
       * 
       * Deactivates the service and stops processing new messages.
       * Existing sessions are terminated gracefully.
       */
      void Stop() override;

      /**
       * @brief Notifies that the OTA Manager is ready
       *
       * Publishes a NotifyOtaManagerReady message to inform clients
       * that the OTA Manager has completed initialization and is
       * ready to accept client registrations and process updates.
       *
       * @throws std::runtime_error if message publishing fails
       *
       * @note This should be called after successful service initialization
       */
      void NotifyManagerReady();

      /**
       * @brief Returns a copy of the live (client_id -> OtaClientInfo)
       *        registry. OtamClientService remains the source-of-truth
       *        for client registrations; consumers (e.g. OtamActorService
       *        servicing the GetAppList query) take a snapshot via this
       *        accessor instead of touching the private member directly.
       */
      std::map<int32_t, OtaClientInfo> GetClientInfoListSnapshot() const;

      /**
       * @brief Removes a previously registered client and all its apps
       *        addressed by client_id. The registry is persisted to disk
       *        after a successful delete.
       *
       * @return ReturnValue::SUCCESS if client_id was found and removed,
       *         ReturnValue::FAIL    if no entry matched (no-op).
       */
      ReturnValue DeleteClientById(int32_t client_id);

      /**
       * @brief Sets the diagnostic service for error reporting
       * @param svc Pointer to OtamDiagnosticService
       */
      void SetOtamDiagnosticService(OtamDiagnosticService* svc);

      /**
       * @brief Handles incoming client registration messages
       * 
       * Processes SetOtaClientRegistration messages received from OTA clients.
       * Validates the registration data, updates internal client registry,
       * and sends appropriate registration response.
       * 
       * @param registration_data The client registration message containing
       *                         client name and application list
       * 
       * @note This method is called automatically by the DDS listener
       */
      void OnMessageReceived(const otamanager::SetOtaClientRegistration& registration_data);

      /**
       * @brief Publishes client registration response
       * 
       * Sends a SetOtaClientRegistrationResponse message to the requesting client
       * with the assigned client ID and registration result.
       * 
       * @param client_id Unique identifier assigned to the client
       * @param ret_code Registration result (SUCCESS or FAILURE)
       * @param client_name Name of the client this response is addressed to
       *        (echoed back so each client can pick its own response off the
       *        shared response topic)
       *
       * @throws std::runtime_error if message publishing fails
       */
      void PublishRegistrationResponse(long client_id, ReturnValue ret_code, const std::string& client_name);

      /**
       * @brief Notifies application update status
       * 
       * Publishes a NotifyAppUpdateStatus message to inform clients
       * about the current status of application updates.
       * 
       * @param app_name Name of the application being updated
       * @param status Current update status (e.g., "downloading", "installing", "complete")
       * 
       * @throws std::runtime_error if message publishing fails
       * 
       * @note The client_id is currently hardcoded and should be made configurable
       */
      virtual void NotifyAppUpdateStatus(const std::string& app_name, const std::string& status);

  private:
      /**
       * @brief DDS instance for receiving client registration messages
       *
       * Subscribes to SetOtaClientRegistration topic to receive
       * registration requests from OTA clients.
       */
      std::unique_ptr<OtamDds> registration_dds_;

      /**
       * @brief DDS instance for publishing registration responses
       *
       * Publishes SetOtaClientRegistrationResponse messages to
       * respond to client registration requests.
       */
      std::unique_ptr<OtamDds> registration_response_dds_;

      /**
       * @brief DDS instance for publishing manager ready notifications
       *
       * Publishes NotifyOtaManagerReady messages to inform clients
       * that the manager is operational.
       */
      std::unique_ptr<OtamDds> manager_ready_dds_;

      /**
       * @brief DDS instance for publishing application status updates
       *
       * Publishes NotifyAppUpdateStatus messages to inform clients
       * about application update progress.
       */
      std::unique_ptr<OtamDds> app_status_dds_;
      /** @brief ROS2/rosbridge channel publisher for NotifyAppUpdateStatus */

      std::shared_ptr<otamanager::SetOtaClientRegistrationPubSubType> set_ota_client_reg_;
      std::shared_ptr<otamanager::SetOtaClientRegistrationResponsePubSubType> set_ota_client_reg_res_;
      std::shared_ptr<otamanager::NotifyOtaManagerReadyPubSubType> notify_ota_mgr_ready_;
      std::shared_ptr<otamanager::NotifyAppUpdateStatusPubSubType> notify_app_update_status_;

      /**
       * @class RegistrationListener
       * @brief DDS listener for client registration messages
       * 
       * Implements DataReaderListener interface to handle incoming
       * SetOtaClientRegistration messages asynchronously.
       */
      class RegistrationListener : public eprosima::fastdds::dds::DataReaderListener {
      public:
          /**
           * @brief Constructor
           * @param service Pointer to the parent OtamClientService instance
           */
          RegistrationListener(OtamClientService* service) : service_(service) {}

          /**
           * @brief Called when new data is available
           * 
           * This callback is invoked by FastDDS when a new SetOtaClientRegistration
           * message is received. It reads the message and forwards it to the
           * parent service for processing.
           * 
           * @param reader Pointer to the DataReader that received the data
           */
          void on_data_available(eprosima::fastdds::dds::DataReader* reader) override;

      private:
          /** @brief Pointer to the parent service instance */
          OtamClientService* service_;
      };

      /** @brief Listener instance for registration messages */
      std::shared_ptr<RegistrationListener> registration_listener_;

      /**
       * @brief Initializes DDS communication infrastructure
       * 
       * Creates DDS instances and listener objects for all message types.
       * This is called during service initialization.
       */
      void InitializeDDS();

      /**
       * @brief Sets up DDS publishers for outgoing messages
       * 
       * Configures publishers for:
       * - Registration responses
       * - Manager ready notifications  
       * - Application status updates
       * 
       * @throws std::runtime_error if publisher setup fails
       */
      void SetupPublishers();

      /**
       * @brief Sets up DDS subscribers for incoming messages
       * 
       * Configures subscribers for:
       * - Client registration requests
       * 
       * @throws std::runtime_error if subscriber setup fails
       */
      void SetupSubscribers();

      /** @brief Topic name for client registration messages */
      static constexpr const char* TOPIC_REGISTRATION = "/mmf/otamanager/otaclient_service/SetOtaClientRegistration";

      /** @brief Topic name for registration response messages */
      static constexpr const char* TOPIC_REGISTRATION_RESPONSE = "/mmf/otamanager/otaclient_service/SetOtaClientRegistrationResponse";

      /** @brief Topic name for manager ready notifications */
      static constexpr const char* TOPIC_MANAGER_READY = "/mmf/otamanager/otaclient_service/NotifyOtaManagerReady";

      /** @brief Topic name for application status notifications */
      static constexpr const char* TOPIC_APP_STATUS = "/mmf/otamanager/otaclient_service/NotifyAppUpdateStatus";

      /** @brief ROS2/rosbridge compatible topic name (rt/ prefix) */

      /** @brief Flag indicating if an OTA session is currently active */
      bool is_session_active_;

      /** @brief Pointer to diagnostic service for error reporting */
      OtamDiagnosticService* diagnostic_service_{nullptr};

      /** @brief List of currently registered applications */
      std::map<int32_t, OtaClientInfo> ota_client_info_list_;

      /**
       * @brief Handles client registration logic
       * 
       * Internal method to process client registration data and
       * update the internal application registry.
       * 
       * @param app_list List of applications from the client
       * @return true if registration succeeds, false otherwise
       */
      int32_t HandleClientRegistration(OtaClientInfo& app_list);

      std::map<int32_t, OtaClientInfo>::iterator FindClient(const std::string& client_name);

      int32_t FindClientIdWithApp(const std::string& app_name);

      /**
       * @brief Reports errors to the diagnostic service
       *
       * Logs error messages and forwards them to the diagnostic
       * service for monitoring and troubleshooting.
       *
       * @param error_message Description of the error that occurred
       */
      void ReportErrorToDiagnosticService(int32_t err_code, const std::string& error_message);

      /**
       * @brief Reads ota_client_info_list_ from a JSON file. Missing or
       *        malformed files are tolerated (treated as empty registry).
       *
       * Defaults to "config/otam_client_apps.json"; overridable via the
       * OTAM_CLIENT_APPS_CONFIG environment variable. Called by
       * Initialize() so previously-registered clients are restored
       * across Manager restarts.
       */
      void LoadClientListFromFile();

      /**
       * @brief Writes ota_client_info_list_ to the same JSON file used
       *        by LoadClientListFromFile(). Best-effort: write errors
       *        are reported through the diagnostic service but do not
       *        throw, since persistence failures must not break the
       *        live request being serviced.
       */
      void SaveClientListToFile() const;

  public:
      /** @brief Default config path (relative to Manager working dir).
       *         Exposed so the free path-resolver function can fall
       *         back to it when the OTAM_CLIENT_APPS_CONFIG env var is
       *         unset. Treat as read-only.
       */
      static constexpr const char* DEFAULT_CLIENT_APPS_CONFIG_PATH =
          "config/otam_client_apps.json";
  };

  #endif // OTAM_CLIENT_SERVICE_H_
