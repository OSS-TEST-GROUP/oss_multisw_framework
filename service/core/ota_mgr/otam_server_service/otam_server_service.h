#pragma once

#include <memory>
#include <map>
#include <string>
#include "otam_common/service.h"
#include "otam_common/api_client.h"
#include "otam_app_settings.h"
#include "otam_server_result_types.h"

class OtamServerApiClientImpl;

struct OtamServerConfig {
    std::string config_path;
    std::string server_url{"https://localhost:8080"};
    std::string username;
    std::string password;
    std::map<std::string, std::string> headers;
};

class OtamServerService : public Service {
public:
    OtamServerService() = default;
    ~OtamServerService() override = default;

    void Initialize() override;
    void Start() override;
    void Stop() override;

    void SetConfigPath(std::string path);

    // === ArgoCD Token 생성 ===
    SessionResult CreateOtaSession(const std::string& user_info_json) override; // now calls ArgoCD API
    ResultCode    DestroyOtaSession();

    AppSettingsResult    GetOtaSettings(const std::string& app_name);
    AppUpdateStateResult GetOtaUpdateState(const std::string& app_name);
    ResultCode           RunOtaUpdate(const std::string& app_name);
    ResultCode           SetOtaSettings(const std::string& app_name, const std::string& app_settings_json);

private:
    OtamServerConfig LoadConfigFromFile(const std::string& path);
    std::map<std::string, std::string> ParseResponse(const std::string& response_json);

private:
    std::string                          session_id_;
    std::shared_ptr<OtamAppSettings>     app_settings_;
    std::unique_ptr<ApiClient>           api_client_;
    std::string                          config_path_{"config.json"};
    OtamServerConfig                     config_;
};