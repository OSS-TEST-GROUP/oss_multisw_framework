#include "Message.hpp"

namespace diagnostic_manager {

Message::Message() {
    update_time_ = std::chrono::steady_clock::now();
}
Message::Message(const std::string& name, MessageProtocolType protocol_type, const std::string& type, std::shared_ptr<VariantData> data, const std::string& identifier)
         : name_(name), protocol_(protocol_type), type_name_(type), data_(data), identifier_(identifier) {
    Message();
}

Message::~Message() {}

void Message::SetName(const std::string& name) {
    name_ = name;
}
std::string Message::GetName() {
    return name_;
}

void Message::SetProtocolType(MessageProtocolType type) {
    protocol_ = type;
}
MessageProtocolType Message::GetProtocolType() {
    return protocol_;
}

void Message::SetTypeName(const std::string& name) {
    type_name_ = name;
}
std::string Message::GetTypeName() {
    return type_name_;
}

void Message::SetData(std::shared_ptr<VariantData> data) {
    data_ = data;
}
std::shared_ptr<VariantData> Message::GetData() {
    return data_;
}

void Message::SetIdentifier(const std::string& identifier) {
    identifier_ = identifier;
}
std::string Message::GetIdentifier() {
    return identifier_;
}

MessageTime Message::GetUpdateTime() {
    return update_time_;
}

} // namespace diagnostic_manager
