#include <iostream>

#include "PolicyManager.hpp"

namespace lifecycle_manager {

PolicyManager::PolicyManager() {}
PolicyManager::~PolicyManager() {}

bool PolicyManager::Start() {
    std::cout << __FILE__ << " :: " << __LINE__ << std::endl;

    return true;
}
void PolicyManager::Stop() {
    
}


    
std::shared_ptr<PolicyBase> PolicyManager::GetPolicy(std::string name){
    return nullptr;
}

ServiceControlAction PolicyManager::Evaluate(std::string service){
    return ServiceControlAction::NONE;
}


} // namespace lifecycle_manager
