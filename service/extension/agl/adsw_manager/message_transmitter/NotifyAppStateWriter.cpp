#include "NotifyAppStateWriter.hpp"

using namespace eprosima::fastdds::dds;
using namespace adsw_manager_agl;

namespace adsw_manager::agl {

    NotifyAppStateWriter::~NotifyAppStateWriter() {

        if (publisher_ && writer_) publisher_->delete_datawriter(writer_);
        if (participant_ && topic_) participant_->delete_topic(topic_);
        
    }

    bool NotifyAppStateWriter::Initialize(Publisher* publisher, DomainParticipant* participant) {

        participant_ = participant;
        publisher_ = publisher;

        if (!participant_ || !publisher_) {
            std::cerr << "[NotifyAppStateWriter] Invalid participant or publisher." << std::endl;
            return false;
        }

        type_ = TypeSupport(new NotifyAppStatePubSubType());
        type_.register_type(participant_);

        topic_ = participant_->create_topic(
            "mmf/lifecyclemgr/NotifyAppState",
            type_.get_type_name(),
            TOPIC_QOS_DEFAULT
        );

        if (!topic_) {
            std::cerr << "[NotifyAppStateWriter] Failed to create topic" << std::endl;
            return false;
        }

        writer_ = publisher_->create_datawriter(topic_, DATAWRITER_QOS_DEFAULT, nullptr);
        if (!writer_) {
            std::cerr << "[NotifyAppStateWriter] Failed to create DataWriter." << std::endl;
            return false;
        }

        std::cout << "[NotifyAppStateWriter] Initialized successfully." << std::endl;
        return true;
    }

    bool NotifyAppStateWriter::Publish(const NotifyAppState& msg)
    {
        ReturnCode_t rc = writer_->write((void*)&msg);
        if (rc != RETCODE_OK) {
            std::cerr << "[NotifyAppStateWriter] write() failed: " << (int)rc << std::endl;
            return false;
        }
        return true;
    }


}