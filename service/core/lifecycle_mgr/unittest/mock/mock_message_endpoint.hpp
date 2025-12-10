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
        Mock_MessageEndPoint = this;
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
    MockMessageEndPoint::Mock_MessageEndPoint = nullptr;
}
std::string MessageEndPoint::GetTopic() {
    return MockMessageEndPoint::Mock_MessageEndPoint->GetTopic();
}
std::string MessageEndPoint::GetTypeName() {
    return MockMessageEndPoint::Mock_MessageEndPoint->GetTypeName();
}
std::shared_ptr<TypeSupport> MessageEndPoint::GetTypeSupport() {
    return MockMessageEndPoint::Mock_MessageEndPoint->GetTypeSupport();  
}   
void MessageEndPoint::SetTypeSupport(std::shared_ptr<TypeSupport> typesupport) {
    MockMessageEndPoint::Mock_MessageEndPoint->SetTypeSupport(typesupport);  
}

void MessageEndPoint::OnDataPublished(bool ret) {
    MockMessageEndPoint::Mock_MessageEndPoint->OnDataPublished(ret);     
}   
void MessageEndPoint::OnDataAvailable() {
    MockMessageEndPoint::Mock_MessageEndPoint->OnDataAvailable();      
}

} // namespace lifecycle_manager

#endif // MOCK_LIFECYCLE_MESSAGE_ENDPOINT_HPP_