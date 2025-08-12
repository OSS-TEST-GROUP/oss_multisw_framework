#include "Container.hpp"
#include "Service.hpp"

namespace ros2 {
namespace adsw_manager {

Container::Container(const std::string& name, const std::string& image, const std::string& deploy_file, const std::string& namespace_)
    : name_(name),
      image_(image),
      deploy_file_(deploy_file),
      namespace_(namespace_)
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
    // Example pod name generation logic
    return name_; // This can be customized as needed
}

} // namespace adsw_manager
} // namespace ros2
