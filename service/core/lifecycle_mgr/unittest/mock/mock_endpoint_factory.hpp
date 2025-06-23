// Google Mock for EndPointFactory
#pragma once
#include <gmock/gmock.h>
#include "endpoint/EndPointFactory.hpp"

namespace lifecycle_manager {
class MockEndPointFactory : public EndPointFactory {
public:
    MOCK_METHOD(void, Initialize, (), ());
    MOCK_METHOD(void, CreateEndPoints, (std::shared_ptr<DDSTransmitter>), ());
    // Add more mocks as needed
};

MockEndPointFactory* Mock_EndPointFactory;

EndPointFactory::EndPointFactory() {}
EndPointFactory::~EndPointFactory() {}

bool EndPointFactory::Initialize() {
    return Mock_EndPointFactory->Initialize();
}


bool EndPointFactory::CreateEndPoints(std::shared_ptr<DDSTransmitter> transmitter) {
    return Mock_EndPointFactory->CreateEndPoints(transmitter);
}


} // namespace lifecycle_manager
