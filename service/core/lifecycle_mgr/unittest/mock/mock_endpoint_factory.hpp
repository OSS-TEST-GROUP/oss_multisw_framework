#ifndef MOCK_LIFECYCLE_ENDPOINT_FACTORY_HPP_
#define MOCK_LIFECYCLE_ENDPOINT_FACTORY_HPP_

#include <gmock/gmock.h>
#include "endpoint/EndPointFactory.hpp"

namespace lifecycle_manager {

class MockEndPointFactory : public EndPointFactory {
public:
    MockEndPointFactory() : EndPointFactory() {}
    ~MockEndPointFactory() = default;

    // Only mock virtual methods
    MOCK_METHOD(int, Initialize, (), ());
    MOCK_METHOD(int32_t, CreateEndPoints, (std::shared_ptr<DDSTransmitter>, const EndPointHandlers&), ());
    MOCK_METHOD(const char*, GetTopic, (MessageType mtype), ());
};

inline MockEndPointFactory* Mock_EndPointFactory = nullptr;


EndPointFactory::EndPointFactory() {}
EndPointFactory::~EndPointFactory() {}

int EndPointFactory::Initialize() {
    return Mock_EndPointFactory->Initialize();
}

int32_t EndPointFactory::CreateEndPoints(std::shared_ptr<DDSTransmitter> transmitter,
                                         const EndPointHandlers& handlers) {
    return Mock_EndPointFactory->CreateEndPoints(transmitter, handlers);
}
const char* EndPointFactory::GetTopic(MessageType mtype) {
    return Mock_EndPointFactory->GetTopic(mtype);
}

} // namespace lifecycle_manager

#endif // MOCK_LIFECYCLE_ENDPOINT_FACTORY_HPP_