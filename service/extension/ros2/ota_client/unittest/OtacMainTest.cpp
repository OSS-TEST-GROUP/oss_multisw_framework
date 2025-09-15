#include <gtest/gtest.h>
#include <gmock/gmock.h>

// ---- private 멤버 접근 트릭 ----
#define private public
#define protected public
#include "OtacMain.hpp"
#include "OtacManagerService.hpp"
#undef private
#undef protected

#include "mock/MockDdsAbstraction.hpp"
#include "mock/MockOtacStatusStore.hpp"

#include "OtamClientServiceMsg.hpp"
#include <fastdds/dds/subscriber/SampleInfo.hpp>

using ::testing::_;
using ::testing::Return;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::SaveArg;

using eprosima::fastdds::dds::SampleInfo;
using OtaManager::SetOtaClientRegisteration;
using namespace ros2::ota_client;

// Stop() 호출 여부를 보기 위한 Spy (다른 테스트에는 사용 안 함)
class SpyManagerService : public OtacManagerService {
public:
    explicit SpyManagerService(std::shared_ptr<OtacStatusStore> store)
        : OtacManagerService(std::move(store)) {}

    void Stop() override { ++stop_called_; }
    int stop_called_{0};
};

// -------------------- Fixture --------------------
class OtacMainTest : public ::testing::Test {
protected:
    OtacMain main_;

    // 주로 Start() 테스트에서 사용할 실서비스 + 내부 DDS 목들
    std::shared_ptr<OtacManagerService> svc_;
    MockDdsAbstraction *reg_{nullptr}, *reg_resp_{nullptr}, *mgr_ready_{nullptr}, *upd_{nullptr};

    // 공통 store 목
    MockOtacStatusStore* store_{nullptr};

    void SetUp() override {
        // store 주입
        store_ = new MockOtacStatusStore();
        main_.status_store_ = std::shared_ptr<OtacStatusStore>(
            store_, [](OtacStatusStore* p){ delete static_cast<MockOtacStatusStore*>(p); });

        // OtacManagerService 인스턴스 생성 (Initialize 호출 안 함)
        svc_.reset(new OtacManagerService(main_.status_store_));

        // 내부 DDS 목 주입 (private→public 트릭으로 접근)
        svc_->registration_dds_          = std::unique_ptr<DdsAbstraction>(reg_      = new MockDdsAbstraction());
        svc_->registration_response_dds_ = std::unique_ptr<DdsAbstraction>(reg_resp_ = new MockDdsAbstraction());
        svc_->manager_ready_dds_         = std::unique_ptr<DdsAbstraction>(mgr_ready_ = new MockDdsAbstraction());
        svc_->update_status_dds_         = std::unique_ptr<DdsAbstraction>(upd_      = new MockDdsAbstraction());

        // main 에 서비스 주입
        main_.manager_service_ = svc_;
    }

    void TearDown() override {
        // DdsAbstraction 에 가상 소멸자가 없다면 release 후 파생형으로 delete
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

// -------------------- Tests --------------------

// Start 성공 + 첫 Publish 성공 → 총 2회 Publish
TEST_F(OtacMainTest, Start_PublishesTwice_WhenStartOkAndFirstPublishOk) {
    std::vector<std::string> apps = {"nav_app", "music_app"};
    EXPECT_CALL(*store_, GetAppNameList())
        .Times(1)
        .WillOnce(Return(apps));

    // OtacManagerService::Start() 경로가 true가 되도록 Create 4회 모두 성공
    EXPECT_CALL(*reg_,       Create("/mmf/otamanager/otaclient_service/SetOtaClientRegisteration",        true,  false)).WillOnce(Return(true));
    EXPECT_CALL(*reg_resp_,  Create("/mmf/otamanager/otaclient_service/SetOtaClientRegisterationResponse", false, true )).WillOnce(Return(true));
    EXPECT_CALL(*mgr_ready_, Create("/mmf/otamanager/otaclient_service/NotifyOtaManagerReady",             false, true )).WillOnce(Return(true));
    EXPECT_CALL(*upd_,       Create("/mmf/otamanager/otaclient_service/NotifyAppUpdateStatus",            false, true )).WillOnce(Return(true));

    // Write는 2번 호출되어야 함. 두 번의 payload를 전부 수집해서 검증
    std::vector<std::shared_ptr<void>> writes;
    EXPECT_CALL(*reg_, Write(_))
        .Times(2)
        .WillRepeatedly(Invoke([&](std::shared_ptr<void> p){
            writes.push_back(p);
            return true;
        }));

    main_.Start(); // 내부에서 500ms sleep 한 번 포함

    ASSERT_EQ(writes.size(), 2u);
    for (auto& w : writes) {
        auto msg = std::static_pointer_cast<SetOtaClientRegisteration>(w);
        ASSERT_TRUE(msg);
        EXPECT_EQ(msg->client_name(), "ros2_ota_client");
        ASSERT_EQ(msg->app_list().size(), apps.size());
        for (size_t i = 0; i < apps.size(); ++i) {
            EXPECT_EQ(msg->app_list()[i], apps[i]);
        }
    }
}

// Start 가 false → Publish 호출 없음 (단, app list 조회는 호출됨)
TEST_F(OtacMainTest, Start_WhenManagerStartFails_NoPublish) {
    std::vector<std::string> apps = {"nav_app", "music_app"};
    EXPECT_CALL(*store_, GetAppNameList())
        .Times(1)
        .WillOnce(Return(apps));

    // 첫 Create에서 실패하도록 하여 Start() == false
    EXPECT_CALL(*reg_, Create("/mmf/otamanager/otaclient_service/SetOtaClientRegisteration", true, false))
        .WillOnce(Return(false));
    // Publish는 불리지 않아야 함
    EXPECT_CALL(*reg_, Write(_)).Times(0);

    main_.Start();
}

// Start 는 true지만 첫 Publish 가 false → 두 번째 Publish는 없음
TEST_F(OtacMainTest, Start_WhenFirstPublishFails_NoSecondPublish) {
    std::vector<std::string> apps = {"nav_app", "music_app"};
    EXPECT_CALL(*store_, GetAppNameList())
        .Times(1)
        .WillOnce(Return(apps));

    EXPECT_CALL(*reg_,       Create("/mmf/otamanager/otaclient_service/SetOtaClientRegisteration",        true,  false)).WillOnce(Return(true));
    EXPECT_CALL(*reg_resp_,  Create("/mmf/otamanager/otaclient_service/SetOtaClientRegisterationResponse", false, true )).WillOnce(Return(true));
    EXPECT_CALL(*mgr_ready_, Create("/mmf/otamanager/otaclient_service/NotifyOtaManagerReady",             false, true )).WillOnce(Return(true));
    EXPECT_CALL(*upd_,       Create("/mmf/otamanager/otaclient_service/NotifyAppUpdateStatus",            false, true )).WillOnce(Return(true));

    EXPECT_CALL(*reg_, Write(_))
        .Times(1)               // 첫 호출만 발생
        .WillOnce(Return(false));

    main_.Start();
}

// Stop() 이 manager_service_->Stop() 을 호출하는지 확인 (Spy 사용)
TEST_F(OtacMainTest, Stop_DelegatesToManagerServiceStop) {
    // Spy로 교체
    auto spy = new SpyManagerService(main_.status_store_);
    main_.manager_service_ = std::shared_ptr<OtacManagerService>(spy, [](OtacManagerService* p){
        delete static_cast<SpyManagerService*>(p);
    });

    main_.Stop();
    EXPECT_EQ(spy->stop_called_, 1);
}