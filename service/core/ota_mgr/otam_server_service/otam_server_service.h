#ifndef OTAM_SERVER_SERVICE_H_
#define OTAM_SERVER_SERVICE_H_

#include <memory>
#include <map>
#include <string>
#include "otam_common/otam_service.h"
#include "otam_server_api_client.h"
#include "otam_app_settings.h"
#include "otam_server_result_types.h"

class OtamServerApiClientImpl;
class OtamDiagnosticService;

struct OtamServerConfig {
    std::string config_path;
    std::string server_url{"http://localhost:8080"};
    std::string username;
    std::string password;
    std::map<std::string, std::string> headers;
};

class OtamServerService : public OtamService {
public:
    OtamServerService() = default;
    ~OtamServerService() override = default;

    void Initialize() override;
    void Start() override;
    void Stop() override;

    void SetConfigPath(std::string path);
    void SetOtamDiagnosticService(OtamDiagnosticService* svc);

    // === ArgoCD Token 생성 ===
    SessionResult CreateOtaSession(); // now calls ArgoCD API
    ReturnValue    DestroyOtaSession();

    AppSettingsResult    GetOtaSettings(const std::string& app_name);
    AppUpdateStateResult GetOtaUpdateState(const std::string& app_name);
    ReturnValue           RunOtaUpdate(const std::string& app_name);
    ReturnValue           SetOtaSettings(const std::string& app_name, const std::string& app_settings_json);

private:
    OtamServerConfig LoadConfigFromFile(const std::string& path);
    
private:
    std::string                          session_id_;
    std::shared_ptr<OtamAppSettings>     app_settings_;
    std::unique_ptr<OtamServerApiClient>           api_client_;
    std::string                          config_path_{"config/otam_config.json"};
    OtamServerConfig                     config_;
    OtamDiagnosticService*               diagnostic_service_{nullptr};
};

#endif //OTAM_SERVER_SERVICE_H_