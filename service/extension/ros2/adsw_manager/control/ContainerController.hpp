#ifndef ROS2_ADSW_MANAGER_CONTROL_CONTAINER_CONTROLLER_HPP_
#define ROS2_ADSW_MANAGER_CONTROL_CONTAINER_CONTROLLER_HPP_

#include <string>

namespace ros2 {
namespace adsw_manager {

class ContainerController {
  public:
    ContainerController(const std::string& deployment_file_path,
                        const std::string& pod_name,
                        const std::string& namespace_,
                        const std::string& container_name);
    ~ContainerController();
    bool Initialize();
    void Finalize();

    bool StartContainer(const std::string& container_name);
    bool StopContainer(const std::string& container_name);

    std::string GetPodName() const { return pod_name_; }
    std::string GetNamespace() const { return namespace_; }
    std::string GetContainerName() const { return container_name_; }


    bool GetContainerStatus(const std::string& container_name);
    bool GetContainerLogs(const std::string& container_name);
    // Add more methods as needed for container management

    std::string BaseCommand();
    std::string BaseRos2Command();

  private:
    std::string deployment_file_path_; // Path to the Kubernetes deployment file
    std::string pod_name_; // Name of the pod to manage
    std::string namespace_; // Namespace in which the pod is running
    std::string container_name_; // Name of the container to manage
};

} // namespace adsw_manager
} // namespace ros2


#endif
