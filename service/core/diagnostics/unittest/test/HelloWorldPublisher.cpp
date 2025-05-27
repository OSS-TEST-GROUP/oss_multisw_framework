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
 * @file HelloWorldPublisher.cpp
 *
 */

#include "DataTypeTestPubSubTypes.hpp"

#include <chrono>
#include <thread>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>
#include <fastdds/rtps/transport/UDPv4TransportDescriptor.hpp>

using namespace eprosima::fastdds::dds;

class HelloWorldPublisher
{
private:

    DataTypeTest data_;

    DomainParticipant* participant_;

    Publisher* publisher_;

    Topic* topic_;

    DataWriter* writer_;

    TypeSupport type_;

    class PubListener : public DataWriterListener
    {
    public:

        PubListener()
            : matched_(0)
        {
        }

        ~PubListener() override
        {
        }

        void on_publication_matched(
                DataWriter*,
                const PublicationMatchedStatus& info) override
        {
            if (info.current_count_change == 1)
            {
                matched_ = info.total_count;
                std::cout << "Publisher matched." << std::endl;
            }
            else if (info.current_count_change == -1)
            {
                matched_ = info.total_count;
                std::cout << "Publisher unmatched." << std::endl;
            }
            else
            {
                std::cout << info.current_count_change
                        << " is not a valid value for PublicationMatchedStatus current count change." << std::endl;
            }
        }

        std::atomic_int matched_;

    } listener_;

public:

    HelloWorldPublisher()
        : participant_(nullptr)
        , publisher_(nullptr)
        , topic_(nullptr)
        , writer_(nullptr)
        , type_(new DataTypeTestPubSubType())
    {
    }

    virtual ~HelloWorldPublisher()
    {
        if (writer_ != nullptr)
        {
            publisher_->delete_datawriter(writer_);
        }
        if (publisher_ != nullptr)
        {
            participant_->delete_publisher(publisher_);
        }
        if (topic_ != nullptr)
        {
            participant_->delete_topic(topic_);
        }
        DomainParticipantFactory::get_instance()->delete_participant(participant_);
    }

    //!Initialize the publisher
    bool init()
    {
        data_.v_char8(11);
        data_.v_char16(12);
        data_.v_uint8(13);
        data_.v_int16(14);
        data_.v_uint16(15);
        data_.v_int32(16);
        data_.v_uint32(17);
        data_.v_int64(18);
        data_.v_uint64(19);
        data_.v_float(123.45);
        data_.v_double(234.56);
        data_.v_bool(true);
        data_.v_string("Test String");

        data_.v_structure().id(1234);
        data_.v_structure().name("HelloPinky");

        data_.seq_char8(std::vector<char>({1,2}));
        data_.seq_char16(std::vector<wchar_t>({1,2}));
        data_.seq_uint8(std::vector<uint8_t>({1,2}));
        data_.seq_int16(std::vector<int16_t>({1,2}));
        data_.seq_uint16(std::vector<uint16_t>({1,2}));
        data_.seq_int32(std::vector<int32_t>({1,2}));
        data_.seq_uint32(std::vector<uint32_t>({1,2}));
        data_.seq_int64(std::vector<int64_t>({1,2}));
        data_.seq_uint64(std::vector<uint64_t>({1,2}));
        data_.seq_float(std::vector<float>({1.2,2.3}));
        data_.seq_double(std::vector<double>({1.23,2.34}));
        data_.seq_bool(std::vector<bool>({false, true}));
        data_.seq_string(std::vector<std::string>({"abc", "def"}));
        data_.seq_structure(std::vector<SubStructure>({SubStructure(), SubStructure()}));

        DomainParticipantQos participantQos;
        participantQos.name("Participant_publisher");

        // Coerce to use UDP protocol for testing
        // auto udp_transport = std::make_shared<eprosima::fastdds::rtps::UDPv4TransportDescriptor>();
        // udp_transport->sendBufferSize = 9216;
        // udp_transport->receiveBufferSize = 9216;
        // udp_transport->maxMessageSize = 9216;
        // udp_transport->non_blocking_send = true;
        
        // participantQos.transport().user_transports.push_back(udp_transport);
        // participantQos.transport().use_builtin_transports = false;

        participant_ = DomainParticipantFactory::get_instance()->create_participant(0, participantQos);

        if (participant_ == nullptr)
        {
            return false;
        }

        std::cout << "Created a participant. GUID=" << participant_->guid() << std::endl;

        // Register the Type
        type_.register_type(participant_);

        // Create the publications Topic
        topic_ = participant_->create_topic("/HelloWorldTopic", "DataTypeTest", TOPIC_QOS_DEFAULT);

        if (topic_ == nullptr)
        {
            return false;
        }

        // Create the Publisher
        publisher_ = participant_->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr);

        if (publisher_ == nullptr)
        {
            return false;
        }

        // Create the DataWriter
        writer_ = publisher_->create_datawriter(topic_, DATAWRITER_QOS_DEFAULT, &listener_);

        if (writer_ == nullptr)
        {
            return false;
        }
        return true;
    }

    //!Send a publication
    bool publish()
    {
        if (listener_.matched_ > 0)
        {
            writer_->write(&data_);
            return true;
        }
        return false;
    }

    //!Run the Publisher
    void run(
            uint32_t samples)
    {
        uint32_t samples_sent = 0;
        while (true)
        {
            if (publish())
            {
                samples_sent++;
                //std::cout << "Message: " << hello_.message() << " with index: " << hello_.index() << " SENT" << std::endl;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
};

int main(
        int argc,
        char** argv)
{
    std::cout << "Starting publisher." << std::endl;
    uint32_t samples = 10;

    HelloWorldPublisher* mypub = new HelloWorldPublisher();
    if(mypub->init())
    {
        mypub->run(samples);
    }

    delete mypub;
    return 0;
}
