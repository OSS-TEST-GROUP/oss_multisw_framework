#ifndef MOCK_LIFECYCLE_MESSAGE_ENDPOINT_HPP_
#define MOCK_LIFECYCLE_MESSAGE_ENDPOINT_HPP_

#include <gmock/gmock.h>
#include "transmitter/MessageEndPoint.hpp"

namespace lifecycle_manager {
class MockMessageEndPoint : public MessageEndPoint {
public:
    // Declare the global pointer before using it
    inline static MockMessageEndPoint* Mock_MessageEndPoint = nullptr;

    MockMessageEndPoint(std::string topic) : MessageEndPoint(topic) {
        // Don't auto-set Mock_MessageEndPoint in constructor
    }
    ~MockMessageEndPoint() {
        if (Mock_MessageEndPoint == this) Mock_MessageEndPoint = nullptr;
    }
    // Only mock virtual methods
    MOCK_METHOD(std::string, GetTopic, (), ());
    MOCK_METHOD(std::string, GetTypeName, (), ());
    MOCK_METHOD(std::shared_ptr<TypeSupport>, GetTypeSupport, (), ());
    MOCK_METHOD(void, SetTypeSupport, (std::shared_ptr<TypeSupport> typesupport), ());
    MOCK_METHOD(void, OnDataPublished, (bool ret), ());
    MOCK_METHOD(void, OnDataAvailable, (), ());
};


MessageEndPoint::MessageEndPoint(std::string topic) : topic_(topic) {
}
MessageEndPoint::~MessageEndPoint() {
}
std::string MessageEndPoint::GetTopic() {
    // Don't forward to mock - use real implementation
    return topic_;
}
std::string MessageEndPoint::GetTypeName() {
    if (type_support_) {
        return type_support_->get_type_name();
    }
    return "";
}
std::shared_ptr<TypeSupport> MessageEndPoint::GetTypeSupport() {
    return type_support_;
}   
void MessageEndPoint::SetTypeSupport(std::shared_ptr<TypeSupport> typesupport) {
    type_support_ = typesupport;
}

void MessageEndPoint::OnDataPublished(bool ret) {
    // Base implementation does nothing
}   
void MessageEndPoint::OnDataAvailable() {
    // Base implementation does nothing
}

} // namespace lifecycle_manager

#endif // MOCK_LIFECYCLE_MESSAGE_ENDPOINT_HPP_