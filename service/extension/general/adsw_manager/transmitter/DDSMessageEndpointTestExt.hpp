#ifndef GENERAL_ADSW_MANAGER_TRANSMITTER_DDS_MESSAGE_ENDPOINT_TEST_EXT_HPP_
#define GENERAL_ADSW_MANAGER_TRANSMITTER_DDS_MESSAGE_ENDPOINT_TEST_EXT_HPP_

#ifdef UNITTEST_MOCK
#include "transmitter/DDSMessageEndpoint.hpp"
#include "lifecycle/GetServiceStateRequest.hpp"
#include "lifecycle/StartServiceRequest.hpp"
#include "lifecycle/StopServiceRequest.hpp"
#include "lifecycle/NotifyServiceState.hpp"
#include "lifecycle/UpdateServiceListRequest.hpp"

namespace general {
namespace adsw_manager {

class DDSMessageEndpointTestExt : public DDSMessageEndpoint {
public:
    // Virtual Read methods for each request type (use correct namespace)
    virtual bool Read(lifecyclemgr::GetServiceStateRequest& req) { return false; }
    virtual bool Read(lifecyclemgr::StartServiceRequest& req) { return false; }
    virtual bool Read(lifecyclemgr::StopServiceRequest& req) { return false; }
    virtual bool Read(lifecyclemgr::NotifyServiceState& req) { return false; }
    virtual bool Read(lifecyclemgr::UpdateServiceListRequest& req) { return false; }
};

} // namespace adsw_manager
} // namespace general
#endif // UNITTEST_MOCK

#endif // GENERAL_ADSW_MANAGER_TRANSMITTER_DDS_MESSAGE_ENDPOINT_TEST_EXT_HPP_
