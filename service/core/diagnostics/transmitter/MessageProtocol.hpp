#ifndef DIAGNOSTICS_TRASMITTER_MESSAGE_PROTOCOL_HPP_
#define DIAGNOSTICS_TRASMITTER_MESSAGE_PROTOCOL_HPP_

#include <iostream>

#include "Message.hpp"
#include "MessageProtocolType.hpp"
#include "MessageTransmitterType.hpp"

namespace diagnostic_manager {

class MessageProtocol {
  public:
    virtual bool Initialize() = 0;
    virtual void Finalize() = 0;

    void SetForwardCallback(ForwardFunctionType function);
    void ForwardMessage(std::shared_ptr<Message> message);

    virtual MessageProtocolType GetProtocolType() = 0;

    virtual bool PublishMessage(std::shared_ptr<Message>) = 0;

    virtual bool SubscribeMessage(const std::string& name) = 0;
    virtual bool UnsubscribeMessage(const std::string& name) = 0;

  private:
    ForwardFunctionType forward_cb_;
};

} // namespace diagnostic_manager

#endif
