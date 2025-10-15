#include "otam_server_service.hpp"
#include "otam_server_api_client_impl.hpp"
#include "otam_app_settings.hpp"

#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <cstdlib>

using json = nlohmann::json;

// ===== Lifecycle =====
void OtamServerService::Initialize() {
    if (state_ != ServiceState::Uninitialized) {
        std::cout << "[OtamServerService] Already initialized.\n";
        return;
    }

    if (const char* env_p = std::getenv("OTAM_CONFIG")) {
        config_path_ = env_p;
    }

    config_ = LoadConfigFromFile(config_path_);
    std::cout << "[OtamServerService] Loaded ArgoCD config: "
              << config_.server_url << " user=" << config_.username << "\n";

    state_ = ServiceState::Initialized;
    std::cout << "OtamServerService Initialized." << std::endl;
}

void OtamServerService::Start() {
    if (state_ == ServiceState::Started) return;
    if (state_ == ServiceState::Uninitialized) Initialize();

    try {
        if (!api_client_) {
            api_client_ = std::make_unique<OtamServerApiClientImpl>(
                config_.server_url, config_.headers);
        }
        state_ = ServiceState::Started;
        std::cout << "OtamServerService Started." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[OtamServerService] Start failed: " << e.what() << "\n";
        state_ = ServiceState::Uninitialized;
    }
}

void OtamServerService::Stop() {
    state_ = ServiceState::Stopped;
    std::cout << "OtamServerService Stopped." << std::endl;
}

// ===== ArgoCD Token 생성 =====
SessionResult OtamServerService::CreateOtaSession(
    const std::string& /*user_info_json*/) {
    SessionResult result;
    result.ret_code = ResultCode::FAIL;

    if (state_ != ServiceState::Started || !api_client_) {
        std::cerr << "[OtamServerService] Not ready.\n";
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
            result.ret_code = ResultCode::SUCCESS;

            // Set Authorization header for subsequent requests.
            api_client_->SetAuthToken(session_id_);

            std::cout << "[OtamServerService] Token acquired.\n";
        } else {
            std::cerr << "[OtamServerService] Token not found in response.\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "[OtamServerService] CreateOtaSession failed: "
                  << e.what() << "\n";
        api_client_->RemoveHeader("Content-Type");
    }

    return result;
}

// ===== Helper =====
std::map<std::string, std::string> OtamServerService::ParseResponse(const std::string& response_json) {
    std::map<std::string, std::string> out;
    try {
        auto j = json::parse(response_json);
        for (auto& [k, v] : j.items()) {
            out[k] = v.is_string() ? v.get<std::string>() : v.dump();
        }
    } catch (...) {
        std::cerr << "[OtamServerService] Failed to parse JSON response.\n";
    }
    return out;
}

// ========== Public OTA methods ==========
ResultCode OtamServerService::DestroyOtaSession() {
    std::cout << "[OtamServerService] DestroyOtaSession Not Supported.\n";
}

AppSettingsResult OtamServerService::GetOtaSettings(
    const std::string& app_name) {
    AppSettingsResult result;
    result.ret_code = ResultCode::FAIL;

    if (state_ != ServiceState::Started || !api_client_) {
        std::cerr << "[OtamServerService] Not ready.\n";
        return result;
    }
    if (session_id_.empty()) {
        std::cerr << "[OtamServerService] No ArgoCD token. "
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
            result.ret_code = ResultCode::SUCCESS;
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
        std::cerr << "[OtamServerService] GetOtaSettings failed: "
                  << e.what() << "\n";
        result.app_settings = "{}";
    }

    return result;
}

