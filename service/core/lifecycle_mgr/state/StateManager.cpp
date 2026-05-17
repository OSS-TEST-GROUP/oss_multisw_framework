#include <iostream>
#include <map>

#include "StateManager.hpp"
#include "include/OsType.hpp"

#include "manager/ManagerBase.hpp"
#include "transmitter/DDSTransmitter.hpp"
#include "service/ServiceRegistry.hpp"
#include "include/TypeConvert.hpp"

#include "service/Service.hpp"
#include "logger/Log.hpp"

namespace lifecycle_manager {

StateManager::StateManager(LifecycleManager& lifecycle)
    : ManagerBase(lifecycle){
}
StateManager::~StateManager() {}


bool StateManager::Start() {

    return true;
}

void StateManager::Stop() {

    
}

bool StateManager::StartService(std::shared_ptr<Service> service) {

    std::shared_ptr<DDSTransmitter> transmitter = GetTransmitter();

    start_request_.service_list().clear();
     
    if (transmitter != nullptr)
    {
        start_request_.os_type() = TypeConvert::ToOsType(service->GetOsType());
        start_request_.service_list().push_back(service->GetName());
        start_request_.is_group() = false;
        return transmitter->SendMessage(MessageType::START_SERVICE_REQUEST, reinterpret_cast<void*>(&start_request_));
    }

    return false;
}

void StateManager::StopService(std::shared_ptr<Service> service) {

    std::shared_ptr<DDSTransmitter> transmitter = GetTransmitter();
    stop_request_.service_list().clear();

    if (transmitter != nullptr)
    {
        stop_request_.os_type() = TypeConvert::ToOsType(service->GetOsType());
        stop_request_.service_list().push_back(service->GetName());
        stop_request_.is_group() = false;        
        transmitter->SendMessage(MessageType::STOP_SERVICE_REQUEST, reinterpret_cast<void*>(&stop_request_));
    }
}

bool StateManager::SetServiceMode(lifecyclemgr::SetServiceModeRequest* message){

    std::vector<std::string> *slist = &message->service_list();
    
    ServiceMode service_mode = TypeConvert::ToServiceMode(message->service_mode());

    for( auto svc = slist->begin(); svc != slist->end(); ++svc ){
        std::shared_ptr<Service> service = GetService(*svc);

        if (service != nullptr){
            service->SetServiceMode(service_mode);
        }
    }
    std::shared_ptr<DDSTransmitter> transmitter = GetTransmitter();
    if (transmitter != nullptr)
    {
        set_response_.ret_code() = lifecyclemgr::ReturnCode::SUCCESS;
        transmitter->SendMessage(MessageType::SET_SERVICE_MODE_RESPONSE, reinterpret_cast<void*>(&set_response_));
    }
    return true;
}

void StateManager::ProcessServiceHandling(std::set<std::shared_ptr<Service>> service_list){

    /* udpated service list passed */

    std::shared_ptr<PolicyManager> policy_manager = GetPolicy();

    // OsType별로 START/STOP 목록 분리
    std::map<OsType, std::vector<std::string>> start_map;
    std::map<OsType, std::vector<std::string>> stop_map;

    for( auto it_service = service_list.begin(); it_service != service_list.end(); ++it_service ){

        std::shared_ptr<Service> service = *it_service;
        ServiceControlAction action = policy_manager->Evaluate(service);

        switch (action)
        {
        case ServiceControlAction::START:
            start_map[service->GetOsType()].push_back(service->GetName());
            break;

        case ServiceControlAction::STOP:
            stop_map[service->GetOsType()].push_back(service->GetName());
            break;

        default:
            break;
        }
    }

    if (IsK3sMode()) {
        // K3S 모드: 향후 K3SInterface 호출로 교체
        LOG_DEBUG("StateManager::ProcessServiceHandling - K3S mode (not implemented yet)");
        return;
    }

    // Non-K3S 모드: adsw_manager에 DDS 명령 전송
    std::shared_ptr<DDSTransmitter> transmitter = GetTransmitter();
    if (transmitter == nullptr) return;

    for (auto& entry : start_map){
        start_request_.service_list() = entry.second;
        start_request_.os_type() = TypeConvert::ToOsType(entry.first);
        start_request_.is_group() = false;
        LOG_DEBUG("StateManager: START " << entry.second.size() << " service(s) for OsType "
                  << static_cast<int>(entry.first));
        transmitter->SendMessage(MessageType::START_SERVICE_REQUEST, reinterpret_cast<void*>(&start_request_));
    }

    for (auto& entry : stop_map){
        stop_request_.service_list() = entry.second;
        stop_request_.os_type() = TypeConvert::ToOsType(entry.first);
        stop_request_.is_group() = false;
        LOG_DEBUG("StateManager: STOP " << entry.second.size() << " service(s) for OsType "
                  << static_cast<int>(entry.first));
        transmitter->SendMessage(MessageType::STOP_SERVICE_REQUEST, reinterpret_cast<void*>(&stop_request_));
    }
}


void StateManager::RequestStopAll(){

    std::shared_ptr<DDSTransmitter> transmitter = GetTransmitter();

    if (transmitter != nullptr)
    {
        std::shared_ptr<ServiceRegistry> registry = GetRegistry();

        if (registry){
            
            std::vector<std::string> groups = registry->GetGroupList();

            for (auto& group : groups){
                std::vector<std::string> list = registry->GetServicesNameByGroup(group);    
                stop_request_.service_list().clear();
                for( auto svc = list.begin(); svc != list.end(); ++svc ){
                    stop_request_.service_list().push_back(*svc);
                }
                stop_request_.is_group() = false;
            
                transmitter->SendMessage(MessageType::STOP_SERVICE_REQUEST, reinterpret_cast<void*>(&stop_request_));
            }
        }
    }
}

void StateManager::OnStart(lifecyclemgr::StartServiceResponse* message){

    if (message){
        if( message->ret_code() != lifecyclemgr::ReturnCode::SUCCESS){
            // error 
            LOG_ERROR("Service Start Failure " << static_cast<int32_t>(message->ret_code()));
        }
    }

}

void StateManager::OnStop(lifecyclemgr::StopServiceResponse* message){
    if (message){
        if( message->ret_code() != lifecyclemgr::ReturnCode::SUCCESS){
            // error
            LOG_ERROR("Service Stop Failure " << static_cast<int32_t>(message->ret_code()));
        }
    }
}

// ── F013: actuator response relays ─────────────────────────────────────

void StateManager::OnStartContainer(lifecyclemgr::StartContainerResponse* message){
    if (!message) return;
    if (message->ret_code() != lifecyclemgr::ReturnCode::SUCCESS){
        LOG_ERROR("Container Start Failure " << static_cast<int32_t>(message->ret_code()));
    }
    // Relay verdict to ctrl/* response so the monitor sees the final outcome
    // of the CtrlStartContainerRequest it issued.
    PublishCtrlResponse(MessageType::CTRL_START_CONTAINER_RESPONSE, message->ret_code());
}

void StateManager::OnStopContainer(lifecyclemgr::StopContainerResponse* message){
    if (!message) return;
    if (message->ret_code() != lifecyclemgr::ReturnCode::SUCCESS){
        LOG_ERROR("Container Stop Failure " << static_cast<int32_t>(message->ret_code()));
    }
    PublishCtrlResponse(MessageType::CTRL_STOP_CONTAINER_RESPONSE, message->ret_code());
}

// ── F013: ctrl/* gate handlers ────────────────────────────────────────
//
// Gating policy (option (b) chosen at design time):
//   - Empty request list   → REQ_FAIL (nothing to act on).
//   - Unknown service/container name → REQ_FAIL.
//   - Service mode mismatch (e.g. control while in OTA_MODE) → REQ_FAIL.
//   - Otherwise forward to the actuator topic (adsw_manager) and publish
//     a SUCCESS CtrlResponse.  For container ops the *final* outcome is
//     re-published by OnStartContainer / OnStopContainer when the
//     adsw_manager response arrives — the synchronous CtrlResponse here
//     just acknowledges that the request was accepted (i.e. forwarded).

void StateManager::HandleCtrlStartService(lifecyclemgr::CtrlStartServiceRequest* message){
    if (!message){ return; }

    auto& names = message->service_list();
    if (names.empty()){
        PublishCtrlResponse(MessageType::CTRL_START_SERVICE_RESPONSE,
                            lifecyclemgr::ReturnCode::FAILED);
        return;
    }

    // Group services by OsType for the actuator topic shape.
    std::map<OsType, std::vector<std::string>> by_os;
    for (const auto& name : names){
        std::shared_ptr<Service> service = GetService(name);
        if (service == nullptr){
            LOG_ERROR("CtrlStartService: unknown service '" << name << "'");
            PublishCtrlResponse(MessageType::CTRL_START_SERVICE_RESPONSE,
                                lifecyclemgr::ReturnCode::FAILED);
            return;
        }
        if (service->GetServiceMode() == ServiceMode::OTA_MODE){
            LOG_ERROR("CtrlStartService: service '" << name << "' is in OTA_MODE — rejected");
            PublishCtrlResponse(MessageType::CTRL_START_SERVICE_RESPONSE,
                                lifecyclemgr::ReturnCode::FAILED);
            return;
        }
        by_os[service->GetOsType()].push_back(name);
    }

    std::shared_ptr<DDSTransmitter> transmitter = GetTransmitter();
    if (transmitter == nullptr){
        PublishCtrlResponse(MessageType::CTRL_START_SERVICE_RESPONSE,
                            lifecyclemgr::ReturnCode::FAILED);
        return;
    }

    for (auto& entry : by_os){
        start_request_.service_list() = entry.second;
        start_request_.os_type() = TypeConvert::ToOsType(entry.first);
        start_request_.is_group() = false;
        transmitter->SendMessage(MessageType::START_SERVICE_REQUEST,
                                  reinterpret_cast<void*>(&start_request_));
    }
    PublishCtrlResponse(MessageType::CTRL_START_SERVICE_RESPONSE,
                        lifecyclemgr::ReturnCode::SUCCESS);
}

void StateManager::HandleCtrlStopService(lifecyclemgr::CtrlStopServiceRequest* message){
    if (!message){ return; }

    auto& names = message->service_list();
    if (names.empty()){
        PublishCtrlResponse(MessageType::CTRL_STOP_SERVICE_RESPONSE,
                            lifecyclemgr::ReturnCode::FAILED);
        return;
    }

    std::map<OsType, std::vector<std::string>> by_os;
    for (const auto& name : names){
        std::shared_ptr<Service> service = GetService(name);
        if (service == nullptr){
            LOG_ERROR("CtrlStopService: unknown service '" << name << "'");
            PublishCtrlResponse(MessageType::CTRL_STOP_SERVICE_RESPONSE,
                                lifecyclemgr::ReturnCode::FAILED);
            return;
        }
        by_os[service->GetOsType()].push_back(name);
    }

    std::shared_ptr<DDSTransmitter> transmitter = GetTransmitter();
    if (transmitter == nullptr){
        PublishCtrlResponse(MessageType::CTRL_STOP_SERVICE_RESPONSE,
                            lifecyclemgr::ReturnCode::FAILED);
        return;
    }

    for (auto& entry : by_os){
        stop_request_.service_list() = entry.second;
        stop_request_.os_type() = TypeConvert::ToOsType(entry.first);
        stop_request_.is_group() = false;
        transmitter->SendMessage(MessageType::STOP_SERVICE_REQUEST,
                                  reinterpret_cast<void*>(&stop_request_));
    }
    PublishCtrlResponse(MessageType::CTRL_STOP_SERVICE_RESPONSE,
                        lifecyclemgr::ReturnCode::SUCCESS);
}

void StateManager::HandleCtrlSetServiceMode(lifecyclemgr::CtrlSetServiceModeRequest* message){
    if (!message){ return; }

    auto& names = message->service_list();
    if (names.empty()){
        PublishCtrlResponse(MessageType::CTRL_SET_SERVICE_MODE_RESPONSE,
                            lifecyclemgr::ReturnCode::FAILED);
        return;
    }
    for (const auto& name : names){
        if (GetService(name) == nullptr){
            LOG_ERROR("CtrlSetServiceMode: unknown service '" << name << "'");
            PublishCtrlResponse(MessageType::CTRL_SET_SERVICE_MODE_RESPONSE,
                                lifecyclemgr::ReturnCode::FAILED);
            return;
        }
    }

    std::shared_ptr<DDSTransmitter> transmitter = GetTransmitter();
    if (transmitter == nullptr){
        PublishCtrlResponse(MessageType::CTRL_SET_SERVICE_MODE_RESPONSE,
                            lifecyclemgr::ReturnCode::FAILED);
        return;
    }

    // Forward through the existing SetServiceMode actuator path —
    // adsw_manager will pause / resume the matching ROS2 services.
    set_mode_request_.service_list() = names;
    set_mode_request_.service_mode() = message->service_mode();
    transmitter->SendMessage(MessageType::SET_SERVICE_MODE_REQUST,
                              reinterpret_cast<void*>(&set_mode_request_));

    // Mirror the mode update into the local registry so subsequent
    // CtrlStart/Stop gating decisions see the new mode immediately —
    // adsw_manager has no callback path for SetServiceMode.
    ServiceMode mode = TypeConvert::ToServiceMode(message->service_mode());
    for (const auto& name : names){
        auto svc = GetService(name);
        if (svc) svc->SetServiceMode(mode);
    }

    PublishCtrlResponse(MessageType::CTRL_SET_SERVICE_MODE_RESPONSE,
                        lifecyclemgr::ReturnCode::SUCCESS);
}

void StateManager::HandleCtrlStartContainer(lifecyclemgr::CtrlStartContainerRequest* message){
    if (!message){ return; }

    auto& names = message->container_list();
    if (names.empty()){
        PublishCtrlResponse(MessageType::CTRL_START_CONTAINER_RESPONSE,
                            lifecyclemgr::ReturnCode::FAILED);
        return;
    }

    std::shared_ptr<ServiceRegistry> registry = GetRegistry();
    if (registry){
        for (const auto& name : names){
            if (registry->GetContainer(name) == nullptr){
                LOG_ERROR("CtrlStartContainer: unknown container '" << name << "'");
                PublishCtrlResponse(MessageType::CTRL_START_CONTAINER_RESPONSE,
                                    lifecyclemgr::ReturnCode::FAILED);
                return;
            }
        }
    }

    std::shared_ptr<DDSTransmitter> transmitter = GetTransmitter();
    if (transmitter == nullptr){
        PublishCtrlResponse(MessageType::CTRL_START_CONTAINER_RESPONSE,
                            lifecyclemgr::ReturnCode::FAILED);
        return;
    }

    start_container_request_.container_list() = names;
    transmitter->SendMessage(MessageType::START_CONTAINER_REQUEST,
                              reinterpret_cast<void*>(&start_container_request_));
    // Note: the *final* SUCCESS/FAIL CtrlResponse is published by
    // OnStartContainer() when adsw_manager's StartContainerResponse arrives.
    // We don't publish an interim SUCCESS here to avoid double-firing the
    // monitor's pending state.
}

void StateManager::HandleCtrlStopContainer(lifecyclemgr::CtrlStopContainerRequest* message){
    if (!message){ return; }

    auto& names = message->container_list();
    if (names.empty()){
        PublishCtrlResponse(MessageType::CTRL_STOP_CONTAINER_RESPONSE,
                            lifecyclemgr::ReturnCode::FAILED);
        return;
    }

    std::shared_ptr<ServiceRegistry> registry = GetRegistry();
    if (registry){
        for (const auto& name : names){
            if (registry->GetContainer(name) == nullptr){
                LOG_ERROR("CtrlStopContainer: unknown container '" << name << "'");
                PublishCtrlResponse(MessageType::CTRL_STOP_CONTAINER_RESPONSE,
                                    lifecyclemgr::ReturnCode::FAILED);
                return;
            }
        }
    }

    std::shared_ptr<DDSTransmitter> transmitter = GetTransmitter();
    if (transmitter == nullptr){
        PublishCtrlResponse(MessageType::CTRL_STOP_CONTAINER_RESPONSE,
                            lifecyclemgr::ReturnCode::FAILED);
        return;
    }

    stop_container_request_.container_list() = names;
    transmitter->SendMessage(MessageType::STOP_CONTAINER_REQUEST,
                              reinterpret_cast<void*>(&stop_container_request_));
}

bool StateManager::PublishCtrlResponse(MessageType response_topic,
                                        lifecyclemgr::ReturnCode code){
    std::shared_ptr<DDSTransmitter> transmitter = GetTransmitter();
    if (transmitter == nullptr) return false;
    ctrl_response_.ret_code() = code;
    return transmitter->SendMessage(response_topic,
                                     reinterpret_cast<void*>(&ctrl_response_));
}


} // namespace lifecycle_manager
