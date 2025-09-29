#include "MessageTransmitter.hpp"

using namespace eprosima::fastdds::dds;
using namespace lifecyclemgr;

namespace adsw_manager::agl {

    bool MessageTransmitter::Initialize(DomainParticipant* participant) {

        if(!dds_manager_->Initialize(participant)){
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
        
        return true;
    }

    void MessageTransmitter::PublishAppState() {

        auto app_list = service_registry_->GetAppList();
        
        NotifyServiceState msg;

        msg.os_type(OsType::AGL);

        for (const auto& app : app_list) {
            msg.service_list().push_back(app.name());
            msg.service_state_list().push_back(app.service_state());
        }

        dds_manager_->PublishNotifyAppState(msg);

    }

    void MessageTransmitter::HandleGetAppStateRequest(const GetServiceStateRequest& req) {

        if (req.os_type() == OsType::AGL) {
            
            std::vector<std::string> app_list;
            std::vector<ServiceState> app_state_list;
    
            for (const auto& app_name : req.service_list()) {
                app_list.push_back(app_name);
                const ServiceStatus* info = service_registry_->GetAppInfo(app_name);
                // 존재하지 않는 앱일 때 
                app_state_list.push_back(info ? info->service_state() : ServiceState::NOT_RUNNING);
                //app_state_list.push_back(info->service_state());
            }

            GetServiceStateResponse res;
            res.service_list().assign(app_list.begin(), app_list.end());
            res.service_state_list().assign(app_state_list.begin(), app_state_list.end());

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
                    // std::cerr << "[StartAppHandler] Failed to start app: " << app_name << std::endl;
                    res.ret_code(ReturnCode::FAILED);
                    break;  // 하나라도 실패하면 루프 중단
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
                    // std::cerr << "[StartAppHandler] Failed to stop app: " << app_name << std::endl;
                    res.ret_code(ReturnCode::FAILED);
                    break;  // 하나라도 실패하면 루프 중단
                }
            }

            dds_manager_->GetStopAppHandler()->PublishResponse(res);

        }
    }
    
}