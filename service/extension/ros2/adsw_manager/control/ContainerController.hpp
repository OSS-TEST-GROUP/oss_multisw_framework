#ifndef ROS2_ADSW_MANAGER_CONTROL_CONTAINER_CONTROLLER_HPP_
#define ROS2_ADSW_MANAGER_CONTROL_CONTAINER_CONTROLLER_HPP_

#include <string>
#include <memory>
#include <vector>

#include "service/Container.hpp"

namespace ros2 {
namespace adsw_manager {

class ContainerController {
  public:
    ContainerController();
    ~ContainerController();

    bool Initialize();
    void Finalize();

    bool StartContainer(const std::string& container_name);
    bool StopContainer(const std::string& container_name);

    bool GetContainerStatus(const std::string& container_name);
    bool GetContainerLogs(const std::string& container_name);
    // Add more methods as needed for container management

    std::string BaseCommand(const std::string& container_name);
    std::string BaseRos2Command(const std::string& container_name);

    bool ContainerSupported();

    std::shared_ptr<Container> GetContainer(const std::string& container_name);
    bool AddContainer(std::shared_ptr<Container> container);

  private:
    bool k3s_container_;
    std::vector<std::shared_ptr<Container>> containers_;
};

} // namespace adsw_manager
} // namespace ros2


#endif
