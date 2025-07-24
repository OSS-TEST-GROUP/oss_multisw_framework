#include "Service.hpp"

namespace ros2 {
namespace adsw_manager {

Service::Service(const std::string& name, ServiceType service_type, RecoveryPolicy recovery_policy, bool is_enable)
    : name_(name),
      service_type_(service_type),
      recovery_policy_(recovery_policy),
      is_enabled_(is_enable),
      service_state_(ServiceState::NOT_RUNNING),
      packag_name_(""),
      launch_file_(""),
      max_nodes_(0),
      current_nodes_(0)
{
}

Service::~Service() {}

std::string Service::GetName() {
    return name_;
}

ServiceType Service::GetServiceType() {
    return service_type_;
}

RecoveryPolicy Service::GetRecoveryPolicy() {
    return recovery_policy_;
}

bool Service::IsEnabled() {
    return is_enabled_;
}

ServiceState Service::GetServiceState() {
    return service_state_;
}

void Service::SetServiceState(ServiceState state) {
    service_state_ = state;
}

void Service::SetLaunchFile(const std::string& launchfile){
    launch_file_ = launchfile;
}

void Service::SetPackage(const std::string& package){
    packag_name_ = package;
}

const std::string Service::GetPackage(){
    if (!packag_name_.empty()){
        return packag_name_;
    }

    return name_;
}

void Service::SetRunningNodes(int32_t count){

    int32_t prev_nodes = current_nodes_;
    if (max_nodes_ < count){
        max_nodes_ = count;
    }
    current_nodes_ = count;

    if (prev_nodes > current_nodes_){
        // node decrease
    }else if (prev_nodes < current_nodes_){
        // node increasing
    }

}

int32_t Service::GetMaxNodes(){
    return max_nodes_;
}



} // namespace adsw_manager
} // namespace ros2