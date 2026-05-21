#ifndef LIFECYCLE_CONTAINER_HPP_
#define LIFECYCLE_CONTAINER_HPP_

#include <cstdint>
#include <string>

#include "include/ContainerState.hpp"
#include "include/ContainerType.hpp"

// F013: lifecycle_mgr's view of a container registered by adsw_manager.
//
// adsw_manager has its own richer Container class (with docker/k8s helpers);
// this is the lifecycle_mgr counterpart — metadata + last-known state only.
// Mutated when an UpdateServiceListRequest arrives (refreshes metadata)
// or when a NotifyContainerState message comes in (refreshes state).

namespace lifecycle_manager {

class Container {
  public:
    Container() = default;
    Container(const std::string& name,
              const std::string& image,
              const std::string& deploy_file,
              const std::string& container_namespace,
              ContainerType type);

    const std::string& GetName() const            { return name_; }
    const std::string& GetImage() const           { return image_; }
    const std::string& GetDeployFile() const      { return deploy_file_; }
    const std::string& GetNamespace() const       { return container_namespace_; }
    ContainerType      GetType() const            { return type_; }
    ContainerState     GetState() const           { return state_; }
    int32_t            GetExitCode() const        { return exit_code_; }

    void SetImage(const std::string& v)           { image_ = v; }
    void SetDeployFile(const std::string& v)      { deploy_file_ = v; }
    void SetNamespace(const std::string& v)       { container_namespace_ = v; }
    void SetType(ContainerType v)                 { type_ = v; }
    void SetState(ContainerState v)               { state_ = v; }
    void SetExitCode(int32_t v)                   { exit_code_ = v; }

  private:
    std::string    name_;
    std::string    image_;
    std::string    deploy_file_;
    std::string    container_namespace_;
    ContainerType  type_       = ContainerType::CT_UNKNOWN;
    ContainerState state_      = ContainerState::CS_UNKNOWN;
    int32_t        exit_code_  = -1;
};

} // namespace lifecycle_manager

#endif // LIFECYCLE_CONTAINER_HPP_
