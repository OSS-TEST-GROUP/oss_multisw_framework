#include "Container.hpp"

namespace lifecycle_manager {

Container::Container(const std::string& name,
                     const std::string& image,
                     const std::string& deploy_file,
                     const std::string& container_namespace,
                     ContainerType type)
    : name_(name),
      image_(image),
      deploy_file_(deploy_file),
      container_namespace_(container_namespace),
      type_(type) {}

} // namespace lifecycle_manager
