#ifndef LIFECYCLE_STOP_SERVICE_STATE_ENDPOINT_HPP_
#define LIFECYCLE_STOP_SERVICE_STATE_ENDPOINT_HPP_

#include <string>
#include <set>
#include <memory>

#include "transmitter/DDSMessageEndPoint.hpp"

#include "lifecycle/StopServiceRequestPubSubTypes.hpp"

namespace lifecycle_manager {

using namespace lifecyclemgr;

class StopServiceEndPoint : public DDSMessageEndPoint {
  public:
    StopServiceEndPoint(MessageType msgtype, std::string topic);
    virtual ~StopServiceEndPoint();

    virtual void  OnDataAvailable() override;

    virtual void OnDataPublished(bool ret) override;

  private:
    StopServiceRequest req_;
    StopServiceResponse resp_;

};

} // namespace lifecycle_manager

#endif // LIFECYCLE_STOP_SERVICE_STATE_ENDPOINT_HPP_
