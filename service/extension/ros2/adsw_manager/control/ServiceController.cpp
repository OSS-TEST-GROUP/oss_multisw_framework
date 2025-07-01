#include "ServiceController.hpp"
#include "logger/Log.hpp"

namespace ros2 {
namespace adsw_manager {


ServiceController::ServiceController(){

}
ServiceController::~ServiceController(){

}

bool ServiceController::Initialize(){
    LOG_DEBUG("ServiceController::Initialize start ");
    return true;
}
void ServiceController::Finalize(){
}

bool ServiceController::StartGroup(){

}

bool ServiceController::StopGroup(){

}

bool ServiceController::StartService(const std::string& service_name){

}

bool ServiceController::StopService(const std::string& service_name){

}
    
} // namespace adsw_manager
} // namespace ros2
