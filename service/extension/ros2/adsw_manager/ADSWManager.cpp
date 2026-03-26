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

    // 초기 컨테이너 상태를 lifecycle_mgr에 보고 후 폴링 시작.
    // 컨테이너 직접 시작 없이 lifecycle_mgr의 recovery policy가 재시작을 결정한다.
    if (container_health_monitor_) {
        container_health_monitor_->NotifyInitialContainerState();
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

    auto now = std::chrono::steady_clock::now();
    auto state = GetState();

    if (state == AdswState::ADSW_STARTED) {
        // Response 수신 전까지 kUpdateListRetryMs 간격으로 재전송
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
    } else if (state == AdswState::ADSW_RUNNING) {
        // lifecycle_mgr 재시작 대비: kUpdateListResyncMs 간격으로 서비스 리스트 재동기화
        auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - last_update_list_sent_).count();
        if (elapsed_ms >= kUpdateListResyncMs) {
            LOG_DEBUG("UpdateServiceListRequest: periodic resync");
            if (service_registry_->ProcessUpdateServiceListRequest()) {
                last_update_list_sent_ = now;
            }
        }
    }
}

void ADSWManager::ConfirmUpdateServiceList() {
    auto now = std::chrono::steady_clock::now();
    if (GetState() == AdswState::ADSW_STARTED) {
        LOG_DEBUG("UpdateServiceListResponse received — transitioning to ADSW_RUNNING");
        SetState(AdswState::ADSW_RUNNING);
        update_list_sent_ = false;
        last_update_list_sent_ = now;
        if (service_monitor_ != nullptr) {
            service_monitor_->NotifyAdswStatus(1, "ADSW start completed");
        }
    } else if (GetState() == AdswState::ADSW_RUNNING) {
        // lifecycle_mgr 재시작 후 resync 완료 — 타임스탬프 갱신
        LOG_DEBUG("UpdateServiceListResponse received (resync) — service list updated");
        last_update_list_sent_ = now;
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
