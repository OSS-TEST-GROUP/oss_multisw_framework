#include "AppStateReader.hpp"

#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>

#include "agl/String_PubSubTypes.hpp"

#include <iostream>

using namespace eprosima::fastdds::dds;
using namespace std_msgs::msg::dds_;

namespace adsw_manager::agl {

    AppStateReader::AppStateReader(DomainParticipant* participant, const std::string& app_name)
        : participant_(participant),
          app_name_(app_name),
          type_(new String_PubSubType())
    {
        type_.register_type(participant_);

        std::string topic_name = "rt/app_status/" + app_name_;

        topic_ = participant_->create_topic(
            topic_name,
            type_.get_type_name(),
            TOPIC_QOS_DEFAULT
        );

        subscriber_ = participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT);

        reader_ = subscriber_->create_datareader(
            topic_,
            DATAREADER_QOS_DEFAULT,
            this
        );

        last_received_time_ = std::chrono::steady_clock::time_point::min();

    }

    AppStateReader::AppStateReader() : last_received_time_(std::chrono::steady_clock::time_point::min()) {}

    AppStateReader::~AppStateReader() {
        if (subscriber_ && reader_) {
            subscriber_->delete_datareader(reader_);
        }
        if (participant_ && subscriber_) {
            participant_->delete_subscriber(subscriber_);
        }
        if (participant_ && topic_) {
            participant_->delete_topic(topic_);
        }

    }

    std::chrono::steady_clock::time_point AppStateReader::GetLastReceivedTime() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return last_received_time_;
    }

    void AppStateReader::on_data_available(DataReader* reader) {
        SampleInfo info;
        String_ msg;

        if (reader->take_next_sample(&msg, &info) == RETCODE_OK && info.valid_data) {
            if (info.instance_state == ALIVE_INSTANCE_STATE) {
                std::lock_guard<std::mutex> lock(mutex_);
                last_received_time_ = std::chrono::steady_clock::now();
                std::cout << "[AppStateReader] app_status received : [["
                          << app_name_ << "]] " << msg.data() << std::endl;
            }
        }
    }

}
