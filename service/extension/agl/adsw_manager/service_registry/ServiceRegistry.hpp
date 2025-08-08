#ifndef SERVICE_REGISTRY_HPP_
#define SERVICE_REGISTRY_HPP_

#include "common/AppUtils.hpp"

#include <string>
#include <vector>

namespace adsw_manager::agl {

    class ServiceRegistry {

        public:
            virtual bool Initialize();
            virtual std::vector<adsw_manager_agl::AppInfo> GetAppList();
            void AddApp(adsw_manager_agl::AppInfo& info);
            void RemoveApp(const std::string& app_name);
            virtual void UpdateAppState(const std::string& app_name, adsw_manager_agl::AppState state);
            virtual const adsw_manager_agl::AppInfo* GetAppInfo(std::string app_name);

            virtual ~ServiceRegistry() = default;

        private:
            std::vector<adsw_manager_agl::AppInfo> app_list_;
// 
    };

} // namespace adsw_manager::agl

#endif