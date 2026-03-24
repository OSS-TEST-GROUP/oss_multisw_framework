#include "ADSWManager.hpp"
#include "logger/Log.hpp"
#include "endpoint/MessageTypes.hpp"

namespace ros2 {
namespace adsw_manager {

static std::shared_ptr<ADSWManager> shared_instance_;

ADSWManager::ADSWManager() {}

ADSWManager::~ADSWManager() {}

bool ADSWManager::Initialize() {

    LOG_DEBUG("ADSWManager::Initialize start ");

    SetState(AdswState::ADSW_INIT);

    dds_transmitter_ = std::make_shared<DDSTransmitter>();
    service_registry_ = std::make_shared<ServiceRegistry>();
    service_monitor_ = std::make_shared<ServiceMonitor>();
    container_health_monitor_ = std::make_shared<ContainerHealthMonitor>();
    service_controller_ = std::make_shared<ROSServiceController>();
    endpoint_factory_ = std::make_shared<EndPointFactory>();

    // initialize dependency : dds_transmitter --> endpoint_factory --> service_registry
    if (!dds_transmitter_->Initialize()) {
        return false;
    }
    if (!endpoint_factory_->Initialize()){
        return false;
    }

    if (!service_monitor_->Initialize()) {
        return false;
    }
    if (!service_controller_->Initialize()) {
        return false;
    }
    LOG_DEBUG("ADSWManager::Initialize End ");
    return true;
}

bool ADSWManager::Start() {
    if (endpoint_factory_ != nullptr){
        endpoint_factory_->CreateEndPoints(dds_transmitter_);
    }
    if (!service_registry_->LoadServiceList()) {
        return false;
    }

    // Auto-start containers and send initial state to lifecycle_mgr.
    if (container_health_monitor_) {
        container_health_monitor_->AutoStartContainers();
        container_health_monitor_->Start();
    }

    SetState(AdswState::ADSW_STARTED);
    return true;
}

void ADSWManager::Stop() {
    if (container_health_monitor_) {
        container_health_monitor_->Stop();
    }
    if (service_controller_) {
        service_controller_->Finalize();
    }
    if (service_monitor_) {
        service_monitor_->Finalize();
    }
    if (dds_transmitter_) {
        dds_transmitter_->Finalize();
    }
    SetState(AdswState::ADSW_TERMINATE);
}

void ADSWManager::Process(){

    if (GetState() == AdswState::ADSW_STARTED) {
        auto now = std::chrono::steady_clock::now();

        bool should_send = !update_list_sent_;
        if (update_list_sent_) {
            auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - last_update_list_sent_).count();
            if (elapsed_ms >= kUpdateListRetryMs) {
                LOG_DEBUG("UpdateServiceListRequest: no response for "
                          << elapsed_ms << "ms, retrying...");
                should_send = true;
            }
        }

        if (should_send) {
            if (service_registry_->ProcessUpdateServiceListRequest()) {
                last_update_list_sent_ = now;
                update_list_sent_ = true;
            }
        }
    }
}

void ADSWManager::ConfirmUpdateServiceList() {
    if (GetState() == AdswState::ADSW_STARTED) {
        LOG_DEBUG("UpdateServiceListResponse received — transitioning to ADSW_RUNNING");
        SetState(AdswState::ADSW_RUNNING);
        update_list_sent_ = false;
        if (service_monitor_ != nullptr) {
            service_monitor_->NotifyAdswStatus(1, "ADSW start completed");
        }
    }
}

std::shared_ptr<DDSTransmitter> ADSWManager::GetTransmitter() {
    return dds_transmitter_;
}

std::shared_ptr<ServiceRegistry> ADSWManager::GetServiceRegistry() {
    return service_registry_;
}

std::shared_ptr<ServiceMonitor> ADSWManager::GetServiceMonitor() {
    return service_monitor_;
}

std::shared_ptr<ROSServiceController> ADSWManager::GetServiceController() {
    return service_controller_;
}

std::shared_ptr<ROSServiceDiscovery> ADSWManager::GetServiceDiscovery() {
    if (service_monitor_ != nullptr){
        return service_monitor_->GetServiceDiscovery();
    }
    return nullptr;
}


const char* ADSWManager::GetTopic(MessageType topic){
    return endpoint_factory_->GetTopic(topic);
}


void ADSWManager::SetState(AdswState state) { 
    adsw_state_ = state; 
}

AdswState ADSWManager::GetState() { 
    return adsw_state_; 
}


ADSWManager* ADSWManager::Get(){
   return getShared().get();
}

std::shared_ptr<ADSWManager> ADSWManager::getShared()
{
    // Note we need a custom deleter, since the destructor is protected.
    static std::shared_ptr<ADSWManager> instance(
        new ADSWManager(),
        [](ADSWManager* p)
        {
            delete p;
        });

    shared_instance_ = instance;
    return instance;
}

void ADSWManager::SetShared(std::shared_ptr<ADSWManager> instance) {
    shared_instance_ = instance;
}

} // namespace adsw_manager
} // namespace ros2
