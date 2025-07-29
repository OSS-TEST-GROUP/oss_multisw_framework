#include "DDSTransmitter.hpp"

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>

#include "logger/Log.hpp"

namespace lifecycle_manager {

DDSTransmitter::DDSTransmitter() {}
DDSTransmitter::~DDSTransmitter() {}


bool DDSTransmitter::Initialize(){
    LOG_DEBUG("DDSTransmitter::Initialize");

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

    message_queue_ = std::make_shared<MessageQueue>(this);

    if (message_queue_ != nullptr){
        message_queue_->Start();
    }


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

        endpoints_[endpoint->GetTopic()] = endpoint;
    }

    return false;

}

std::shared_ptr<DDSMessageEndPoint> DDSTransmitter::GetMessageEndPoint(std::string topic){

    auto search = endpoints_.find(topic);

    if (search != endpoints_.end()){
        LOG_DEBUG("GetEndpoint for " << topic << ", found " << search->first);
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

bool DDSTransmitter::SendMessage(MessageType mtype, void* ptr){

    if (message_queue_ != nullptr){

        MsgType message = std::make_pair(mtype, ptr);
        message_queue_->SendMessage(message);

        return true;
    }

    LOG_DEBUG("SendMessage failure !! " << static_cast<int32_t>(mtype)  );
    return false;

}

} // namespace lifecycle_manager
