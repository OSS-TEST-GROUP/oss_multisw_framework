#ifndef ADSW_MANAGER_HPP_
#define ADSW_MANAGER_HPP_

#include "service_monitor/ServiceMonitor.hpp"
#include "service_registry/ServiceRegistry.hpp"
#include "message_transmitter/MessageTransmitter.hpp"
#include "message_transmitter/DDSManager.hpp"
#include "service_controller/ServiceController.hpp"
#include "service_controller/AppController.hpp"

#include <memory>
#include <chrono>
#include <string>

#include <fastdds/dds/domain/DomainParticipant.hpp>

namespace adsw_manager::agl {

    enum class AdswState {
        ADSW_INIT,
        ADSW_STARTED,
        ADSW_RUNNING,
        ADSW_TERMINATE
    };

    class ADSWManager {

        public:
            ADSWManager(eprosima::fastdds::dds::DomainParticipant* participant,
                        std::shared_ptr<ServiceRegistry> registry,
                        std::shared_ptr<AppController> app_controller,
                        std::shared_ptr<ServiceController> service_controller,
                        std::shared_ptr<DDSManager> dds_manager,
                        std::shared_ptr<MessageTransmitter> transmitter,
                        std::shared_ptr<ServiceMonitor> monitor);

            bool Initialize(const std::string& config_path = "");
            bool StartService();
            bool StopService();
            void Process();

            void ConfirmUpdateServiceList();

        private:
            eprosima::fastdds::dds::DomainParticipant* participant_ {nullptr};

            std::shared_ptr<ServiceRegistry> service_registry_;
            std::shared_ptr<AppController> app_controller_;
            std::shared_ptr<ServiceController> service_controller_;
            std::shared_ptr<DDSManager> dds_manager_;
            std::shared_ptr<MessageTransmitter> message_transmitter_;
            std::shared_ptr<ServiceMonitor> service_monitor_;

            AdswState state_ {AdswState::ADSW_INIT};
            std::chrono::steady_clock::time_point last_publish_time_;
    };

} // namespace adsw_manager::agl

#endif
