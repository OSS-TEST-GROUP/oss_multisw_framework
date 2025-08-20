#include "ADSWManager.hpp"

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>

using namespace eprosima::fastdds::dds;

namespace adsw_manager::agl {

    bool ADSWManager::Initialize() {

        DomainParticipantQos participant_qos;
        participant_qos.name("adsw_manager_participant");

        participant_ = DomainParticipantFactory::get_instance()->create_participant(0, participant_qos);

        if (!participant_) {
            std::cerr << "[ADSWManager] Failed to create DomainParticipant." << std::endl;
            return false;
        }

        if (!service_registry_) {
            service_registry_ = std::make_shared<ServiceRegistry>();
        }
        if (!service_registry_->Initialize()) {
            return false;
        }

        if (!app_controller_) {
            app_controller_ = std::make_shared<AppController>();
        }
        if (!service_controller_) {
            service_controller_ = std::make_shared<ServiceController>(app_controller_.get());
        }
        if (!service_controller_->Initialize()) {
            return false;
        }

        if (!dds_manager_) {
            dds_manager_ = std::make_shared<DDSManager>();
        }
        if (!message_transmitter_) {
            message_transmitter_ = std::make_shared<MessageTransmitter>(service_registry_, service_controller_, dds_manager_);
        }
        if (!message_transmitter_->Initialize(participant_)) {
            return false;
        }

        if (!service_monitor_) {
            service_monitor_ = std::make_shared<ServiceMonitor>();
        }
        if(!service_monitor_->Initialize(participant_, service_registry_, message_transmitter_)) {
            return false;
        }
        
        return true;
    }

    bool ADSWManager::StartService() {

        service_monitor_->StartMonitoring();

        return true;

    }

    bool ADSWManager::StopService() {

        service_monitor_->StopMonitoring();

        return true;

    }

}