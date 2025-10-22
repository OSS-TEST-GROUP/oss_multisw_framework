#ifndef OTAM_DIAGNOSTIC_SERVICE_H_
#define OTAM_DIAGNOSTIC_SERVICE_H_

#include "otam_common/otam_service.h"
#include "otam_common/otam_dds_impl.h"
#include "otam_common/otam_dds_idl.h"

#include <map>
#include <memory>
#include <string>
#include <cstdint>

// OTA Manager Diagnostic Service
// Manages DDS communication for OTA application state updates
class OtamDiagnosticService : public OtamService {
 public:
  OtamDiagnosticService();
  ~OtamDiagnosticService() = default;

  // Initialize the diagnostic service and DDS entities
  void Initialize() override;
  void Start() override;
  void Stop() override;

  // Publish OTA manager information with error code and message
  // @param err_code: Error code representing OTA state
  // @param err_msg: Error message describing the error
  void PublishOtaMgrInfo(int32_t err_code, const std::string& err_msg);

 private:
  // Registry of DDS abstractions keyed by topic name
  std::map<std::string, std::shared_ptr<OtamDdsImpl>>
      dds_registry_;

  // Flag indicating if service is initialized
  bool is_initialized_;

  // Topic name constant
  static constexpr const char* kOtaMgrInfoTopic = "/mmf/otamanager/diagnostic_service/NotifyOtaMgrInfo";

  std::shared_ptr<OtaManager::NotifyOtaMgrInfoPubSubType> notify_ota_mgr_info_;

  // Register a DDS abstraction
  void RegisterDds(const std::string& topic_name,
                   std::shared_ptr<OtamDdsImpl> dds);

  // Get DDS abstraction by topic name
  std::shared_ptr<OtamDdsImpl> GetDds(
      const std::string& topic_name);
};

#endif  // OTAM_DIAGNOSTIC_SERVICE_H_