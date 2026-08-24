#ifndef LIFECYCLE_NOTIFY_SERVICE_STATE_ENDPOINT_HPP_
#define LIFECYCLE_NOTIFY_SERVICE_STATE_ENDPOINT_HPP_

#include <string>
#include <set>
#include <memory>

#include "transmitter/DDSMessageEndpoint.hpp"
#include "lifecycle/NotifyServiceStatePubSubTypes.hpp"
#include "MessageTypes.hpp"

namespace ros2 {
namespace adsw_manager {

using namespace lifecyclemgr;

class NotifyServiceStateEndPoint : public DDSMessageEndpoint {
  public:
    NotifyServiceStateEndPoint(MessageType msgtype, std::string topic);
    ~NotifyServiceStateEndPoint() override;

    std::string GetTopic() override;
    std::string GetTypeName() override;
    std::shared_ptr<TypeSupport> GetTypeSupport() override;

    void OnDataAvailable() override;

  private:
    std::string topic_;
    std::shared_ptr<TypeSupport> type_support_;    

};


} // namespace adsw_manager
} // namespace ros2

#endif // LIFECYCLE_NOTIFY_SERVICE_STATE_ENDPOINT_HPP_
