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
 * @file PublisherApp.cpp
 *
 */

#include "PublisherApp.hpp"
#include "MessageHandle.hpp"

#include <condition_variable>
#include <stdexcept>

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/qos/DataWriterQos.hpp>
#include <fastdds/dds/publisher/qos/PublisherQos.hpp>

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

PublisherApp::PublisherApp(
        const CLIParser::publisher_config& config,
        const std::string& topic_name,
        const CLIParser::MessageType& message_kind)
    : participant_(nullptr)
    , publisher_(nullptr)
    , topic_(nullptr)
    , writer_(nullptr)
    , type_(nullptr)
    , matched_(0)
    , samples_(config.samples)
    , expected_matches_(config.matched)
    , stop_(false)
    , message_kind_(message_kind)
{
    index_ = 0;
std::cout << __FUNCTION__ << "::" << __LINE__ << std::endl;
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
        {
            type_ = std::make_shared<TypeSupport>(new GetServiceStateRequestPubSubType());
        }
        break;        
        case CLIParser::MessageType::GET_SERVICE_STATE_RESPONSE:
        {
            type_ = std::make_shared<TypeSupport>(new GetServiceStateResponsePubSubType());
        }
        break;

        case CLIParser::MessageType::NOTIFY_SERVICE_STATE:
        {
            type_ = std::make_shared<TypeSupport>(new NotifyServiceStatePubSubType());
        }
        break;

        case CLIParser::MessageType::SET_SERVICE_MODE_REQUST:
        {
            type_ = std::make_shared<TypeSupport>(new SetServiceModeRequestPubSubType());
        }
        break;        
        case CLIParser::MessageType::SET_SERVICE_MODE_RESPONSE:
        {
            type_ = std::make_shared<TypeSupport>(new SetServiceModeResponsePubSubType());
        }
        break;

        case CLIParser::MessageType::START_SERVICE_REQUEST:
        {
            type_ = std::make_shared<TypeSupport>(new StartServiceRequestPubSubType());
        }
        break;        
        case CLIParser::MessageType::START_SERVICE_RESPONSE:
        {
            type_ = std::make_shared<TypeSupport>(new StartServiceResponsePubSubType());
        }
        break;

        case CLIParser::MessageType::STOP_SERVICE_REQUEST:
        {
            type_ = std::make_shared<TypeSupport>(new StopServiceRequestPubSubType());
        }
        break;        
        case CLIParser::MessageType::STOP_SERVICE_RESPONSE:
        {
            type_ = std::make_shared<TypeSupport>(new StopServiceResponsePubSubType());
        }
        break;

        case CLIParser::MessageType::UPDATE_SERVICE_LIST_REQUEST:
        {
            type_ = std::make_shared<TypeSupport>(new UpdateServiceListRequestPubSubType());
        }
        break;        
        case CLIParser::MessageType::UPDATE_SERVICE_LIST_RESPONSE:
        {
            type_ = std::make_shared<TypeSupport>(new UpdateServiceListResponsePubSubType());
        }
        break;
    }

    if (type_ == nullptr)
    {
        throw std::runtime_error("Participant initialization failed type null");
    }
 
    // Register the type
    type_->register_type(participant_);

    // Create the publisher
    PublisherQos pub_qos = PUBLISHER_QOS_DEFAULT;
    participant_->get_default_publisher_qos(pub_qos);

    publisher_ = participant_->create_publisher(pub_qos, nullptr, StatusMask::none());
    if (publisher_ == nullptr)
    {
        throw std::runtime_error("Publisher initialization failed");
    }

    // Create the topic
    TopicQos topic_qos = TOPIC_QOS_DEFAULT;
    participant_->get_default_topic_qos(topic_qos);
    topic_ = participant_->create_topic(topic_name, type_->get_type_name(), topic_qos);
    if (topic_ == nullptr)
    {
        throw std::runtime_error("Topic initialization failed");
    }

    // Create the data writer
    DataWriterQos writer_qos = DATAWRITER_QOS_DEFAULT;
    writer_qos.history().depth = 5;
    publisher_->get_default_datawriter_qos(writer_qos);
    writer_ = publisher_->create_datawriter(topic_, writer_qos, this, StatusMask::all());
    if (writer_ == nullptr)
    {
        throw std::runtime_error("DataWriter initialization failed");
    }
std::cout << __FUNCTION__ << "::" << __LINE__ << std::endl;    
}

PublisherApp::~PublisherApp()
{
    if (nullptr != participant_)
    {
        // Delete DDS entities contained within the DomainParticipant
        participant_->delete_contained_entities();

        // Delete DomainParticipant
        DomainParticipantFactory::get_instance()->delete_participant(participant_);
    }
}

void PublisherApp::on_publication_matched(
        DataWriter* /*writer*/,
        const PublicationMatchedStatus& info)
{
    if (info.current_count_change == 1)
    {
        matched_ = static_cast<int16_t>(info.current_count);
        std::cout << "Publisher matched." << std::endl;
        cv_.notify_one();
    }
    else if (info.current_count_change == -1)
    {
        matched_ = static_cast<int16_t>(info.current_count);
        std::cout << "Publisher unmatched." << std::endl;
    }
    else
    {
        std::cout << info.current_count_change
                  << " is not a valid value for PublicationMatchedStatus current count change" << std::endl;
    }
}

