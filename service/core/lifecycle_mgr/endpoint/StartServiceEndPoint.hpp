#ifndef LIFECYCLE_START_SERVICE_ENDPOINT_HPP_
#define LIFECYCLE_START_SERVICE_ENDPOINT_HPP_

#include <string>
#include <set>
#include <memory>
#include <functional>

#include "transmitter/DDSMessageEndPoint.hpp"

#include "lifecycle/StartServiceRequestPubSubTypes.hpp"

namespace lifecycle_manager {

using namespace lifecyclemgr;

class StartServiceEndPoint : public DDSMessageEndPoint {
  public:
    using Callback = std::function<void(StartServiceResponse*)>;

    StartServiceEndPoint(MessageType msgtype, std::string topic, Callback cb = nullptr);
    virtual ~StartServiceEndPoint();

    virtual void  OnDataAvailable() override;

    virtual void OnDataPublished(bool ret) override;

  private:
    StartServiceRequest req_;
    StartServiceResponse resp_;
    Callback on_data_cb_;

};

} // namespace lifecycle_manager

#endif // LIFECYCLE_START_SERVICE_ENDPOINT_HPP_
