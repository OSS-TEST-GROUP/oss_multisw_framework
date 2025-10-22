#ifndef OTAM_APP_SETTINGS_H_
#define OTAM_APP_SETTINGS_H_
#include <string>
#include <map>
#include <stdexcept>

struct AppSetting {
    std::string app_name;
    bool        auto_update{false};
    std::string version;
    std::string update_url;
    std::string settings;   // raw JSON string (app-specific)
};

class OtamAppSettings {
public:
    AppSetting GetSetting(const std::string& app_name) const {
        auto it = app_settings_map.find(app_name);
        if (it == app_settings_map.end()) {
            throw std::runtime_error("AppSetting not found: " + app_name);
        }
        return it->second;
    }

    void AddOrUpdateSetting(const AppSetting& setting) {
        app_settings_map[setting.app_name] = setting;
    }

    const std::map<std::string, AppSetting>& All() const { return app_settings_map; }

private:
    std::map<std::string, AppSetting> app_settings_map; // app_name -> setting
};

#endif  // OTAM_APP_SETTINGS_H_
