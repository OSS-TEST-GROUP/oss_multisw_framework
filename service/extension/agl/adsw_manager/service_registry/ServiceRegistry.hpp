#ifndef SERVICE_REGISTRY_HPP_
#define SERVICE_REGISTRY_HPP_

#include "lifecycle/UpdateServiceListRequest.hpp"
#include "common/AppUtils.hpp"

#include <string>
#include <vector>
#include <optional>
#include <map>

namespace adsw_manager::agl {

    class ServiceRegistry {

        public:
            virtual bool Initialize();
            
            virtual std::vector<lifecyclemgr::Service> GetAppList() const;
            virtual std::optional<std::pair<lifecyclemgr::Service, lifecyclemgr::ServiceState>> GetAppInfo(const std::string& app_name) const;
            virtual std::vector<std::pair<std::string, lifecyclemgr::ServiceState>> GetNameStatePairs() const;
            virtual lifecyclemgr::ServiceState GetAppState(const std::string& app_name) const;

            void AddApp(const lifecyclemgr::Service& info);
            void AddApp(const lifecyclemgr::Service& info, lifecyclemgr::ServiceState init_state);
            void RemoveApp(const std::string& app_name);
            virtual void UpdateAppState(const std::string& app_name, lifecyclemgr::ServiceState state);

            virtual ~ServiceRegistry() = default;

        private:
            //std::vector<lifecyclemgr::Service> app_list_;
            std::map<lifecyclemgr::Service, lifecyclemgr::ServiceState> app_list_;

    };

} // namespace adsw_manager::agl

#endif