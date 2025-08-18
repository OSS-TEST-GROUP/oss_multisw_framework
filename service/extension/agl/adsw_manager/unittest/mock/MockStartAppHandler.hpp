// mock/MockStartAppHandler.hpp
#pragma once
#include <gmock/gmock.h>
#include "StartAppHandler.hpp"

class MockStartAppHandler : public adsw_manager::agl::StartAppHandler {
public:
  MOCK_METHOD(bool, PublishResponse, (const adsw_manager_agl::StartAppResponse&), (override));
};