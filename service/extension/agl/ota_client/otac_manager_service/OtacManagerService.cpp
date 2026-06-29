#include "OtacManagerService.hpp"
#include "common/OtacManagerDdsImpl.hpp"
#include "ota_mgr/OtamClientServiceMsg.hpp"

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>

using namespace otamanager;
using namespace eprosima::fastdds::dds;

namespace ota_client::agl
{

    OtacManagerService::OtacManagerService(std::shared_ptr<OtacStatusStore> status_store) : status_store_(status_store) {}

    bool OtacManagerService::Initialize()
    {

        DomainParticipantQos participant_qos = PARTICIPANT_QOS_DEFAULT;
        participant_qos.name("ota client participant");
        participant_ = DomainParticipantFactory::get_instance()->create_participant(0, participant_qos);

        if (participant_ == nullptr)
            return false;

        std::cout << "participant create complete\n";

        publisher_ = participant_->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr);
        if (publisher_ == nullptr)
            return false;

        subscriber_ = participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr);
        if (subscriber_ == nullptr)
            return false;

        std::cout << "publisher and subscriber create complete\n";

        registration_dds_ = std::make_unique<OtacManagerDdsImpl>(participant_, publisher_, subscriber_,
                                                                 TypeSupport(new SetOtaClientRegistrationPubSubType()));

        std::cout << "otac manager ddsimpl 1 create\n";
        registration_response_dds_ = std::make_unique<OtacManagerDdsImpl>(participant_, publisher_, subscriber_,
                                                                          TypeSupport(new SetOtaClientRegistrationResponsePubSubType()));
        registration_response_dds_->SetOnMessage([this](std::shared_ptr<void> raw, const SampleInfo &)
                                                 {
            const auto& msg = *static_cast<SetOtaClientRegistrationResponse*>(raw.get());
            this->OnClientRegistrationResponse(msg); });

        manager_ready_dds_ = std::make_unique<OtacManagerDdsImpl>(participant_, publisher_, subscriber_,
                                                                  TypeSupport(new NotifyOtaManagerReadyPubSubType()));
        manager_ready_dds_->SetOnMessage([this](std::shared_ptr<void> raw, const SampleInfo &)
                                         {
            const auto& msg = *static_cast<NotifyOtaManagerReady*>(raw.get());
            this->OnManagerReady(msg); });

        update_status_dds_ = std::make_unique<OtacManagerDdsImpl>(participant_, publisher_, subscriber_,
                                                                  TypeSupport(new NotifyAppUpdateStatusPubSubType()));
        update_status_dds_->SetOnMessage([this](std::shared_ptr<void> raw, const SampleInfo &)
                                         {
            const auto& msg = *static_cast<NotifyAppUpdateStatus*>(raw.get());
            this->OnAppUpdateStatus(msg); });

        return true;
    }

    bool OtacManagerService::Start()
    {

        std::cout << "It is OtacManagerService::Start.\n";

        // Fix②: 등록 writer 를 TRANSIENT_LOCAL 로 — core(리더)가 늦게 디스커버리돼도
        // 마지막 등록 샘플을 retained 로 수신한다(기동 레이스로 등록 유실 방지).
        if (!registration_dds_->Create("/mmf/otamanager/otaclient_service/SetOtaClientRegistration", true, false, true))
        {
            return false;
        }
        if (!registration_response_dds_->Create("/mmf/otamanager/otaclient_service/SetOtaClientRegistrationResponse", false, true, true))
        {
            return false;
        }
        if (!manager_ready_dds_->Create("/mmf/otamanager/otaclient_service/NotifyOtaManagerReady", false, true, true))
        {
            return false;
        }
        if (!update_status_dds_->Create("/mmf/otamanager/otaclient_service/NotifyAppUpdateStatus", false, true))
        {
            return false;
        }

        std::cout << "OtacManagerService::Start complete!!\n";
        return true;
    }

    void OtacManagerService::Stop()
    {
        if (participant_ && publisher_)
            participant_->delete_publisher(publisher_);
        if (participant_ && subscriber_)
            participant_->delete_subscriber(subscriber_);
        if (participant_)
            DomainParticipantFactory::get_instance()->delete_participant(participant_);
    }

    bool OtacManagerService::PublishClientRegistration(std::string client_name, std::vector<std::string> app_list)
    {

        // auto reg = std::make_shared<SetOtaClientRegistration>();

        // reg->client_name(std::move(client_name));
        // reg->app_list().assign(app_list.begin(), app_list.end());

        // if (registration_dds_->Write(reg))
        // {
        //     return true;
        // }
        // else
        // {
        //     return false;
        // }

        auto reg = std::make_shared<otamanager::SetOtaClientRegistration>();
        reg->client_name(std::move(client_name));

        auto &seq = reg->app_list();
        seq.clear();
        seq.reserve(app_list.size());
        for (const auto &s : app_list)
            seq.push_back(s);

        return registration_dds_->Write(reg);
    }

    void OtacManagerService::OnClientRegistrationResponse(const SetOtaClientRegistrationResponse &msg)
    {
        // Fix①: 응답은 공유 토픽에 브로드캐스트되므로, 응답에 echo 된 client_name 이
        // 내 client_name 과 다르면 (타 클라이언트/잔존 응답) 무시한다.
        if (msg.client_name() != status_store_->GetClientName())
        {
            return;
        }
        std::cout << "Client id: " << msg.client_id() << "/ result: " << static_cast<std::int32_t>(msg.ret_code()) << std::endl;
    }

    void OtacManagerService::OnManagerReady(const NotifyOtaManagerReady &msg)
    {
        std::fprintf(stderr, "[SVC] OnManagerReady called\n");

        std::vector<std::string> app_list = status_store_->GetAppNameList();

        std::cout << "Ota Manager is ready!\nCall PublishClientRegistration()\n";

        if (PublishClientRegistration(status_store_->GetClientName(), app_list))
            std::cout << "Successfully Call Publish.\n";
    }

    void OtacManagerService::OnAppUpdateStatus(const NotifyAppUpdateStatus &msg)
    {
        status_store_->UpdateStatus(msg.app_name(), msg.status());
        std::cout << "App Update Status Update complete.\n";
    }

}
