#include "StateManager.hpp"

namespace lifecycle_manager {

StateManager::StateManager() {}
StateManager::~StateManager() {}


bool StateManager::Start() {
    return true;
}
void StateManager::Stop() {
    
}

bool StateManager::StartService(std::string name) {
    return true;
}
void StateManager::StopService(std::string name) {
    
}

bool StateManager::SetServiceMode(std::string service, ServiceMode mode){
    return false;
}

void StateManager::ProcessServiceHandling(std::vector<std::string> service_list){
    
}

} // namespace lifecycle_manager
