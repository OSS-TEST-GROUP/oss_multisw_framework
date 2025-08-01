#ifndef ROS2_ADSW_MANAGER_UNITTEST_MOCK_DDS_TRANSMITTER_HPP_
#define ROS2_ADSW_MANAGER_UNITTEST_MOCK_DDS_TRANSMITTER_HPP_

#include <gmock/gmock.h>

#include "transmitter/DDSTransmitter.hpp"

using namespace eprosima::fastdds::dds;

namespace ros2 {
namespace adsw_manager {


class MockDDSTransmitter : public DDSTransmitter {
  public:
    MockDDSTransmitter() : DDSTransmitter() {}
    virtual ~MockDDSTransmitter() {}

    MOCK_METHOD(bool, Initialize, (), ());
    MOCK_METHOD(void, Finalize, (), ());
    MOCK_METHOD(DomainParticipant*, GetDomain, (), ());
    MOCK_METHOD(Publisher*, GetPublisher, (), ());
    MOCK_METHOD(Subscriber*, GetSubscriber, (), ());
    MOCK_METHOD(std::shared_ptr<DDSMessageEndpoint>, GetMessageEndpoint, (std::string topic), ());
    MOCK_METHOD(void, AddMessageEndpoint, (std::shared_ptr<DDSMessageEndpoint> endpoint), ());
    MOCK_METHOD(bool, SendMessage, (MessageType mtype, void* ptr), ());
};

MockDDSTransmitter* Mock_DDSTransmitter;

DDSTransmitter::DDSTransmitter() {
    Mock_DDSTransmitter = new MockDDSTransmitter();
}

DDSTransmitter::~DDSTransmitter() {
    Mock_DDSTransmitter = nullptr;
}
bool DDSTransmitter::Initialize() {
    return Mock_DDSTransmitter->Initialize();
}
bool DDSTransmitter::AddMessageEndpoint(std::shared_ptr<DDSMessageEndpoint> endpoint) {
    Mock_DDSTransmitter->AddMessageEndpoint(endpoint);
    return true; // Mock implementation
}
void DDSTransmitter::Finalize() {
    Mock_DDSTransmitter->Finalize();
}
std::shared_ptr<DDSMessageEndpoint> DDSTransmitter::GetMessageEndpoint(std::string topic) {
    return Mock_DDSTransmitter->GetMessageEndpoint(topic);
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
bool DDSTransmitter::SendMessage(MessageType mtype, void* ptr) {
    return Mock_DDSTransmitter->SendMessage(mtype, ptr);
}

}
}

#endif