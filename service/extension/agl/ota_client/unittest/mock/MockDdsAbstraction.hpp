#ifndef MOCK_DDS_ABSTRACTION_HPP_
#define MOCK_DDS_ABSTRACTION_HPP_

#include <gmock/gmock.h>
#include "DdsAbstraction.hpp"

class MockDdsAbstraction : public ota_client::agl::DdsAbstraction {
public:
    MOCK_METHOD(bool, Create, (const std::string&, bool, bool), (override));
    MOCK_METHOD(bool, Write, (std::shared_ptr<void>), (override));
    MOCK_METHOD(void, SetQos, (uint32_t qos), (override));
    MOCK_METHOD(void, SetOnMessage, (OnMessageCB cb), (override));
};

#endif