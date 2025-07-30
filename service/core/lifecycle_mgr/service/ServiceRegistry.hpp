#ifndef LIFECYCLE_SERVICE_REGISTRY_HPP_
#define LIFECYCLE_SERVICE_REGISTRY_HPP_

#include <string>
#include <map>
#include <memory>
#include <vector>
#include <mutex>

#include "Service.hpp"

#include "lifecycle/UpdateServiceListRequest.hpp"

namespace lifecycle_manager {

using namespace lifecyclemgr;

class ServiceRegistry {
  public:
    ServiceRegistry();
    virtual ~ServiceRegistry();

    bool UpdateServiceList(UpdateServiceListRequest* update);

    void AddGroup(std::string name);
    void RemoveGroup(std::string name);
    bool IsGroup(std::string name);

    bool AddService(std::shared_ptr<Service> service, std::string group);
    void RemoveService(std::string name);

    std::shared_ptr<Service> GetService(std::string name);
    std::vector<std::shared_ptr<Service>> GetServicesByGroup(std::string group);
    std::vector<std::string> GetServicesNameByGroup(std::string group);
    std::string GetGroupNameByService(std::shared_ptr<Service> service);
    std::vector<std::string> GetGroupList();

    uint64_t GetServiceNum();
    uint64_t GetServiceNumInGroup(std::string group);
    uint64_t GetGroupNum();

  private:
    bool hasGroup(std::string name);
    std::shared_ptr<std::vector<std::string>> getGroup(std::string name);

    bool vectorAdd(std::vector<std::string>& vec, std::string item);
    bool vectorRemove(std::vector<std::string>& vec, std::string item);
    bool vectorHas(std::vector<std::string> vec, std::string item);

    bool hasService(std::string name);
    std::shared_ptr<Service> getService(std::string name);
    std::string newGroup();

  private:
    std::map<std::string, std::shared_ptr<Service>> services_;
    std::map<std::string, std::shared_ptr<std::vector<std::string>>> groups_;
    std::mutex lock_mutex_; 
    
    UpdateServiceListResponse   udpate_response_;

};

} // namespace lifecycle_manager

#endif // LIFECYCLE_SERVICE_REGISTRY_HPP_

