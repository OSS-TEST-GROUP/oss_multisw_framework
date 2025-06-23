// Google Mock for ServiceRegistry
#pragma once
#include <gmock/gmock.h>
#include "service/ServiceRegistry.hpp"
#include "service/Service.hpp"

namespace lifecycle_manager {
class MockServiceRegistry : public ServiceRegistry {
public:
    MOCK_METHOD(std::shared_ptr<lifecycle_manager::Service>, GetService, (std::string), ());
    // Add more mocks as needed
};

MockServiceRegistry* Mock_ServiceRegistry;

ServiceRegistry::ServiceRegistry() {}
ServiceRegistry::~ServiceRegistry() {}

bool ServiceRegistry::UpdateServiceList(UpdateServiceListRequest* update) {
    return Mock_ServiceRegistry->UpdateServiceList(update);
}
bool ServiceRegistry::AddService(std::shared_ptr<Service> service, std::string group) {
    return Mock_ServiceRegistry->AddService(service, group);
}

} // namespace lifecycle_manager
