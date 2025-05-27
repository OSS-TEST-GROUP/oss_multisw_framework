#ifndef DIAGNOSTICS_TRASMITTER_MESSAGE_TRANSMITTER_TYPE_HPP_
#define DIAGNOSTICS_TRASMITTER_MESSAGE_TRANSMITTER_TYPE_HPP_

#include <iostream>
#include <memory>

#include "Message.hpp"
#include "ReceivedMessage.hpp"

namespace diagnostic_manager {

using ReceiveMessageCreator = std::function<std::shared_ptr<ReceivedMessage>()>;
using ForwardFunctionType = std::function<void(std::shared_ptr<Message>)>;

} // namespace diagnostic_manager

#endif
