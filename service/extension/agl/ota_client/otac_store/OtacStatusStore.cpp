#include "OtacStatusStore.hpp"

namespace ota_client::agl {

    bool OtacStatusStore::Initialize() {
        app_status.emplace("adsw_monitor", "installed");
        return true;
    }

    std::string OtacStatusStore::GetStatus(std::string app_name) {
        return app_status[app_name];
    }

    bool OtacStatusStore::UpdateStatus(std::string app_name, std::string status) {
        app_status[app_name] = status;
        if (app_status.find(app_name)->second == status) {
            return true;
        }
        return false;
    }

}