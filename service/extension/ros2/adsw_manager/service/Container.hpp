#ifndef ROS2_ADSW_MANAGER_SERVICE_CONTAINER_HPP_
#define ROS2_ADSW_MANAGER_SERVICE_CONTAINER_HPP_

#include <string>
#include <set>
#include <memory>
#include <vector>

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
    virtual ~Container() = default;
    std::string GetName() const;
    std::string GetImage() const;
    std::string GetDeployFile() const;
    std::string GetNamespace() const;
    std::string GetPodName() const;
    ContainerType GetContainerType() const;
    void SetContainerType(ContainerType type);

    // Extra `docker run` args (e.g. --network host, -e, -v) and the container
    // command, interleaved by ContainerController::StartDockerContainer as
    //   docker run -d --init --name <n> <extra_args...> <image> <command_args...>
    // Mirrors the AGL adsw_manager's service extra_args/command_args model so a
    // container whose own CMD is the workload (e.g. an Autoware role script)
    // can be launched with the network/volume/env it needs.
    void SetExtraArgs(const std::vector<std::string>& args);
    const std::vector<std::string>& GetExtraArgs() const;
    void SetCommandArgs(const std::vector<std::string>& args);
    const std::vector<std::string>& GetCommandArgs() const;

  private:
    std::string name_;
    std::string image_;
    std::string deploy_file_;
    std::string namespace_;
    ContainerType container_type_;
    std::vector<std::string> extra_args_;
    std::vector<std::string> command_args_;
};

} // namespace adsw_manager
} // namespace ros2

#endif
