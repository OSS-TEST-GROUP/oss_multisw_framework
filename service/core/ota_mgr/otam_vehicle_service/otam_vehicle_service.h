#ifndef OTAM_VEHICLE_SERVICE_H_
#define OTAM_VEHICLE_SERVICE_H_

#include "otam_common/otam_service.h"
#include "otam_common/otam_dds_impl.h"
#include "otam_common/otam_dds_idl.h"

#include <map>
#include <memory>
#include <string>
#include <cstdint>
#include <functional>
#include <vector>

// VissRequest IDL generated types
#include "vehicle_info_mgr/VissRequest.hpp"
#include "vehicle_info_mgr/VissRequestPubSubTypes.hpp"

// OTA Manager Vehicle Service
// Manages DDS communication for vehicle information requests/responses
class OtamVehicleService : public OtamService {
 public:
  OtamVehicleService();
  ~OtamVehicleService() = default;

  // Initialize the vehicle service and DDS entities
  void Initialize();
  void Start() override;
  void Stop() override;

  // Request vehicle information
  // @param request_id: Unique request ID
  // @param name: Vehicle info name to request (e.g., "VIN", "Model")
  void RequestVehicleInfo(int32_t request_id,
                          const std::string& name);

  // Callback when vehicle info response is received from VISS
  // @param request_id: Request ID
  // @param name: Vehicle info name (path)
  // @param value: Vehicle info value from VISS
  void OnVehicleInfoResponseReceived(
      int32_t request_id,
      const std::string& name,
      const std::string& value);

  std::string GetVehicleInfo(const std::string& topic_name);

  // Register callback for vehicle info response
  // @param callback: Callback function to be called when response is received
  using VehicleInfoCallback = std::function<void(int32_t request_id, const std::string& name, const std::string& value)>;
  void RegisterCallback(VehicleInfoCallback callback);

 private:
  // Registry of DDS abstractions keyed by topic name
  std::map<std::string, std::shared_ptr<OtamDdsImpl<VissRequestTopic>>>
      dds_registry_;

  // Topic name constants
  static constexpr const char* kVissRequestTopic = "/VIN/Vehicle";
  static constexpr const char* kVehicleInfoResponseTopic =
      "/mmf/otamanager/vehicleinfo_service/GetVehicleInfoResponse";

  std::shared_ptr<OtaManager::GetVehicleInfoPubSubType> get_vehicle_info_;
  std::shared_ptr<OtaManager::GetVehicleInfoResponsePubSubType> get_vehicle_info_res_;
  std::shared_ptr<VissRequestTopicPubSubType> viss_request_;
  std::shared_ptr<VissRequestTopicPubSubType> viss_response_;

  // Register a DDS abstraction
  void RegisterDds(const std::string& topic_name,
                   std::shared_ptr<OtamDdsImpl<VissRequestTopic>> dds);

  // Get DDS abstraction by topic name
  std::shared_ptr<OtamDdsImpl<VissRequestTopic>> GetDds(
      const std::string& topic_name);

  class ResponseListener : public eprosima::fastdds::dds::DataReaderListener {
  public:
      ResponseListener(OtamVehicleService* service) : service_(service) {}

      void on_data_available(eprosima::fastdds::dds::DataReader* reader) override;

  private:
      /** @brief Pointer to the parent service instance */
      OtamVehicleService* service_;
  };

  std::shared_ptr<ResponseListener> response_listener_;
  std::map<std::string, std::string> vehicle_info_;

  // Registered callbacks for vehicle info responses
  std::vector<VehicleInfoCallback> callbacks_;
};

#endif  // OTAM_VEHICLE_SERVICE_H_