AppUpdateStateResult OtamServerService::GetOtaUpdateState(
    const std::string& app_name) {
    AppUpdateStateResult result;
    result.ret_code = ResultCode::FAIL;

    if (state_ != ServiceState::Started || !api_client_) {
        std::cerr << "[OtamServerService] Not ready.\n";
        return result;
    }
    if (session_id_.empty()) {
        std::cerr << "[OtamServerService] No ArgoCD token. "
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
            result.app_state_info =
                j["status"]["sync"]["status"].get<std::string>();
            result.ret_code = ResultCode::SUCCESS;
            std::cout << "[OtamServerService] Sync status for "
                      << app_name << ": "
                      << result.app_state_info << std::endl;
        } else {
            result.app_state_info = "unknown";
            std::cerr << "[OtamServerService] Sync status not found for "
                      << app_name << ".\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "[OtamServerService] GetOtaUpdateState failed: "
                  << e.what() << "\n";
        result.app_state_info = "error";
    }

    return result;
}

ResultCode OtamServerService::RunOtaUpdate(
    const std::string& app_name) {
    if (state_ != ServiceState::Started || !api_client_) {
        std::cerr << "[OtamServerService] Not ready.\n";
        return ResultCode::FAIL;
    }
    if (session_id_.empty()) {
        std::cerr << "[OtamServerService] No ArgoCD token. "
                  << "Call CreateOtaSession() first.\n";
        return ResultCode::FAIL;
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
            return ResultCode::SUCCESS;
        } else {
            std::cerr << "[OtamServerService] Sync response unexpected.\n";
            return ResultCode::FAIL;
        }
    } catch (const std::exception& e) {
        std::cerr << "[OtamServerService] RunOtaUpdate failed: "
                  << e.what() << "\n";
        api_client_->RemoveHeader("Content-Type");
        return ResultCode::FAIL;
    }
}

ResultCode OtamServerService::SetOtaSettings(
    const std::string& app_name,
    const std::string& app_settings_json) {
    if (state_ != ServiceState::Started || !api_client_) {
        std::cerr << "[OtamServerService] Not ready.\n";
        return ResultCode::FAIL;
    }

    if (session_id_.empty()) {
        std::cerr << "[OtamServerService] No ArgoCD token. "
                  << "Call CreateOtaSession() first.\n";
        return ResultCode::FAIL;
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
            std::cerr << "[OtamServerService] Invalid argument "
                      << "(use 'on' or 'off').\n";
            return ResultCode::FAIL;
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
        return ResultCode::SUCCESS;

    } catch (const std::exception& e) {
        std::cerr << "[OtamServerService] SetOtaSettings failed: "
                  << e.what() << "\n";
        api_client_->RemoveHeader("Content-Type");
        return ResultCode::FAIL;
    }
}

// ========== Private helpers ==========
OtamServerConfig OtamServerService::LoadConfigFromFile(const std::string& path) {
    OtamServerConfig cfg;
    cfg.config_path = path;

    std::ifstream ifs(path);
    if (!ifs.good()) {
        std::cerr << "[OtamServerService] config.json not found: " << path << "\n";
        return cfg;
    }

    try {
        json j;
        ifs >> j;

        if (j.contains("server")) {
            auto s = j["server"];
            if (s.contains("url"))       cfg.server_url = s["url"].get<std::string>();
            if (s.contains("username"))  cfg.username  = s["username"].get<std::string>();
            if (s.contains("password"))  cfg.password  = s["password"].get<std::string>();
        }
    } catch (const std::exception& e) {
        std::cerr << "[OtamServerService] Failed to parse config: " << e.what() << "\n";
    }

    return cfg;
}

std::map<std::string, std::string> OtamServerService::ParseResponse(const std::string& response_json) {
    std::map<std::string, std::string> out;
    try {
        auto j = json::parse(response_json);
        if (j.contains("ret_code"))   out["ret_code"]   = std::to_string(j["ret_code"].get<int>());
        if (j.contains("session_id")) out["session_id"] = j["session_id"].get<std::string>();
        if (j.contains("detail"))     out["detail"]     = j["detail"].is_string()
                                                        ? j["detail"].get<std::string>()
                                                        : j["detail"].dump();
    } catch (...) {
        // parsing 실패 시 빈 맵 반환 → 호출부에서 FAIL 처리
    }
    return out;
}