#include "StateMonitor.hpp"

#include "manager/ManagerBase.hpp"
#include "transmitter/DDSTransmitter.hpp"
#include "service/ServiceRegistry.hpp"
#include "include/TypeConvert.hpp"

namespace lifecycle_manager {

StateMonitor::StateMonitor(LifecycleManager& lifecycle)
    : ManagerBase(lifecycle){
}

StateMonitor::~StateMonitor() {}

bool StateMonitor::Start() {
    
    StartThread();

    RunThread();

    return true;
}

void StateMonitor::Stop() {

    StopThread();
    
}

void StateMonitor::UpdateServicesState(::NotifyServiceState* update){

    if (update){
        std::vector<std::string> *slist = &update->service_list();
        std::vector<::ServiceState> *statelist = &update->service_state_list();
        updateServices(slist, statelist);
    }
}


void StateMonitor::UpdateServicesState(::GetServiceStateResponse* response){

    if (response){
        std::vector<std::string> *slist = &response->service_list();
        std::vector<::ServiceState> *statelist = &response->service_state_list();
        updateServices(slist, statelist);
    }
}

void StateMonitor::SetUpdateStateFlag(std::shared_ptr<Service> service){
    std::lock_guard<std::mutex> lock_guard(lock_mutex_);

    udpated_service_list_.insert(service);
}

void StateMonitor::ClearUpdateStateFlag(std::shared_ptr<Service> service){
    std::lock_guard<std::mutex> lock_guard(lock_mutex_);

    if (udpated_service_list_.find(service) != udpated_service_list_.end()) {
        udpated_service_list_.erase(service);
    }
}


bool StateMonitor::GetUpdateStateFlag(std::shared_ptr<Service> service){
    std::lock_guard<std::mutex> lock_guard(lock_mutex_);

    if (udpated_service_list_.find(service) != udpated_service_list_.end()) {
        return true;
    }    
    return false;
}

void StateMonitor::ClearAllUpdatedFlag(){
    std::lock_guard<std::mutex> lock_guard(lock_mutex_);

    udpated_service_list_.clear();
    
}

std::set<std::shared_ptr<Service>> StateMonitor::GetUpdateServiceList(){
    return udpated_service_list_;
}

void StateMonitor::NotifyLifecycleDiagnosticInfo(bool all){

    std::shared_ptr<DDSTransmitter> transmitter = GetTransmitter();

    if (transmitter != nullptr)
    {
        const char* topic = GetTopic(MessageType::LIFECYCLE_DIAG_REPORT);
        
        std::shared_ptr<DDSMessageEndPoint> endpoint = transmitter->GetMessageEndPoint(topic);

        if (endpoint != nullptr){
            void* writeptr = endpoint->GetTopicBuffer();

            LifecycleDiagReport * report = reinterpret_cast<LifecycleDiagReport*>(writeptr);

            if (all == true){
                std::shared_ptr<ServiceRegistry> registry = GetRegistry();

                if (registry){
                    std::vector<std::shared_ptr<Service>> list = registry->GetServicesByGroup("");

                    for( auto svc = list.begin(); svc != list.end(); ++svc ){
                        
                        ::ServiceStatus service;
                        
                        getServiceState(*svc, service);
                        report->service_list().push_back(service);
                    }
                }
            }else{
                
                for( auto svc = udpated_service_list_.begin(); svc != udpated_service_list_.end(); ++svc ){
                        
                    ::ServiceStatus service;
                        
                    getServiceState(*svc, service);
                    report->service_list().push_back(service);
                }
            }
            endpoint->Write(writeptr);
        }
    }
}

bool StateMonitor::getServiceState(std::shared_ptr<Service> svc, ::ServiceStatus& entry){

    entry.name() = svc->GetName();
    entry.service_type() = TypeConvert::ToServiceType(svc->GetServiceType());
    entry.service_state() = TypeConvert::ToServiceState(svc->GetServiceState());
    entry.is_enabled() = svc->isEnabled();
    entry.recovered_count() = svc->GetRecoveredCount();

    return false;
}


void StateMonitor::StartThread()
{
    stop_thread_ = false;
    processing_thread_ = std::thread(&StateMonitor::RunThread, this);
}

void StateMonitor::StopThread()
{
    stop_thread_ = true;
    
    if (processing_thread_.joinable())
    {
        processing_thread_.join();
    }
}

void StateMonitor::RunThread()
{
    while(!stop_thread_)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(10000));

        RequestGetState();
	}
}


void StateMonitor::RequestGetState(){

    std::shared_ptr<DDSTransmitter> transmitter = GetTransmitter();

    if (transmitter != nullptr)
    {
        const char* topic = GetTopic(MessageType::GET_SERVICE_STATE_REQUEST);
        
        std::shared_ptr<DDSMessageEndPoint> endpoint = transmitter->GetMessageEndPoint(topic);

        if (endpoint != nullptr){
            void* writeptr = endpoint->GetTopicBuffer();
            GetServiceStateRequest* data = reinterpret_cast<GetServiceStateRequest*>(writeptr);

            std::shared_ptr<ServiceRegistry> registry = GetRegistry();

            if (registry){
                std::vector<std::string> list = registry->GetServicesNameByGroup("");

                for( auto svc = list.begin(); svc != list.end(); ++svc ){
                    data->service_list().push_back(*svc);
                }
                data->is_all_service() = false;
            }
            endpoint->Write(writeptr);
        }
    }
}

void StateMonitor::updateServices(std::vector<std::string>* slist, std::vector<::ServiceState> *statelist){

    if (slist->size() != statelist->size()){

    }

    for (size_t idx=0; idx < slist->size(); ++idx){
            
        std::string sname = slist->at(idx);
        ::ServiceState state = statelist->at(idx);

        std::shared_ptr<Service> service = GetService(sname);

        if (service){
            service->SetServiceState(TypeConvert::ToServiceState(state));
            SetUpdateStateFlag(service);
        }
    }
}


} // namespace lifecycle_manager
