#ifndef MESSAGE_TRANSMITTER_HPP_
#define MESSAGE_TRANSMITTER_HPP_

#include "DDSManager.hpp"
#include "ServiceRegistry.hpp"
#include "ServiceController.hpp"

#include "common/AppUtils.hpp"
#include "lifecycle/LifecycleDiagReport.hpp"

#include <memory>

namespace adsw_manager::agl {

    class MessageTransmitter {

        public:
            MessageTransmitter(std::shared_ptr<ServiceRegistry> registry,
                               std::shared_ptr<ServiceController> controller,
                               std::shared_ptr<DDSManager> dds_manager)
                : service_registry_(std::move(registry)),
                  service_controller_(std::move(controller)),
                  dds_manager_(std::move(dds_manager)) {}

            virtual bool Initialize(eprosima::fastdds::dds::DomainParticipant* participant);
            virtual void PublishAppState();

            virtual ~MessageTransmitter() = default;

        private:
            std::shared_ptr<DDSManager> dds_manager_;
            std::shared_ptr<ServiceRegistry> service_registry_;
            std::shared_ptr<ServiceController> service_controller_;
            
            void HandleGetAppStateRequest(const lifecyclemgr::GetServiceStateRequest& req);
            void HandleStartAppRequest(const lifecyclemgr::StartServiceRequest& req);
            void HandleStopAppRequest(const lifecyclemgr::StopServiceRequest& req);
    };

} // namespace adsw_manager::agl

#endif