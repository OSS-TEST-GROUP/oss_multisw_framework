#include "MessageEndPoint.hpp"

namespace lifecycle_manager {

MessageEndPoint::MessageEndPoint(std::string topic, std::string type, std::shared_ptr<TypeSupport> type_support) 
    : topic_(topic),
    type_name_(type),
    type_support_(type_support){
}

MessageEndPoint::~MessageEndPoint() {}


std::string MessageEndPoint::GetTopic() { 
    return topic_; 
}

std::string MessageEndPoint::GetTypeName() {
    return type_name_; 
}

std::shared_ptr<TypeSupport> MessageEndPoint::GetTypeSupport() { 
    return type_support_; 
}


} // namespace lifecycle_manager
