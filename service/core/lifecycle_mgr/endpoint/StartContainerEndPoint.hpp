#ifndef LIFECYCLE_START_CONTAINER_ENDPOINT_HPP_
#define LIFECYCLE_START_CONTAINER_ENDPOINT_HPP_

#include <functional>
#include <memory>
#include <string>

#include "transmitter/DDSMessageEndPoint.hpp"

#include "lifecycle/StartContainerRequestPubSubTypes.hpp"

namespace lifecycle_manager {

using namespace lifecyclemgr;

// F013: paired endpoint for StartContainerRequest / StartContainerResponse,
// matching the StartServiceEndPoint convention (single class handles both
// directions; the factory decides PUBLISH_ONLY vs SUBSCRIBE_ONLY).
class StartContainerEndPoint : public DDSMessageEndPoint {
  public:
    using Callback = std::function<void(StartContainerResponse*)>;

    StartContainerEndPoint(MessageType msgtype, std::string topic,
                            Callback cb = nullptr);
    virtual ~StartContainerEndPoint();

    virtual void OnDataAvailable() override;
    virtual void OnDataPublished(bool ret) override;

  private:
    StartContainerRequest  req_;
    StartContainerResponse resp_;
    Callback               on_data_cb_;
};

} // namespace lifecycle_manager

#endif // LIFECYCLE_START_CONTAINER_ENDPOINT_HPP_
