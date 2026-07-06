#ifndef MOCK_MESSAGE_TRANSMITTER_HPP_
#define MOCK_MESSAGE_TRANSMITTER_HPP_

#include "message_transmitter/MessageTransmitter.hpp"
#include <gmock/gmock.h>

using namespace lifecyclemgr;

class MockMessageTransmitter : public adsw_manager::agl::MessageTransmitter {
public:
    MockMessageTransmitter(std::shared_ptr<adsw_manager::agl::ServiceRegistry> reg,
                               std::shared_ptr<adsw_manager::agl::ServiceController> cont,
                               std::shared_ptr<adsw_manager::agl::DDSManager> dmgr)
        : MessageTransmitter(reg, cont, dmgr) {}
    MockMessageTransmitter()
    : MessageTransmitter(nullptr, nullptr, nullptr) {}

    MOCK_METHOD(bool, Initialize, (eprosima::fastdds::dds::DomainParticipant*), (override));
    MOCK_METHOD(void, PublishAppState, (), (override));
    // F009 (C-1): PublishContainerState was added to MessageTransmitter; mock it
    // so ServiceMonitorTest does not fall through to the real implementation
    // (which dereferences a null dds_manager_ and SEGFAULTs).
    MOCK_METHOD(void, PublishContainerState, (), (override));
    MOCK_METHOD(void, PublishAppList, (), (override));
};

#endif