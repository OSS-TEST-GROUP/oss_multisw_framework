#ifndef MOCK_DDS_ABSTRACTION_HPP_
#define MOCK_DDS_ABSTRACTION_HPP_

#include <gmock/gmock.h>
#include "DdsAbstraction.hpp"

class MockDdsAbstraction : public ros2::ota_client::DdsAbstraction {
public:
    // DdsAbstraction::Create 는 4번째 인자 transient_local(기본값 false)을 받는다.
    // gmock 은 기본 인자를 모르므로 시그니처를 4개로 정확히 맞춰야 override 성립
    // (불일치 시 MockDdsAbstraction 이 추상으로 남아 인스턴스화 불가).
    MOCK_METHOD(bool, Create, (const std::string&, bool, bool, bool), (override));
    MOCK_METHOD(bool, Write, (std::shared_ptr<void>), (override));
    MOCK_METHOD(void, SetQos, (uint32_t qos), (override));
    MOCK_METHOD(void, SetOnMessage, (OnMessageCB cb), (override));
};

#endif