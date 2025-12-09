#include <iostream>

#include "Service.hpp"
#include "logger/Log.hpp"
#include "include/Helper.hpp"

namespace lifecycle_manager {

Service::Service() : name_(""), service_type_(ServiceType::ONESHOT), recovery_policy_(RecoveryPolicy::R_DEFAULT), is_enabled_(false),
    service_mode_(ServiceMode::NORMAL), service_state_(ServiceState::NOT_RUNNING), 
    service_state_last_(ServiceState::NOT_RUNNING), recovered_count_(0), os_type_(OsType::ROS2)
{}

Service::Service(const std::string& name, const ServiceType type, const RecoveryPolicy policy, const bool enabled, const OsType os)
    : name_(name), service_type_(type), recovery_policy_(policy), is_enabled_(enabled),
    service_mode_(ServiceMode::NORMAL), service_state_(ServiceState::NOT_RUNNING), 
    service_state_last_(ServiceState::NOT_RUNNING), recovered_count_(0), os_type_(os)
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
        LOG_DEBUG("Service " << name_ << " state changed " << 
            Helper::StateStr(service_state_last_) << " --> " << Helper::StateStr(service_state_));
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
                        << ", " << Helper::SvcTypeStr(service_type_) 
                        << ", " << Helper::RecoveryStr(recovery_policy_) 
                        << ", " << static_cast<bool>(is_enabled_) << std::endl;

}

OsType Service::GetOsType(){
    return os_type_;
}

} // namespace lifecycle_manager
