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
    LOG_DEBUG("ServiceMonitor::Initialize start ");
    ros_service_discovery_ = std::make_shared<ROSServiceDiscovery>();

    if (ros_service_discovery_){
        ros_service_discovery_->Initialize();
    }
    LOG_DEBUG("ServiceMonitor::Initialize end ");
    return true;
}

void ServiceMonitor::Finalize() {
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
        }
    }

    // Notify the service state after updating
    NotifyServiceState();
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

} // namespace adsw_manager
} // namespace ros2
