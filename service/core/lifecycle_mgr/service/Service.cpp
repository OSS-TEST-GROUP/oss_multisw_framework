#include <iostream>

#include "Service.hpp"
#include "logger/Log.hpp"
#include "include/Helper.hpp"

namespace lifecycle_manager {

Service::Service() : service_type_(ServiceType::ONESHOT), recovery_policy_(RecoveryPolicy::R_DEFAULT), is_enabled_(false),
                     service_mode_(ServiceMode::NORMAL), service_state_(ServiceState::NOT_RUNNING),
                     service_state_last_(ServiceState::NOT_RUNNING), recovered_count_(0), exit_code_(-1), mw_type_(MwType::ROS2) {}

Service::Service(const std::string& name, const ServiceType type, const RecoveryPolicy policy, const bool enabled, const MwType os)
    : name_(name), service_type_(type), recovery_policy_(policy), is_enabled_(enabled),
    service_mode_(ServiceMode::NORMAL), service_state_(ServiceState::NOT_RUNNING),
    service_state_last_(ServiceState::NOT_RUNNING), recovered_count_(0), exit_code_(-1), mw_type_(os)
{

}



std::string Service::GetName(){
    return name_;
}

ServiceType Service::GetServiceType(){
    return service_type_;
}

void Service::SetServiceType(ServiceType type){
    service_type_ = type;
}

RecoveryPolicy Service::GetRecoveryPolicy(){
    return recovery_policy_;
}

void Service::SetRecoveryPolicy(RecoveryPolicy policy){
    recovery_policy_ = policy;
}

bool Service::isEnabled(){
    return is_enabled_;
}

void Service::SetEnabled(bool enabled){
    is_enabled_ = enabled;
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
    // 멤버는 uint32_t, 인터페이스는 int32_t 다. 재시작 횟수라 int32 범위를
    // 넘길 일이 없다 — 축소를 명시해 둔다.
    return static_cast<int32_t>(recovered_count_);
}

void Service::IncreaseRecoveredCount(){
    recovered_count_++;
}

void Service::ClearRecoveredCount(){
    recovered_count_ = 0;
}

int32_t Service::GetExitCode(){
    return exit_code_;
}

void Service::SetExitCode(int32_t code){
    exit_code_ = code;
}

void Service::Print(){
    
    std::cout << "service " << name_
                        << ", " << Helper::SvcTypeStr(service_type_) 
                        << ", " << Helper::RecoveryStr(recovery_policy_) 
                        << ", " << static_cast<bool>(is_enabled_) << std::endl;

}

MwType Service::GetMwType(){
    return mw_type_;
}

} // namespace lifecycle_manager
