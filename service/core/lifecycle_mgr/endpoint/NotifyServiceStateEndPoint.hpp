#ifndef LIFECYCLE_NOTIFY_SERVICE_STATE_ENDPOINT_HPP_
#define LIFECYCLE_NOTIFY_SERVICE_STATE_ENDPOINT_HPP_

#include <string>
#include <set>
#include <memory>
#include <functional>

#include "transmitter/DDSMessageEndPoint.hpp"

#include "lifecycle/NotifyServiceStatePubSubTypes.hpp"

namespace lifecycle_manager {

using namespace lifecyclemgr;

class NotifyServiceStateEndPoint : public DDSMessageEndPoint {
  public:
    using Callback = std::function<void(NotifyServiceState*)>;

    NotifyServiceStateEndPoint(MessageType msgtype, std::string topic, Callback cb = nullptr);
    virtual ~NotifyServiceStateEndPoint();

    virtual void  OnDataAvailable() override;

    virtual void OnDataPublished(bool ret) override;

  private:
    NotifyServiceState data_;
    Callback on_data_cb_;

};

} // namespace lifecycle_manager

#endif // LIFECYCLE_NOTIFY_SERVICE_STATE_ENDPOINT_HPP_
