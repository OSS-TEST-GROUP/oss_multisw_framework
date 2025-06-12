
#ifndef LIFECYCLE_ENDPOINT_FACTORY_HPP_
#define LIFECYCLE_ENDPOINT_FACTORY_HPP_

#include <string>
#include <set>
#include <memory>

#include "transmitter/DDSTransmitter.hpp"

namespace lifecycle_manager {

class EndPointFactory  {
  public:
    EndPointFactory();
    virtual ~EndPointFactory();

    int32_t CreateEndPoints(std::shared_ptr<DDSTransmitter> transmitter);

  private:

};

} // namespace lifecycle_manager

#endif // LIFECYCLE_ENDPOINT_FACTORY_HPP_
