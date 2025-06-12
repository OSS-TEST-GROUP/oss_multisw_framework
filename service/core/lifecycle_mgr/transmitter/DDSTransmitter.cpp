#include "DDSTransmitter.hpp"

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>

namespace lifecycle_manager {

DDSTransmitter::DDSTransmitter() {}
DDSTransmitter::~DDSTransmitter() {}


bool DDSTransmitter::Initialize(){
    std::cout << __FILE__ << " :: " << __LINE__ << std::endl;

   // Create the participant
    DomainParticipantQos pqos = PARTICIPANT_QOS_DEFAULT;
    pqos.name("Shape_pub_participant");
    // Set Shapes metadata
    pqos.properties().properties().emplace_back("fastdds.application.id", "SHAPES_DEMO", true);
    pqos.properties().properties().emplace_back("fastdds.application.metadata", "", true);
    participant_ = DomainParticipantFactory::get_instance()->create_participant_with_default_profile(
         nullptr, StatusMask::none());
    if (participant_ == nullptr)
    {
        throw std::runtime_error("Participant initialization failed");
    }

    // Create the publisher
    PublisherQos pub_qos = PUBLISHER_QOS_DEFAULT;
    participant_->get_default_publisher_qos(pub_qos);
    publisher_ = participant_->create_publisher(pub_qos, nullptr, StatusMask::none());
    if (publisher_ == nullptr)
    {
        throw std::runtime_error("Publisher initialization failed");
    }

    // Create the subscriber
    SubscriberQos sub_qos = SUBSCRIBER_QOS_DEFAULT;
    participant_->get_default_subscriber_qos(sub_qos);
    subscriber_ = participant_->create_subscriber(sub_qos, nullptr, StatusMask::none());
    if (subscriber_ == nullptr)
    {
        std::cout << "Subscriber initialization failed." << std::endl;
        return false;
    }
    std::cout << __FILE__ << " :: " << __LINE__ << std::endl;

    return true;    
}


void DDSTransmitter::Finalize(){

    if (nullptr != participant_)
    {
        // Delete DDS entities contained within the DomainParticipant
        participant_->delete_contained_entities();

        // Delete DomainParticipant
        DomainParticipantFactory::get_instance()->delete_participant(participant_);
    }
}


bool DDSTransmitter::AddMessageEndpoint(std::shared_ptr<DDSMessageEndPoint> endpoint){

    if (endpoint){
    std::cout << __FILE__ << " :: " << __LINE__ << std::endl;

        endpoints_[endpoint->GetTopic()] = endpoint;
    }
    std::cout << __FILE__ << " :: " << __LINE__ << std::endl;

    return false;

}

std::shared_ptr<DDSMessageEndPoint> DDSTransmitter::GetMessageEndPoint(std::string topic){

    auto search = endpoints_.find(topic);

    if (search != endpoints_.end()){
        std::cout << "Found " << search->first  << '\n';
        return search->second;
    } else{ 
        std::cout << "Not found\n";
    }
    return nullptr;
    
}


DomainParticipant* DDSTransmitter::GetDomain(){
    return participant_;
}

Publisher* DDSTransmitter::GetPublisher(){
    return publisher_;
}


Subscriber* DDSTransmitter::GetSubscriber(){
    return subscriber_;
}


} // namespace lifecycle_manager
