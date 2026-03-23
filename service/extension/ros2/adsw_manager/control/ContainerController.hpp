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
    bool RestartContainer(const std::string& container_name);

    bool GetContainerStatus(const std::string& container_name);
    bool GetContainerLogs(const std::string& container_name);

    std::string BaseCommand(const std::string& container_name);
    std::string BaseRos2Command(const std::string& container_name);

    bool ContainerSupported();

    std::shared_ptr<Container> GetContainer(const std::string& container_name);
    bool AddContainer(std::shared_ptr<Container> container);

  private:
    // k3s (kubectl) operations
    bool StartK3sContainer(std::shared_ptr<Container> container);
    bool StopK3sContainer(std::shared_ptr<Container> container);
    bool RestartK3sContainer(std::shared_ptr<Container> container);

    // docker run/stop operations
    bool StartDockerContainer(std::shared_ptr<Container> container);
    bool StopDockerContainer(std::shared_ptr<Container> container);
    bool RestartDockerContainer(std::shared_ptr<Container> container);

    // docker compose operations
    bool StartDockerComposeContainer(std::shared_ptr<Container> container);
    bool StopDockerComposeContainer(std::shared_ptr<Container> container);
    bool RestartDockerComposeContainer(std::shared_ptr<Container> container);

    int RunCommand(const std::string& cmd);

    std::vector<std::shared_ptr<Container>> containers_;
};

} // namespace adsw_manager
} // namespace ros2

#endif
