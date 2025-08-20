#include "StopAppHandler.hpp"

using namespace eprosima::fastdds::dds;
using namespace lifecyclemgr;

namespace adsw_manager::agl {

    StopAppHandler::~StopAppHandler() {
        if (subscriber_ && reader_) subscriber_->delete_datareader(reader_);
        if (publisher_ && writer_) publisher_->delete_datawriter(writer_);
        if (participant_ && req_topic_) participant_->delete_topic(req_topic_);
        if (participant_ && res_topic_) participant_->delete_topic(res_topic_);
    }

    bool StopAppHandler::Initialize(DomainParticipant* participant, Publisher* publisher,
                                            Subscriber* subscriber, RequestCallback cb) {
        if (!publisher || !subscriber || !participant || !cb) {
            std::cerr << "[StopAppHandler] Invalid argument during initialization.\n";
            return false;
        }

        participant_ = participant;
        publisher_ = publisher;
        subscriber_ = subscriber;

        req_type_ = TypeSupport(new StopServiceRequestPubSubType());
        req_type_.register_type(participant_);

        res_type_ = TypeSupport(new StopServiceResponsePubSubType());
        res_type_.register_type(participant_);

        req_topic_ = participant_->create_topic(
            "mmf/lifecyclemgr/StopServiceRequest",
            req_type_.get_type_name(),
            TOPIC_QOS_DEFAULT
        );
        res_topic_ = participant_->create_topic(
            "mmf/lifecyclemgr/StopServiceResponse",
            res_type_.get_type_name(),
            TOPIC_QOS_DEFAULT
        );

        if (!req_topic_ || !res_topic_) {
            std::cerr << "[StopAppHandler] Failed to create topic(s).\n";
            return false;
        }

        listener_ = std::make_unique<RequestListener>(std::move(cb));

        reader_ = subscriber_->create_datareader(req_topic_, DATAREADER_QOS_DEFAULT, listener_.get());
        if (!reader_) {
            std::cerr << "[StopAppHandler] Failed to create DataReader.\n";
            return false;
        }

        writer_ = publisher_->create_datawriter(res_topic_, DATAWRITER_QOS_DEFAULT, nullptr);
        if (!writer_) {
            std::cerr << "[StopAppHandler] Failed to create DataWriter.\n";
            return false;
        }

        std::cout << "[StopAppHandler] Initialized successfully.\n";
        return true;
    }

    bool StopAppHandler::PublishResponse(const StopServiceResponse& response) {
        
        ReturnCode_t rc = writer_->write((void*)&response);
        if (rc!=RETCODE_OK) {
            std::cerr << "[StopAppHandler] write() failed: " << (int)rc << std::endl;
            return false;
        }
        return true;
    }

}