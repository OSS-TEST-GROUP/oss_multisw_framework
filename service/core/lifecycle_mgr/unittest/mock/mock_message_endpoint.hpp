#ifndef MOCK_LIFECYCLE_MESSAGE_ENDPOINT_HPP_
#define MOCK_LIFECYCLE_MESSAGE_ENDPOINT_HPP_

#include <gmock/gmock.h>
#include "transmitter/MessageEndPoint.hpp"

namespace lifecycle_manager {

class MockMessageEndPoint : public MessageEndPoint {
public:
    MockMessageEndPoint(std::string topic) : MessageEndPoint(topic) {}
    virtual ~MockMessageEndPoint() {}

    MOCK_METHOD(std::string, GetTopic, (), ());
    MOCK_METHOD(std::string, GetTypeName, (), ());
    MOCK_METHOD(std::shared_ptr<TypeSupport>, GetTypeSupport, (), ());
    MOCK_METHOD(void, SetTypeSupport, (std::shared_ptr<TypeSupport> typesupport), ());
    MOCK_METHOD(void, OnDataPublished, (bool ret), ());
    MOCK_METHOD(void, OnDataAvailable, (), ());
};

MockMessageEndPoint* Mock_MessageEndPoint;

MessageEndPoint::MessageEndPoint(std::string topic) : topic_(topic) {
}
MessageEndPoint::~MessageEndPoint() {
    Mock_MessageEndPoint = nullptr;
}
std::string MessageEndPoint::GetTopic() {
    return Mock_MessageEndPoint->GetTopic();
}
std::string MessageEndPoint::GetTypeName() {
    return Mock_MessageEndPoint->GetTypeName();
}
std::shared_ptr<TypeSupport> MessageEndPoint::GetTypeSupport() {
    return Mock_MessageEndPoint->GetTypeSupport();  
}   
void MessageEndPoint::SetTypeSupport(std::shared_ptr<TypeSupport> typesupport) {
    Mock_MessageEndPoint->SetTypeSupport(typesupport);  
}

void MessageEndPoint::OnDataPublished(bool ret) {
    Mock_MessageEndPoint->OnDataPublished(ret);     
}   
void MessageEndPoint::OnDataAvailable() {
    Mock_MessageEndPoint->OnDataAvailable();      
}

} // namespace lifecycle_manager

#endif // MOCK_LIFECYCLE_MESSAGE_ENDPOINT_HPP_