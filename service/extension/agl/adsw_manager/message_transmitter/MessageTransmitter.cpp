#include "MessageTransmitter.hpp"
// #include <iostream>

using namespace eprosima::fastdds::dds;
using namespace adsw_manager_agl;

namespace adsw_manager::agl {

    bool MessageTransmitter::Initialize(DomainParticipant* participant) {

        if(!dds_manager_->Initialize(participant)){
            return false;
        }

        dds_manager_->SetOnGetAppStateRequest(
            [this](const GetAppStateRequest& req) {
                this->HandleGetAppStateRequest(req);
            }
        );
        dds_manager_->SetOnStartAppRequest(
            [this](const StartAppRequest& req) {
                this->HandleStartAppRequest(req);
            }
        );
        dds_manager_->SetOnStopAppRequest(
            [this](const StopAppRequest& req) {
                this->HandleStopAppRequest(req);
            }
        );
        
        return true;
    }

    void MessageTransmitter::PublishAppState(const std::string& app_name, AppState state) {

        NotifyAppState msg;

        msg.app_name() = app_name;
        msg.app_state() = state;

        dds_manager_->PublishNotifyAppState(msg);

    }

    void MessageTransmitter::HandleGetAppStateRequest(const GetAppStateRequest& req) {

        std::vector<std::string> app_list;
        std::vector<AppState> app_state_list;
    
        for (const auto& app_name : req.app_list()) {
            app_list.push_back(app_name);
            const AppInfo* info = service_registry_->GetAppInfo(app_name);
            app_state_list.push_back(info ? info->app_state() : AppState::UNKNOWN);
        }

        GetAppStateResponse res;
        res.app_list().assign(app_list.begin(), app_list.end());
        res.app_state_list().assign(app_state_list.begin(), app_state_list.end());

        dds_manager_->GetAppStateHandler()->PublishResponse(res);

    }

    void MessageTransmitter::HandleStartAppRequest(const StartAppRequest& req) {

        StartAppResponse res;

        if (service_controller_->StartService(req.app_info().app_name(), "")) {
            res.ret_code(ReturnCode::SUCCESS);
        } else {
            res.ret_code(ReturnCode::FAILED);
        }

        dds_manager_->GetStartAppHandler()->PublishResponse(res);
    }

    void MessageTransmitter::HandleStopAppRequest(const StopAppRequest& req) {

        StopAppResponse res;

        if (service_controller_->StopService(req.app_info().app_name(), "")) {
            res.ret_code(ReturnCode::SUCCESS);
        } else {
            res.ret_code(ReturnCode::FAILED);
        }

        dds_manager_->GetStopAppHandler()->PublishResponse(res);
    }
    
}