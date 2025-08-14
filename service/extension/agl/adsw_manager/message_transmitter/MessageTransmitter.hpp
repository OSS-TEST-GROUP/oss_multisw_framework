#ifndef MESSAGE_TRANSMITTER_HPP_
#define MESSAGE_TRANSMITTER_HPP_

#include "DDSManager.hpp"
#include "ServiceRegistry.hpp"
#include "ServiceController.hpp"

#include "common/AppUtils.hpp"

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
            virtual void PublishAppState(const std::string& app_name, adsw_manager_agl::AppState state);

            virtual ~MessageTransmitter() = default;

        private:
            std::shared_ptr<DDSManager> dds_manager_;
            std::shared_ptr<ServiceRegistry> service_registry_;
            std::shared_ptr<ServiceController> service_controller_;
            
            void HandleGetAppStateRequest(const adsw_manager_agl::GetAppStateRequest& req);
            void HandleStartAppRequest(const adsw_manager_agl::StartAppRequest& req);
            void HandleStopAppRequest(const adsw_manager_agl::StopAppRequest& req);
    };

} // namespace adsw_manager::agl

#endif