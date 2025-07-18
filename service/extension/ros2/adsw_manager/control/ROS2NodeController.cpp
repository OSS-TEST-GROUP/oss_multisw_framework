#include "ROS2NodeController.hpp"
#include "logger/Log.hpp"

namespace ros2 {
namespace adsw_manager {

bool ROS2NodeController::StartNode(const std::string& basecommand, const std::string& package, const std::string& nodename) {

    // kubectl exec -n default ros2-sample-all -- bash -c "source /opt/ros/humble/setup.bash && source /ros2_ws/install/setup.bash && ros2 run sample_ros2_app listener"

    LOG_DEBUG("Starting ROS2 node: " << basecommand << "/" << nodename);
    // Logic to start the ROS2 node
    std::string command = basecommand + " ros2 run " + package + " " + nodename ;
    // ros2 launch sample_ros2_app sample_listener.launch.py;

    LOG_DEBUG("Executing command: " << command);
    int result = system(command.c_str());
    if (result != 0) {
        LOG_ERROR("Failed to start node: " << nodename);
        return false; // Return false if the command failed
    }
    LOG_DEBUG("Node started successfully: " << nodename);
    return true ;
}

bool ROS2NodeController::StopNode(const std::string& basecommand, const std::string& package, const std::string& nodename) {
    LOG_DEBUG("Stopping ROS2 node: " << basecommand << "/" << nodename);
    // Logic to stop the ROS2 node
    std::string command = basecommand + " kill ";
    LOG_DEBUG("Executing command: " << command);
    int result = system(command.c_str());
    if (result != 0) {
        LOG_ERROR("Failed to stop node: " << nodename);
        return false; // Return false if the command failed
    }
    LOG_DEBUG("Node stopped successfully: " << nodename);
    return true;
}

bool ROS2NodeController::GetState(const std::string& basecommand, const std::string& package, const std::string& nodename) {
    LOG_DEBUG("Getting state of ROS2 node: " << basecommand << "/" << nodename);
    // Logic to get the state of the ROS2 node  
    std::string command = basecommand + " ros2 node list /" + nodename;
    LOG_DEBUG("Executing command: " << command);
    int result = system(command.c_str());
    if (result != 0) {
        LOG_ERROR("Failed to get state for node: " << nodename);
        return false; // Return false if the command failed
    }
    LOG_DEBUG("State retrieved successfully for node: " << nodename);
    return true; // Return true if successful
}

bool ROS2NodeController::SetState(const std::string& basecommand, const std::string& package, const std::string& nodename, const std::string& state) {
    LOG_DEBUG("Setting state of ROS2 node: " << basecommand << "/" << nodename << " to " << state);
    // Logic to set the state of the ROS2 node
    return false; // Not Supported in this implementation
}

bool ROS2NodeController::RestartNode(const std::string& basecommand, const std::string& package, const std::string& nodename) {
    LOG_DEBUG("Restarting ROS2 node: " << basecommand << "/" << nodename);
    // Logic to restart the ROS2 node
    if (!StopNode(basecommand, package, nodename)) {
        LOG_ERROR("Failed to stop node: " << nodename);
        return false; // Return false if stopping the node failed
    }
    if (!StartNode(basecommand, package, nodename)) {
        LOG_ERROR("Failed to start node: " << nodename);
        return false; // Return false if starting the node failed
    }
    LOG_DEBUG("Node restarted successfully: " << nodename);
    return true; // Return true if successful
}

    
} // namespace adsw_manager
} // namespace ros2
