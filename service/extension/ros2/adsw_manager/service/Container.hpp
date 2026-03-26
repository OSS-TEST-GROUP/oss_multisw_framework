#ifndef ROS2_ADSW_MANAGER_SERVICE_CONTAINER_HPP_
#define ROS2_ADSW_MANAGER_SERVICE_CONTAINER_HPP_

#include <string>
#include <set>
#include <memory>

namespace ros2 {
namespace adsw_manager {

enum class ContainerType {
    UNKNOWN        = 0,
    K3S            = 1,  // kubectl apply/delete
    DOCKER         = 2,  // docker run/stop/rm
    DOCKER_COMPOSE = 3,  // docker compose up/down
};

class Service;

class Container {
  public:
    Container(const std::string& name, const std::string& image,
              const std::string& deploy_file, const std::string& namespace_,
              ContainerType type = ContainerType::DOCKER);
    virtual ~Container();

    std::string GetName() const;
    std::string GetImage() const;
    std::string GetDeployFile() const;
    std::string GetNamespace() const;
    std::string GetPodName() const;
    ContainerType GetContainerType() const;
    void SetContainerType(ContainerType type);

  private:
    std::string name_;
    std::string image_;
    std::string deploy_file_;
    std::string namespace_;
    ContainerType container_type_;
};

} // namespace adsw_manager
} // namespace ros2

#endif
