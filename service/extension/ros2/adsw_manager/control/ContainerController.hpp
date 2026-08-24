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
    virtual ~ContainerController() = default;
    bool Initialize();
    void Finalize();

    bool StartContainer(const std::string& container_name);
    bool StopContainer(const std::string& container_name);
    bool RestartContainer(const std::string& container_name);

    bool GetContainerStatus(const std::string& container_name);
    bool GetContainerLogs(const std::string& container_name);

    // 런타임 상태 조회
    std::string GetContainerStatusStr(const std::string& container_name);
    int32_t     GetContainerExitCode(const std::string& container_name);
    bool        IsContainerRunning(const std::string& container_name);

    std::vector<std::shared_ptr<Container>> GetAllContainers();

    std::string BaseCommand(const std::string& container_name);
    std::string BaseRos2Command(const std::string& container_name);

    bool ContainerSupported();

    std::shared_ptr<Container> GetContainer(const std::string& container_name);
    bool AddContainer(const std::shared_ptr<Container>& container);

  protected:
    // 외부 프로세스 실행 seam.
    //
    // start/stop/restart 와 status/exit-code 경로는 전부 여기를 지나 docker /
    // kubectl 을 부른다. 그래서 실제 런타임 없이는 한 줄도 커버되지 않았다
    // (262줄 중 184줄 미커버). 유닛테스트에서 서브클래싱해 가로챌 수 있도록
    // virtual 로 연다. 프로덕션 동작은 그대로다.
    //
    // RunCommand      : system() 과 같은 반환값 규약
    // CaptureCommand  : popen 으로 stdout 첫 줄을 max_len-1 바이트까지 읽는다.
    //                   실패하면 빈 문자열 — 호출부가 그걸 "unknown" / -1 로 접는다.
    virtual int RunCommand(const std::string& cmd);
    virtual std::string CaptureCommand(const std::string& cmd, size_t max_len);

  private:
    // k3s (kubectl) operations
    bool StartK3sContainer(const std::shared_ptr<Container>& container);
    bool StopK3sContainer(const std::shared_ptr<Container>& container);
    bool RestartK3sContainer(const std::shared_ptr<Container>& container);

    // docker run/stop operations
    bool StartDockerContainer(const std::shared_ptr<Container>& container);
    bool StopDockerContainer(const std::shared_ptr<Container>& container);
    bool RestartDockerContainer(const std::shared_ptr<Container>& container);

    // docker compose operations
    bool StartDockerComposeContainer(const std::shared_ptr<Container>& container);
    bool StopDockerComposeContainer(const std::shared_ptr<Container>& container);
    bool RestartDockerComposeContainer(const std::shared_ptr<Container>& container);

    std::vector<std::shared_ptr<Container>> containers_;
};

} // namespace adsw_manager
} // namespace ros2

#endif
