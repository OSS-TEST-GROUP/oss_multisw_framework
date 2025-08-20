#ifndef MOCK_SERVICE_CONTROLLER_HPP_
#define MOCK_SERVICE_CONTROLLER_HPP_

#include "service_controller/ServiceController.hpp"
#include "service_controller/AppController.hpp"
#include <gmock/gmock.h>

using namespace lifecyclemgr;

class MockServiceController : public adsw_manager::agl::ServiceController {
public:
    MockServiceController(adsw_manager::agl::AppController* app)
        : ServiceController(app) {}

    MOCK_METHOD(bool, Initialize, (), (override));
    MOCK_METHOD(bool, StartService, (std::string app_name), (override));
    MOCK_METHOD(bool, StopService, (std::string app_name), (override));
};

#endif