#ifndef LIFECYCLE_MW_SERVICE_HPP_
#define LIFECYCLE_MW_SERVICE_HPP_

#include <string>
#include <vector>

#include "ArgumentConfig.hpp"

namespace lifecycle_manager {

class MWService {
  public:
    MWService(int32_t code, const std::string& name, const std::string& exepath, bool active, const std::vector<ArgumentConfig>& args);
    virtual ~MWService() = default;
    bool IsActive();

    std::string GetName();
    std::string GetPath();
    std::vector<std::string> GetArgs();

    void SetPID(pid_t pid);
    pid_t GetPID();    

  private:
    int32_t code_;
    bool active_;
    pid_t pid_;
    std::string mw_service_name_;
    std::string mw_service_path_;
    std::vector<ArgumentConfig> mw_run_args_;

};

} // namespace lifecycle_manager

#endif // LIFECYCLE_MW_SERVICE_HPP_
