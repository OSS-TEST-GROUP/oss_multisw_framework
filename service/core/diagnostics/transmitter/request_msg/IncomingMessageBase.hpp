#ifndef DIAGNOSTICS_TRASMITTER_RECEIVED_MESSAGE_HPP_
#define DIAGNOSTICS_TRASMITTER_RECEIVED_MESSAGE_HPP_

#include <iostream>
#include <memory>

#include "executor/RequestBase.hpp"
#include "transmitter/Message.hpp"

namespace diagnostic_manager {

class IncomingMessageBase : public RequestBase {
  public:
    void SetMessage(std::shared_ptr<Message> msg);
    std::shared_ptr<Message> GetMessage();

  private:
    std::shared_ptr<Message> msg_;
};

} // namespace diagnostic_manager

#endif
