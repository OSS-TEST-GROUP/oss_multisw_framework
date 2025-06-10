#include "LifecycleDiagReportEndPoint.hpp"

namespace lifecycle_manager {


LifecycleDiagReportEndPoint::LifecycleDiagReportEndPoint(std::string topic, std::string type) 
    : type_support_(new LifecycleDiagReportPubSubTypes())
    , DDSMessageEndPoint(topic, type, type_support_){

}

LifecycleDiagReportEndPoint::~LifecycleDiagReportEndPoint() {}



void LifecycleDiagReportEndPoint::OnDataAvailable(){



}

void* LifecycleDiagReportEndPoint::GetTopicBuffer(){
    return &data_;
}


} // namespace lifecycle_manager
