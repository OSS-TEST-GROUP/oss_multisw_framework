#ifndef MOCK_CONTAINER_HPP_
#define MOCK_CONTAINER_HPP_

#include <gmock/gmock.h>
#include "service/Container.hpp"

namespace ros2 {
namespace adsw_manager {

class MockContainer : public Container {
public:
    MockContainer()
        : Container("mock_name", "mock_image", "mock_deploy_file", "mock_namespace", ContainerType::DOCKER) {}
    ~MockContainer() override = default;

    MOCK_METHOD(std::string, GetName, (), (const));
    MOCK_METHOD(std::string, GetImage, (), (const));
    MOCK_METHOD(std::string, GetDeployFile, (), (const));
    MOCK_METHOD(std::string, GetNamespace, (), (const));
    MOCK_METHOD(std::string, GetPodName, (), (const));

};


MockContainer* Mock_Container;

Container::Container(const std::string& name, const std::string& image, const std::string& deploy_file, const std::string& namespace_, ContainerType type)
    : name_(name),
      image_(image),
      deploy_file_(deploy_file),
      namespace_(namespace_),
      container_type_(type) {
}

Container::~Container() {
}
std::string Container::GetName() const {
    return Mock_Container->GetName();
}
std::string Container::GetImage() const {
    return Mock_Container->GetImage();  
}   
std::string Container::GetDeployFile() const {
    return Mock_Container->GetDeployFile();
}
std::string Container::GetNamespace() const {
    return Mock_Container->GetNamespace();
}
std::string Container::GetPodName() const {
    return Mock_Container->GetPodName();
}
// F013: container-type accessor — backed by the real member set in the ctor.
ContainerType Container::GetContainerType() const {
    return container_type_;
}


} // namespace adsw_manager
} // namespace ros2

#endif // MOCK_CONTAINER_HPP_
