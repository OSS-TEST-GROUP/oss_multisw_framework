#ifndef LIFECYCLE_STOP_SERVICE_STATE_ENDPOINT_HPP_
#define LIFECYCLE_STOP_SERVICE_STATE_ENDPOINT_HPP_

#include <string>
#include <set>
#include <memory>
#include <functional>

#include "transmitter/DDSMessageEndPoint.hpp"

#include "lifecycle/StopServiceRequestPubSubTypes.hpp"

namespace lifecycle_manager {

using namespace lifecyclemgr;

class StopServiceEndPoint : public DDSMessageEndPoint {
  public:
    using Callback = std::function<void(StopServiceResponse*)>;

    StopServiceEndPoint(MessageType msgtype, std::string topic, Callback cb = nullptr);
    ~StopServiceEndPoint() override;

    void OnDataAvailable() override;

    void OnDataPublished(bool ret) override;

  private:
    StopServiceRequest req_;
    StopServiceResponse resp_;
    Callback on_data_cb_;

};

} // namespace lifecycle_manager

#endif // LIFECYCLE_STOP_SERVICE_STATE_ENDPOINT_HPP_
