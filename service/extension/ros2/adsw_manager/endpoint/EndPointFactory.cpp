
#include "EndPointFactory.hpp"
#include "logger/Log.hpp"
#include "transmitter/DDSTransmitter.hpp"
#include "ROSDiscoveryInfoEndpoint.hpp"

namespace ros2 {
namespace adsw_manager {

EndPointFactory::EndPointFactory() {
}
EndPointFactory::~EndPointFactory() {
}


bool EndPointFactory::Initialize(){
    topic_lists_[MessageType::ROS2_DISCOVREY_INFO] = "ros_discovery_info";
    return true;
}


void EndPointFactory::Finalize(){
    topic_lists_.clear();    
} 

int32_t EndPointFactory::CreateEndPoints(std::shared_ptr<DDSTransmitter> transmitter){


    DomainParticipant* participant = transmitter->GetDomain();
    Publisher* publisher = transmitter->GetPublisher();
    Subscriber* subscriber = transmitter->GetSubscriber();


    for (auto const& [key, val] : topic_lists_)
    {
        LOG_DEBUG(__FUNCTION__ << "::" << static_cast<int32_t>(key) << "," << val);

        std::shared_ptr<DDSMessageEndpoint> msg_endpoint = nullptr;
        PubSubType pubsub = PubSubType::PUBSUB_NONE;

        switch(key){
            case MessageType::ROS2_DISCOVREY_INFO :{
                std::shared_ptr<ROSDiscoveryInfoEndpoint> discovery = std::make_shared<ROSDiscoveryInfoEndpoint>(val);

                msg_endpoint = std::dynamic_pointer_cast<DDSMessageEndpoint>(discovery);
                pubsub = PubSubType::SUBSCRIBE_ONLY;
            }
            break;
        }

        if (msg_endpoint){
            if (pubsub == PubSubType::PUBLISH_ONLY){
                msg_endpoint->Initialize(participant, publisher, nullptr);
            }else if (pubsub == PubSubType::SUBSCRIBE_ONLY){
                msg_endpoint->Initialize(participant, nullptr, subscriber);
            }else if (pubsub == PubSubType::PUBSUB_BOTH){
                msg_endpoint->Initialize(participant, publisher, subscriber);   
            }
            
            transmitter->AddMessageEndpoint(std::dynamic_pointer_cast<DDSMessageEndpoint>(msg_endpoint));
        }
    }

    return topic_lists_.size();
}



} // namespace adsw_manager
} // namespace ros2
