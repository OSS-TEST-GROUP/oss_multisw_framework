#include "ROS2NodeController.hpp"
#include "logger/Log.hpp"

namespace ros2 {
namespace adsw_manager {


ROS2NodeController::ROS2NodeController(){
    LOG_DEBUG("ROS2NodeController create ");
}
ROS2NodeController::~ROS2NodeController(){

}

bool ROS2NodeController::Initialize(){

}
void ROS2NodeController::Finalize(){
}

bool ROS2NodeController::StartROS2Node(const std::string& name_space, const std::string& name){

}

bool ROS2NodeController::StopNode(const std::string& name_space, const std::string& name){

}
    
} // namespace adsw_manager
} // namespace ros2
