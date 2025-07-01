#include "ROSServiceController.hpp"
#include "logger/Log.hpp"

namespace ros2 {
namespace adsw_manager {


ROSServiceController::ROSServiceController(){

}
ROSServiceController::~ROSServiceController(){

}

bool ROSServiceController::Initialize(){

    LOG_DEBUG("ROSServiceController::Initialize start ");    
    return true;

}
void ROSServiceController::Finalize(){
}

bool ROSServiceController::StartGroup(){

}

bool ROSServiceController::StopGroup(){

}


bool ROSServiceController::StartService(const std::string& service_name){

}

bool ROSServiceController::StopService(const std::string& service_name){

}

} // namespace adsw_manager
} // namespace ros2
