
#ifndef LIFECYCLE_DDS_TRANSMITTER_HPP_
#define LIFECYCLE_DDS_TRANSMITTER_HPP_

#include <string>
#include <map>
#include <memory>

#include "DDSMessageEndPoint.hpp"

namespace lifecycle_manager {

class DDSTransmitter {
  public:
    DDSTransmitter();
    virtual ~DDSTransmitter();

    bool Initialize();
    void Finalize();

    bool AddMessageEndpoint(std::shared_ptr<DDSMessageEndPoint>);
    std::shared_ptr<DDSMessageEndPoint> GetMessageEndPoint(std::string topic);

  private:
    
    std::map<std::string, std::shared_ptr<DDSMessageEndPoint>> endpoints_;

};

} // namespace lifecycle_manager

#endif // LIFECYCLE_DDS_TRANSMITTER_HPP_
