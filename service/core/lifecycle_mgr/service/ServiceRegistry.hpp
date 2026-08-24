#ifndef LIFECYCLE_SERVICE_REGISTRY_HPP_
#define LIFECYCLE_SERVICE_REGISTRY_HPP_

#include <string>
#include <map>
#include <memory>
#include <vector>
#include <mutex>

#include "Container.hpp"
#include "Service.hpp"

#include "lifecycle/UpdateServiceListRequest.hpp"

namespace lifecycle_manager {

using namespace lifecyclemgr;

class ServiceRegistry {
  public:
    ServiceRegistry();
    virtual ~ServiceRegistry() = default;
    bool UpdateServiceList(UpdateServiceListRequest* update);

    void AddGroup(const std::string& name);
    void RemoveGroup(const std::string& name);
    bool IsGroup(std::string name);

    bool AddService(const std::shared_ptr<Service>& service, std::string group);
    void RemoveService(const std::string& name);

    std::shared_ptr<Service> GetService(const std::string& name);
    std::vector<std::shared_ptr<Service>> GetServicesByGroup(const std::string& group);
    std::vector<std::string> GetServicesNameByGroup(const std::string& group);
    std::string GetGroupNameByService(const std::shared_ptr<Service>& service);
    std::vector<std::string> GetGroupList();

    uint64_t GetServiceNum();
    uint64_t GetServiceNumInGroup(const std::string& group);
    uint64_t GetGroupNum();

    // ── F013: Container registry surface ────────────────────────────────
    // Populated by UpdateServiceListRequest.container_list (adsw_manager
    // now also publishes its known containers) and refreshed by
    // NotifyContainerState arrivals.
    std::shared_ptr<Container> GetContainer(const std::string& name);
    std::vector<std::shared_ptr<Container>> GetAllContainers();
    void UpsertContainer(const std::shared_ptr<Container>& c);
    // Returns true if the stored state actually changed (or the container was
    // seen for the first time), so callers can publish an aggregated report
    // on-change only — mirror of the service publish-on-change path.
    bool UpdateContainerState(const std::string& name,
                              ContainerState state,
                              int32_t exit_code);
    // Returns the names of services known to live in the given container.
    // Uses Service::GetContainerName() set during UpdateServiceList.
    std::vector<std::string> GetServicesInContainer(const std::string& name);

  private:
    bool hasGroup(const std::string& name);
    std::shared_ptr<std::vector<std::string>> getGroup(const std::string& name);

    bool vectorAdd(std::vector<std::string>& vec, const std::string& item);
    bool vectorRemove(std::vector<std::string>& vec, const std::string& item);
    bool vectorHas(std::vector<std::string> vec, const std::string& item);

    bool hasService(const std::string& name);
    std::shared_ptr<Service> getService(const std::string& name);
    std::string newGroup();

  private:
    std::map<std::string, std::shared_ptr<Service>> services_;
    std::map<std::string, std::shared_ptr<std::vector<std::string>>> groups_;
    // F013: container metadata keyed by container_name. Distinct from
    // groups_ — groups_ remains a registration-unit abstraction, containers_
    // is the per-container metadata + last-known state.
    std::map<std::string, std::shared_ptr<Container>> containers_;
    std::mutex lock_mutex_;

    UpdateServiceListResponse   udpate_response_;

};

} // namespace lifecycle_manager

#endif // LIFECYCLE_SERVICE_REGISTRY_HPP_

