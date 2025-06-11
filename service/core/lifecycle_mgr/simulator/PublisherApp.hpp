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
 * @file PublisherApp.hpp
 *
 */

#ifndef FASTDDS_EXAMPLES_CPP_HELLO_WORLD__PUBLISHERAPP_HPP
#define FASTDDS_EXAMPLES_CPP_HELLO_WORLD__PUBLISHERAPP_HPP

#include <condition_variable>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

#include "Application.hpp"
#include "CLIParser.hpp"
#include "lifecycle/LifecycleDiagReport.hpp"
#include "lifecycle/SetServiceModeRequest.hpp"
#include "lifecycle/UpdateServiceListRequest.hpp"

#include "lifecycle/LifecycleDiagReportPubSubTypes.hpp"
#include "lifecycle/SetServiceModeRequestPubSubTypes.hpp"
#include "lifecycle/UpdateServiceListRequestPubSubTypes.hpp"

using namespace eprosima::fastdds::dds;

namespace eprosima {
namespace fastdds {
namespace examples {
namespace hello_world {

class PublisherApp : public Application, public DataWriterListener
{
public:

    PublisherApp(
            const CLIParser::publisher_config& config,
            const std::string& topic_name,
            const CLIParser::MessageKind& message_kind);

    ~PublisherApp();

    //! Publisher matched method
    void on_publication_matched(
            DataWriter* writer,
            const PublicationMatchedStatus& info) override;

    //! Run publisher
    void run() override;

    //! Stop publisher
    void stop() override;

private:

    //! Return the current state of execution
    bool is_stopped();

    //! Publish a sample
    bool publish();

    LifecycleDiagReport diag_report_;
    SetServiceModeRequest set_service_mode_;
    UpdateServiceListRequest update_service_list_;

    CLIParser::MessageKind message_kind_;

    DomainParticipant* participant_;

    Publisher* publisher_;

    Topic* topic_;

    DataWriter* writer_;

    std::shared_ptr<TypeSupport> type_;

    int16_t matched_;

    uint16_t samples_;

    uint16_t index_;

    uint16_t expected_matches_;

    std::mutex mutex_;

    std::condition_variable cv_;

    std::atomic<bool> stop_;

    const uint32_t period_ms_ = 100; // in ms

    std::shared_ptr<LifecycleDiagReportPubSubType> diag_report_type_;
    std::shared_ptr<SetServiceModeRequestPubSubType> set_service_mode_type_;
    std::shared_ptr<UpdateServiceListRequestPubSubType> update_service_list_type_;

};

} // namespace hello_world
} // namespace examples
} // namespace fastdds
} // namespace eprosima

#endif // FASTDDS_EXAMPLES_CPP_HELLO_WORLD__PUBLISHERAPP_HPP
