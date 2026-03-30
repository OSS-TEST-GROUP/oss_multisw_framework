#include "OtacStatusStore.hpp"

#include <cstdio>
#include <fstream>
#include <nlohmann/json.hpp>

namespace ros2::ota_client
{

    void OtacStatusStore::LoadDefaults()
    {
        if (client_name_.empty()) {
            client_name_ = "ros2_ota_client";
        }
        app_status.emplace("adsw_monitor", "installed");
    }

    bool OtacStatusStore::Initialize(const std::string& config_path)
    {
        if (!config_path.empty()) {
            std::ifstream file(config_path);
            if (!file.is_open()) {
                fprintf(stderr, "[OtacStatusStore] Warning: cannot open config file: %s - using defaults\n",
                        config_path.c_str());
                LoadDefaults();
                return true;
            }
            try {
                nlohmann::json j;
                file >> j;

                if (j.contains("client_name") && j["client_name"].is_string()) {
                    client_name_ = j["client_name"].get<std::string>();
                }

                if (j.contains("apps") && j["apps"].is_array()) {
                    for (const auto& app : j["apps"]) {
                        if (app.contains("name") && app.contains("status")) {
                            app_status.emplace(app["name"].get<std::string>(),
                                               app["status"].get<std::string>());
                        }
                    }
                }

                if (app_status.empty()) {
                    fprintf(stderr, "[OtacStatusStore] Warning: no apps in config - using defaults\n");
                    LoadDefaults();
                }
                return true;
            } catch (const nlohmann::json::exception& e) {
                fprintf(stderr, "[OtacStatusStore] Warning: JSON parse error in %s: %s - using defaults\n",
                        config_path.c_str(), e.what());
                app_status.clear();
                LoadDefaults();
                return true;
            }
        }

        LoadDefaults();
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

    const std::string& OtacStatusStore::GetClientName() const
    {
        if (client_name_.empty()) {
            static const std::string kDefault = "ros2_ota_client";
            return kDefault;
        }
        return client_name_;
    }

}
