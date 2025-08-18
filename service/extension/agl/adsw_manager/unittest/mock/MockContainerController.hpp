#ifndef MOCK_CONTAINER_CONTROLLER_HPP_
#define MOCK_CONTAINER_CONTROLLER_HPP_

#include "ContainerController.hpp"
#include <gmock/gmock.h>

class MockContainerController : public adsw_manager::agl::ContainerController {
public:
    MOCK_METHOD(bool, Initialize, (), (override));
    MOCK_METHOD(bool, StartContainer, (std::string), (override));
    MOCK_METHOD(bool, StopContainer, (std::string), (override));
    MOCK_METHOD(bool, IsContainerRunning, (std::string), (override));
};

#endif