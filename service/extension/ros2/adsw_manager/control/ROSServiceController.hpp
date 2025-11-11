#ifndef ROS2_ADSW_MANAGER_CONTROL_ROSSERVICE_CONTROLLER_HPP_
#define ROS2_ADSW_MANAGER_CONTROL_ROSSERVICE_CONTROLLER_HPP_

#include <string>
#include <memory>

#include "ServiceController.hpp"
#include "ROS2NodeController.hpp"
#include "ContainerController.hpp"

#include "lifecycle/StartServiceRequest.hpp"
#include "lifecycle/StopServiceRequest.hpp"

namespace ros2 {
namespace adsw_manager {

class ROSServiceController  : public ServiceController {

  public:
    ROSServiceController();
    ~ROSServiceController();

    virtual bool Initialize();
    virtual void Finalize();

    virtual bool StartGroup(const std::string& group);
    virtual bool StopGroup(const std::string& group);

    virtual bool StartService(const std::string& service_name);
    virtual bool StopService(const std::string& service_name);


    virtual bool StartService(std::shared_ptr<Service> service);
    virtual bool StopService(std::shared_ptr<Service> service);

    bool ProcessServiceStartRequest(lifecyclemgr::StartServiceRequest* message);
    bool ProcessServiceStopRequest(lifecyclemgr::StopServiceRequest* message);


    bool GetNodeLists(std::shared_ptr<Service> service);
    uint64_t GetPID(std::shared_ptr<Service> service);

    std::shared_ptr<ContainerController> GetContainer() {return container_controller_;}

  private:
    std::shared_ptr<ROS2NodeController>  ros2node_controller_;
    std::shared_ptr<ContainerController>  container_controller_;

    lifecyclemgr::StartServiceResponse start_service_response_;
    lifecyclemgr::StopServiceResponse stop_service_response_;
    
    // Environment detection results (detected once during initialization)
    bool is_container_env_;
    bool is_ros2_env_;
    bool is_native_arch_;  // x86_64, i386, i686, aarch64, arm64
    bool use_native_execution_;  // Calculated based on environment priority
    std::string execution_path_;  // For logging: "Container", "ROS2", or "Native"
    
    // Environment detection methods (used only during initialization)
    bool DetectNativeArchitecture();  // Detects x86 and ARM64 architectures
    bool DetectROS2();
    bool DetectContainer();
    
    // Helper methods for process management
    bool KillProcessAndChildren(uint64_t pid);
    std::string FindExecutablePath(const std::string& packageName);
};

} // namespace adsw_manager
} // namespace ros2

#endif
