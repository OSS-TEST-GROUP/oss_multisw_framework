#ifndef MOCK_SERVICE_CONTROLLER_HPP_
#define MOCK_SERVICE_CONTROLLER_HPP_

#include "service_controller/ServiceController.hpp"
#include "service_controller/AppController.hpp"
#include "service_controller/ContainerController.hpp"
#include <gmock/gmock.h>

using namespace adsw_manager_agl;

class MockServiceController : public adsw_manager::agl::ServiceController {
public:
    MockServiceController(adsw_manager::agl::AppController* app, adsw_manager::agl::ContainerController* cont)
        : ServiceController(app, cont) {}

    MOCK_METHOD(bool, Initialize, (), (override));
    MOCK_METHOD(bool, StartService, (std::string appId, std::string contName), (override));
    MOCK_METHOD(bool, StopService, (std::string appId, std::string contName), (override));
};

#endif