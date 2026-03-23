
#ifndef LIFECYCLE_UPDATE_SERVICE_LIST_MESSAGE_ENDPOINT_HPP_
#define LIFECYCLE_UPDATE_SERVICE_LIST_MESSAGE_ENDPOINT_HPP_

#include <string>
#include <set>
#include <memory>
#include <functional>

#include "transmitter/DDSMessageEndPoint.hpp"

#include "lifecycle/UpdateServiceListRequestPubSubTypes.hpp"

namespace lifecycle_manager {

using namespace lifecyclemgr;

class UpdateServiceListEndPoint : public DDSMessageEndPoint {
  public:
    using Callback = std::function<void(UpdateServiceListRequest*)>;

    UpdateServiceListEndPoint(MessageType msgtype, std::string topic, Callback cb = nullptr);
    virtual ~UpdateServiceListEndPoint();

    virtual void  OnDataAvailable() override;
    
    virtual void OnDataPublished(bool ret) override;


  private:
    UpdateServiceListRequest data_;
    Callback on_data_cb_;

};

} // namespace lifecycle_manager

#endif // LIFECYCLE_UPDATE_SERVICE_LIST_MESSAGE_ENDPOINT_HPP_
