#ifndef LIFECYCLE_STOP_CONTAINER_ENDPOINT_HPP_
#define LIFECYCLE_STOP_CONTAINER_ENDPOINT_HPP_

#include <functional>
#include <memory>
#include <string>

#include "transmitter/DDSMessageEndPoint.hpp"

#include "lifecycle/StopContainerRequestPubSubTypes.hpp"

namespace lifecycle_manager {

using namespace lifecyclemgr;

// F013: paired endpoint for StopContainerRequest / StopContainerResponse,
// matching the StopServiceEndPoint convention.
class StopContainerEndPoint : public DDSMessageEndPoint {
  public:
    using Callback = std::function<void(StopContainerResponse*)>;

    StopContainerEndPoint(MessageType msgtype, std::string topic,
                           Callback cb = nullptr);
    ~StopContainerEndPoint() override= default;
    void OnDataAvailable() override;
    void OnDataPublished(bool ret) override;

  private:
    StopContainerRequest  req_;
    StopContainerResponse resp_;
    Callback              on_data_cb_;
};

} // namespace lifecycle_manager

#endif // LIFECYCLE_STOP_CONTAINER_ENDPOINT_HPP_
