#ifndef LIFECYCLE_NOTIFY_SERVICE_STATE_ENDPOINT_HPP_
#define LIFECYCLE_NOTIFY_SERVICE_STATE_ENDPOINT_HPP_

#include <string>
#include <set>
#include <memory>

#include "transmitter/DDSMessageEndPoint.hpp"

#include "lifecycle/NotifyServiceStatePubSubTypes.hpp"

namespace lifecycle_manager {

class NotifyServiceStateEndPoint : public DDSMessageEndPoint {
  public:
    NotifyServiceStateEndPoint(std::string topic);
    virtual ~NotifyServiceStateEndPoint();

    virtual void  OnDataAvailable(void* buffer) override;
    virtual void* GetTopicBuffer() override;

    virtual void OnDataPublished(bool ret) override;

  private:
    NotifyServiceState data_;

};

} // namespace lifecycle_manager

#endif // LIFECYCLE_NOTIFY_SERVICE_STATE_ENDPOINT_HPP_
