#include "ROS2LifecyleController.hpp"
#include "logger/Log.hpp"

namespace ros2 {
namespace adsw_manager {

bool ROS2LifecycleController::StartNode(const std::string& basecommand, const std::string& package, const std::string& nodename) {
    LOG_DEBUG("Starting ROS2 node: "  << "/" << nodename << " with command: " << basecommand);

    std::string command = basecommand + " ros2 lifecycle set " + nodename + " configure";
    LOG_DEBUG("Executing command: " << command);
    int result = system(command.c_str());
    if (result != 0) {
        LOG_ERROR("Failed to configure node: " << nodename);
        return false; // Return false if the command failed
    }

    command = basecommand + " ros2 lifecycle set " + nodename + " activate";
    LOG_DEBUG("Executing command: " << command);
    result = system(command.c_str());
    if (result != 0) {
        LOG_ERROR("Failed to activate node: " << nodename);
        return false; // Return false if the command failed
    }

    LOG_DEBUG("Node started successfully: " << nodename);
    return true; // Return true if successful   
}

bool ROS2LifecycleController::StopNode(const std::string& basecommand, const std::string& package, const std::string& nodename) {
    // Logic to stop the ROS2 node
    // Deactivate (Active → Inactive), then Cleanup or Shutdown
    // ros2 lifecycle set /my_node deactivate
    // ros2 lifecycle set /my_node cleanup
    // ros2 lifecycle set /my_node shutdown
    LOG_DEBUG("StopNode of ROS2 node: /" << nodename);

    std::string command = basecommand + " ros2 lifecycle set " + nodename + " deactivate";
    LOG_DEBUG("Executing command: " << command);
    int result = system(command.c_str());
    if (result != 0) {
        LOG_ERROR("Failed to stop node: " << nodename);
        return false; // Return false if the command failed
    }
/*
    command = basecommand + " ros2 lifecycle set " + nodename + " cleanup";
    LOG_DEBUG("Executing command: " << command);
    result = system(command.c_str());
    if (result != 0) {
        LOG_ERROR("Failed to stop node: " << nodename);
        return false; // Return false if the command failed
    }
*/
    command = basecommand + " ros2 lifecycle set " + nodename + " shutdown";
    LOG_DEBUG("Executing command: " << command);
    result = system(command.c_str());
    if (result != 0) {
        LOG_ERROR("Failed to stop node: " << nodename);
        return false; // Return false if the command failed
    }
    LOG_DEBUG("Node stopped successfully: " << nodename);
    return true; // Return true if successful
}

bool ROS2LifecycleController::GetState(const std::string& basecommand, const std::string& package, const std::string& nodename) {
    LOG_DEBUG("Getting state of ROS2 node: /" << nodename);

    std::string command = basecommand + " ros2 lifecycle get " + nodename;
    LOG_DEBUG("Executing command: " << command);
    int result = system(command.c_str());
    if (result != 0) {
        LOG_ERROR("Failed to get state for node: " << package << "/" << nodename);
        return false; // Return false if the command failed
    }

    LOG_DEBUG("State retrieved successfully for node: " <<  "/" << nodename);
    return true; // Return true if successful
}

bool ROS2LifecycleController::SetState(const std::string& basecommand, const std::string& package, const std::string& nodename, const std::string& state) {
    LOG_DEBUG("Setting state of ROS2 node: /" << nodename << " to " << state);
     // Ensure the state is valid before setting it
    if (state != "configure" && state != "activate" && state != "deactivate" && state != "cleanup" && state != "shutdown") {
        LOG_ERROR("Invalid state: " << state);
        return false; // Return false if the state is invalid
    }
    std::string command = basecommand + " ros2 lifecycle set " + nodename + " " + state;  
    LOG_DEBUG("Executing command: " << command);
    int result = system(command.c_str());
    if (result != 0) {
        LOG_ERROR("Failed to set state for node: " << package << "/" << nodename);
        return false; // Return false if the command failed
    }
    
    LOG_DEBUG("State set successfully for node: " << package << "/" << nodename);
    return true; // Return true if successful
}


bool ROS2LifecycleController::RestartNode(const std::string& basecommand, const std::string& package, const std::string& nodename) {
    LOG_DEBUG("Restarting ROS2 node: " << basecommand << "/" << nodename);
    // Logic to restart the ROS2 node
    // deactivate / cleanup 의 실패는 의도적으로 무시한다.
    //
    // 노드가 이미 inactive 거나 unconfigured 면 이 두 전이는 정당하게 실패한다.
    // Restart 는 "어떤 상태였든 configure → activate 로 되돌린다" 가 목적이므로
    // 여기서 끊으면 안 된다. 실패로 처리하는 지점은 아래 configure / activate 뿐이고
    // ROS2LifecycleControllerTest.RestartNodeFailureOn{Configure,Activate} 가 그
    // 계약을 고정하고 있다.
    //
    // 반환값을 받아만 두고 안 읽으면 clang-analyzer-deadcode.DeadStores 가 잡는데,
    // 그게 "검사를 빠뜨린 것"인지 "일부러 무시한 것"인지 코드만 봐선 구분이 안 된다.
    // 로그로 남겨 의도를 드러낸다.
    std::string command = basecommand + " ros2 lifecycle set " + nodename + " deactivate";
    int result = system(command.c_str());
    LOG_DEBUG("ROS2 node deactivated: " << basecommand << "/" << nodename
              << " (rc=" << result << ", 실패는 무시 — 이미 inactive 일 수 있다)");

    command = basecommand + " ros2 lifecycle set " + nodename + " cleanup";
    result = system(command.c_str());
    LOG_DEBUG("ROS2 node cleaned up: " << basecommand << "/" << nodename
              << " (rc=" << result << ", 실패는 무시 — 이미 unconfigured 일 수 있다)");

    command = basecommand + " ros2 lifecycle set " + nodename + " configure";
    result = system(command.c_str());
    LOG_DEBUG("ROS2 node configured: " << basecommand << "/" << nodename);

    if (result != 0) {
        LOG_ERROR("Failed to restart node: " << basecommand << "/" << nodename);
        return false; // Return false if the command failed
    }
    command = basecommand + " ros2 lifecycle set " + nodename + " activate";
    result = system(command.c_str());
    if (result != 0) {
        LOG_ERROR("Failed to activate node: " << basecommand << "/" << nodename);
        return false; // Return false if the command failed
    }
    LOG_DEBUG("ROS2 node restarted successfully: " << basecommand << "/" << nodename);

    return true; // Return true if successful
}



} // namespace adsw_manager
} // namespace ros2
