#ifndef DIAGNOSTICS_UNITTEST_MOCK_MESSAGE_PROTOCOL_MOCK_HPP_
#define DIAGNOSTICS_UNITTEST_MOCK_MESSAGE_PROTOCOL_MOCK_HPP_

#include <gmock/gmock.h>
#include "transmitter/MessageProtocol.hpp"

namespace diagnostic_manager {

class MessageProtocolMock : public MessageProtocol {
    MOCK_METHOD0(Initialize, bool());
    MOCK_METHOD0(Finalize, void());
    MOCK_METHOD0(GetProtocolType, MessageProtocolType());
    MOCK_METHOD1(PublishMessage, bool(std::shared_ptr<Message>));
    MOCK_METHOD1(SubscribeMessage, bool(const std::string& name));
    MOCK_METHOD1(UnsubscribeMessage, bool(const std::string& name));

};

MessageProtocolMock* Mock_MessageProtocolMock;

} // namespace diagnostic_manager

#endif 