#include <iostream>

#include "StateManager.hpp"

#include "manager/ManagerBase.hpp"
#include "transmitter/DDSTransmitter.hpp"

#include "lifecycle/GetServiceStateRequest.hpp"
#include "lifecycle/StartServiceRequest.hpp"
#include "lifecycle/StopServiceRequest.hpp"

namespace lifecycle_manager {

StateManager::StateManager(LifecycleManager& lifecycle)
    : ManagerBase(lifecycle){

}
StateManager::~StateManager() {}


bool StateManager::Start() {

    StartThread();
    RunThread();

    return true;
}

void StateManager::Stop() {

    StopThread();
    
}

bool StateManager::StartService(std::string name) {

    std::shared_ptr<DDSTransmitter> transmitter = GetTransmitter();

    if (transmitter != nullptr)
    {
        const char* topic = GetTopic(MessageType::START_SERVICE_REQUEST);
        
        std::shared_ptr<DDSMessageEndPoint> endpoint = transmitter->GetMessageEndPoint(topic);

        if (endpoint != nullptr){
            void* writeptr = endpoint->GetTopicBuffer();
            StartServiceRequest* data = reinterpret_cast<StartServiceRequest*>(writeptr);

            data->service_list().push_back(name);
            data->is_group() = true;

            endpoint->Write(writeptr);
        }
    }
    return true;
}
void StateManager::StopService(std::string name) {

    std::shared_ptr<DDSTransmitter> transmitter = GetTransmitter();

    if (transmitter != nullptr)
    {
        const char* topic = GetTopic(MessageType::STOP_SERVICE_REQUEST);
        
        std::shared_ptr<DDSMessageEndPoint> endpoint = transmitter->GetMessageEndPoint(topic);

        if (endpoint != nullptr){
            void* writeptr = endpoint->GetTopicBuffer();
            StopServiceRequest* data = reinterpret_cast<StopServiceRequest*>(writeptr);

            data->service_list().push_back(name);
            data->is_group() = true;

            endpoint->Write(writeptr);
        }
    }
}

bool StateManager::SetServiceMode(std::string service, ServiceMode mode){
    return false;
}

void StateManager::ProcessServiceHandling(std::vector<std::string> service_list){
    
}


void StateManager::StartThread()
{
    stop_thread_ = false;
    processing_thread_ = std::thread(&StateManager::RunThread, this);
}

void StateManager::StopThread()
{
    stop_thread_ = true;
    
    if (processing_thread_.joinable())
    {
        processing_thread_.join();
    }
}

void StateManager::RunThread()
{
    while(!stop_thread_)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(10000));

        RequestGetState();

        std::cout << __FILE__ << " :: " << __LINE__ << std::endl;
	}
}


void StateManager::RequestGetState(){

    std::shared_ptr<DDSTransmitter> transmitter = GetTransmitter();

    if (transmitter != nullptr)
    {
        const char* topic = GetTopic(MessageType::GET_SERVICE_STATE_REQUEST);
        
        std::shared_ptr<DDSMessageEndPoint> endpoint = transmitter->GetMessageEndPoint(topic);

        if (endpoint != nullptr){
            void* writeptr = endpoint->GetTopicBuffer();
            GetServiceStateRequest* data = reinterpret_cast<GetServiceStateRequest*>(writeptr);

            data->service_list().push_back("service1");
            data->service_list().push_back("service2");
            data->is_all_service() = true;

            endpoint->Write(writeptr);
        }
    }
}

void StateManager::OnGetServiceResponse(void* data){

    if (data){

        GetServiceStateResponse* response = reinterpret_cast<GetServiceStateResponse*>(data);

        std::vector<std::string> *slist = &response->service_list();
        std::vector<::ServiceState> *statelist = &response->service_state_list();

        for( auto svc = slist->begin(); svc != slist->end(); ++svc ){
            
            //GetServieManager()->UpdateService();
        }
        response->service_list();
    }

}

} // namespace lifecycle_manager
