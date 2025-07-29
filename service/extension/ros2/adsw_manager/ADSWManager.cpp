#include "ADSWManager.hpp"
#include "logger/Log.hpp"
#include "endpoint/MessageTypes.hpp"

namespace ros2 {
namespace adsw_manager {

ADSWManager::ADSWManager() {}

ADSWManager::~ADSWManager() {}

bool ADSWManager::Initialize() {

    LOG_DEBUG("ADSWManager::Initialize start ");

    SetState(AdswState::ADSW_INIT);

    dds_transmitter_ = std::make_shared<DDSTransmitter>();
    service_registry_ = std::make_shared<ServiceRegistry>();
    service_monitor_ = std::make_shared<ServiceMonitor>();
    service_controller_ = std::make_shared<ROSServiceController>();
    endppoint_factory_ = std::make_shared<EndPointFactory>();

    // initialize dependency : dds_transmitter --> endpoint_factory --> service_registry
    if (!dds_transmitter_->Initialize()) {
        return false;
    }
    if (!endppoint_factory_->Initialize()){
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
    if (endppoint_factory_ != nullptr){
        endppoint_factory_->CreateEndPoints(dds_transmitter_);
    }
    if (!service_registry_->LoadServiceList()) {
        return false;
    }
    
    SetState(AdswState::ADSW_STARTED);
    return true;
}

void ADSWManager::Stop() {
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

	if (GetState() == AdswState::ADSW_STARTED){
        if (service_registry_->ProcessUpdateServiceListRequest() == true){
    	    SetState(AdswState::ADSW_RUNNING);
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
    return endppoint_factory_->GetTopic(topic);
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
    return instance;
}


} // namespace adsw_manager
} // namespace ros2
