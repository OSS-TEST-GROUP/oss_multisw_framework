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
 * @file DiagnosticInfoTestClient.cpp
 *
 */

#include "diagnostic/DiagnosticInfoPubSubTypes.hpp"

#include <chrono>
#include <thread>
#include <string>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

using namespace eprosima::fastdds::dds;
using namespace diagnosticmgr;

class DiagnosticInfoTestClient
{
private:

    DomainParticipant* participant_;

    Subscriber* subscriber_;
    Publisher* publisher_;

    DataReader* reader_info_;
    DataReader* reader_info_list_;
    DataWriter* writer_info_;
    DataWriter* writer_info_list_;

    std::map<std::string, Topic*> topic_list;

    std::vector<TypeSupport> type_list;

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
            std::string type_name = reader->type()->get_name();
            if (type_name == "diagnosticmgr::DiagnosticInfoResponse") {
                SampleInfo info;
                if (reader->take_next_sample(&diag_info_resp, &info) == eprosima::fastdds::dds::RETCODE_OK)
                {
                    if (info.valid_data)
                    {
                        std::cout << "- DiagnosticInfoResponse" << std::endl;
                        if (diag_info_resp.ret_code() == CommonStatusType::PASS) {
                            std::cout << "diag_code=" << diag_info_resp.diag_code()
                                    << ", diag_name=" << diag_info_resp.diag_name()
                                    << ", status=" << (int32_t)diag_info_resp.status()
                                    << ", message=" << diag_info_resp.message() << std::endl;
                        } else {
                            std::cout << "No DiagnosticInfo." << std::endl;
                        }
                    }
                }
            } else if (type_name == "diagnosticmgr::DiagnosticInfoListResponse") {
                SampleInfo info;
                if (reader->take_next_sample(&diag_info_list_resp, &info) == eprosima::fastdds::dds::RETCODE_OK)
                {
                    if (info.valid_data)
                    {
                        auto diag_codes = diag_info_list_resp.diag_codes();
                        auto diag_names = diag_info_list_resp.diag_names();

                        std::cout << "- DiagnosticInfoListResponse" << std::endl;
                        if (diag_info_list_resp.ret_code() == CommonStatusType::PASS) {
                            for (uint32_t i = 0; i < diag_codes.size(); i++) {
                                std::cout << "diag_code=" << diag_codes[i] << ", diag_name=" << diag_names[i] << std::endl;
                            }
                            if (diag_codes.size() == 0) {
                                std::cout << "No DiagnosticInfo." << std::endl;
                            }
                        } else {
                            std::cout << "Failed to get DiagnosticInfoList." << std::endl;
                        }
                    }
                }
            }

        }

        DiagnosticInfoResponse          diag_info_resp;
        DiagnosticInfoListResponse      diag_info_list_resp;

    }
    listener_;

public:

    DiagnosticInfoTestClient()
        : participant_(nullptr)
        , subscriber_(nullptr)
        , publisher_(nullptr)
    {
    }

    virtual ~DiagnosticInfoTestClient()
    {

        if (subscriber_ != nullptr)
        {
            participant_->delete_subscriber(subscriber_);
        }
        if (publisher_ != nullptr)
        {
            participant_->delete_publisher(publisher_);
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
        type_list.emplace_back(new DiagnosticInfoRequestPubSubType());
        type_list.emplace_back(new DiagnosticInfoResponsePubSubType());
        type_list.emplace_back(new DiagnosticInfoListRequestPubSubType());
        type_list.emplace_back(new DiagnosticInfoListResponsePubSubType());
        for (auto& iter : type_list) {
            iter.register_type(participant_);
        }

        // Create the subscriptions Topic
        topic_list["mmf/diagnosticmgr/DiagnosticInfoRequest"] = (participant_->create_topic("mmf/diagnosticmgr/DiagnosticInfoRequest", "diagnosticmgr::DiagnosticInfoRequest", TOPIC_QOS_DEFAULT));
        topic_list["mmf/diagnosticmgr/DiagnosticInfoResponse"] = (participant_->create_topic("mmf/diagnosticmgr/DiagnosticInfoResponse", "diagnosticmgr::DiagnosticInfoResponse", TOPIC_QOS_DEFAULT));
        topic_list["mmf/diagnosticmgr/DiagnosticInfoListRequest"] = (participant_->create_topic("mmf/diagnosticmgr/DiagnosticInfoListRequest", "diagnosticmgr::DiagnosticInfoListRequest", TOPIC_QOS_DEFAULT));
        topic_list["mmf/diagnosticmgr/DiagnosticInfoListResponse"] = (participant_->create_topic("mmf/diagnosticmgr/DiagnosticInfoListResponse", "diagnosticmgr::DiagnosticInfoListResponse", TOPIC_QOS_DEFAULT));

        // Create the Subscriber
        subscriber_ = participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr);

        if (subscriber_ == nullptr)
        {
            return false;
        }

        publisher_ = participant_->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr);

        DataReaderQos reader_qos = DATAREADER_QOS_DEFAULT;
        reader_qos.reliability().kind = RELIABLE_RELIABILITY_QOS;
        reader_qos.durability().kind = TRANSIENT_LOCAL_DURABILITY_QOS;

        // Create the DataReader
        reader_info_ = subscriber_->create_datareader(topic_list["mmf/diagnosticmgr/DiagnosticInfoResponse"], reader_qos, &listener_);
        reader_info_list_ = subscriber_->create_datareader(topic_list["mmf/diagnosticmgr/DiagnosticInfoListResponse"], reader_qos, &listener_);
        writer_info_ = publisher_->create_datawriter(topic_list["mmf/diagnosticmgr/DiagnosticInfoRequest"], DATAWRITER_QOS_DEFAULT);
        writer_info_list_ = publisher_->create_datawriter(topic_list["mmf/diagnosticmgr/DiagnosticInfoListRequest"], DATAWRITER_QOS_DEFAULT);

        return true;
    }

    //!Run the Subscriber
    void run(
            uint32_t samples)
    {
        DiagnosticInfoRequest           diag_info_req;
        DiagnosticInfoListRequest       diag_info_list_req;

        while (true) {
            std::cout << std::endl;
            std::cout << "\n==== Diagnostic Console ====\n";
            std::cout << "Select a request type:\n";
            std::cout << "1. DiagnosticInfoRequest\n";
            std::cout << "2. DiagnosticInfoListRequest\n";
            std::cout << "3. Exit\n";
            std::cout << "Choice: ";

            int choice;
            std::cin >> choice;

            if (choice == 1) {
                std::cout << "Enter DiagnosticInfoRequest parameters:\n";

                int32_t diag_code;
                std::cout << "  diag_code (integer): ";
                std::cin >> diag_code;
                diag_info_req.diag_code(diag_code);

                std::string diag_name;
                std::cin.ignore(); // clear newline from input buffer
                std::cout << "  diag_name (string): ";
                std::getline(std::cin, diag_name);
                diag_info_req.diag_name(diag_name);

                writer_info_->write(&diag_info_req);
            } else if (choice == 2) {
                writer_info_list_->write(&diag_info_list_req);
            } else if (choice == 3) {
                std::cout << "Exiting console.\n";
                break;

            } else {
                std::cout << "Invalid choice. Please try again.\n";
            }

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

    DiagnosticInfoTestClient* mysub = new DiagnosticInfoTestClient();
    if (mysub->init())
    {
        mysub->run(samples);
    }

    delete mysub;
    return 0;
}
