#ifndef LIFECYCLE_SERVICE_REGISTRY_HPP_
#define LIFECYCLE_SERVICE_REGISTRY_HPP_

#include <string>
#include <map>
#include <memory>
#include <vector>

#include "Service.hpp"

namespace lifecycle_manager {

class ServiceRegistry {
  public:
    ServiceRegistry();
    virtual ~ServiceRegistry();

    bool UpdateServiceList();

    void AddGroup(std::string name);
    void RemoveGroup(std::string name);
    bool IsGroup(std::string name);

    bool AddService(std::string service, std::string group);
    void RemoveService(std::string name);

    std::shared_ptr<Service> GetService(std::string name);
    std::vector<std::shared_ptr<Service>> GetServicesByGroup(std::string group);
    std::string GetGroupNameByService(std::string service);

    uint64_t GetServiceNum();
    uint64_t GetServiceNumInGroup(std::string group);
    uint64_t GetGroupNum();


  private:
    std::map<std::string, std::shared_ptr<Service>> services_;
};

} // namespace lifecycle_manager

#endif // LIFECYCLE_SERVICE_REGISTRY_HPP_
