#ifndef LIFECYCLE_GET_SERVICE_STATE_ENDPOINT_HPP_
#define LIFECYCLE_GET_SERVICE_STATE_ENDPOINT_HPP_

#include <string>
#include <set>
#include <memory>

#include "transmitter/DDSMessageEndPoint.hpp"

#include "lifecycle/GetServiceStateRequestPubSubTypes.hpp"

namespace lifecycle_manager {

class GetServiceStateEndPoint : public DDSMessageEndPoint {
  public:
    GetServiceStateEndPoint(std::string topic);
    virtual ~GetServiceStateEndPoint();

    virtual void  OnDataAvailable() override;

    virtual void OnDataPublished(bool ret) override;

  private:
    GetServiceStateRequest req_;
    GetServiceStateResponse resp_;

};

} // namespace lifecycle_manager

#endif // LIFECYCLE_GET_SERVICE_STATE_ENDPOINT_HPP_
