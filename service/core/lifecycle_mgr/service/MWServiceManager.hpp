#ifndef LIFECYCLE_MW_SERVICE_MANAGER_HPP_
#define LIFECYCLE_MW_SERVICE_MANAGER_HPP_

#include <string>
#include <vector>
#include <map>
#include <memory>

#include "MWService.hpp"

namespace lifecycle_manager {

class MWServiceManager {
  public:
    MWServiceManager();
    virtual ~MWServiceManager();

    bool Initialize();

    bool Start();
    void Stop();

    void LoadMWConfig(std::string path);

    void StartMWService(std::string mw_service);
    void StopMWService(std::string mw_service);

    void RunMWServiceManagement();

  private:
     std::map<std::string, std::shared_ptr<MWService>> mw_services_;
};

} // namespace lifecycle_manager

#endif // LIFECYCLE_MW_SERVICE_MANAGER_HPP_
