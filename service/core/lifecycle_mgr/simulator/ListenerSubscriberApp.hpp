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
 * @file ListenerSubscriberApp.hpp
 *
 */

#ifndef FASTDDS_EXAMPLES_CPP_HELLO_WORLD__LISTENERSUBSCRIBERAPP_HPP
#define FASTDDS_EXAMPLES_CPP_HELLO_WORLD__LISTENERSUBSCRIBERAPP_HPP

#include <condition_variable>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

#include "CLIParser.hpp"
#include "Application.hpp"

#include "lifecycle/LifecycleDiagReport.hpp"
#include "lifecycle/SetServiceModeRequest.hpp"
#include "lifecycle/UpdateServiceListRequest.hpp"
#include "lifecycle/GetServiceStateRequest.hpp"
#include "lifecycle/NotifyServiceState.hpp"
#include "lifecycle/StartServiceRequest.hpp"
#include "lifecycle/StopServiceRequest.hpp"

using namespace eprosima::fastdds::dds;

namespace eprosima {
namespace fastdds {
namespace examples {
namespace hello_world {

class ReaderTopic {
public :
        ReaderTopic(const std::string& topic, const CLIParser::MessageType& mtype);
        ~ReaderTopic();

private:
    CLIParser::MessageType mtype_;
    Topic* topic_;
    DataReader* reader_;
    std::shared_ptr<TypeSupport> type_;
};

class ListenerSubscriberApp : public Application,  public DataReaderListener
{
public:

    ListenerSubscriberApp(
            const CLIParser::subscriber_config& config,
            const std::string& topic_name,
            const CLIParser::MessageType& message_kind);

    ~ListenerSubscriberApp();

    //! Subscription callback
    void on_data_available(
            DataReader* reader) override;

    //! Subscriber matched method
    void on_subscription_matched(
            DataReader* reader,
            const SubscriptionMatchedStatus& info) override;

    //! Run subscriber
    void run() override;

    //! Trigger the end of execution
    void stop() override;

private:

    //! Return the current state of execution
    bool is_stopped();

    LifecycleDiagReport diag_report_;
    SetServiceModeRequest set_service_req_;
    SetServiceModeResponse set_service_resp_;
    UpdateServiceListRequest update_service_req_;
    UpdateServiceListResponse update_service_resp_;
    GetServiceStateRequest get_svc_state_req_;
    GetServiceStateResponse get_svc_state_resp_;
    NotifyServiceState notify_svc_state;
    StartServiceRequest start_svc_req_;
    StartServiceResponse start_svc_resp_;
    StopServiceRequest stop_svc_req_;
    StopServiceResponse stop_svc_resp_;

    CLIParser::MessageType message_kind_;

    DomainParticipant* participant_;

    Subscriber* subscriber_;

    Topic* topic_;

    DataReader* reader_;

    std::shared_ptr<TypeSupport> type_;

    uint16_t samples_;

    uint16_t received_samples_;

    std::atomic<bool> stop_;

    mutable std::mutex terminate_cv_mtx_;

    std::condition_variable terminate_cv_;
};

} // namespace hello_world
} // namespace examples
} // namespace fastdds
} // namespace eprosima

#endif // FASTDDS_EXAMPLES_CPP_HELLO_WORLD__LISTENERSUBSCRIBERAPP_HPP
