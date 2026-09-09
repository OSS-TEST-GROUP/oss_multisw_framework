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
    // Whether to query vehicle state from vehicle_info_mgr (default: disabled)
    bool vehicle_info_enabled{false};
    // Whether to request OTA_MODE via Lifecycle Manager before update (default: enabled)
    bool lifecycle_enabled{true};
    // Minimum battery State-of-Charge (%) required to permit an OTA update
    double battery_threshold_percent{20.0};
    // Max seconds to wait for Lifecycle Manager's SetServiceModeResponse
    int lifecycle_timeout_sec{5};
    // Max seconds to wait for VISS signals after RequestVehicleInfo
    int viss_timeout_sec{3};
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

    virtual bool   IsVehicleInfoEnabled()      const { return config_.vehicle_info_enabled; }
    virtual bool   IsLifecycleEnabled()        const { return config_.lifecycle_enabled; }
    virtual double GetBatteryThresholdPercent() const { return config_.battery_threshold_percent; }
    virtual int    GetLifecycleTimeoutSec()     const { return config_.lifecycle_timeout_sec; }
    virtual int    GetVissTimeoutSec()          const { return config_.viss_timeout_sec; }

    // === ArgoCD Token Management ===
    SessionResult CreateOtaSession(); // now calls ArgoCD API
    ReturnValue    DestroyOtaSession();

    // virtual: 테스트에서 Mock 으로 오버라이드해 actor 의 server-available 경로를 커버.
    virtual AppSettingsResult    GetOtaSettings(const std::string& app_name);
    virtual AppUpdateStateResult GetOtaUpdateState(const std::string& app_name);
    virtual ReturnValue  RunOtaUpdate(const std::string& app_name);
    virtual ReturnValue          SetOtaSettings(const std::string& app_name, const std::string& app_settings_json);

    // ---------------------------------------------------------------------------
    // 유닛테스트 전용 seam (QA/docs/qa) — 실 HTTP(ArgoCD) 없이 API 클라이언트를
    // MockOtamServerApiClient 로 주입해 GetOtaSettings/SetOtaSettings/RunOtaUpdate
    // 로직(엔드포인트 구성·응답 파싱)을 검증한다. 프로덕션 흐름에서는 쓰이지 않는다.
    // ---------------------------------------------------------------------------
    void ForceStartedForTest() { state_ = ServiceState::STARTED; }
    void SetSessionIdForTest(const std::string& id) { session_id_ = id; }
    void InjectApiClientForTest(std::unique_ptr<OtamServerApiClient> c) {
        api_client_ = std::move(c);
    }

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