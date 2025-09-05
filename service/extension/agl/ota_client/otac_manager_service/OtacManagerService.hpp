#ifndef OTAC_MANAGER_SERVICE_HPP_
#define OTAC_MANAGER_SERVICE_HPP_

#include <vector>
#include <iostream>
#include <memory>

#include "common/IBaseService.hpp"
#include "common/DdsAbstraction.hpp"
#include "OtacStatusStore.hpp"
#include "OtamClientServiceMsgPubSubTypes.hpp"
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>

namespace ota_client::agl {

    class OtacManagerService : public IBaseService {

        public:
            OtacManagerService(std::shared_ptr<OtacStatusStore> status_store);
            bool Initialize() override;
            bool Start() override;
            void Stop() override;
            bool PublishClientRegistration(std::string client_name, std::vector<std::string> app_list);
            void OnClientRegisterationResponse(const OtaManager::SetOtaClientRegisterationResponse& msg);
            void OnManagerReady(const OtaManager::NotifyOtaManagerReady& msg);
            void OnAppUpdateStatus(const OtaManager::NotifyAppUpdateStatus& msg);

        private:
            eprosima::fastdds::dds::DomainParticipant* participant_;
            eprosima::fastdds::dds::Publisher* publisher_;
            eprosima::fastdds::dds::Subscriber* subscriber_;

            std::unique_ptr<DdsAbstraction> registration_dds_;
            std::unique_ptr<DdsAbstraction> registration_response_dds_;
            std::unique_ptr<DdsAbstraction> manager_ready_dds_;
            std::unique_ptr<DdsAbstraction> update_status_dds_;

            std::shared_ptr<OtacStatusStore> status_store_;

    };
}; // namespace ota_client::agl

#endif