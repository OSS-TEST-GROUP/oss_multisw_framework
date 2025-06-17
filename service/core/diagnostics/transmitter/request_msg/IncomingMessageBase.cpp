#include "IncomingMessageBase.hpp"

namespace diagnostic_manager {

void IncomingMessageBase::SetMessage(std::shared_ptr<Message> msg) {
    msg_ = msg;
}

std::shared_ptr<Message> IncomingMessageBase::GetMessage() {
    return msg_;
}

} // namespace diagnostic_manager
