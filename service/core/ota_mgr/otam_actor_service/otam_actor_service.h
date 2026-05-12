#ifndef OTAM_ACTOR_SERVICE_H_
#define OTAM_ACTOR_SERVICE_H_

#include "otam_common/otam_service.h"
#include "otam_common/otam_dds_impl.h"
#include "otam_common/otam_dds_idl.h"

// Include SetServiceModeRequest for lifecycle management
#include "lifecycle/SetServiceModeRequest.hpp"
#include "lifecycle/SetServiceModeRequestPubSubTypes.hpp"

// OtaClientInfo is the value type passed to BuildGetAppListResponse below.
// Full definition is required because std::map<K,V> needs sizeof(V) here.
#include "otam_client_service/otam_client_service.h"

#include <map>
#include <memory>
#include <string>
#include <mutex>
#include <set>
#include <functional>
#include <thread>
#include <vector>
#include <atomic>

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
      const otamanager::GetAppUpdateSettings& msg);

  /**
   * @brief Handle SetAppUpdateSettings request
   * @param msg SetAppUpdateSettings message
   */
  void OnSetAppUpdateSettingsReceived(
      const otamanager::SetAppUpdateSettings& msg);

  /**
   * @brief Handle GetAppUpdateState request
   * @param msg GetAppUpdateState message
   */
  void OnAppUpdateStateRequested(
      const otamanager::GetAppUpdateState& msg);

  /**
   * @brief Handle SetAppUpdate request
   * @param msg SetAppUpdate message
   */
  void OnAppUpdateRequested(const otamanager::SetAppUpdate& msg);

  /**
   * @brief Handle GetAppList request — snapshots OtamClientService's
   *        registry, builds the response, and publishes it on
   *        kTopicGetAppListResponse (and the rt/ rosbridge variant).
   */
  void OnGetAppListRequested(const otamanager::GetAppList& msg);

  /**
   * @brief Pure aggregator. Builds a GetAppListResponse from a snapshot
   *        of (client_id -> OtaClientInfo). Static so unit tests can
   *        exercise the dedupe / sort / "lowest client_id wins"
   *        semantics without standing up DDS.
   */
  static otamanager::GetAppListResponse BuildGetAppListResponse(
      int64_t request_id,
      const std::map<int32_t, OtaClientInfo>& clients);

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
   * @brief Test/observability hook: number of SetServiceMode DDS writes attempted
   */
  int GetLifecycleRequestCount() const { return lifecycle_request_count_.load(); }

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
      pending_info.insert("Vehicle.LowVoltageSystemState");
      pending_info.insert("Vehicle.Speed");
      pending_info.insert("Vehicle.Powertrain.TractionBattery.StateOfCharge.Current");
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

  /**
   * @brief Background notification threads (managed, not detached)
   */
  std::vector<std::thread> notify_threads_;

  /**
   * @brief Flag to stop background threads on shutdown
   */
  std::atomic<bool> stopping_{false};

  /**
   * @brief Mutex to protect notify_threads_
   */
  std::mutex notify_threads_mutex_;

  // Topic name constants (raw DDS)
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
  // GetAppList request/response topics (relocated from OtamClientService —
  // ota_monitor now talks to otaactor_service exclusively).
  static constexpr const char* kTopicGetAppList =
      "/mmf/otamanager/otaactor_service/GetAppList";
  static constexpr const char* kTopicGetAppListResponse =
      "/mmf/otamanager/otaactor_service/GetAppListResponse";
  static constexpr const char* kTopicSetServiceModeRequest =
      "mmf/lifecyclemgr/SetServiceModeRequest";
  static constexpr const char* kTopicSetServiceModeResponse =
      "mmf/lifecyclemgr/SetServiceModeResponse";

  // ROS 2 / rosbridge interop is provided by service/extension/ros2/dds_bridge/
  // (a dynamic-IDL forwarder running inside the rosbridge container). The
  // bridge subscribes to these raw bare-DDS topics and republishes on the
  // rt/-prefixed equivalents using ROS-canonical type names. The earlier
  // attempt at registering the rt/ topics from OtaManager itself never
  // matched on the wire (raw IDL TypeObject vs rosidl's TypeObject) so
  // those constants were removed.

  /**
   * @brief DDS registry mapping topic names to DDS implementations
   */
  std::map<std::string, std::unique_ptr<OtamDds>> dds_registry_;

  /**
   * @brief Create DDS implementation for a specific topic
   * @tparam MessageType The DDS message type
   * @param topic_name Topic name
   * @param type_support TypeSupport shared pointer
   * @param listener DataReaderListener shared pointer (optional)
   */
  template<typename MessageType>
  void CreateDdsForTopic(
      const std::string& topic_name,
      std::shared_ptr<void> type_support,
      std::shared_ptr<void> listener = nullptr) {
    auto dds_impl = std::make_unique<OtamDdsImpl<MessageType>>();
    dds_impl->Create(topic_name, type_support, listener);
    dds_registry_[topic_name] = std::move(dds_impl);
  }

  /**
   * @brief Process pending update request when all vehicle info is collected
   * @param request Pending update request
   */
  void ProcessPendingUpdate(const PendingUpdateRequest& request);

  /**
   * @brief Write a SetServiceModeRequest for the given mode. Does not start a timer.
   */
  void WriteServiceModeRequestInternal(int32_t request_id, lifecyclemgr::ServiceMode mode);

  /**
   * @brief Start a timeout watcher for the given request. If no response arrives
   *        within lifecycle_timeout_sec, fails the request and restores NORMAL_MODE.
   */
  void StartLifecycleTimeoutWatcher(int32_t request_id);

  /**
   * @brief Start a VISS signal timeout watcher. If pending_info is not empty after
   *        viss_timeout_sec, the update is aborted with a FAIL response.
   */
  void StartVissTimeoutWatcher(int32_t request_id);


  /**
   * @brief Count of SetServiceMode DDS writes attempted (test hook).
   */
  std::atomic<int> lifecycle_request_count_{0};

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

  class GetAppListListener : public eprosima::fastdds::dds::DataReaderListener {
    public:
        GetAppListListener(OtamActorService* service) : service_(service) {}

        void on_data_available(eprosima::fastdds::dds::DataReader* reader) override;

    private:
        /** @brief Pointer to the parent service instance */
        OtamActorService* service_;
  };
  std::shared_ptr<GetAppListListener> get_app_list_listener_;

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