#include "NotifyContainerStateWriter.hpp"

#include <iostream>

using namespace eprosima::fastdds::dds;
using namespace lifecyclemgr;

namespace adsw_manager::agl {

    NotifyContainerStateWriter::~NotifyContainerStateWriter() {
        if (publisher_ && writer_) publisher_->delete_datawriter(writer_);
        if (participant_ && topic_) participant_->delete_topic(topic_);
    }

    bool NotifyContainerStateWriter::Initialize(Publisher* publisher, DomainParticipant* participant) {

        participant_ = participant;
        publisher_ = publisher;

        if (!participant_ || !publisher_) {
            std::cerr << "[NotifyContainerStateWriter] Invalid participant or publisher." << std::endl;
            return false;
        }

        type_ = TypeSupport(new NotifyContainerStatePubSubType());
        type_.register_type(participant_);

        topic_ = participant_->create_topic(
            "mmf/lifecyclemgr/NotifyContainerState",
            type_.get_type_name(),
            TOPIC_QOS_DEFAULT
        );

        if (!topic_) {
            std::cerr << "[NotifyContainerStateWriter] Failed to create topic" << std::endl;
            return false;
        }

        writer_ = publisher_->create_datawriter(topic_, DATAWRITER_QOS_DEFAULT, nullptr);
        if (!writer_) {
            std::cerr << "[NotifyContainerStateWriter] Failed to create DataWriter." << std::endl;
            return false;
        }

        std::cout << "[NotifyContainerStateWriter] Initialized successfully." << std::endl;
        return true;
    }

    bool NotifyContainerStateWriter::Publish(const NotifyContainerState& msg)
    {
        ReturnCode_t rc = writer_->write((void*)&msg);
        if (rc != RETCODE_OK) {
            std::cerr << "[NotifyContainerStateWriter] write() failed: " << (int)rc << std::endl;
            return false;
        }
        return true;
    }

}
