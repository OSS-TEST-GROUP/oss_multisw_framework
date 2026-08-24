
#ifndef LIFECYCLE_SET_SERVICE_MODE_MESSAGE_ENDPOINT_HPP_
#define LIFECYCLE_SET_SERVICE_MODE_MESSAGE_ENDPOINT_HPP_

#include <string>
#include <set>
#include <memory>
#include <functional>

#include "transmitter/DDSMessageEndPoint.hpp"

#include "lifecycle/SetServiceModeRequestPubSubTypes.hpp"

namespace lifecycle_manager {

using namespace lifecyclemgr;

class SetServiceModeEndPoint : public DDSMessageEndPoint {
  public:
    using Callback = std::function<void(SetServiceModeRequest*)>;

    SetServiceModeEndPoint(MessageType msgtype, std::string topic, Callback cb = nullptr);
    ~SetServiceModeEndPoint() override;

    void OnDataAvailable() override;

    void OnDataPublished(bool ret) override;

  private:
    SetServiceModeRequest data_;
    Callback on_data_cb_;

};

} // namespace lifecycle_manager

#endif // LIFECYCLE_SET_SERVICE_MODE_MESSAGE_ENDPOINT_HPP_
