#ifndef MOCK_APP_CONTROLLER_HPP_
#define MOCK_APP_CONTROLLER_HPP_

#include "AppController.hpp"
#include <gmock/gmock.h>

//using namespace adsw_manager_agl;

class MockAppController : public adsw_manager::agl::AppController {
public:
    MOCK_METHOD(bool, Initialize, (), (override));
    MOCK_METHOD(bool, LaunchApp, (std::string), (override));
    MOCK_METHOD(bool, TerminateApp, (std::string), (override));
    MOCK_METHOD(bool, IsAppRunning, (std::string), (override));
    MOCK_METHOD(bool, CanControlApp, (std::string), (const, override));
    MOCK_METHOD(bool, ShouldAutoRestartApp, (std::string), (const, override));
};

#endif
