#include "AdswServiceStatusEndPoint.hpp"

#include <utility>

#include "ros2/AdswServiceStatus.hpp"
#include "logger/Log.hpp"

namespace ros2 {
namespace adsw_manager {

ADSWServiceStatusEndPoint::ADSWServiceStatusEndPoint(MessageType msgtype, std::string topic) : topic_(std::move(topic)) {

    if (msgtype == MessageType::ADSW_SERVICE_STATUS){
        type_support_ = std::make_shared<TypeSupport>(new AdswServiceStatusPubSubType());
    }
}

ADSWServiceStatusEndPoint::~ADSWServiceStatusEndPoint() {

}

std::string ADSWServiceStatusEndPoint::GetTopic() {
    return topic_;
}

std::string ADSWServiceStatusEndPoint::GetTypeName() {
    return type_support_->get_type_name();
}

std::shared_ptr<TypeSupport> ADSWServiceStatusEndPoint::GetTypeSupport() {
    return type_support_;
}

void ADSWServiceStatusEndPoint::OnDataAvailable(){
    LOG_DEBUG("Message Delivery for " << GetTopic() );
}


} // namespace adsw_manager
} // namespace ros2
