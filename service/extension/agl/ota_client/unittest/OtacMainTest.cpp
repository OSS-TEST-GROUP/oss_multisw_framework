#include <gtest/gtest.h>
#include <gmock/gmock.h>

#define private public
#define protected public
#include "OtacMain.hpp"
#include "OtacManagerService.hpp"
#undef private
#undef protected

#include "mock/MockDdsAbstraction.hpp"
#include "mock/MockOtacStatusStore.hpp"

#include "ota_mgr/OtamClientServiceMsg.hpp"
#include <fastdds/dds/subscriber/SampleInfo.hpp>

using ::testing::_;
using ::testing::Return;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::SaveArg;

using eprosima::fastdds::dds::SampleInfo;
using OtaManager::SetOtaClientRegistration;
using namespace ota_client::agl;

TEST(OtacMainInitializeTest, Initialize_AllOk_ReturnsTrue) {
    OtacMain main;
    EXPECT_TRUE(main.Initialize());
}

class SpyManagerService : public OtacManagerService {
public:
    explicit SpyManagerService(std::shared_ptr<OtacStatusStore> store)
        : OtacManagerService(std::move(store)) {}

    void Stop() override { ++stop_called_; }
    int stop_called_{0};
};

class OtacMainTest : public ::testing::Test {
protected:
    OtacMain main_;

    std::shared_ptr<OtacManagerService> svc_;
    MockDdsAbstraction *reg_{nullptr}, *reg_resp_{nullptr}, *mgr_ready_{nullptr}, *upd_{nullptr};

    MockOtacStatusStore* store_{nullptr};

    void SetUp() override {
        store_ = new MockOtacStatusStore();
        main_.status_store_ = std::shared_ptr<OtacStatusStore>(
            store_, [](OtacStatusStore* p){ delete static_cast<MockOtacStatusStore*>(p); });

        svc_.reset(new OtacManagerService(main_.status_store_));

        svc_->registration_dds_          = std::unique_ptr<DdsAbstraction>(reg_      = new MockDdsAbstraction());
        svc_->registration_response_dds_ = std::unique_ptr<DdsAbstraction>(reg_resp_ = new MockDdsAbstraction());
        svc_->manager_ready_dds_         = std::unique_ptr<DdsAbstraction>(mgr_ready_ = new MockDdsAbstraction());
        svc_->update_status_dds_         = std::unique_ptr<DdsAbstraction>(upd_      = new MockDdsAbstraction());

        main_.manager_service_ = svc_;
    }

    void TearDown() override {
        if (svc_) {
            if (auto p = svc_->registration_dds_.release())          delete static_cast<MockDdsAbstraction*>(p);
            if (auto p = svc_->registration_response_dds_.release()) delete static_cast<MockDdsAbstraction*>(p);
            if (auto p = svc_->manager_ready_dds_.release())         delete static_cast<MockDdsAbstraction*>(p);
            if (auto p = svc_->update_status_dds_.release())         delete static_cast<MockDdsAbstraction*>(p);
        }
        main_.manager_service_.reset();
        main_.status_store_.reset();
    }
};

TEST_F(OtacMainTest, Start_PublishesTwice_WhenStartOkAndFirstPublishOk) {
    std::vector<std::string> apps = {"nav_app", "music_app"};
    EXPECT_CALL(*store_, GetAppNameList())
        .Times(1)
        .WillOnce(Return(apps));

    EXPECT_CALL(*reg_,       Create("/mmf/otamanager/otaclient_service/SetOtaClientRegistration",        true,  false, _)).WillOnce(Return(true));
    EXPECT_CALL(*reg_resp_,  Create("/mmf/otamanager/otaclient_service/SetOtaClientRegistrationResponse", false, true , _)).WillOnce(Return(true));
    EXPECT_CALL(*mgr_ready_, Create("/mmf/otamanager/otaclient_service/NotifyOtaManagerReady",             false, true , _)).WillOnce(Return(true));
    EXPECT_CALL(*upd_,       Create("/mmf/otamanager/otaclient_service/NotifyAppUpdateStatus",            false, true , _)).WillOnce(Return(true));

    std::vector<std::shared_ptr<void>> writes;
    EXPECT_CALL(*reg_, Write(_))
        .Times(2)
        .WillRepeatedly(Invoke([&](std::shared_ptr<void> p){
            writes.push_back(p);
            return true;
        }));

    main_.Start();

    ASSERT_EQ(writes.size(), 2u);
    for (auto& w : writes) {
        auto msg = std::static_pointer_cast<SetOtaClientRegistration>(w);
        ASSERT_TRUE(msg);
        EXPECT_EQ(msg->client_name(), "agl_ota_client");
        ASSERT_EQ(msg->app_list().size(), apps.size());
        for (size_t i = 0; i < apps.size(); ++i) {
            EXPECT_EQ(msg->app_list()[i], apps[i]);
        }
    }
}

TEST_F(OtacMainTest, Start_WhenManagerStartFails_NoPublish) {
    std::vector<std::string> apps = {"nav_app", "music_app"};
    EXPECT_CALL(*store_, GetAppNameList())
        .Times(1)
        .WillOnce(Return(apps));

    EXPECT_CALL(*reg_, Create("/mmf/otamanager/otaclient_service/SetOtaClientRegistration", true, false, _))
        .WillOnce(Return(false));
    EXPECT_CALL(*reg_, Write(_)).Times(0);

    main_.Start();
}

TEST_F(OtacMainTest, Start_WhenFirstPublishFails_NoSecondPublish) {
    std::vector<std::string> apps = {"nav_app", "music_app"};
    EXPECT_CALL(*store_, GetAppNameList())
        .Times(1)
        .WillOnce(Return(apps));

    EXPECT_CALL(*reg_,       Create("/mmf/otamanager/otaclient_service/SetOtaClientRegistration",        true,  false, _)).WillOnce(Return(true));
    EXPECT_CALL(*reg_resp_,  Create("/mmf/otamanager/otaclient_service/SetOtaClientRegistrationResponse", false, true , _)).WillOnce(Return(true));
    EXPECT_CALL(*mgr_ready_, Create("/mmf/otamanager/otaclient_service/NotifyOtaManagerReady",             false, true , _)).WillOnce(Return(true));
    EXPECT_CALL(*upd_,       Create("/mmf/otamanager/otaclient_service/NotifyAppUpdateStatus",            false, true , _)).WillOnce(Return(true));

    EXPECT_CALL(*reg_, Write(_))
        .Times(1)
        .WillOnce(Return(false));

    main_.Start();
}

TEST_F(OtacMainTest, Stop_DelegatesToManagerServiceStop) {
    auto spy = new SpyManagerService(main_.status_store_);
    main_.manager_service_ = std::shared_ptr<OtacManagerService>(spy, [](OtacManagerService* p){
        delete static_cast<SpyManagerService*>(p);
    });

    main_.Stop();
    EXPECT_EQ(spy->stop_called_, 1);
}