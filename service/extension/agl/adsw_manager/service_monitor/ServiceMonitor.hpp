#ifndef SERVICE_MONITOR_HPP_
#define SERVICE_MONITOR_HPP_

#include "service_registry/ServiceRegistry.hpp"
#include "message_transmitter/MessageTransmitter.hpp"
#include "AppStateReader.hpp"

#include <memory>
#include <chrono>
#include <unordered_map>
#include <thread>

#include <fastdds/dds/domain/DomainParticipant.hpp>

namespace adsw_manager::agl {

    class ServiceMonitor {

        public:
            virtual bool Initialize(eprosima::fastdds::dds::DomainParticipant* participant, 
                            std::shared_ptr<ServiceRegistry> registry, 
                            std::shared_ptr<MessageTransmitter> transmitter);
            virtual void StartMonitoring();
            virtual void StopMonitoring();
            void CheckAppState();

            virtual ~ServiceMonitor() = default;

        protected:
            std::unordered_map<std::string, std::unique_ptr<AppStateReader>> readers_;

        private:
            std::shared_ptr<ServiceRegistry> service_registry_{nullptr};
            std::shared_ptr<MessageTransmitter> message_transmitter_{nullptr};

            eprosima::fastdds::dds::DomainParticipant* participant_{nullptr};

            std::chrono::seconds timeout_{std::chrono::seconds(5)}; 

            std::thread monitoring_thread_;
            std::atomic<bool> running_{false};

    };

} // namespace adsw_manager::agl

#endif