#include "OtacStatusStore.hpp"

namespace ros2::ota_client
{

    bool OtacStatusStore::Initialize()
    {
        app_status.emplace("adsw_monitor", "installed");
        return true;
    }

    std::optional<std::string> OtacStatusStore::GetStatus(const std::string& app_name) const
    {
        auto it = app_status.find(app_name);
        if (it == app_status.end()) return std::nullopt;
        return it->second;
    }

    std::vector<std::string> OtacStatusStore::GetAppNameList() const
    {
        std::vector<std::string> keys;
        keys.reserve(app_status.size());

        for (const auto &[k, _] : app_status) keys.push_back(k);

        return keys;
    }

    bool OtacStatusStore::UpdateStatus(const std::string app_name, const std::string status)
    {
        app_status[app_name] = status;
        if (app_status.find(app_name)->second == status)
        {
            return true;
        }
        return false;
    }

}