#ifndef ROS2_ADSW_MANAGER_SERIVCE_SERVICE_REGISTRY_HPP_
#define ROS2_ADSW_MANAGER_SERIVCE_SERVICE_REGISTRY_HPP_

#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "RecoveryPolicy.hpp"
#include "Service.hpp"
#include "ServiceState.hpp"
#include "ServiceType.hpp"
#include "lifecycle/GetServiceStateRequest.hpp"

namespace ros2 {
namespace adsw_manager {

class ServiceRegistry {
public:
  ServiceRegistry();
  virtual ~ServiceRegistry();

  bool LoadServiceList();

  void AddGroup(const std::string &name);
  void RemoveGroup(const std::string &name);
  bool IsGroup(const std::string &name);

  bool AddService(std::shared_ptr<Service> service, const std::string &group);
  void RemoveService(const std::string &name);

  std::shared_ptr<Service> GetService(const std::string &name);
  std::vector<std::shared_ptr<Service>>
  GetServicesByGroup(const std::string &group);
  std::string GetGroupNameByService(std::shared_ptr<Service> service);

  uint64_t GetServicesNum();
  uint64_t GetServiceNumInGroup(const std::string &group);
  uint64_t GetGroupNum();

  bool
  ProcessGetServiceStateRequest(lifecyclemgr::GetServiceStateRequest *message);

  bool LoadServiceList(const std::string &path);
  std::vector<std::tuple<std::string, std::string, std::string, std::string>>
  extractRos2LaunchCommands(const std::string &script);
  std::vector<std::tuple<std::string, std::string, std::string, std::string>>
  extractRos2RunCommands(const std::string &script);
  bool ProcessUpdateServiceListRequest();

  bool Save();
  bool LoadServiceSaved(const std::string &path);

private:
  // recovery 값 변경 여부 확인: 마지막으로 lifecycle_mgr에 전송한 값과 비교
  bool HasRecoveryChanged() const;
  void UpdateLastSentRecovery();

  std::unordered_map<std::string, std::shared_ptr<Service>> services_;
  std::unordered_map<std::string, std::set<std::shared_ptr<Service>>> group_;

  lifecyclemgr::GetServiceStateResponse get_service_response_;
  bool ready_service_list_;
  std::string yaml_path_;

  // 마지막으로 lifecycle_mgr에 전송한 recovery policy 값
  std::unordered_map<std::string, RecoveryPolicy> last_sent_recovery_;
};

} // namespace adsw_manager
} // namespace ros2

#endif
