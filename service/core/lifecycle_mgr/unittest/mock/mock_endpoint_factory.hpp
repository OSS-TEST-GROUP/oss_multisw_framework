#ifndef MOCK_LIFECYCLE_ENDPOINT_FACTORY_HPP_
#define MOCK_LIFECYCLE_ENDPOINT_FACTORY_HPP_

#include <gmock/gmock.h>
#include "endpoint/EndPointFactory.hpp"

namespace lifecycle_manager {

class MockEndPointFactory : public EndPointFactory {
public:
    MockEndPointFactory() : EndPointFactory() {}
    virtual ~MockEndPointFactory() {}

    // Mock methods for EndPointFactory
    MOCK_METHOD(int, Initialize, (), ());
    MOCK_METHOD(int, CreateEndPoints, (std::shared_ptr<DDSTransmitter>), ());
    MOCK_METHOD(const char*, GetTopic, (MessageType mtype), ());

    // Add more mocks as needed
};

MockEndPointFactory* Mock_EndPointFactory;

EndPointFactory::EndPointFactory() {}
EndPointFactory::~EndPointFactory() {}

int EndPointFactory::Initialize() {
    return Mock_EndPointFactory->Initialize();
}

int EndPointFactory::CreateEndPoints(std::shared_ptr<DDSTransmitter> transmitter) {
    return Mock_EndPointFactory->CreateEndPoints(transmitter);
}
const char* EndPointFactory::GetTopic(MessageType mtype) {
    return Mock_EndPointFactory->GetTopic(mtype);
}


} // namespace lifecycle_manager

#endif // MOCK_LIFECYCLE_ENDPOINT_FACTORY_HPP_