void PublisherApp::run()
{
    while (!is_stopped() && ((samples_ == 0) || (index_ < samples_)))
    {
std::cout << __FUNCTION__ << "::" << __LINE__ << std::endl;    
        if (publish())
        {
            std::cout << "Message: " << static_cast<uint8_t>(message_kind_) << " index: '" << index_
                      << "' SENT" << std::endl;

            if (index_ == 1u)
            {
                ReturnCode_t acked = RETCODE_ERROR;
                do
                {
                    dds::Duration_t acked_wait{1, 0};
                    acked = writer_->wait_for_acknowledgments(acked_wait);
                    std::cout << "Message: " << static_cast<uint8_t>(message_kind_) << " wait ack" << std::endl;
                }
                while (acked != RETCODE_OK);
            }
        }
        // Wait for period or stop event
        std::unique_lock<std::mutex> period_lock(mutex_);
        cv_.wait_for(period_lock, std::chrono::milliseconds(period_ms_), [&]()
                {
                    return is_stopped();
                });
    }
}

bool PublisherApp::publish()
{
    bool ret = false;
    // Wait for the data endpoints discovery
    std::unique_lock<std::mutex> matched_lock(mutex_);
    cv_.wait(matched_lock, [&]()
            {
std::cout << __FUNCTION__ << "::" << __LINE__ << ", " << matched_ << " >= " << expected_matches_ << std::endl;   
                // at least one has been discovered
                return ((matched_ >= expected_matches_) || is_stopped());
            });
std::cout << __FUNCTION__ << "::" << __LINE__ << ", " << matched_ << " >= " << expected_matches_ << std::endl;   
    if (!is_stopped())
    {
        switch(message_kind_)
        {
        case CLIParser::MessageType::LIFECYCLE_DIAG_REPORT:
            MessageHandle::SetMessage(diag_report_);
            ret = (RETCODE_OK == writer_->write(&diag_report_)); break;

        case CLIParser::MessageType::GET_SERVICE_STATE_REQUEST:
            MessageHandle::SetMessage(get_svc_state_req_);
            ret = (RETCODE_OK == writer_->write(&get_svc_state_req_)); break;

        case CLIParser::MessageType::GET_SERVICE_STATE_RESPONSE:
            MessageHandle::SetMessage(get_svc_state_resp_);
            ret = (RETCODE_OK == writer_->write(&get_svc_state_resp_)); break;

        case CLIParser::MessageType::NOTIFY_SERVICE_STATE:
            MessageHandle::SetMessage(notify_svc_state);
            ret = (RETCODE_OK == writer_->write(&notify_svc_state)); break;

        case CLIParser::MessageType::START_SERVICE_REQUEST:
            MessageHandle::SetMessage(start_svc_req_);
            ret = (RETCODE_OK == writer_->write(&start_svc_req_)); break;

        case CLIParser::MessageType::START_SERVICE_RESPONSE:
            MessageHandle::SetMessage(start_svc_resp_);
            ret = (RETCODE_OK == writer_->write(&start_svc_resp_)); break;

        case CLIParser::MessageType::STOP_SERVICE_REQUEST:
            MessageHandle::SetMessage(stop_svc_req_);
            ret = (RETCODE_OK == writer_->write(&stop_svc_req_)); break;

        case CLIParser::MessageType::STOP_SERVICE_RESPONSE:
            MessageHandle::SetMessage(stop_svc_resp_);
            ret = (RETCODE_OK == writer_->write(&stop_svc_resp_)); break;

        case CLIParser::MessageType::SET_SERVICE_MODE_REQUST:
            MessageHandle::SetMessage(set_service_req_);
            ret = (RETCODE_OK == writer_->write(&set_service_req_)); break;

        case CLIParser::MessageType::SET_SERVICE_MODE_RESPONSE:
            MessageHandle::SetMessage(set_service_resp_);
            ret = (RETCODE_OK == writer_->write(&set_service_resp_)); break;

        case CLIParser::MessageType::UPDATE_SERVICE_LIST_REQUEST:
            MessageHandle::SetMessage(update_service_req_);
            ret = (RETCODE_OK == writer_->write(&update_service_req_)); break;
            
        case CLIParser::MessageType::UPDATE_SERVICE_LIST_RESPONSE:
            MessageHandle::SetMessage(update_service_resp_);
            ret = (RETCODE_OK == writer_->write(&update_service_resp_)); break;        
        
        }

        ++index_;

    }
    return ret;
}

bool PublisherApp::is_stopped()
{
    return stop_.load();
}

void PublisherApp::stop()
{
    stop_.store(true);
    cv_.notify_one();
}

} // namespace hello_world
} // namespace examples
} // namespace fastdds
} // namespace eprosima
