#ifndef ADSW_MANAGER_HPP_
#define ADSW_MANAGER_HPP_

#include "service_monitor/ServiceMonitor.hpp"
#include "service_registry/ServiceRegistry.hpp"
#include "message_transmitter/MessageTransmitter.hpp"
#include "message_transmitter/DDSManager.hpp"
#include "service_controller/ServiceController.hpp"
#include "service_controller/AppController.hpp"
#include "service_controller/ContainerController.hpp"

#include <memory>

#include <fastdds/dds/domain/DomainParticipant.hpp>

namespace adsw_manager::agl {

    class ADSWManager {

        public:
        
            bool Initialize();
            bool StartService();
            bool StopService();

#ifdef UNIT_TEST
        void InjectMocks(std::shared_ptr<ServiceRegistry> reg,
                         std::shared_ptr<ServiceController> ctrl,
                         std::shared_ptr<MessageTransmitter> tx,
                         std::shared_ptr<ServiceMonitor> mon,
                         std::shared_ptr<AppController> ac,
                         std::shared_ptr<ContainerController> cc,
                         eprosima::fastdds::dds::DomainParticipant* part)
        {
            service_registry_ = std::move(reg);
            service_controller_ = std::move(ctrl);
            message_transmitter_ = std::move(tx);
            service_monitor_ = std::move(mon);
            app_controller_ = std::move(ac);
            container_controller_ = std::move(cc);
            participant_ = part;
        }
#endif

        private:
            eprosima::fastdds::dds::DomainParticipant* participant_;
            std::shared_ptr<ServiceMonitor> service_monitor_;
            std::shared_ptr<ServiceRegistry> service_registry_;
            std::shared_ptr<MessageTransmitter> message_transmitter_;
            std::shared_ptr<DDSManager> dds_manager_;
            std::shared_ptr<ServiceController> service_controller_;
            std::shared_ptr<AppController> app_controller_;
            std::shared_ptr<ContainerController> container_controller_;

    };

} // namespace adsw_manager::agl

#endif