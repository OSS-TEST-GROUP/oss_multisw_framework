#ifndef MOCK_SERVICE_REGISTRY_HPP_
#define MOCK_SERVICE_REGISTRY_HPP_

#include "service_registry/ServiceRegistry.hpp"
#include <gmock/gmock.h>

using namespace lifecyclemgr;

class MockServiceRegistry : public adsw_manager::agl::ServiceRegistry {
public:
    MOCK_METHOD(bool, Initialize, (), (override));
    MOCK_METHOD(std::vector<Service>, GetAppList, (), (const, override));
    MOCK_METHOD(void, UpdateAppState, (const std::string&, ServiceState), (override));
    MOCK_METHOD((std::optional<std::pair<Service, ServiceState>>), GetAppInfo, (const std::string& app_name), (const, override));
    MOCK_METHOD((std::vector<std::pair<std::string, ServiceState>>), GetNameStatePairs, (), (const, override));
    MOCK_METHOD(ServiceState, GetAppState, (const std::string&), (const, override));
};

#endif