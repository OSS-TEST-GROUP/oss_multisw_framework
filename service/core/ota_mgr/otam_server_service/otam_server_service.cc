#include "otam_server_service.h"
#include "otam_server_api_client_impl.h"
#include "otam_app_settings.h"
#include "otam_diagnostic_service/otam_diagnostic_service.h"

#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <cstdlib>

using json = nlohmann::json;

void OtamServerService::SetOtamDiagnosticService(OtamDiagnosticService* svc) {
    diagnostic_service_ = svc;
}

// ===== Lifecycle =====
// LCOV_EXCL_START — 프레임워크 경계(Initialize/Start/Stop): 실 ApiClientImpl(libcurl)
// 생성 + 세션 lifecycle. 유닛 불가 → 통합 레인이 검증.
void OtamServerService::Initialize() {
    if (state_ != ServiceState::UNINITIALIZED) {
        std::cout << "[OtamServerService] Already initialized.\n";
        return;
    }
    name_ = "OtamServerService";

    // Initialize CURL globally
    curl_global_init(CURL_GLOBAL_DEFAULT);

    if (const char* env_p = std::getenv("OTAM_CONFIG")) {
        config_path_ = env_p;
    }

    config_ = LoadConfigFromFile(config_path_);
    std::cout << "[OtamServerService] Loaded ArgoCD config: "
              << config_.server_url << " user=" << config_.username << "\n";
    std::cout << "[OtamServerService] vehicle_info_enabled: "
              << (config_.vehicle_info_enabled ? "true" : "false") << "\n";
    std::cout << "[OtamServerService] lifecycle_enabled: "
              << (config_.lifecycle_enabled ? "true" : "false") << "\n";

    state_ = ServiceState::INITIALIZED;
    std::cout << "OtamServerService Initialized." << std::endl;
}

void OtamServerService::Start() {
    if (state_ == ServiceState::STARTED) return;
    if (state_ == ServiceState::UNINITIALIZED) Initialize();

    try {
        if (!api_client_) {
            api_client_ = std::make_unique<OtamServerApiClientImpl>(
                config_.server_url, config_.headers);
        }
        state_ = ServiceState::STARTED;
        std::cout << "OtamServerService Started." << std::endl;

        // Create OTA session (ArgoCD token)
        SessionResult session_result = CreateOtaSession();
        if (session_result.ret_code != ReturnValue::SUCCESS) {
            std::string err_msg = "Failed: OtamServerService could not create OTA session (ArgoCD auth failed)";
            std::cout << "[OtamServerService] " << err_msg << "\n";
            if (diagnostic_service_) {
                diagnostic_service_->PublishOtaMgrInfo(static_cast<int32_t>(OtamError::FAILED), err_msg);
            }
            state_ = ServiceState::UNINITIALIZED;
            return;
        }
    } catch (const std::exception& e) {
        std::string err_msg = "Failed: OtamServerService Start exception: " + std::string(e.what());
        std::cout << "[OtamServerService] " << err_msg << "\n";
        if (diagnostic_service_) {
            diagnostic_service_->PublishOtaMgrInfo(static_cast<int32_t>(OtamError::FAILED), err_msg);
        }
        state_ = ServiceState::UNINITIALIZED;
    }
}

void OtamServerService::Stop() {
    // Cleanup CURL globally
    curl_global_cleanup();

    state_ = ServiceState::UNINITIALIZED;
    std::cout << "OtamServerService Stopped." << std::endl;
}

// ===== ArgoCD Token Creation =====
// LCOV_EXCL_STOP
SessionResult OtamServerService::CreateOtaSession() {
    SessionResult result;
    result.ret_code = ReturnValue::FAIL;

    if (state_ != ServiceState::STARTED || !api_client_) {
        std::cout << "[OtamServerService] Not ready.\n";
        return result;
    }

    try {
        // Build request body for ArgoCD session creation.
        nlohmann::json body = {
            {"username", config_.username},
            {"password", config_.password}
        };

        const std::string endpoint = "/api/v1/session";

        // Add Content-Type header temporarily for JSON POST.
        api_client_->AddHeader("Content-Type", "application/json");

        // Perform POST request.
        const std::string resp = api_client_->Post(endpoint, body.dump());

        // Remove Content-Type after request.
        api_client_->RemoveHeader("Content-Type");

        // Parse response and extract token.
        nlohmann::json j = nlohmann::json::parse(resp, nullptr, true);
        if (j.contains("token") && j["token"].is_string()) {
            session_id_ = j["token"].get<std::string>();
            result.session_id = session_id_;
            result.ret_code = ReturnValue::SUCCESS;

            // Set Authorization header for subsequent requests.
            api_client_->SetAuthToken(session_id_);

            std::cout << "[OtamServerService] Token acquired.\n";
        } else {
            std::cout << "[OtamServerService] Token not found in response.\n";
        }
    } catch (const std::exception& e) {
        std::cout << "[OtamServerService] CreateOtaSession failed: "
                  << e.what() << "\n";
        api_client_->RemoveHeader("Content-Type");
    }

    return result;
}

