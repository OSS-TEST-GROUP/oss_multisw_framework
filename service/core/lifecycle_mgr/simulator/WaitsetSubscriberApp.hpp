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
 * @file WaitsetSubscriberApp.hpp
 *
 */

#ifndef FASTDDS_EXAMPLES_CPP_HELLO_WORLD__WAITSETSUBSCRIBERAPP_HPP
#define FASTDDS_EXAMPLES_CPP_HELLO_WORLD__WAITSETSUBSCRIBERAPP_HPP

#include <fastdds/dds/core/condition/GuardCondition.hpp>
#include <fastdds/dds/core/condition/WaitSet.hpp>
#include <fastdds/dds/core/status/SubscriptionMatchedStatus.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

#include "Application.hpp"
#include "CLIParser.hpp"

#include "lifecycle/LifecycleDiagReport.hpp"
#include "lifecycle/SetServiceModeRequest.hpp"
#include "lifecycle/UpdateServiceListRequest.hpp"
#include "lifecycle/GetServiceStateRequest.hpp"
#include "lifecycle/NotifyServiceState.hpp"
#include "lifecycle/StartServiceRequest.hpp"
#include "lifecycle/StopServiceRequest.hpp"

using namespace eprosima::fastdds::dds;
using namespace lifecyclemgr;

namespace eprosima {
namespace fastdds {
namespace examples {
namespace hello_world {

class WaitsetSubscriberApp : public Application
{
public:

    WaitsetSubscriberApp(
            const CLIParser::subscriber_config& config,
            const std::string& topic_name,
            const CLIParser::MessageType& message_kind);

    ~WaitsetSubscriberApp();

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

    WaitSet wait_set_;

    uint16_t samples_;

    uint16_t received_samples_;

    std::atomic<bool> stop_;

    GuardCondition terminate_condition_;
};

} // namespace hello_world
} // namespace examples
} // namespace fastdds
} // namespace eprosima

#endif // FASTDDS_EXAMPLES_CPP_HELLO_WORLD__WAITSETSUBSCRIBERAPP_HPP
