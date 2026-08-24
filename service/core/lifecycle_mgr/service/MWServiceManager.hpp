#ifndef LIFECYCLE_MW_SERVICE_MANAGER_HPP_
#define LIFECYCLE_MW_SERVICE_MANAGER_HPP_

#include <string>
#include <vector>
#include <map>
#include <memory>

#include "MWService.hpp"

namespace lifecycle_manager {

// F011: Previously this class fork()/exec()'d every middleware service listed
// in config_middleware.yaml (diagnostics, OtaManager, etc.).  That role was
// moved to docker/entrypoint/core_entrypoint.sh so each service has its own
// supervisor (Docker/k8s) and lifecycle_mgr stays a single-purpose process.
//
// What's left here:
//   - LoadMWConfig() still parses config_middleware.yaml so we can read
//     lifecycle_mgr_config.k3s_mode (consumed by LifecycleManager).
//   - mw_services_ is still populated so callers can inspect the configured
//     middleware list, but nothing is spawned from it.
// Everything related to fork/exec/wait has been deleted.
class MWServiceManager {
  public:
    MWServiceManager();
    virtual ~MWServiceManager() = default;
    bool Initialize();

    // Start/Stop are intentional no-ops — kept so the call sites in
    // LifecycleManager don't need a special case for the post-F011 contract.
    bool Start();
    void Stop();

    bool LoadMWConfig(const std::string& path);

    bool IsK3sMode() const;

  private:
    std::map<std::string, std::shared_ptr<MWService>> mw_services_;
    bool k3s_mode_{false};
};

} // namespace lifecycle_manager

#endif // LIFECYCLE_MW_SERVICE_MANAGER_HPP_