// ========== Public OTA methods ==========
ReturnValue OtamServerService::DestroyOtaSession() {
    std::cout << "[OtamServerService] DestroyOtaSession Not Supported.\n";
    return ReturnValue::SUCCESS;
}

AppSettingsResult OtamServerService::GetOtaSettings(
    const std::string& app_name) {
    AppSettingsResult result;
    result.ret_code = ReturnValue::FAIL;

    if (state_ != ServiceState::STARTED || !api_client_) {
        std::cout << "[OtamServerService] Not ready.\n";
        return result;
    }
    if (session_id_.empty()) {
        std::cout << "[OtamServerService] No ArgoCD token. "
                  << "Call CreateOtaSession() first.\n";
        return result;
    }

    try {
        // Build endpoint and perform GET. Do not send Content-Type for GET.
        const std::string endpoint =
            "/api/v1/applications/" + app_name;
        const std::string resp = api_client_->Get(endpoint, "");

        // Parse and extract .spec.syncPolicy.automated.
        nlohmann::json j = nlohmann::json::parse(resp, nullptr, true);
        if (j.contains("spec") && j["spec"].contains("syncPolicy") &&
            j["spec"]["syncPolicy"].contains("automated")) {
            result.app_settings =
                j["spec"]["syncPolicy"]["automated"].dump(2);
            result.ret_code = ReturnValue::SUCCESS;
            std::cout << "[OtamServerService] Automated policy for "
                      << app_name << ":\n"
                      << result.app_settings << std::endl;
        } else {
            // Return empty object if automated is not present.
            result.app_settings = "{}";
            std::cout << "[OtamServerService] No automated policy for "
                      << app_name << ".\n";
        }
    } catch (const std::exception& e) {
        std::cout << "[OtamServerService] GetOtaSettings failed: "
                  << e.what() << "\n";
        result.app_settings = "{}";
    }

    return result;
}

AppUpdateStateResult OtamServerService::GetOtaUpdateState(
    const std::string& app_name) {
    AppUpdateStateResult result;
    result.ret_code = ReturnValue::FAIL;

    if (state_ != ServiceState::STARTED || !api_client_) {
        std::cout << "[OtamServerService] Not ready.\n";
        return result;
    }
    if (session_id_.empty()) {
        std::cout << "[OtamServerService] No ArgoCD token. "
                  << "Call CreateOtaSession() first.\n";
        return result;
    }

    try {
        // Build endpoint and perform GET. No Content-Type for GET requests.
        const std::string endpoint =
            "/api/v1/applications/" + app_name;
        const std::string resp = api_client_->Get(endpoint, "");

        // Parse .status.sync.status field.
        nlohmann::json j = nlohmann::json::parse(resp, nullptr, true);
        if (j.contains("status") &&
            j["status"].contains("sync") &&
            j["status"]["sync"].contains("status")) {
            std::string sync_status = j["status"]["sync"]["status"].get<std::string>();

            // Synced: cluster resources match the desired state in Git
            // OutOfSync: cluster resources diverge from Git
            // Unknown: sync status not yet computed

            // Extract revision from .status.sync.revision
            std::string revision = "Unknown";
            if (j["status"]["sync"].contains("revision")) {
                revision = j["status"]["sync"]["revision"].get<std::string>();
            }

            // Extract operationState if exists
            std::string operation_state = "None";
            if (j["status"].contains("operationState") &&
                j["status"]["operationState"].contains("phase")) {
                operation_state = j["status"]["operationState"]["phase"].get<std::string>();
            }
            // operationState phases: Running, Succeeded, Failed, Error, Terminating

            // Create JSON object with status, revision, and operationState fields
            nlohmann::json state_json;
            state_json["status"] = sync_status;
            state_json["revision"] = revision;
            state_json["operationState"] = operation_state;
            result.app_state_info = state_json.dump();
            result.ret_code = ReturnValue::SUCCESS;

            std::cout << "[OtamServerService] Sync status for "
                      << app_name << ": "
                      << result.app_state_info << std::endl;
        } else {
            // Create JSON object with unknown status
            nlohmann::json state_json;
            state_json["status"] = "Failed";
            state_json["revision"] = "Unknown";
            state_json["operationState"] = "None";
            result.app_state_info = state_json.dump();
            std::cout << "[OtamServerService] Sync status not found for "
                      << app_name << ".\n";
        }
    } catch (const std::exception& e) {
        std::cout << "[OtamServerService] GetOtaUpdateState failed: "
                  << e.what() << "\n";
        // Create JSON object with error status
        nlohmann::json state_json;
        state_json["status"] = "error";
        state_json["revision"] = "Unknown";
        state_json["operationState"] = "None";
        result.app_state_info = state_json.dump();
    }

    return result;
}

