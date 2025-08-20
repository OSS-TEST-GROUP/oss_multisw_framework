#ifndef SERVICE_REGISTRY_HPP_
#define SERVICE_REGISTRY_HPP_

#include "lifecycle/LifecycleDiagReport.hpp"
#include "common/AppUtils.hpp"

#include <string>
#include <vector>

namespace adsw_manager::agl {

    class ServiceRegistry {

        public:
            virtual bool Initialize();
            virtual std::vector<lifecyclemgr::ServiceStatus> GetAppList();
            void AddApp(lifecyclemgr::ServiceStatus& info);
            void RemoveApp(const std::string& app_name);
            virtual void UpdateAppState(const std::string& app_name, lifecyclemgr::ServiceState state);
            virtual const lifecyclemgr::ServiceStatus* GetAppInfo(std::string app_name);

            virtual ~ServiceRegistry() = default;

        private:
            std::vector<lifecyclemgr::ServiceStatus> app_list_;
// 
    };

} // namespace adsw_manager::agl

#endif