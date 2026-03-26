#ifndef LIFECYCLE_GET_SERVICE_STATE_ENDPOINT_HPP_
#define LIFECYCLE_GET_SERVICE_STATE_ENDPOINT_HPP_

#include <string>
#include <set>
#include <memory>
#include <functional>

#include "transmitter/DDSMessageEndPoint.hpp"

#include "lifecycle/GetServiceStateRequestPubSubTypes.hpp"

namespace lifecycle_manager {

using namespace lifecyclemgr;

class GetServiceStateEndPoint : public DDSMessageEndPoint {
  public:
    using Callback = std::function<void(GetServiceStateResponse*)>;

    GetServiceStateEndPoint(MessageType msgtype, std::string topic, Callback cb = nullptr);
    virtual ~GetServiceStateEndPoint();

    virtual void  OnDataAvailable() override;

    virtual void OnDataPublished(bool ret) override;

  private:
    GetServiceStateRequest req_;
    GetServiceStateResponse resp_;
    Callback on_data_cb_;

};

} // namespace lifecycle_manager

#endif // LIFECYCLE_GET_SERVICE_STATE_ENDPOINT_HPP_
