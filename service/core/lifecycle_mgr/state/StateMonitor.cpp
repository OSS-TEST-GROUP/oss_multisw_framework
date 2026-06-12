#include "StateMonitor.hpp"
#include "StateManager.hpp"

#include "manager/ManagerBase.hpp"
#include "transmitter/DDSTransmitter.hpp"
#include "service/ServiceRegistry.hpp"
#include "include/TypeConvert.hpp"
#include "lifecycle/NotifyContainerState.hpp"  // F008: aggregated container report
#include "logger/Log.hpp"


namespace lifecycle_manager {

StateMonitor::StateMonitor(LifecycleManager& lifecycle)
    : ManagerBase(lifecycle){
}

StateMonitor::~StateMonitor() {
    std::cout << "StateMonitor destructor called" << std::endl;
    if (processing_thread_.joinable()) {
        processing_thread_.join();
    }
}

bool StateMonitor::Start() {
    
    StartThread();
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
    int32_t running_cnt = 0;

    diag_report_.service_list().clear();
    diag_report_.is_started() = true;

    if (transmitter != nullptr)
    {
        std::shared_ptr<ServiceRegistry> registry = GetRegistry();
        diag_report_.total_services_num() = registry ? registry->GetServiceNum() : 0;

        if (all == true){

            if (registry){
                std::vector<std::shared_ptr<Service>> list = registry->GetServicesByGroup("");

                for( auto svc = list.begin(); svc != list.end(); ++svc ){
                        
                    lifecyclemgr::ServiceStatus service;
                        
                    getServiceState(*svc, service);
                    
                    if(service.service_state() == lifecyclemgr::ServiceState::RUNNING ){
                        running_cnt++;
                    }
                    diag_report_.service_list().push_back(service);
                }
            }
        }else{
            for( auto svc = udpated_service_list_.begin(); svc != udpated_service_list_.end(); ++svc ){
                lifecyclemgr::ServiceStatus service;
                        
                getServiceState(*svc, service);
                if(service.service_state() == lifecyclemgr::ServiceState::RUNNING ){
                    running_cnt++;
                }
                diag_report_.service_list().push_back(service);
            }
        }
        diag_report_.running_service_num() = running_cnt;
        LOG_DEBUG("StateMonitor::NotifyLifecycleDiagnosticInfo - running service num = " << running_cnt);
        transmitter->SendMessage(MessageType::LIFECYCLE_DIAG_REPORT, reinterpret_cast<void*>(&diag_report_));

        // F008: publish the aggregated container view. lifecycle_mgr is the SOLE
        // publisher of CONTAINER_STATE_REPORT, so the monitor (via dds_bridge)
        // sees every container regardless of how many adsw_managers (ros2 + agl)
        // reported them — no dds_bridge trust-on-first-use drop of a 2nd publisher.
        if (registry){
            // container_report_ is a member: SendMessage queues this pointer and
            // the publish loop reads it asynchronously, so it must outlive the call.
            container_report_.container_list().clear();
            container_report_.container_state_list().clear();
            container_report_.exit_code_list().clear();
            std::vector<std::shared_ptr<Container>> containers = registry->GetAllContainers();
            for (auto& c : containers){
                container_report_.container_list().push_back(c->GetName());
                container_report_.container_state_list().push_back(
                    TypeConvert::ToContainerState(c->GetState()));
                container_report_.exit_code_list().push_back(c->GetExitCode());
            }
            transmitter->SendMessage(MessageType::CONTAINER_STATE_REPORT,
                                     reinterpret_cast<void*>(&container_report_));
        }
    }
}

bool StateMonitor::getServiceState(std::shared_ptr<Service> svc, lifecyclemgr::ServiceStatus& entry){

    entry.name() = svc->GetName();
    entry.service_type() = TypeConvert::ToServiceType(svc->GetServiceType());
    entry.service_state() = TypeConvert::ToServiceState(svc->GetServiceState());
    entry.is_enabled() = svc->isEnabled();
    entry.recovered_count() = svc->GetRecoveredCount();
    // F013/F014 follow-up: enrich the periodic diag report so monitors
    // (e.g. lfc_monitor) see policy + last exit + hosting container without
    // a second channel.
    entry.recovery_policy() = TypeConvert::ToRecoveryPolicy(svc->GetRecoveryPolicy());
    entry.exit_code() = svc->GetExitCode();
    entry.container_name() = svc->GetContainerName();

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
    loop_count_ = 0;

    while(!stop_thread_)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(kPollIntervalMs));

        RequestGetState();

        loop_count_++;

        if (loop_count_ >= kDiagReportInterval){
            NotifyLifecycleDiagnosticInfo(true);
            loop_count_ = 0;
        }
	}
}


void StateMonitor::RequestGetState(){

    std::shared_ptr<DDSTransmitter> transmitter = GetTransmitter();

    if (transmitter != nullptr)
    {
        std::shared_ptr<ServiceRegistry> registry = GetRegistry();

        if (registry){
            std::vector<std::string> groups = registry->GetGroupList();

            for (auto& group : groups){
                std::vector<std::string> list = registry->GetServicesNameByGroup(group);                
                get_request_.service_list().clear();

                const std::string& name = list.front();
                auto svc = registry->GetService(name);
                if (!svc) {
                    // 해당 서비스 없음
                    LOG_ERROR("service not found.");
                    return;
                }
                get_request_.os_type() = TypeConvert::ToOsType(svc->GetOsType()); 
                // get_request_.os_type() = registry->GetService(list.front())->GetOsType();

                for( auto svc = list.begin(); svc != list.end(); ++svc ){
                    get_request_.service_list().push_back(*svc);
                }
                get_request_.is_all_service() = false;

                transmitter->SendMessage(MessageType::GET_SERVICE_STATE_REQUEST, reinterpret_cast<void*>(&get_request_));
            }
        }
    }
}

void StateMonitor::updateServices(std::vector<std::string>* slist, std::vector<lifecyclemgr::ServiceState> *statelist){

    if (slist->size() != statelist->size()){
        LOG_ERROR("service list size mismatch " << slist->size() << " != "  << statelist->size());
    }

    std::set<std::shared_ptr<Service>> service_list;
    size_t count = std::min(slist->size(), statelist->size());

    for (size_t idx=0; idx < count; ++idx){
            
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
