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

        app_state_handler_ = std::make_unique<AppStateHandler>();
        app_state_handler_->Initialize(participant_, publisher_, subscriber_,
            [this](const GetServiceStateRequest& req) {
                if (on_get_app_state_request_) {
                    on_get_app_state_request_(req);
                }
            }
        );

        start_app_handler_ = std::make_unique<StartAppHandler>();
        start_app_handler_->Initialize(participant_, publisher_, subscriber_,
            [this](const StartServiceRequest& req) {
                if (on_start_app_request_) {
                    on_start_app_request_(req);
                }
            }
        );

        stop_app_handler_ = std::make_unique<StopAppHandler>();
        stop_app_handler_->Initialize(participant_, publisher_, subscriber_,
            [this](const StopServiceRequest& req) {
                if (on_stop_app_request_) {
                    on_stop_app_request_(req);
                }
            }
        );

        std::cout << "[DDSManager] Creating NotifyAppStateWriter...\n";
        if (!notify_app_state_writer_) {
            notify_app_state_writer_ = std::make_unique<NotifyAppStateWriter>();
        }
        std::cout << "[DDSManager] NotifyAppStateWriter created.\n";

        if (!notify_app_state_writer_->Initialize(publisher_, participant_)) {
            std::cerr << "[DDSManager] Failed to initialize NotifyAppStateWriter.\n";
            return false;
        }

        std::cout << "[DDSManager] NotifyAppStateWriter initialized.\n";

        return true;

    }

    void DDSManager::PublishNotifyAppState(const NotifyServiceState& msg) {
        if (notify_app_state_writer_) {
            notify_app_state_writer_->Publish(msg);
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

}