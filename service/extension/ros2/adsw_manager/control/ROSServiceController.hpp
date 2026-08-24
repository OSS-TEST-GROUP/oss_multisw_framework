#ifndef ROS2_ADSW_MANAGER_CONTROL_ROSSERVICE_CONTROLLER_HPP_
#define ROS2_ADSW_MANAGER_CONTROL_ROSSERVICE_CONTROLLER_HPP_

#include <memory>
#include <string>

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

    bool Initialize() override;
    void Finalize() override;

    bool StartGroup(const std::string& group) override;
    bool StopGroup(const std::string& group) override;

    bool StartService(const std::string& service_name) override;
    bool StopService(const std::string& service_name) override;

    bool StartService(std::shared_ptr<Service> service) override;
    bool StopService(std::shared_ptr<Service> service) override;

    bool ProcessServiceStartRequest(lifecyclemgr::StartServiceRequest* message);
    bool ProcessServiceStopRequest(lifecyclemgr::StopServiceRequest* message);

    bool GetNodeLists(const std::shared_ptr<Service>& service);
    uint64_t GetPID(const std::shared_ptr<Service>& service);

    // virtual: MockROSServiceController 가 이미 GetContainer 를 MOCK_METHOD 로
    // 선언해 두었지만 여기가 non-virtual 이라 오버라이드가 아니라 가려짐(shadow)
    // 이었다. 그래서 ContainerHealthMonitor 처럼 ROSServiceController* 를 통해
    // 부르는 쪽은 항상 실제 멤버를 받아 테스트에서 컨테이너를 주입할 수 없었다.
    virtual std::shared_ptr<ContainerController> GetContainer() {return container_controller_;}

    // True when adsw_manager is starting services as native host processes
    // (no docker exec wrapper, no `ros2 run/launch` indirection). Used by
    // ContainerHealthMonitor to pick the right liveness-check token: native
    // services' cmdlines end with the package binary name, container-based
    // services end with the ros2 node/launch identifier.
    bool IsNativeExecution() const { return use_native_execution_; }

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
