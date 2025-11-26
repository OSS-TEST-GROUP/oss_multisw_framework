#include "DDSManager.hpp"

using namespace eprosima::fastdds::dds;
using namespace lifecyclemgr;

namespace adsw_manager::agl {

    DDSManager::~DDSManager() {

        // delete subscriber

        if (participant_ && subscriber_) {
            participant_->delete_subscriber(subscriber_);
        }

        // delete publisher

        if (participant_ && publisher_) {
            participant_->delete_publisher(publisher_);
        }
    }

    bool DDSManager::Initialize(DomainParticipant* participant) {

        participant_ = participant;

        publisher_ = participant_->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr);
        subscriber_ = participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr);

        if (!app_state_handler_) {
            app_state_handler_ = std::make_unique<AppStateHandler>();
        }
        app_state_handler_->Initialize(participant_, publisher_, subscriber_,
            [this](const GetServiceStateRequest& req) {
                if (on_get_app_state_request_) {
                    on_get_app_state_request_(req);
                }
            }
        );

        if (!start_app_handler_) {
            start_app_handler_ = std::make_unique<StartAppHandler>();
        }
        start_app_handler_->Initialize(participant_, publisher_, subscriber_,
            [this](const StartServiceRequest& req) {
                if (on_start_app_request_) {
                    on_start_app_request_(req);
                }
            }
        );

        if (!stop_app_handler_) {
            stop_app_handler_ = std::make_unique<StopAppHandler>();
        }
        stop_app_handler_->Initialize(participant_, publisher_, subscriber_,
            [this](const StopServiceRequest& req) {
                if (on_stop_app_request_) {
                    on_stop_app_request_(req);
                }
            }
        );

        if (!notify_app_state_writer_) {
            notify_app_state_writer_ = std::make_unique<NotifyAppStateWriter>();
        }
        if (!notify_app_state_writer_->Initialize(publisher_, participant_)) {
            std::cerr << "[DDSManager] Failed to initialize NotifyAppStateWriter.\n";
            return false;
        }

        if (!update_app_list_handler_) {
            update_app_list_handler_ = std::make_unique<UpdateAppListHandler>();
        }
        update_app_list_handler_->Initialize(participant_, publisher_, subscriber_,
            [this](const UpdateServiceListResponse& res) {
                if (on_update_list_response_) {
                    on_update_list_response_(res);
                }
            }
        );

        return true;

    }

    void DDSManager::PublishNotifyAppState(const NotifyServiceState& msg) {
        if (notify_app_state_writer_) {
            notify_app_state_writer_->Publish(msg);
        }
    }

    void DDSManager::PublishUpdateAppList(const UpdateServiceListRequest& msg) {
        if (update_app_list_handler_) {
            update_app_list_handler_->Publish(msg);
        }
    }

    void DDSManager::SetOnGetAppStateRequest(std::function<void(const GetServiceStateRequest&)> cb) {

        on_get_app_state_request_ = cb;

    }

    void DDSManager::SetOnStartAppRequest(std::function<void(const StartServiceRequest&)> cb) {

        on_start_app_request_ = cb;

    }

    void DDSManager::SetOnStopAppRequest(std::function<void(const StopServiceRequest&)> cb) {

        on_stop_app_request_ = cb;

    }

    void DDSManager::SetOnUpdateListResponse(std::function<void(const UpdateServiceListResponse&)> cb) {

        on_update_list_response_ = cb;

    }

}