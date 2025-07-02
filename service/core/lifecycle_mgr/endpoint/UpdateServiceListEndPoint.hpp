
#ifndef LIFECYCLE_UPDATE_SERVICE_LIST_MESSAGE_ENDPOINT_HPP_
#define LIFECYCLE_UPDATE_SERVICE_LIST_MESSAGE_ENDPOINT_HPP_

#include <string>
#include <set>
#include <memory>

#include "transmitter/DDSMessageEndPoint.hpp"

#include "lifecycle/UpdateServiceListRequestPubSubTypes.hpp"

namespace lifecycle_manager {

class UpdateServiceListEndPoint : public DDSMessageEndPoint {
  public:
    UpdateServiceListEndPoint(std::string topic);
    virtual ~UpdateServiceListEndPoint();

    virtual void  OnDataAvailable() override;
    
    virtual void OnDataPublished(bool ret) override;


  private:
    UpdateServiceListRequest data_;

};

} // namespace lifecycle_manager

#endif // LIFECYCLE_UPDATE_SERVICE_LIST_MESSAGE_ENDPOINT_HPP_