ReturnValue OtamServerService::RunOtaUpdate(
    const std::string& app_name) {
    if (state_ != ServiceState::STARTED || !api_client_) {
        std::cout << "[OtamServerService] Not ready.\n";
        return ReturnValue::FAIL;
    }
    if (session_id_.empty()) {
        std::cout << "[OtamServerService] No ArgoCD token. "
                  << "Call CreateOtaSession() first.\n";
        return ReturnValue::FAIL;
    }

    try {
        // Endpoint: /api/v1/applications/<APP_NAME>/sync
        const std::string endpoint =
            "/api/v1/applications/" + app_name + "/sync";

        // Body: empty JSON object "{}"
        const std::string body = "{}";

        // Add Content-Type header temporarily for JSON POST.
        api_client_->AddHeader("Content-Type", "application/json");

        // Perform POST request.
        const std::string resp = api_client_->Post(endpoint, body);

        // Remove Content-Type header after request.
        api_client_->RemoveHeader("Content-Type");

        // Check for response keywords such as "status" or "message".
        nlohmann::json j = nlohmann::json::parse(resp, nullptr, true);
        if (j.contains("status") || j.contains("message")) {
            std::cout << "[OtamServerService] Sync triggered for "
                      << app_name << ".\n";
            return ReturnValue::SUCCESS;
        } else {
            std::cout << "[OtamServerService] Sync response unexpected.\n";
            return ReturnValue::FAIL;
        }
    } catch (const std::exception& e) {
        std::cout << "[OtamServerService] RunOtaUpdate failed: "
                  << e.what() << "\n";
        api_client_->RemoveHeader("Content-Type");
        return ReturnValue::FAIL;
    }
}

ReturnValue OtamServerService::SetOtaSettings(
    const std::string& app_name,
    const std::string& app_settings_json) {
    if (state_ != ServiceState::STARTED || !api_client_) {
        std::cout << "[OtamServerService] Not ready.\n";
        return ReturnValue::FAIL;
    }

    if (session_id_.empty()) {
        std::cout << "[OtamServerService] No ArgoCD token. "
                  << "Call CreateOtaSession() first.\n";
        return ReturnValue::FAIL;
    }

    try {
        nlohmann::json body;
        if (app_settings_json == "on" ||
            app_settings_json == "true") {
            body["patch"] =
                R"({"spec":{"syncPolicy":{"automated":{"prune":true,)"
                R"("selfHeal":true}}}})";
        } else if (app_settings_json == "off" ||
                   app_settings_json == "false") {
            body["patch"] =
                R"({"spec":{"syncPolicy":null}})";
        } else {
            std::cout << "[OtamServerService] Invalid argument "
                      << "(use 'on' or 'off').\n";
            return ReturnValue::FAIL;
        }

        body["patchType"] = "merge";
        const std::string endpoint =
            "/api/v1/applications/" + app_name;

        // Add Content-Type header only for this call
        api_client_->AddHeader("Content-Type", "application/json");

        std::string response =
            api_client_->Patch(endpoint, body.dump());

        // Remove temporary header to avoid affecting other calls
        api_client_->RemoveHeader("Content-Type");

        std::cout << "[OtamServerService] Patch response: "
                  << response << std::endl;
        return ReturnValue::SUCCESS;

    } catch (const std::exception& e) {
        std::cout << "[OtamServerService] SetOtaSettings failed: "
                  << e.what() << "\n";
        api_client_->RemoveHeader("Content-Type");
        return ReturnValue::FAIL;
    }
}

// ========== Private helpers ==========
// LCOV_EXCL_START — config 파일 I/O 경계(LoadConfigFromFile): 통합/파일 픽스처 검증.
OtamServerConfig OtamServerService::LoadConfigFromFile(const std::string& path) {
    OtamServerConfig cfg;
    cfg.config_path = path;

    std::ifstream ifs(path);
    if (!ifs.good()) {
        std::cout << "[OtamServerService] config.json not found: " << path << "\n";
        return cfg;
    }
    std::cout << "[OtamServerService] config.json found: " << path << "\n";

    try {
        json j;
        ifs >> j;

        if (j.contains("server")) {
            auto s = j["server"];
            if (s.contains("url"))       cfg.server_url = s["url"].get<std::string>();
            if (s.contains("username"))  cfg.username  = s["username"].get<std::string>();
            if (s.contains("password"))  cfg.password  = s["password"].get<std::string>();
        }
        if (j.contains("vehicle_info_enabled")) {
            cfg.vehicle_info_enabled = j["vehicle_info_enabled"].get<bool>();
        }
        if (j.contains("lifecycle_enabled")) {
            cfg.lifecycle_enabled = j["lifecycle_enabled"].get<bool>();
        }
        if (j.contains("battery_threshold_percent")) {
            cfg.battery_threshold_percent = j["battery_threshold_percent"].get<double>();
        }
        if (j.contains("lifecycle_timeout_sec")) {
            cfg.lifecycle_timeout_sec = j["lifecycle_timeout_sec"].get<int>();
        }
        if (j.contains("viss_timeout_sec")) {
            cfg.viss_timeout_sec = j["viss_timeout_sec"].get<int>();
        }
    } catch (const std::exception& e) {
        std::string err_msg = "Failed: OtamServerService config parse error: " + std::string(e.what());
        std::cout << "[OtamServerService] " << err_msg << "\n";
        if (diagnostic_service_) {
            diagnostic_service_->PublishOtaMgrInfo(static_cast<int32_t>(OtamError::FAILED), err_msg);
        }
    }

    return cfg;
}
