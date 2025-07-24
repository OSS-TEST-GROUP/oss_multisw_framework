#ifndef ROS2_ADSW_MANAGER_SERIVCE_SERVICE_REGISTRY_HPP_
#define ROS2_ADSW_MANAGER_SERIVCE_SERVICE_REGISTRY_HPP_

#include <iostream>
#include <string>
#include <memory>
#include <unordered_map>
#include <set>
#include <vector>

#include "Service.hpp"
#include "core/lifecycle_mgr/include/ServiceState.hpp"
#include "core/lifecycle_mgr/include/ServiceType.hpp"
#include "core/lifecycle_mgr/include/RecoveryPolicy.hpp"
#include "lifecycle/GetServiceStateRequest.hpp"


using namespace lifecycle_manager;

namespace ros2 {
namespace adsw_manager {


class ServiceRegistry {
  public:
    ServiceRegistry();
    virtual ~ServiceRegistry();

    bool LoadServiceList();

    void AddGroup(const std::string& name);
    void RemoveGroup(const std::string& name);
    bool IsGroup(const std::string& name);

    bool AddService(std::shared_ptr<Service> service, const std::string& group);
    void RemoveService(const std::string& name);

    std::shared_ptr<Service> GetService(const std::string& name);
    std::vector<std::shared_ptr<Service>> GetServicesByGroup(const std::string& group);
    std::string GetGroupNameByService(std::shared_ptr<Service> service);

    uint64_t GetServicesNum();
    uint64_t GetServiceNumInGroup(const std::string& group);
    uint64_t GetGroupNum();

    bool ProcessGetServiceStateRequest(lifecyclemgr::GetServiceStateRequest* message);

    bool LoadServiceList(const std::string& path);
    std::vector<std::tuple<std::string, std::string, std::string, std::string>> extractRos2LaunchCommands(const std::string& script);
    std::vector<std::tuple<std::string, std::string, std::string, std::string>> extractRos2RunCommands(const std::string& script);
    bool ProcessUpdateServiceListRequest();

    bool Save();
    bool LoadServiceSaved(const std::string& path);

  private:
    std::unordered_map<std::string, std::shared_ptr<Service>> services_;
    std::unordered_map<std::string, std::set<std::shared_ptr<Service>>> group_;

    lifecyclemgr::GetServiceStateResponse get_service_response_;
};

} // namespace adsw_manager
} // namespace ros2

#endif
