#ifndef ROS2_ADSW_MANAGER_UNITTEST_MOCK_ENDPOINT_FACTORY_HPP_
#define ROS2_ADSW_MANAGER_UNITTEST_MOCK_ENDPOINT_FACTORY_HPP_

#include <gmock/gmock.h>
#include "endpoint/EndPointFactory.hpp"

using namespace eprosima::fastdds::dds;

namespace ros2 {
namespace adsw_manager {
    


class MockEndPointFactory : public EndPointFactory {
  public:
    MockEndPointFactory() : EndPointFactory() {}
     ~MockEndPointFactory() override = default;

    MOCK_METHOD(bool, Initialize, (), ());
    MOCK_METHOD(int32_t, CreateEndPoints, (std::shared_ptr<DDSTransmitter> transmitter), ());
    MOCK_METHOD(const char*, GetTopic, (MessageType mtype), ());


};

MockEndPointFactory* Mock_EndPointFactory;

EndPointFactory::EndPointFactory() {
}
EndPointFactory::~EndPointFactory() {
}
bool EndPointFactory::Initialize() {
    return Mock_EndPointFactory->Initialize();
}
int32_t EndPointFactory::CreateEndPoints(const std::shared_ptr<DDSTransmitter>& transmitter) {
    return Mock_EndPointFactory->CreateEndPoints(transmitter);
}
const char* EndPointFactory::GetTopic(MessageType mtype) {
    return Mock_EndPointFactory->GetTopic(mtype);
}

}
}

#endif