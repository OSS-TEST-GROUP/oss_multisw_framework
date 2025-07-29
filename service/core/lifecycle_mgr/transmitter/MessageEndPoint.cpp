#include "MessageEndPoint.hpp"

namespace lifecycle_manager {

MessageEndPoint::MessageEndPoint(std::string topic) 
    : topic_(topic),
    type_support_(nullptr){
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

void MessageEndPoint::SetTypeSupport(std::shared_ptr<TypeSupport> typesupport){
    type_support_ = typesupport;
    type_name_ =  type_support_->get_type_name();
}


} // namespace lifecycle_manager
