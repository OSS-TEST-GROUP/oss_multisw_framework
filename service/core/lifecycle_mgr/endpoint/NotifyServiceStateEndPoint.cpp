#include "NotifyServiceStateEndPoint.hpp"

#include <utility>

#include "lifecycle/NotifyServiceState.hpp"
#include "logger/Log.hpp"
#include "manager/LifecycleFactory.hpp"

namespace lifecycle_manager {

NotifyServiceStateEndPoint::NotifyServiceStateEndPoint(MessageType msgtype, std::string topic, Callback cb)
    : DDSMessageEndPoint(std::move(topic)), on_data_cb_(std::move(cb)) {

    if (msgtype == MessageType::NOTIFY_SERVICE_STATE){
        SetTypeSupport(std::make_shared<TypeSupport>(new NotifyServiceStatePubSubType()));
    }
}

NotifyServiceStateEndPoint::~NotifyServiceStateEndPoint() {

}



void NotifyServiceStateEndPoint::OnDataAvailable(){
    LOG_DEBUG("Message Delivery for " << GetTopic() );

    if (Read<NotifyServiceState>(data_)) {

        // Phantom-publisher guard. A stray publisher writing NOT_RUNNING to
        // this topic flips lifecycle_mgr's service state to NotRunning, which
        // then trips PolicyAlways/OnFailure into emitting a START — kicking
        // off an endless Stop/Start oscillation visible to the operator.
        // Drop anything from a participant other than the pinned adsw_manager.
        if (!LifecycleFactory::GetInstance()
                ->IsKnownAdswPublisher(LastPublicationHandle())) {
            LOG_DEBUG("NotifyServiceStateEndPoint: dropping NotifyServiceState "
                      "from untrusted publisher (phantom guard)");
            return;
        }

        NotifyServiceState* message = reinterpret_cast<NotifyServiceState*>(&data_);

        if (on_data_cb_) {
            on_data_cb_(message);
        }
    }
}

void NotifyServiceStateEndPoint::OnDataPublished(bool ret){

    LOG_DEBUG("Message Published for " << GetTopic() << ", " << ((ret == true) ? "Success" : "Fail"));
}

} // namespace lifecycle_manager
