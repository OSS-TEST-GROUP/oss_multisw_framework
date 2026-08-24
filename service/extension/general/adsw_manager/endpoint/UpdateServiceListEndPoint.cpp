#include "UpdateServiceListEndPoint.hpp"

#include <utility>

#include "lifecycle/UpdateServiceListRequest.hpp"
#include "ADSWManager.hpp"
#include "logger/Log.hpp"

namespace general {
namespace adsw_manager {

UpdateServiceListEndPoint::UpdateServiceListEndPoint(MessageType msgtype, std::string topic) : topic_(std::move(topic)) {

    if (msgtype == MessageType::UPDATE_SERVICE_LIST_REQUEST){
        type_support_ = std::make_shared<TypeSupport>(new UpdateServiceListRequestPubSubType());
    }else if (msgtype == MessageType::UPDATE_SERVICE_LIST_RESPONSE){
       type_support_ = std::make_shared<TypeSupport>(new UpdateServiceListResponsePubSubType()); 
    }
}

UpdateServiceListEndPoint::~UpdateServiceListEndPoint() {

}

std::string UpdateServiceListEndPoint::GetTopic() {
    return topic_;
}

std::string UpdateServiceListEndPoint::GetTypeName() {
    return type_support_->get_type_name();
}

std::shared_ptr<TypeSupport> UpdateServiceListEndPoint::GetTypeSupport() {
    return type_support_;
}

void UpdateServiceListEndPoint::OnDataAvailable(){
    LOG_DEBUG("Message Delivery for " << GetTopic() );

    if (Read<UpdateServiceListResponse>(resp_)) {

        UpdateServiceListResponse* message = reinterpret_cast<UpdateServiceListResponse*>(&resp_);

        LOG_DEBUG("UpdateServiceListResponse received, ret_code="
                  << static_cast<int32_t>(message->ret_code()));

        // Trust anchor for the phantom guard. UpdateServiceListResponse is
        // published only by lifecycle_mgr, so the publisher prefix observed
        // here is the prefix we'll require on every subsequent
        // Start/StopServiceRequest. Pinning happens once (first sender wins).
        ADSWManager::Get()->SetKnownLifecyclePublisher(LastPublicationHandle());

        ADSWManager::Get()->ConfirmUpdateServiceList();
    }
}


} // namespace adsw_manager
} // namespace general
