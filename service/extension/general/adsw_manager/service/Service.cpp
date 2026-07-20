#include "Service.hpp"

namespace general {
namespace adsw_manager {

Service::Service(const std::string& name, ServiceType service_type, RecoveryPolicy recovery_policy, bool is_enable)
    : name_(name),
      service_type_(service_type),
      recovery_policy_(recovery_policy),
      is_enabled_(is_enable),
      service_state_(ServiceState::NOT_RUNNING),
      packag_name_(""),
      launch_file_(""),
      match_pattern_(""),
      is_direct_exec_(false),
      is_container_workload_(false),
      max_nodes_(0),
      current_nodes_(0),
      pid_(0),
      exit_code_(-1)
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

void Service::SetRecoveryPolicy(RecoveryPolicy policy) {
    recovery_policy_ = policy;
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

int32_t Service::GetRunningNodes(){
    return current_nodes_;
}
 
int32_t Service::GetMaxNodes(){
    return max_nodes_;
}

void Service::SetPID(uint64_t pid){
    pid_ = pid;
}

uint64_t Service::GetPID(){
    return pid_;
}

void Service::SetExitCode(int32_t code) {
    exit_code_ = code;
}

int32_t Service::GetExitCode() {
    return exit_code_;
}

void Service::SetProcessMatchPattern(const std::string& pattern) {
    match_pattern_ = pattern;
}

const std::string& Service::GetProcessMatchPattern() const {
    return match_pattern_;
}

void Service::SetDirectExec(bool direct) {
    is_direct_exec_ = direct;
}

bool Service::IsDirectExec() const {
    return is_direct_exec_;
}

void Service::SetContainerWorkload(bool v) {
    is_container_workload_ = v;
}

bool Service::IsContainerWorkload() const {
    return is_container_workload_;
}

std::string Service::ResolveProcessMatchToken(bool is_native) const {
    // Explicit hint takes precedence — needed for exec wrappers, fork-and-exit
    // launcher scripts, or anything else where the launching command name is
    // NOT what survives in /proc after startup.
    if (!match_pattern_.empty()) {
        return match_pattern_;
    }
    // ros2 launch → cmdline ends with the launch file (after `ros2 launch pkg`).
    if (!launch_file_.empty()) {
        return launch_file_;
    }
    // Binary/script mode (either host-native or `docker exec <binary>`): the
    // process cmdline tail is the binary/script path or name from `package`.
    if (is_native || is_direct_exec_) {
        return packag_name_.empty() ? name_ : packag_name_;
    }
    // Default: ros2 run pkg name → cmdline ends with the executable name.
    return name_;
}

} // namespace adsw_manager
} // namespace general