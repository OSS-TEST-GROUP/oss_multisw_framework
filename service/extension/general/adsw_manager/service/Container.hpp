#ifndef GENERAL_ADSW_MANAGER_SERVICE_CONTAINER_HPP_
#define GENERAL_ADSW_MANAGER_SERVICE_CONTAINER_HPP_

#include <string>
#include <set>
#include <memory>
#include <vector>

namespace general {
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

    // 관찰 전용(observe-only) 컨테이너.
    //
    // oss-mmf-core / oss-mmf-ext-ros2 처럼 **호스트의 run 스크립트·compose 가
    // 만들고 소유하는** 인프라 컨테이너를 lfc_monitor 에 보이게 하되(상태는
    // ContainerHealthMonitor 가 `docker inspect` 로 읽는다) 제어는 막는다.
    //
    // 막아야 하는 이유가 구체적이다:
    //   · StopContainer 는 `docker stop && docker rm` 이다 — 정지가 아니라 **삭제**다.
    //     호스트가 만든 인자·마운트가 사라지고 run 스크립트 없이는 복구되지 않는다.
    //   · oss-mmf-ext-ros2 를 멈추면 dds_bridge 가 죽어 lfc_monitor 자신의 데이터
    //     경로가 끊긴다. oss-mmf-core 를 멈추면 lifecycle_mgr 이 사라진다.
    //   · 재생성 시 우리 yaml 인자를 쓰므로 호스트가 의도한 것과 다른 컨테이너가 된다.
    // AGL 쪽 AppDockerInfo::observe_only 와 같은 경계 판단이다.
    //
    // 접근자는 **헤더 인라인**이다: 단위 테스트 타깃 일부(ADSWServiceRegistryTest)가
    // ServiceRegistry.cpp 만 컴파일하고 Container.cpp 는 링크하지 않아서, 별도 TU 에
    // 두면 undefined reference 가 난다. 사소한 게터/세터를 위해 테스트 CMake 에
    // 소스를 추가하는 것보다 인라인이 낫다.
    void SetObserveOnly(bool v) { observe_only_ = v; }
    bool IsObserveOnly() const { return observe_only_; }

  private:
    std::string name_;
    std::string image_;
    std::string deploy_file_;
    std::string namespace_;
    ContainerType container_type_;
    std::vector<std::string> extra_args_;
    std::vector<std::string> command_args_;
    bool observe_only_ = false;
};

} // namespace adsw_manager
} // namespace general

#endif
