// Google Mock for DDSTransmitter
#ifndef MOCK_LIFECYCLE_DDS_TRANSMITTER_HPP_
#define MOCK_LIFECYCLE_DDS_TRANSMITTER_HPP_


#include <gmock/gmock.h>
#include "transmitter/DDSTransmitter.hpp"

namespace lifecycle_manager {
class MockDDSTransmitter : public DDSTransmitter {
public:
    MockDDSTransmitter() : DDSTransmitter() {}
    ~MockDDSTransmitter() = default;

    // Only mock virtual methods
    MOCK_METHOD(bool, Initialize, (), ());
    MOCK_METHOD(void, Finalize, (), ());
    MOCK_METHOD(bool, AddMessageEndpoint, (std::shared_ptr<DDSMessageEndPoint>), ());
    MOCK_METHOD(std::shared_ptr<DDSMessageEndPoint>, GetMessageEndPoint, (std::string), ());
    MOCK_METHOD(bool, SendMessage, (MessageType mtype, void* ptr), ());
    MOCK_METHOD(DomainParticipant*, GetDomain, (), ());
    MOCK_METHOD(Publisher*, GetPublisher, (), ());
    MOCK_METHOD(Subscriber*, GetSubscriber, (), ());
};

inline MockDDSTransmitter* Mock_DDSTransmitter = nullptr;

DDSTransmitter::DDSTransmitter() {}
// DDSTransmitter::~DDSTransmitter() 정의는 제거했다. 프로덕션 헤더가 `= default` 로 바뀌어
// 아웃오브라인 정의가 위법(explicitly-defaulted 함수의 재정의)이 되고,
// defaulted 소멸자는 컴파일러가 각 TU 에 만들어주므로 링크 심이 필요 없다.

bool DDSTransmitter::Initialize() {

    return Mock_DDSTransmitter->Initialize();
}
bool DDSTransmitter::AddMessageEndpoint(const std::shared_ptr<DDSMessageEndPoint>& endpoint) {
    return Mock_DDSTransmitter->AddMessageEndpoint(endpoint);
}
void DDSTransmitter::Finalize() {
    return Mock_DDSTransmitter->Finalize();
}
std::shared_ptr<DDSMessageEndPoint> DDSTransmitter::GetMessageEndPoint(const std::string& topic) {
    return Mock_DDSTransmitter->GetMessageEndPoint(topic);  
}

bool DDSTransmitter::SendMessage(MessageType mtype, void* ptr){
    return Mock_DDSTransmitter->SendMessage(mtype, ptr);
}

DomainParticipant* DDSTransmitter::GetDomain() {
    return Mock_DDSTransmitter->GetDomain();
}

Publisher* DDSTransmitter::GetPublisher() {
    return Mock_DDSTransmitter->GetPublisher();
}

Subscriber* DDSTransmitter::GetSubscriber() {
    return Mock_DDSTransmitter->GetSubscriber();
}

} // namespace lifecycle_manager

#endif // MOCK_LIFECYCLE_DDS_TRANSMITTER_HPP_