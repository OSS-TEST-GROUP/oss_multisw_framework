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

    bool LoadMWConfig(std::string path);

    void StartMWService(std::shared_ptr<MWService> mw_service);
    void StopMWService(std::shared_ptr<MWService> mw_service);

    void RunMWServiceManagement();

  private :
    pid_t run_process(const std::vector<std::string>& args);
    bool kill_process(pid_t pid);

  private:
     std::map<std::string, std::shared_ptr<MWService>> mw_services_;
};

} // namespace lifecycle_manager

#endif // LIFECYCLE_MW_SERVICE_MANAGER_HPP_
