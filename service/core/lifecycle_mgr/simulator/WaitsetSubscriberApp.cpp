// Copyright 2024 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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
 * @file WaitsetSubscriberApp.cpp
 *
 */

#include "WaitsetSubscriberApp.hpp"

#include <condition_variable>
#include <stdexcept>

#include <fastdds/dds/core/condition/GuardCondition.hpp>
#include <fastdds/dds/core/condition/WaitSet.hpp>
#include <fastdds/dds/core/status/SubscriptionMatchedStatus.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/subscriber/qos/SubscriberQos.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>

#include "Application.hpp"
#include "CLIParser.hpp"

#include "lifecycle/LifecycleDiagReportPubSubTypes.hpp"
#include "lifecycle/SetServiceModeRequestPubSubTypes.hpp"
#include "lifecycle/UpdateServiceListRequestPubSubTypes.hpp"
#include "lifecycle/GetServiceStateRequestPubSubTypes.hpp"
#include "lifecycle/NotifyServiceStatePubSubTypes.hpp"
#include "lifecycle/StartServiceRequestPubSubTypes.hpp"
#include "lifecycle/StopServiceRequestPubSubTypes.hpp"

using namespace eprosima::fastdds::dds;
using namespace lifecyclemgr;

namespace eprosima {
namespace fastdds {
namespace examples {
namespace hello_world {

WaitsetSubscriberApp::WaitsetSubscriberApp(
        const CLIParser::subscriber_config& config,
        const std::string& topic_name,
        const CLIParser::MessageType& message_kind)
    : participant_(nullptr)
    , subscriber_(nullptr)
    , topic_(nullptr)
    , reader_(nullptr)
    , type_(nullptr)
    , samples_(config.samples)
    , received_samples_(0)
    , stop_(false)
    , message_kind_(message_kind)    
{
    // Create the participant
    auto factory = DomainParticipantFactory::get_instance();
    participant_ = factory->create_participant_with_default_profile(nullptr, StatusMask::none());
    if (participant_ == nullptr)
    {
        throw std::runtime_error("Participant initialization failed");
    }

    switch(message_kind_)
    {
       case CLIParser::MessageType::LIFECYCLE_DIAG_REPORT:
        {
            type_ = std::make_shared<TypeSupport>(new LifecycleDiagReportPubSubType());
        }
        break;

        case CLIParser::MessageType::GET_SERVICE_STATE_REQUEST:
        case CLIParser::MessageType::GET_SERVICE_STATE_RESPONSE:
        {
            type_ = std::make_shared<TypeSupport>(new GetServiceStateRequestPubSubType());
        }
        break;

        case CLIParser::MessageType::NOTIFY_SERVICE_STATE:
        {
            type_ = std::make_shared<TypeSupport>(new NotifyServiceStatePubSubType());
        }
        break;

        case CLIParser::MessageType::SET_SERVICE_MODE_REQUST:
        case CLIParser::MessageType::SET_SERVICE_MODE_RESPONSE:
        {
            type_ = std::make_shared<TypeSupport>(new SetServiceModeRequestPubSubType());
        }
        break;

        case CLIParser::MessageType::START_SERVICE_REQUEST:
        case CLIParser::MessageType::START_SERVICE_RESPONSE:
        {
            type_ = std::make_shared<TypeSupport>(new StartServiceRequestPubSubType());
        }
        break;

        case CLIParser::MessageType::STOP_SERVICE_REQUEST:
        case CLIParser::MessageType::STOP_SERVICE_RESPONSE:
        {
            type_ = std::make_shared<TypeSupport>(new StopServiceRequestPubSubType());
        }
        break;

        case CLIParser::MessageType::UPDATE_SERVICE_LIST_REQUEST:
        case CLIParser::MessageType::UPDATE_SERVICE_LIST_RESPONSE:
        {
            type_ = std::make_shared<TypeSupport>(new UpdateServiceListRequestPubSubType());
        }
        break;
    }

    // Register the type
    type_->register_type(participant_);

    // Create the subscriber
    SubscriberQos sub_qos = SUBSCRIBER_QOS_DEFAULT;
    participant_->get_default_subscriber_qos(sub_qos);
    subscriber_ = participant_->create_subscriber(sub_qos, nullptr, StatusMask::none());
    if (subscriber_ == nullptr)
    {
        throw std::runtime_error("Subscriber initialization failed");
    }

    // Create the topic
    TopicQos topic_qos = TOPIC_QOS_DEFAULT;
    participant_->get_default_topic_qos(topic_qos);
    topic_ = participant_->create_topic(topic_name, type_->get_type_name(), topic_qos);
    if (topic_ == nullptr)
    {
        throw std::runtime_error("Topic initialization failed");
    }

    // Create the reader
    DataReaderQos reader_qos = DATAREADER_QOS_DEFAULT;
    subscriber_->get_default_datareader_qos(reader_qos);
    reader_ = subscriber_->create_datareader(topic_, reader_qos, nullptr, StatusMask::all());
    if (reader_ == nullptr)
    {
        throw std::runtime_error("DataReader initialization failed");
    }

    // Prepare a wait-set
    wait_set_.attach_condition(reader_->get_statuscondition());
    wait_set_.attach_condition(terminate_condition_);
}

WaitsetSubscriberApp::~WaitsetSubscriberApp()
{
    if (nullptr != participant_)
    {
        // Delete DDS entities contained within the DomainParticipant
        participant_->delete_contained_entities();

        // Delete DomainParticipant
        DomainParticipantFactory::get_instance()->delete_participant(participant_);
    }
}

void WaitsetSubscriberApp::run()
{
    while (!is_stopped())
    {
        ConditionSeq triggered_conditions;
        ReturnCode_t ret_code = wait_set_.wait(triggered_conditions, eprosima::fastdds::dds::c_TimeInfinite);
        if (RETCODE_OK != ret_code)
        {
            EPROSIMA_LOG_ERROR(SUBSCRIBER_WAITSET, "Error waiting for conditions");
            continue;
        }
        for (Condition* cond : triggered_conditions)
        {
            StatusCondition* status_cond = dynamic_cast<StatusCondition*>(cond);
            if (nullptr != status_cond)
            {
                Entity* entity = status_cond->get_entity();
                StatusMask changed_statuses = entity->get_status_changes();
                if (changed_statuses.is_active(StatusMask::subscription_matched()))
                {
                    SubscriptionMatchedStatus status_;
                    reader_->get_subscription_matched_status(status_);
                    if (status_.current_count_change == 1)
                    {
                        std::cout << "Waitset Subscriber matched." << std::endl;
                    }
                    else if (status_.current_count_change == -1)
                    {
                        std::cout << "Waitset Subscriber unmatched." << std::endl;
                    }
                    else
                    {
                        std::cout << status_.current_count_change <<
                            " is not a valid value for SubscriptionMatchedStatus current count change" <<
                            std::endl;
                    }
                }
                if (changed_statuses.is_active(StatusMask::data_available()))
                {
                    SampleInfo info;
                    while (!is_stopped())
                    {
                        switch(message_kind_)
                        {
                            case CLIParser::MessageType::LIFECYCLE_DIAG_REPORT:
                                ret_code = reader_->take_next_sample(&diag_report_, &info); break;

                            case CLIParser::MessageType::START_SERVICE_REQUEST:
                                ret_code = reader_->take_next_sample(&start_svc_req_, &info); break;
                            case CLIParser::MessageType::START_SERVICE_RESPONSE:
                                ret_code = reader_->take_next_sample(&start_svc_resp_, &info); break;

                            case CLIParser::MessageType::STOP_SERVICE_REQUEST:
                                ret_code = reader_->take_next_sample(&stop_svc_req_, &info); break;
                            case CLIParser::MessageType::STOP_SERVICE_RESPONSE:
                                ret_code = reader_->take_next_sample(&stop_svc_resp_, &info); break;

                            case CLIParser::MessageType::GET_SERVICE_STATE_REQUEST:
                                ret_code = reader_->take_next_sample(&get_svc_state_req_, &info); break;
                            case CLIParser::MessageType::GET_SERVICE_STATE_RESPONSE:
                                ret_code = reader_->take_next_sample(&get_svc_state_resp_, &info); break;
                            case CLIParser::MessageType::NOTIFY_SERVICE_STATE:
                                ret_code = reader_->take_next_sample(&notify_svc_state, &info); break;

                            case CLIParser::MessageType::SET_SERVICE_MODE_REQUST:
                                ret_code = reader_->take_next_sample(&set_service_req_, &info); break;
                            case CLIParser::MessageType::SET_SERVICE_MODE_RESPONSE:
                                ret_code = reader_->take_next_sample(&set_service_resp_, &info); break;

                            case CLIParser::MessageType::UPDATE_SERVICE_LIST_REQUEST:
                                ret_code = reader_->take_next_sample(&update_service_req_, &info); break;
                            case CLIParser::MessageType::UPDATE_SERVICE_LIST_RESPONSE:
                                ret_code = reader_->take_next_sample(&update_service_resp_, &info); break;
                        }

                        if (RETCODE_OK != ret_code){
                            break;
                        }

                        if ((info.instance_state == ALIVE_INSTANCE_STATE) && info.valid_data)
                        {
                            received_samples_++;
                            // Print Hello world message data
                            std::cout << "Message: " << static_cast<uint8_t>(message_kind_) << " with index: '"
                                      << received_samples_ << "' RECEIVED" << std::endl;
                            if (samples_ > 0 && (received_samples_ >= samples_))
                            {
                                stop();
                            }
                        }
                    }
                }
            }
        }
    }
}

bool WaitsetSubscriberApp::is_stopped()
{
    return stop_.load();
}

void WaitsetSubscriberApp::stop()
{
    stop_.store(true);
    terminate_condition_.set_trigger_value(true);
}

} // namespace hello_world
} // namespace examples
} // namespace fastdds
} // namespace eprosima
