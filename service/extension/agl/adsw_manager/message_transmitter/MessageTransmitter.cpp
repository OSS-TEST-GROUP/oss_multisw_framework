#include "MessageTransmitter.hpp"
#include "logger/Log.hpp"

using namespace eprosima::fastdds::dds;
using namespace lifecyclemgr;

namespace adsw_manager::agl {

    bool MessageTransmitter::Initialize(DomainParticipant* participant) {

        if (!dds_manager_->Initialize(participant)) {
            return false;
        }

        dds_manager_->SetOnGetAppStateRequest(
            [this](const GetServiceStateRequest& req) {
                this->HandleGetAppStateRequest(req);
            }
        );
        dds_manager_->SetOnStartAppRequest(
            [this](const StartServiceRequest& req) {
                this->HandleStartAppRequest(req);
            }
        );
        dds_manager_->SetOnStopAppRequest(
            [this](const StopServiceRequest& req) {
                this->HandleStopAppRequest(req);
            }
        );
        dds_manager_->SetOnUpdateListResponse(
            [this](const UpdateServiceListResponse& res) {
                this->HandleUpdateListResponse(res);
            }
        );

        return true;
    }

    void MessageTransmitter::PublishAppState() {

        auto pairs = service_registry_->GetNameStatePairs();

        NotifyServiceState msg;

        msg.service_list().reserve(pairs.size());
        msg.service_state_list().reserve(pairs.size());

        msg.os_type(OsType::AGL);

        for (auto& [n, s] : pairs) {
            msg.service_list().push_back(std::move(n));
            msg.service_state_list().push_back(s);
        }

        dds_manager_->PublishNotifyAppState(msg);
    }

    void MessageTransmitter::PublishAppList() {

        UpdateServiceListRequest msg;
        msg.service_list(service_registry_->GetAppList());
        dds_manager_->PublishUpdateAppList(msg);
    }

    void MessageTransmitter::HandleGetAppStateRequest(const GetServiceStateRequest& req) {

        if (req.os_type() == OsType::AGL) {

            std::vector<std::string> app_list;
            std::vector<ServiceState> app_state_list;
            app_list.reserve(req.service_list().size());
            app_state_list.reserve(req.service_list().size());

            for (const auto& app_name : req.service_list()) {
                app_list.emplace_back(app_name);
                app_state_list.emplace_back(service_registry_->GetAppState(app_name));
            }

            GetServiceStateResponse res;
            res.service_list(std::move(app_list));
            res.service_state_list(std::move(app_state_list));

            dds_manager_->GetAppStateHandler()->PublishResponse(res);
        }
    }

    void MessageTransmitter::HandleStartAppRequest(const StartServiceRequest& req) {

        if (req.os_type() == OsType::AGL) {

            StartServiceResponse res;
            res.ret_code(ReturnCode::SUCCESS);

            for (const auto& app_name : req.service_list()) {
                bool result = service_controller_->StartService(app_name);
                if (!result) {
                    res.ret_code(ReturnCode::FAILED);
                    break;
                }
            }

            dds_manager_->GetStartAppHandler()->PublishResponse(res);
        }
    }

    void MessageTransmitter::HandleStopAppRequest(const StopServiceRequest& req) {

        if (req.os_type() == OsType::AGL) {

            StopServiceResponse res;
            res.ret_code(ReturnCode::SUCCESS);

            for (const auto& app_name : req.service_list()) {
                bool result = service_controller_->StopService(app_name);
                if (!result) {
                    res.ret_code(ReturnCode::FAILED);
                    break;
                }
            }

            dds_manager_->GetStopAppHandler()->PublishResponse(res);
        }
    }

    void MessageTransmitter::HandleUpdateListResponse(const UpdateServiceListResponse& res) {

        if (res.ret_code() == ReturnCode::SUCCESS) {
            LOG_INFO("[MessageTransmitter] UpdateServiceList confirmed by manager.");
            if (on_update_list_confirm_) {
                on_update_list_confirm_();
            }
        } else {
            LOG_WARN("[MessageTransmitter] UpdateServiceList response: FAILED");
        }
    }

} // namespace adsw_manager::agl
