#include "DDSTransmitter.hpp"
#include "logger/Log.hpp"

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>

namespace ros2 {
namespace adsw_manager {

DDSTransmitter::DDSTransmitter() {}
DDSTransmitter::~DDSTransmitter() {}

bool DDSTransmitter::Initialize(){
    LOG_DEBUG("DDSTransmitter Start");

   // Create the participant
    DomainParticipantQos pqos = PARTICIPANT_QOS_DEFAULT;
    pqos.name("adsw_participant");
    // Set Shapes metadata
    pqos.properties().properties().emplace_back("fastdds.application.id", "ADSW_Manager", true);
    pqos.properties().properties().emplace_back("fastdds.application.metadata", "", true);
    participant_ = DomainParticipantFactory::get_instance()->create_participant_with_default_profile(
         nullptr, StatusMask::none());
    if (participant_ == nullptr)
    {
        LOG_ERROR("Participant initialization failed");
        return false;
    }

    // Create the publisher
    PublisherQos pub_qos = PUBLISHER_QOS_DEFAULT;
    participant_->get_default_publisher_qos(pub_qos);
    publisher_ = participant_->create_publisher(pub_qos, nullptr, StatusMask::none());
    if (publisher_ == nullptr)
    {
        LOG_ERROR("Publisher initialization failed" );
        return false;
    }

    // Create the subscriber
    SubscriberQos sub_qos = SUBSCRIBER_QOS_DEFAULT;
    participant_->get_default_subscriber_qos(sub_qos);
    subscriber_ = participant_->create_subscriber(sub_qos, nullptr, StatusMask::none());
    if (subscriber_ == nullptr)
    {
        LOG_ERROR("Subscriber initialization failed.");
        return false;
    }
    LOG_DEBUG("DDSTransmitter End");

    return true;    
}


void DDSTransmitter::Finalize(){
    for (auto& iter : endpoints_) {
        iter.second->Finalize();
    }

    if (nullptr != participant_)
    {
        // Delete DDS entities contained within the DomainParticipant
        participant_->delete_contained_entities();

        // Delete DomainParticipant
        DomainParticipantFactory::get_instance()->delete_participant(participant_);
    }
}


bool DDSTransmitter::AddMessageEndpoint(std::shared_ptr<DDSMessageEndpoint> endpoint){

    if (endpoint){

        endpoints_[endpoint->GetTopic()] = endpoint;
    }

    return true;

}

std::shared_ptr<DDSMessageEndpoint> DDSTransmitter::GetMessageEndpoint(std::string topic){

    auto search = endpoints_.find(topic);

    if (search != endpoints_.end()){

        LOG_DEBUG("Found " << search->first  );
        return search->second;
    } else{ 
        std::cout << "Not found\n";
    }
    return nullptr;
    
}

DomainParticipant* DDSTransmitter::GetDomain() {
    return participant_;
}

Publisher* DDSTransmitter::GetPublisher() {
    return publisher_;
}

Subscriber* DDSTransmitter::GetSubscriber() {
    return subscriber_;
}


} // namespace lifecycle_manager
} // namespace ros2
