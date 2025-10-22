#ifndef OTAM_ACTOR_SERVICE_H_
#define OTAM_ACTOR_SERVICE_H_

#include "otam_common/otam_service.h"
#include "otam_common/otam_dds_impl.h"
#include "otam_common/otam_dds_idl.h"

// Include SetServiceModeRequest for lifecycle management
#include "lifecycle/SetServiceModeRequest.hpp"
#include "lifecycle/SetServiceModeRequestPubSubTypes.hpp"

#include <map>
#include <memory>
#include <string>
#include <mutex>
#include <set>
#include <functional>

// Forward declaration
class OtamServerService;
class OtamClientService;
class OtamVehicleService;
class OtamDiagnosticService;

/**
 * @class OtamActorService
 * @brief Service class for OTA Actor managing app update operations
 *        via DDS communication
 */
class OtamActorService: public OtamService {
 public:
  /**
   * @brief Constructor
   */
  OtamActorService();

  /**
   * @brief Destructor
   */
  ~OtamActorService();

  /**
   * @brief Initialize DDS communication for all topics
   */
  void Initialize();
  void Start() override;
  void Stop() override;

  /**
   * @brief Set OtamServerService instance
   * @param server_service Pointer to OtamServerService
   */
  void SetOtamServerService(OtamServerService* server_service);

  /**
   * @brief Set OtamClientService instance
   * @param client_service Pointer to OtamClientService
   */
  void SetOtamClientService(OtamClientService* client_service);

  /**
   * @brief Set OtamVehicleService instance
   * @param vehicle_service Pointer to OtamVehicleService
   */
  void SetOtamVehicleService(OtamVehicleService* vehicle_service);

  /**
   * @brief Set OtamDiagnosticService instance
   * @param diagnostic_service Pointer to OtamDiagnosticService
   */
  void SetOtamDiagnosticService(OtamDiagnosticService* diagnostic_service);

  // ========== Request Handlers ==========

  /**
   * @brief Handle GetAppUpdateSettings request
   * @param msg GetAppUpdateSettings message
   */
  void OnAppUpdateSettingsRequested(
      const OtaManager::GetAppUpdateSettings& msg);

  /**
   * @brief Handle SetAppUpdateSettings request
   * @param msg SetAppUpdateSettings message
   */
  void OnSetAppUpdateSettingsReceived(
      const OtaManager::SetAppUpdateSettings& msg);

  /**
   * @brief Handle GetAppUpdateState request
   * @param msg GetAppUpdateState message
   */
  void OnAppUpdateStateRequested(
      const OtaManager::GetAppUpdateState& msg);

  /**
   * @brief Handle SetAppUpdate request
   * @param msg SetAppUpdate message
   */
  void OnAppUpdateRequested(const OtaManager::SetAppUpdate& msg);

  /**
   * @brief Callback for vehicle info response
   * @param request_id Request ID
   * @param name Vehicle info name
   * @param value Vehicle info value
   */
  void OnVehicleInfoReceived(int32_t request_id, const std::string& name, const std::string& value);

  /**
   * @brief Send SetServiceModeRequest to change service mode to OTA_MODE
   * @param request_id Request ID for tracking
   */
  void SendSetServiceModeRequest(int32_t request_id);

  /**
   * @brief Callback for SetServiceModeResponse
   * @param msg SetServiceModeResponse message
   */
  void OnSetServiceModeResponseReceived(const lifecyclemgr::SetServiceModeResponse& msg);

  // ========== Response Publishers ==========

  /**
   * @brief Publish GetAppUpdateSettings response
   * @param id Request ID
   * @param app_name Application name
   * @param app_settings Application settings JSON string
   * @param code Return code
   */
  void PublishAppUpdateSettingsResponse(
      int32_t id,
      const std::string& app_name,
      const std::string& app_settings,
      ReturnValue code);

  /**
   * @brief Publish SetAppUpdateSettings response
   * @param id Request ID
   * @param code Return code
   */
  void PublishSetAppUpdateSettingsResponse(
      int32_t id,
      ReturnValue code);

  /**
   * @brief Publish GetAppUpdateState response
   * @param id Request ID
   * @param app_name Application name
   * @param revision Current revision string
   * @param status Application status string
   * @param code Return code
   */
  void PublishAppUpdateStateResponse(
      int32_t id,
      const std::string& app_name,
      const std::string& app_state_info,
      ReturnValue code);

  /**
   * @brief Publish SetAppUpdate response
   * @param id Request ID
   * @param app_name Application name
   * @param code Return code
   */
  void PublishSetAppUpdateResponse(
      int32_t id,
      const std::string& app_name,
      ReturnValue code);

 private:
  /**
   * @brief Structure to hold pending OTA update requests
   */
  struct PendingUpdateRequest {
    int32_t msg_id;
    std::string app_name;
    std::map<std::string, std::string> vehicle_info;
    std::set<std::string> pending_info;  // Names of info still waiting for
    bool service_mode_pending;  // Flag indicating if waiting for SetServiceModeResponse

    PendingUpdateRequest(int32_t id, const std::string& name)
        : msg_id(id), app_name(name), service_mode_pending(true) {
      pending_info.insert("vehicle_state");
      pending_info.insert("vehicle_speed");
    }
  };

  /**
   * @brief Pointer to OtamServerService for backend operations
   */
  OtamServerService* otam_server_service_{nullptr};

