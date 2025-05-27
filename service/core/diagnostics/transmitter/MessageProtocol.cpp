#include "MessageProtocol.hpp"

namespace diagnostic_manager {

void MessageProtocol::SetForwardCallback(ForwardFunctionType function) {
    forward_cb_ = function;
}

void MessageProtocol::ForwardMessage(std::shared_ptr<Message> message) {
    if (forward_cb_) {
        forward_cb_(message);
    }
}

} // namespace diagnostic_manager
