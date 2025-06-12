#include <iostream>

#include "MWServiceManager.hpp"

namespace lifecycle_manager {

MWServiceManager::MWServiceManager() {}
MWServiceManager::~MWServiceManager() {}


bool MWServiceManager::Initialize(){
    std::cout << __FILE__ << " :: " << __LINE__ << std::endl;

    return false;
}

bool MWServiceManager::Start(){

    std::cout << __FILE__ << " :: " << __LINE__ << std::endl;

    return true;

}

void MWServiceManager::Stop(){

}

void MWServiceManager::LoadMWConfig(std::string path){

}

void MWServiceManager::StartMWService(std::string mw_service){

}

void MWServiceManager::StopMWService(std::string mw_service){

}

void MWServiceManager::RunMWServiceManagement(){

}



} // namespace lifecycle_manager
