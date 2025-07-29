#include <iostream>

#include "StateManager.hpp"

#include "manager/ManagerBase.hpp"
#include "transmitter/DDSTransmitter.hpp"
#include "service/ServiceRegistry.hpp"
#include "include/TypeConvert.hpp"

#include "service/Service.hpp"

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

    if (transmitter != nullptr)
    {
        start_request_.service_list().push_back(service->GetName());
        start_request_.is_group() = false;

#if 0
        std::shared_ptr<ServiceRegistry> registry = GetRegistry();

        if (registry){
                std::vector<std::string> list = registry->GetServicesNameByGroup("");

                for( auto svc = list.begin(); svc != list.end(); ++svc ){
                    data->service_list().push_back(*svc);
                }
                data->is_group() = false;
        }
#endif            
        return transmitter->SendMessage(MessageType::START_SERVICE_REQUEST, reinterpret_cast<void*>(&start_request_));
    }
    return false;
}

void StateManager::StopService(std::shared_ptr<Service> service) {

    std::shared_ptr<DDSTransmitter> transmitter = GetTransmitter();

    if (transmitter != nullptr)
    {
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
    return true;
}

void StateManager::ProcessServiceHandling(std::set<std::shared_ptr<Service>> service_list){

    /* udpated service list passed */

    std::shared_ptr<PolicyManager> policy_manager = GetPolicy();
    
    for( auto service = service_list.begin(); service != service_list.end(); ++service ){

        ServiceControlAction action = policy_manager->Evaluate(*service);

        switch (action)
        {
        case ServiceControlAction::START:
            /* code */
            StartService(*service);
            break;

        case ServiceControlAction::STOP:
            /* code */
            StopService(*service);
            break;

        default:
            break;
        }
    }
}


void StateManager::OnStart(lifecyclemgr::StartServiceResponse* message){

    if (message){
        if( message->ret_code() != lifecyclemgr::ReturnCode::SUCCESS){
            // error 
        }
    }

}

void StateManager::OnStop(lifecyclemgr::StopServiceResponse* message){
    if (message){
        if( message->ret_code() != lifecyclemgr::ReturnCode::SUCCESS){
            // error 
        }
    }
}
    

} // namespace lifecycle_manager
