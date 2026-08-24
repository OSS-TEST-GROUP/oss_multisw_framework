#include <algorithm>
#include <utility>
#include <vector>

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

            // CREATED 와 STATE_UPDATED 는 지금 같은 동작(SetServiceState(info.state))
            // 이라 bugprone-branch-clone 이 잡는다. 합치지 않는다 — 아래 주석이
            // 설명하듯 CREATED 가 전달받은 state 를 그대로 존중하는 건 버그를 잡고
            // 얻은 결론이고, 두 이벤트는 의미가 달라 언제든 다시 갈릴 수 있다.
            // 합쳐두면 그때 무엇을 되돌려야 하는지가 사라진다.
            // NOLINTNEXTLINE(bugprone-branch-clone)
            if (info.service_event == ServiceEvent::CREATED) {
                // Honor the state the caller passed. NodeStarted passes
                // RUNNING when a ROS2 node first appears; hard-coding
                // INITIALIZING here stranded healthy services at
                // INITIALIZING forever because no later event ever upgrades
                // them (the next callback is STATE_UPDATED for ROS2-aware
                // services, never CREATED again).
                service->SetServiceState(info.state);
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

            notify.mw_type() = TypeConvert::ToMwType(MwType::ROS2);

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
            notify.status_msg() = std::move(status_msg);

            endpoint->Write(reinterpret_cast<void*>(&notify));
        }
    }
}

std::shared_ptr<ROSServiceDiscovery> ServiceMonitor::GetServiceDiscovery(){
    return ros_service_discovery_;
}

int32_t ServiceMonitor::NodeStarted(const std::string& service, const std::string& node) {

    if (ros_service_node_list_.find(service) == ros_service_node_list_.end())
    {
        // The first start detected !!
        UpdateServiceInfo serviceinfo(service, service, ServiceEvent::CREATED, ServiceState::RUNNING);
        UpdateServiceState(serviceinfo);
    }

    ros_service_node_list_[service].push_back(node);
    LOG_VERBOSE("ServiceMonitor::NodeStarted  " << service << "::" << node);
    // 서비스당 ROS2 노드 수다. int32 범위를 넘길 일이 없다.
    return static_cast<int32_t>(ros_service_node_list_[service].size());
}

int32_t ServiceMonitor::NodeStopped(const std::string& service, const std::string& node) {

    auto& vlist = ros_service_node_list_[service];

    auto it = find(vlist.begin(), vlist.end(), node);

    if (it != vlist.end()){
        vlist.erase(it);
        LOG_VERBOSE("ServiceMonitor::NodeStopped  " << service << "::" << node);
    }

    if (ros_service_node_list_[service].empty()) {
        // The first start detected !!
        UpdateServiceInfo serviceinfo(service, service, ServiceEvent::DELETED, ServiceState::NOT_RUNNING);
        UpdateServiceState(serviceinfo);
    }

    // 서비스당 ROS2 노드 수다. int32 범위를 넘길 일이 없다.
    return static_cast<int32_t>(ros_service_node_list_[service].size());
}

int32_t ServiceMonitor::NodeCount(const std::string& service) {
    // 서비스당 ROS2 노드 수다. int32 범위를 넘길 일이 없다.
    return static_cast<int32_t>(ros_service_node_list_[service].size());
}

void ServiceMonitor::OnServiceProcessDied(const std::string& service_name,
                                          int32_t exit_code) {
    LOG_DEBUG("ServiceMonitor::OnServiceProcessDied service=" << service_name
              << " exit_code=" << exit_code);

    std::shared_ptr<ServiceRegistry> registry = ADSWManager::Get()->GetServiceRegistry();
    if (!registry) {
        return;
    }

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
    if (!registry) {
        return;
    }

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
            service->SetRunningNodes(static_cast<int32_t>(vlist.size()));
        }
    }
}


} // namespace adsw_manager
} // namespace ros2
