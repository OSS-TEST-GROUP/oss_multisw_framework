#include "StateMonitor.hpp"

namespace lifecycle_manager {

StateMonitor::StateMonitor() {}
StateMonitor::~StateMonitor() {}


void StateMonitor::UpdateServicesState(std::string udpate_msg){
    
}

void StateMonitor::SetUpdateState(std::string service){
    
}

void StateMonitor::ClearUpdateState(std::string service){
    
}


bool StateMonitor::GetUdpateStateFlag(std::string service){
    
}

void StateMonitor::ClearAllUpdatedFlag(){
    
}

std::set<std::shared_ptr<Service>> StateMonitor::GetUpdateServiceList(){
    
}

void StateMonitor::NotifyLifecycleDiagnosticInfo(){
    
}

} // namespace lifecycle_manager