  /**
   * @brief Pointer to OtamClientService for client notifications
   */
  OtamClientService* otam_client_service_{nullptr};

  /**
   * @brief Pointer to OtamVehicleService for vehicle information
   */
  OtamVehicleService* otam_vehicle_service_{nullptr};

  /**
   * @brief Pointer to OtamDiagnosticService for diagnostic reporting
   */
  OtamDiagnosticService* otam_diagnostic_service_{nullptr};

  /**
   * @brief Map of pending update requests keyed by request ID
   */
  std::map<int32_t, PendingUpdateRequest> pending_updates_;

  /**
   * @brief Mutex to protect pending_updates_
   */
  std::mutex pending_updates_mutex_;

  /**
   * @brief Counter for generating unique request IDs
   */
  int32_t next_request_id_{1000};

  // Topic name constants
  static constexpr const char* kTopicGetAppUpdateSettings =
      "/mmf/otamanager/otaactor_service/GetAppUpdateSettings";
  static constexpr const char* kTopicGetAppUpdateSettingsResponse =
      "/mmf/otamanager/otaactor_service/"
      "GetAppUpdateSettingsResponse";
  static constexpr const char* kTopicSetAppUpdateSettings =
      "/mmf/otamanager/otaactor_service/SetAppUpdateSettings";
  static constexpr const char* kTopicSetAppUpdateSettingsResponse =
      "/mmf/otamanager/otaactor_service/"
      "SetAppUpdateSettingsResponse";
  static constexpr const char* kTopicGetAppUpdateState =
      "/mmf/otamanager/otaactor_service/GetAppUpdateState";
  static constexpr const char* kTopicGetAppUpdateStateResponse =
      "/mmf/otamanager/otaactor_service/"
      "GetAppUpdateStateResponse";
  static constexpr const char* kTopicSetAppUpdate =
      "/mmf/otamanager/otaactor_service/SetAppUpdate";
  static constexpr const char* kTopicSetAppUpdateResponse =
      "/mmf/otamanager/otaactor_service/SetAppUpdateResponse";
  static constexpr const char* kTopicSetServiceModeRequest =
      "mmf/lifecyclemgr/SetServiceModeRequest";
  static constexpr const char* kTopicSetServiceModeResponse =
      "mmf/lifecyclemgr/SetServiceModeResponse";

  /**
   * @brief DDS registry mapping topic names to DDS implementations
   */
  std::map<std::string, std::unique_ptr<OtamDdsImpl>> dds_registry_;

  /**
   * @brief Create DDS implementation for a specific topic
   * @param topic_name Topic name
   * @param type_support TypeSupport shared pointer
   * @param listener DataReaderListener shared pointer (optional)
   */
  void CreateDdsForTopic(
      const std::string& topic_name,
      std::shared_ptr<void> type_support,
      std::shared_ptr<void> listener = nullptr);

  /**
   * @brief Process pending update request when all vehicle info is collected
   * @param request Pending update request
   */
  void ProcessPendingUpdate(const PendingUpdateRequest& request);

  // Prevent copying
  OtamActorService(const OtamActorService&) = delete;
  OtamActorService& operator=(const OtamActorService&) = delete;

  class GetAppUpdateSettingsListener : public eprosima::fastdds::dds::DataReaderListener {
    public:
        GetAppUpdateSettingsListener(OtamActorService* service) : service_(service) {}

        void on_data_available(eprosima::fastdds::dds::DataReader* reader) override;

    private:
        /** @brief Pointer to the parent service instance */
        OtamActorService* service_;
  };
  std::shared_ptr<GetAppUpdateSettingsListener> get_app_update_settings_listener_;

  class SetAppUpdateSettingsListener : public eprosima::fastdds::dds::DataReaderListener {
    public:
        SetAppUpdateSettingsListener(OtamActorService* service) : service_(service) {}

        void on_data_available(eprosima::fastdds::dds::DataReader* reader) override;

    private:
        /** @brief Pointer to the parent service instance */
        OtamActorService* service_;
  };
  std::shared_ptr<SetAppUpdateSettingsListener> set_app_update_settings_listener_;

  class GetAppUpdateStateListener : public eprosima::fastdds::dds::DataReaderListener {
    public:
        GetAppUpdateStateListener(OtamActorService* service) : service_(service) {}

        void on_data_available(eprosima::fastdds::dds::DataReader* reader) override;

    private:
        /** @brief Pointer to the parent service instance */
        OtamActorService* service_;
  };
  std::shared_ptr<GetAppUpdateStateListener> get_app_update_state_listener_;

  class SetAppUpdateListener : public eprosima::fastdds::dds::DataReaderListener {
    public:
        SetAppUpdateListener(OtamActorService* service) : service_(service) {}

        void on_data_available(eprosima::fastdds::dds::DataReader* reader) override;

    private:
        /** @brief Pointer to the parent service instance */
        OtamActorService* service_;
  };
  std::shared_ptr<SetAppUpdateListener> set_app_update_listener_;

  class SetServiceModeResponseListener : public eprosima::fastdds::dds::DataReaderListener {
    public:
        SetServiceModeResponseListener(OtamActorService* service) : service_(service) {}

        void on_data_available(eprosima::fastdds::dds::DataReader* reader) override;

    private:
        /** @brief Pointer to the parent service instance */
        OtamActorService* service_;
  };
  std::shared_ptr<SetServiceModeResponseListener> set_service_mode_response_listener_;

};

#endif  // OTAM_ACTOR_SERVICE_H_