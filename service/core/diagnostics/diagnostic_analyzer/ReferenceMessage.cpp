#include "ReferenceMessage.hpp"

namespace diagnostic_manager {

ReferenceMessage::ReferenceMessage(const std::string& name, const std::string& message, MessageProtocolType protocol)
        : name_(name), message_(message), protocol_(protocol) {}

ReferenceMessage::~ReferenceMessage() {}

std::string ReferenceMessage::GetName() {
    return name_;
}

std::string ReferenceMessage::GetMessage() {
    return message_;
}

MessageProtocolType ReferenceMessage::GetProtocol() {
    return protocol_;
}

void ReferenceMessage::SetDiagCode(int32_t code) {
    diag_codes_.insert(code);
}

void ReferenceMessage::RemoveDiagCode(int32_t code) {
    diag_codes_.erase(code);
}

bool ReferenceMessage::CheckDiagCode(int32_t code) {
    return diag_codes_.count(code) > 0;
}

std::set<int32_t> ReferenceMessage::GetAllDiagCode() {
    return diag_codes_;
}

} // namespace diagnostic_manager
