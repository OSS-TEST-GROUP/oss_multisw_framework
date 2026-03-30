#include "ADSWManager.hpp"
#include "logger/Log.hpp"

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>

using namespace eprosima::fastdds::dds;

namespace adsw_manager::agl {

    ADSWManager::ADSWManager(DomainParticipant* participant,
        std::shared_ptr<ServiceRegistry> registry,
        std::shared_ptr<AppController> app_controller,
        std::shared_ptr<ServiceController> service_controller,
        std::shared_ptr<DDSManager> dds_manager,
        std::shared_ptr<MessageTransmitter> transmitter,
        std::shared_ptr<ServiceMonitor> monitor
    )
        : participant_(participant),
          service_registry_(registry),
          app_controller_(app_controller),
          service_controller_(service_controller),
          dds_manager_(dds_manager),
          message_transmitter_(transmitter),
          service_monitor_(monitor) {}

    bool ADSWManager::Initialize(const std::string& config_path) {

        state_ = AdswState::ADSW_INIT;

        if (!participant_) {
            LOG_ERROR("[ADSWManager] DomainParticipant is null");
            return false;
        }

        if (!service_registry_ || !app_controller_ || !service_controller_ ||
            !dds_manager_ || !message_transmitter_ || !service_monitor_) {
            LOG_ERROR("[ADSWManager] dependencies are not set");
            return false;
        }

        if (!service_registry_->Initialize(config_path)) {
            return false;
        }

        app_controller_->SetServiceRegistry(service_registry_.get());

        if (!service_controller_->Initialize()) {
            return false;
        }

        if (!message_transmitter_->Initialize(participant_)) {
            return false;
        }

        message_transmitter_->SetOnUpdateListConfirm([this]() {
            ConfirmUpdateServiceList();
        });

        if (!service_monitor_->Initialize(participant_, service_registry_, message_transmitter_, app_controller_.get())) {
            return false;
        }

        LOG_INFO("[ADSWManager] Initialized. State -> ADSW_INIT");
        return true;
    }

    bool ADSWManager::StartService() {

        message_transmitter_->PublishAppList();
        service_monitor_->StartMonitoring();

        state_ = AdswState::ADSW_STARTED;
        last_publish_time_ = std::chrono::steady_clock::now();

        LOG_INFO("[ADSWManager] Service started. State -> ADSW_STARTED");
        return true;
    }

    bool ADSWManager::StopService() {

        service_monitor_->StopMonitoring();
        state_ = AdswState::ADSW_TERMINATE;

        LOG_INFO("[ADSWManager] Service stopped. State -> ADSW_TERMINATE");
        return true;
    }

    void ADSWManager::ConfirmUpdateServiceList() {
        if (state_ == AdswState::ADSW_STARTED) {
            state_ = AdswState::ADSW_RUNNING;
            last_publish_time_ = std::chrono::steady_clock::now();
            LOG_INFO("[ADSWManager] UpdateServiceList confirmed. State -> ADSW_RUNNING");
        }
    }

    void ADSWManager::Process() {
        using namespace std::chrono;

        auto now = steady_clock::now();

        if (state_ == AdswState::ADSW_STARTED) {
            if (now - last_publish_time_ >= seconds(3)) {
                LOG_DEBUG("[ADSWManager] Retrying PublishAppList (ADSW_STARTED)");
                message_transmitter_->PublishAppList();
                last_publish_time_ = now;
            }
        } else if (state_ == AdswState::ADSW_RUNNING) {
            if (now - last_publish_time_ >= seconds(10)) {
                LOG_DEBUG("[ADSWManager] Resyncing PublishAppList (ADSW_RUNNING)");
                message_transmitter_->PublishAppList();
                last_publish_time_ = now;
            }
        }
    }

} // namespace adsw_manager::agl
