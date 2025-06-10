#include "StateMonitor.hpp"

namespace lifecycle_manager {

StateMonitor::StateMonitor() {}
StateMonitor::~StateMonitor() {}

bool StateMonitor::Start() {
    return true;
}
void StateMonitor::Stop() {
    
}


void StateMonitor::UpdateServicesState(std::string udpate_msg){
    
}

void StateMonitor::SetUpdateState(std::string service){
    
}

void StateMonitor::ClearUpdateState(std::string service){
    
}


bool StateMonitor::GetUdpateStateFlag(std::string service){
    return false;
}

void StateMonitor::ClearAllUpdatedFlag(){
    
}

std::set<std::shared_ptr<Service>> StateMonitor::GetUpdateServiceList(){
    //return std::set<nullptr>();
}

void StateMonitor::NotifyLifecycleDiagnosticInfo(){
    
}

} // namespace lifecycle_manager
