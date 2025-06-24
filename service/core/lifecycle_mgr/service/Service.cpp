#include <iostream>

#include "Service.hpp"

namespace lifecycle_manager {

Service::Service() {}

Service::Service(const std::string& name, const ServiceType type, const RecoveryPolicy policy, const bool enabled)
    : name_(name), service_type_(type), recovery_policy_(policy), is_enabled_(enabled),
    service_mode_(ServiceMode::NORMAL), service_state_(ServiceState::NOT_RUNNING), 
    service_state_last_(ServiceState::NOT_RUNNING), recovered_count_(0)
{

}

Service::~Service() {}


std::string Service::GetName(){
    return name_;
}

ServiceType Service::GetServiceType(){
    return service_type_;
}

RecoveryPolicy Service::GetRecoveryPolicy(){
    return recovery_policy_;
}

bool Service::isEnabled(){
    return is_enabled_;
}

ServiceMode Service::GetServiceMode(){
    return service_mode_;
}

void Service::SetServiceMode(ServiceMode mode){
    service_mode_ = mode;
}

ServiceState Service::GetServiceState(){
    return service_state_;
}

void Service::SetServiceState(ServiceState state){
    if (service_state_ != state){
        service_state_last_ = service_state_;
        service_state_ = state;
    }
}

ServiceState Service::GetLastServiceState(){
    return service_state_last_;
}

int32_t Service::GetRecoveredCount(){
    return recovered_count_;
}

void Service::IncreaseRecoveredCount(){
    recovered_count_++;
}

void Service::ClearRecoveredCount(){
    recovered_count_ = 0;
}


void Service::Print(){
    
    std::cout << "service " << name_
                        << ", " << static_cast<int32_t>(service_type_) 
                        << ", " << static_cast<int32_t>(recovery_policy_) 
                        << ", " << static_cast<bool>(is_enabled_) << std::endl;

}

} // namespace lifecycle_manager
