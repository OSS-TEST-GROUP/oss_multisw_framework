#include "ReceivedMessage.hpp"

namespace diagnostic_manager {

void ReceivedMessage::SetMessage(std::shared_ptr<Message> msg) {
    msg_ = msg;
}

std::shared_ptr<Message> ReceivedMessage::GetMessage() {
    return msg_;
}

} // namespace diagnostic_manager
