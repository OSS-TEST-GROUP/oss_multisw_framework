#include <vector>
#include <algorithm>

#include "ServiceMonitor.hpp"
#include "ADSWManager.hpp"
#include "transmitter/DDSTransmitter.hpp"
#include "transmitter/DDSMessageEndpoint.hpp"
#include "service/ServiceRegistry.hpp"
#include "lifecycle/NotifyServiceState.hpp"
#include "ros2/AdswServiceStatus.hpp"
#include "service/TypeConvert.hpp"

#include "logger/Log.hpp"

namespace ros2 {
namespace adsw_manager {

ServiceMonitor::ServiceMonitor() {}

ServiceMonitor::~ServiceMonitor() {}

bool ServiceMonitor::Initialize() {
    ros_service_discovery_ = std::make_shared<ROSServiceDiscovery>(this);

    if (ros_service_discovery_){
        ros_service_discovery_->Initialize();
    }
    return true;
}

void ServiceMonitor::Finalize() {
    ros_service_node_list_.clear();
}

void ServiceMonitor::UpdateServiceState(UpdateServiceInfo& info) {
    LOG_DEBUG("ServiceMonitor::UpdateServiceState start ");
    
    std::shared_ptr<ServiceRegistry> registry = ADSWManager::Get()->GetServiceRegistry();

    if (registry != nullptr) {
        std::shared_ptr<Service> service = registry->GetService(info.name);

        if (service != nullptr) {
            ServiceState state = service->GetServiceState();
            LOG_DEBUG("ServiceMonitor::UpdateServiceState service: " << info.name << " state: " << static_cast<int>(state));

            if (info.service_event == ServiceEvent::CREATED) {
                service->SetServiceState(ServiceState::INITIALIZING);
            } else if (info.service_event == ServiceEvent::DELETED) {
                service->SetServiceState(ServiceState::NOT_RUNNING);
            } else if (info.service_event == ServiceEvent::STATE_UPDATED) {
                service->SetServiceState(info.state);
            }

            // Notify the service state after updating
            NotifyServiceState();
        }
    }

    LOG_DEBUG("ServiceMonitor::NotifyServiceState end ");    
}

void ServiceMonitor::NotifyServiceState() {

    std::shared_ptr<DDSTransmitter> transmitter = ADSWManager::Get()->GetTransmitter();
    std::shared_ptr<ServiceRegistry> registry = ADSWManager::Get()->GetServiceRegistry();

    if ((transmitter != nullptr) && (registry != nullptr))
    {
        const char* topic = ADSWManager::Get()->GetTopic(MessageType::NOTIFY_SERVICE_STATE);
        
        std::shared_ptr<DDSMessageEndpoint> endpoint = transmitter->GetMessageEndpoint(topic);

        if (endpoint != nullptr){
            lifecyclemgr::NotifyServiceState notify;

            notify.os_type() = TypeConvert::ToOsType(OsType::ROS2);

            std::vector<std::shared_ptr<Service>> group = registry->GetServicesByGroup("");

            for (auto& iter : group) {
                notify.service_list().push_back(iter->GetName());
                ServiceState state = iter->GetServiceState();
                notify.service_state_list().push_back(TypeConvert::ToServiceState(state));
            }
            endpoint->Write(reinterpret_cast<void*>(&notify));
        }
    }
}

void ServiceMonitor::NotifyAdswStatus(int32_t status_code, std::string status_msg) {

    std::shared_ptr<DDSTransmitter> transmitter = ADSWManager::Get()->GetTransmitter();
    std::shared_ptr<ServiceRegistry> registry = ADSWManager::Get()->GetServiceRegistry();

    if ((transmitter != nullptr) && (registry != nullptr))
    {
        const char* topic = ADSWManager::Get()->GetTopic(MessageType::ADSW_SERVICE_STATUS);

        std::shared_ptr<DDSMessageEndpoint> endpoint = transmitter->GetMessageEndpoint(topic);

        if (endpoint != nullptr){
            adswmgr::AdswServiceStatus notify;

            notify.status_code() = status_code;
            notify.status_msg() = status_msg;

            endpoint->Write(reinterpret_cast<void*>(&notify));
        }
    }
}

std::shared_ptr<ROSServiceDiscovery> ServiceMonitor::GetServiceDiscovery(){
    return ros_service_discovery_;
}

int32_t ServiceMonitor::NodeStarted(std::string service, std::string node){

    if (ros_service_node_list_.find(service) == ros_service_node_list_.end())
    {
        // The first start detected !!
        UpdateServiceInfo serviceinfo(service, service, ServiceEvent::CREATED, ServiceState::RUNNING);
        UpdateServiceState(serviceinfo);
    }

    ros_service_node_list_[service].push_back(node);
    LOG_VERBOSE("ServiceMonitor::NodeStarted  " << service << "::" << node);
    return ros_service_node_list_[service].size();
}


int32_t ServiceMonitor::NodeStopped(std::string service, std::string node){

    auto& vlist = ros_service_node_list_[service];

    auto it = find(vlist.begin(), vlist.end(), node);

    if (it != vlist.end()){
        vlist.erase(it);
        LOG_VERBOSE("ServiceMonitor::NodeStopped  " << service << "::" << node);
    }

    if (ros_service_node_list_[service].size() == 0)
    {
        // The first start detected !!
        UpdateServiceInfo serviceinfo(service, service, ServiceEvent::DELETED, ServiceState::NOT_RUNNING);
        UpdateServiceState(serviceinfo);
    }    
    
    return ros_service_node_list_[service].size();
}

int32_t ServiceMonitor::NodeCount(std::string service){
    return ros_service_node_list_[service].size();
}


void ServiceMonitor::OnServiceProcessDied(const std::string& service_name,
                                          int32_t exit_code) {
    LOG_DEBUG("ServiceMonitor::OnServiceProcessDied service=" << service_name
              << " exit_code=" << exit_code);

    std::shared_ptr<ServiceRegistry> registry = ADSWManager::Get()->GetServiceRegistry();
    if (!registry) return;

    std::shared_ptr<Service> svc = registry->GetService(service_name);
    if (!svc) {
        LOG_DEBUG("ServiceMonitor::OnServiceProcessDied unknown service: " << service_name);
        return;
    }

    svc->SetServiceState(ServiceState::NOT_RUNNING);
    svc->SetExitCode(exit_code);
    svc->SetPID(0);

    NotifyServiceState();
}

void ServiceMonitor::OnContainerStateChanged(const std::string& container_name,
                                             bool is_running,
                                             int32_t exit_code) {
    LOG_DEBUG("ServiceMonitor::OnContainerStateChanged container=" << container_name
              << " running=" << (is_running ? "true" : "false")
              << " exit_code=" << exit_code);

    std::shared_ptr<ServiceRegistry> registry = ADSWManager::Get()->GetServiceRegistry();
    if (!registry) return;

    std::vector<std::shared_ptr<Service>> services = registry->GetServicesByGroup(container_name);
    if (services.empty()) {
        LOG_DEBUG("ServiceMonitor::OnContainerStateChanged no services in group: " << container_name);
        return;
    }

    ServiceState new_state = is_running ? ServiceState::INITIALIZING : ServiceState::NOT_RUNNING;

    for (auto& svc : services) {
        svc->SetServiceState(new_state);
        svc->SetExitCode(exit_code);
        LOG_DEBUG("ServiceMonitor::OnContainerStateChanged service=" << svc->GetName()
                  << " -> state=" << static_cast<int>(new_state));
    }

    NotifyServiceState();
}

void ServiceMonitor::UpdateServiceState(){
    LOG_DEBUG("ServiceMonitor::UpdateServiceState start ");
    
    std::shared_ptr<ServiceRegistry> registry = ADSWManager::Get()->GetServiceRegistry();

    std::vector<std::shared_ptr<Service>> services = registry->GetServicesByGroup("");

    for (auto& service : services){
            
        if (ros_service_node_list_.find(service->GetName()) == ros_service_node_list_.end())
        {
            // Not found running node list 
            service->SetServiceState(ServiceState::NOT_RUNNING);
            LOG_DEBUG("UpdateServiceState service " << service->GetName() << " no running node !! ");
        }
        else{
            std::vector<std::string> vlist = ros_service_node_list_[service->GetName()];
            service->SetRunningNodes(vlist.size());
        }
    }
}


} // namespace adsw_manager
} // namespace ros2
