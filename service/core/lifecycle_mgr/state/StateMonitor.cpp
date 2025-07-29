#include "StateMonitor.hpp"
#include "StateManager.hpp"

#include "manager/ManagerBase.hpp"
#include "transmitter/DDSTransmitter.hpp"
#include "service/ServiceRegistry.hpp"
#include "include/TypeConvert.hpp"
#include "logger/Log.hpp"

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

void StateMonitor::UpdateServicesState(lifecyclemgr::NotifyServiceState* update){

    if (update){
        std::vector<std::string> *slist = &update->service_list();
        std::vector<lifecyclemgr::ServiceState> *statelist = &update->service_state_list();
        updateServices(slist, statelist);
    }
}


void StateMonitor::UpdateServicesState(lifecyclemgr::GetServiceStateResponse* response){

    if (response){
        std::vector<std::string> *slist = &response->service_list();
        std::vector<lifecyclemgr::ServiceState> *statelist = &response->service_state_list();
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
        if (all == true){
            std::shared_ptr<ServiceRegistry> registry = GetRegistry();

            if (registry){
                std::vector<std::shared_ptr<Service>> list = registry->GetServicesByGroup("");

                for( auto svc = list.begin(); svc != list.end(); ++svc ){
                        
                    lifecyclemgr::ServiceStatus service;
                        
                    getServiceState(*svc, service);
                    diag_report_.service_list().push_back(service);
                }
            }
        }else{
                
            for( auto svc = udpated_service_list_.begin(); svc != udpated_service_list_.end(); ++svc ){
                        
                lifecyclemgr::ServiceStatus service;
                        
                getServiceState(*svc, service);
                diag_report_.service_list().push_back(service);
            }
        }

        transmitter->SendMessage(MessageType::LIFECYCLE_DIAG_REPORT, reinterpret_cast<void*>(&diag_report_));
    }
}

bool StateMonitor::getServiceState(std::shared_ptr<Service> svc, lifecyclemgr::ServiceStatus& entry){

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
        std::shared_ptr<ServiceRegistry> registry = GetRegistry();

        if (registry){
            std::vector<std::string> list = registry->GetServicesNameByGroup("");

            for( auto svc = list.begin(); svc != list.end(); ++svc ){
                get_request_.service_list().push_back(*svc);
            }
            get_request_.is_all_service() = false;
            
            transmitter->SendMessage(MessageType::GET_SERVICE_STATE_REQUEST, reinterpret_cast<void*>(&get_request_));
        }
    }
}

void StateMonitor::updateServices(std::vector<std::string>* slist, std::vector<lifecyclemgr::ServiceState> *statelist){

    if (slist->size() != statelist->size()){
        LOG_ERROR("service list size mismatch " << slist->size() << " != "  << statelist->size());
    }

    std::set<std::shared_ptr<Service>> service_list;

    for (size_t idx=0; idx < slist->size(); ++idx){
            
        std::string sname = slist->at(idx);
        lifecyclemgr::ServiceState state = statelist->at(idx);

        std::shared_ptr<Service> service = GetService(sname);

        if (service){
            service->SetServiceState(TypeConvert::ToServiceState(state));
            SetUpdateStateFlag(service);
            service_list.insert(service);
        }
    }

    if (service_list.size() > 0){
        std::shared_ptr<StateManager> manager = GetStateManager();
        manager->ProcessServiceHandling(service_list);
    }
}


} // namespace lifecycle_manager
