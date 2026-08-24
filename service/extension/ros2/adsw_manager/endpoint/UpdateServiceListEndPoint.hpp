#ifndef LIFECYCLE_UPDATE_SERVICE_LIST_ENDPOINT_HPP_
#define LIFECYCLE_UPDATE_SERVICE_LIST_ENDPOINT_HPP_

#include <string>
#include <set>
#include <memory>

#include "transmitter/DDSMessageEndpoint.hpp"
#include "lifecycle/UpdateServiceListRequestPubSubTypes.hpp"
#include "MessageTypes.hpp"

namespace ros2 {
namespace adsw_manager {

using namespace lifecyclemgr;

class UpdateServiceListEndPoint : public DDSMessageEndpoint {
  public:
    UpdateServiceListEndPoint(MessageType msgtype, std::string topic);
    ~UpdateServiceListEndPoint() override;

    std::string GetTopic() override;
    std::string GetTypeName() override;
    std::shared_ptr<TypeSupport> GetTypeSupport() override;

    void OnDataAvailable() override;

  private:
    std::string topic_;
    std::shared_ptr<TypeSupport> type_support_;

    UpdateServiceListRequest req_;
    UpdateServiceListResponse resp_;

};

} // namespace adsw_manager
} // namespace ros2

#endif // LIFECYCLE_UPDATE_SERVICE_LIST_ENDPOINT_HPP_
