#include <iostream>

#include "StateManager.hpp"

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

    start_request_.service_list().clear();
    stop_request_.service_list().clear();
    
    for( auto it_service = service_list.begin(); it_service != service_list.end(); ++it_service ){

        std::shared_ptr<Service> service = *it_service;
        ServiceControlAction action = policy_manager->Evaluate(service);

        switch (action)
        {
        case ServiceControlAction::START:
            /* code */
            start_request_.service_list().push_back(service->GetName());
            break;

        case ServiceControlAction::STOP:
            /* code */
            stop_request_.service_list().push_back(service->GetName());
            break;

        default:
            break;
        }
    }
    std::shared_ptr<DDSTransmitter> transmitter = GetTransmitter();

    if (transmitter != nullptr){
        if (start_request_.service_list().size() > 0){
            start_request_.is_group() = true;
            transmitter->SendMessage(MessageType::START_SERVICE_REQUEST, reinterpret_cast<void*>(&start_request_));
        }
        if (stop_request_.service_list().size() > 0){
            stop_request_.is_group() = true;
            transmitter->SendMessage(MessageType::STOP_SERVICE_REQUEST, reinterpret_cast<void*>(&stop_request_));
        }
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
                stop_request_.is_group() = true;
            
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
    

} // namespace lifecycle_manager
