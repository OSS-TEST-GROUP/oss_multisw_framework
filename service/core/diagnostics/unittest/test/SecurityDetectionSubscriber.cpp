// Copyright 2016 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @file DiagnosticNotificationReceiver.cpp
 *
 */

#include "security_mgr/SecurityDetectionPubSubTypes.hpp"

#include <chrono>
#include <thread>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>


using namespace eprosima::fastdds::dds;

class SecurityDetectionReceiver
{
private:

    DomainParticipant* participant_;

    Subscriber* subscriber_;

    DataReader* reader_;

    Topic* topic_;

    TypeSupport type_;

    class SubListener : public DataReaderListener
    {
    public:

        SubListener()
        {
        }

        ~SubListener() override
        {
        }

        void on_subscription_matched(
                DataReader* reader,
                const SubscriptionMatchedStatus& info) override
        {
            if (info.current_count_change == 1)
            {
                std::cout << "Subscriber matched." << std::endl;
                on_data_available(reader);
            }
            else if (info.current_count_change == -1)
            {
                std::cout << "Subscriber unmatched." << std::endl;
            }
            else
            {
                std::cout << info.current_count_change
                          << " is not a valid value for SubscriptionMatchedStatus current count change" << std::endl;
            }
        }

        void on_data_available(
                DataReader* reader) override
        {
            SampleInfo info;
            if (reader->take_next_sample(&message_, &info) == eprosima::fastdds::dds::RETCODE_OK)
            {
                if (info.valid_data)
                {
                    std::cout << "- Receive the security detection report" << std::endl;

                    std::cout << "  Client ID: " << std::string(message_.client_id().data()) << std::endl;
                    std::cout << "  DDS Topic: " << std::string(message_.dds_topic().data()) << std::endl;
                    std::cout << "  Rule: " << std::string(message_.rule().data()) << std::endl;
                    std::cout << "  Timestamp (ms): " << message_.timestamp_ms() << std::endl;
                }
            }
        }

        security::DetectionReport message_;

    }
    listener_;

public:

    SecurityDetectionReceiver()
        : participant_(nullptr)
        , subscriber_(nullptr)
        , topic_(nullptr)
        , reader_(nullptr)
        , type_(new security::DetectionReportPubSubType())
    {
    }

    virtual ~SecurityDetectionReceiver()
    {
        if (reader_ != nullptr)
        {
            subscriber_->delete_datareader(reader_);
        }
        if (topic_ != nullptr)
        {
            participant_->delete_topic(topic_);
        }
        if (subscriber_ != nullptr)
        {
            participant_->delete_subscriber(subscriber_);
        }
        DomainParticipantFactory::get_instance()->delete_participant(participant_);
    }

    //!Initialize the subscriber
    bool init()
    {
        DomainParticipantQos participantQos;
        participantQos.name("Participant_subscriber");
        participant_ = DomainParticipantFactory::get_instance()->create_participant(0, participantQos);

        if (participant_ == nullptr)
        {
            return false;
        }

        // Register the Type
        type_.register_type(participant_);

        // Create the subscriptions Topic
        topic_ = participant_->create_topic("security/DetectionReport", "security::DetectionReport", TOPIC_QOS_DEFAULT);

        if (topic_ == nullptr)
        {
            return false;
        }

        // Create the Subscriber
        subscriber_ = participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr);

        if (subscriber_ == nullptr)
        {
            return false;
        }

        // Create the DataReader
        reader_ = subscriber_->create_datareader(topic_, DATAREADER_QOS_DEFAULT, &listener_);

        if (reader_ == nullptr)
        {
            return false;
        }

        return true;
    }

    //!Run the Subscriber
    void run(
            uint32_t samples)
    {
        while (true)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

};

int main(
        int argc,
        char** argv)
{
    std::cout << "Starting subscriber." << std::endl;
    uint32_t samples = 10;

    SecurityDetectionReceiver* mysub = new SecurityDetectionReceiver();
    if (mysub->init())
    {
        mysub->run(samples);
    }

    delete mysub;
    return 0;
}
