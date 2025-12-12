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
DDSTransmitter::~DDSTransmitter() {}

bool DDSTransmitter::Initialize() {

    return Mock_DDSTransmitter->Initialize();
}
bool DDSTransmitter::AddMessageEndpoint(std::shared_ptr<DDSMessageEndPoint> endpoint) {
    // Mock implementation
    return true;
}
void DDSTransmitter::Finalize() {
    return Mock_DDSTransmitter->Finalize();
}
std::shared_ptr<DDSMessageEndPoint> DDSTransmitter::GetMessageEndPoint(std::string topic) {
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