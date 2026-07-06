#include "Container.hpp"
#include "Service.hpp"

namespace ros2 {
namespace adsw_manager {

Container::Container(const std::string& name, const std::string& image,
                     const std::string& deploy_file, const std::string& namespace_,
                     ContainerType type)
    : name_(name),
      image_(image),
      deploy_file_(deploy_file),
      namespace_(namespace_),
      container_type_(type)
{
}

Container::~Container() {}

std::string Container::GetName() const {
    return name_;
} 
std::string Container::GetImage() const {
    return image_;
}
std::string Container::GetDeployFile() const {
    return deploy_file_;  
}
std::string Container::GetNamespace() const {
    return namespace_;
}
std::string Container::GetPodName() const {
    return name_;
}

ContainerType Container::GetContainerType() const {
    return container_type_;
}

void Container::SetContainerType(ContainerType type) {
    container_type_ = type;
}

void Container::SetExtraArgs(const std::vector<std::string>& args) {
    extra_args_ = args;
}
const std::vector<std::string>& Container::GetExtraArgs() const {
    return extra_args_;
}
void Container::SetCommandArgs(const std::vector<std::string>& args) {
    command_args_ = args;
}
const std::vector<std::string>& Container::GetCommandArgs() const {
    return command_args_;
}

} // namespace adsw_manager
} // namespace ros2
