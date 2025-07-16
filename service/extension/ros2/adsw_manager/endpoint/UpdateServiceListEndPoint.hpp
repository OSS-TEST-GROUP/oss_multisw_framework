#ifndef LIFECYCLE_UPDATE_SERVICE_LIST_ENDPOINT_HPP_
#define LIFECYCLE_UPDATE_SERVICE_LIST_ENDPOINT_HPP_

#include <string>
#include <set>
#include <memory>

#include "transmitter/DDSMessageEndpoint.hpp"
#include "lifecycle/UpdateServiceListRequestPubSubTypes.hpp"

namespace ros2 {
namespace adsw_manager {

using namespace lifecyclemgr;

class UpdateServiceListEndPoint : public DDSMessageEndpoint {
  public:
    UpdateServiceListEndPoint(std::string topic);
    virtual ~UpdateServiceListEndPoint();

    virtual std::string GetTopic();
    virtual std::string GetTypeName();
    virtual std::shared_ptr<TypeSupport> GetTypeSupport();

    virtual void OnDataAvailable();

  private:
    std::string topic_;
    std::shared_ptr<TypeSupport> type_support_;

    UpdateServiceListRequest req_;
    UpdateServiceListResponse resp_;

};

} // namespace adsw_manager
} // namespace ros2

#endif // LIFECYCLE_UPDATE_SERVICE_LIST_ENDPOINT_HPP_
