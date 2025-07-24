#include <vector>
#include <algorithm>

#include "ServiceMonitor.hpp"
#include "ADSWManager.hpp"
#include "transmitter/DDSTransmitter.hpp"
#include "transmitter/DDSMessageEndpoint.hpp"
#include "service/ServiceRegistry.hpp"
#include "lifecycle/NotifyServiceState.hpp"
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

void ServiceMonitor::UpdateServiceState(UpdateServiceInfo info) {
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

    std::vector<std::string> vlist = ros_service_node_list_[service];

    auto it = find(vlist.begin(), vlist.end(), node);

    if (it != vlist.end()){
        ros_service_node_list_[service].erase(it);
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
