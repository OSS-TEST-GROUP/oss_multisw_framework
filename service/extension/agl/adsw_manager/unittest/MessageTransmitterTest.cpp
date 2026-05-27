#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "TestPrinter.hpp"

#include "mock/MockServiceRegistry.hpp"
#include "mock/MockServiceController.hpp"
#include "mock/MockDDSManager.hpp"
#include "mock/MockAppController.hpp"
#include "MessageTransmitter.hpp"

using namespace adsw_manager::agl;
using namespace lifecyclemgr;
using ::testing::_;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::SaveArg;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::AnyNumber;
using ::testing::InSequence;

class MessageTransmitterTest : public ::testing::Test {
protected:
    std::shared_ptr<MockServiceRegistry> registry;
    std::shared_ptr<MockServiceController> controller;
    std::shared_ptr<MockAppController> a_controller;
    std::shared_ptr<MockDDSManager> dds_manager;
    std::unique_ptr<MessageTransmitter> tx;

    void SetUp() override {
        registry = std::make_shared<MockServiceRegistry>();
        a_controller = std::make_shared<MockAppController>();
        controller = std::make_shared<MockServiceController>(a_controller.get());
        dds_manager = std::make_shared<MockDDSManager>();
        tx = std::make_unique<MessageTransmitter>(registry, controller, dds_manager);
    }
};

// ----------------------------- Initialize -----------------------------

TEST_F(MessageTransmitterTest, Initialize_RegistersAllCallbacks_AndReturnsTrue) {
    InSequence seq;

    EXPECT_CALL(*dds_manager, Initialize(_)).WillOnce(Return(true));
    EXPECT_CALL(*dds_manager, SetOnGetAppStateRequest(_)).Times(1);
    EXPECT_CALL(*dds_manager, SetOnStartAppRequest(_)).Times(1);
    EXPECT_CALL(*dds_manager, SetOnStopAppRequest(_)).Times(1);
    EXPECT_CALL(*dds_manager, SetOnUpdateListResponse(_)).Times(1);

    EXPECT_TRUE(tx->Initialize(nullptr));
}

// ----------------------------- PublishAppState -----------------------------

TEST_F(MessageTransmitterTest, PublishAppState_UsesNameStatePairs_AndPublishes) {
    using ::testing::ContainerEq;

    std::vector<std::pair<std::string, ServiceState>> pairs = {
        {"adsw_monitor", ServiceState::NOT_RUNNING},
        {"ota_monitor",  ServiceState::RUNNING},
        {"lfc_monitor",  ServiceState::RUNNING_NO_RESPONSE}
    };

    EXPECT_CALL(*registry, GetNameStatePairs())
        .WillOnce(Return(pairs));

    EXPECT_CALL(*dds_manager, PublishNotifyAppState(_))
        .WillOnce(Invoke([&](const NotifyServiceState& msg){
            EXPECT_THAT(msg.service_list(), 
                        ContainerEq(std::vector<std::string>{"adsw_monitor","ota_monitor","lfc_monitor"}));
            EXPECT_THAT(msg.service_state_list(), 
                        ContainerEq(std::vector<ServiceState>{
                            ServiceState::NOT_RUNNING,
                            ServiceState::RUNNING,
                            ServiceState::RUNNING_NO_RESPONSE
                        }));
        }));

    tx->PublishAppState();
}

// ----------------------------- PublishAppList -----------------------------

TEST_F(MessageTransmitterTest, PublishAppList_PassesVectorService) {
    std::vector<Service> apps;
    Service a; a.name("adsw_monitor");
    Service b; b.name("hmi_launcher");
    apps.push_back(a);
    apps.push_back(b);

    EXPECT_CALL(*registry, GetAppList())
        .WillOnce(Return(apps));

    EXPECT_CALL(*dds_manager, PublishUpdateAppList(_))
        .WillOnce(Invoke([&](const UpdateServiceListRequest& msg){
            ASSERT_EQ(msg.service_list().size(), 2u);
            EXPECT_EQ(msg.service_list()[0].name(), "adsw_monitor");
            EXPECT_EQ(msg.service_list()[1].name(), "hmi_launcher");
        }));

    tx->PublishAppList();
}

// ----------------------------- HandleGetAppStateRequest -----------------------------

TEST_F(MessageTransmitterTest, HandleGetAppStateRequest_AGL_PublishesStates) {
    // Initialize + 콜백 캡처
    std::function<void(const GetServiceStateRequest&)> on_get_cb;
    EXPECT_CALL(*dds_manager, Initialize(_)).WillOnce(Return(true));
    EXPECT_CALL(*dds_manager, SetOnGetAppStateRequest(_))
        .WillOnce(SaveArg<0>(&on_get_cb));
    EXPECT_CALL(*dds_manager, SetOnStartAppRequest(_)).Times(AnyNumber());
    EXPECT_CALL(*dds_manager, SetOnStopAppRequest(_)).Times(AnyNumber());
    EXPECT_CALL(*dds_manager, SetOnUpdateListResponse(_)).Times(AnyNumber());

    // 핸들러 목
    //auto* get_handler = new ::testing::NiceMock<MockAppStateHandler>();
    ::testing::NiceMock<MockAppStateHandler> get_handler;
    EXPECT_CALL(*dds_manager, GetAppStateHandler())
        .WillRepeatedly(Return(&get_handler));

    // Registry: 이름 -> 상태
    EXPECT_CALL(*registry, GetAppState("ok_a")).WillOnce(Return(ServiceState::RUNNING));
    EXPECT_CALL(*registry, GetAppState("ok_b")).WillOnce(Return(ServiceState::NOT_RUNNING));

    GetServiceStateResponse captured;
    EXPECT_CALL(get_handler, PublishResponse(_))
        .WillOnce(DoAll(SaveArg<0>(&captured), Return(true)));

    ASSERT_TRUE(tx->Initialize(nullptr));

    GetServiceStateRequest req;
    req.os_type(OsType::AGL);
    req.service_list().push_back("ok_a");
    req.service_list().push_back("ok_b");

    ASSERT_TRUE(static_cast<bool>(on_get_cb));
    on_get_cb(req);

    ASSERT_EQ(captured.service_list().size(), 2u);
    ASSERT_EQ(captured.service_state_list().size(), 2u);
    EXPECT_EQ(captured.service_list()[0], "ok_a");
    EXPECT_EQ(captured.service_list()[1], "ok_b");
    EXPECT_EQ(captured.service_state_list()[0], ServiceState::RUNNING);
    EXPECT_EQ(captured.service_state_list()[1], ServiceState::NOT_RUNNING);
}

TEST_F(MessageTransmitterTest, HandleGetAppStateRequest_NonAGL_Ignores) {
    // Initialize + 콜백 캡처
    std::function<void(const GetServiceStateRequest&)> on_get_cb;
    EXPECT_CALL(*dds_manager, Initialize(_)).WillOnce(Return(true));
    EXPECT_CALL(*dds_manager, SetOnGetAppStateRequest(_))
        .WillOnce(SaveArg<0>(&on_get_cb));
    EXPECT_CALL(*dds_manager, SetOnStartAppRequest(_)).Times(AnyNumber());
    EXPECT_CALL(*dds_manager, SetOnStopAppRequest(_)).Times(AnyNumber());
    EXPECT_CALL(*dds_manager, SetOnUpdateListResponse(_)).Times(AnyNumber());

    // Non-AGL이면 아무 것도 안 함
    EXPECT_CALL(*dds_manager, GetAppStateHandler()).Times(0);
    EXPECT_TRUE(tx->Initialize(nullptr));

    GetServiceStateRequest req;
    req.os_type(OsType::ROS2); // AGL이 아님
    req.service_list().push_back("x");

    ASSERT_TRUE(static_cast<bool>(on_get_cb));
    on_get_cb(req);
}

// ----------------------------- HandleStartAppRequest -----------------------------

TEST_F(MessageTransmitterTest, HandleStartAppRequest_AllSuccess_PublishesSuccess) {
    std::function<void(const StartServiceRequest&)> on_start_cb;
    EXPECT_CALL(*dds_manager, Initialize(_)).WillOnce(Return(true));
    EXPECT_CALL(*dds_manager, SetOnStartAppRequest(_))
        .WillOnce(SaveArg<0>(&on_start_cb));
    EXPECT_CALL(*dds_manager, SetOnGetAppStateRequest(_)).Times(AnyNumber());
    EXPECT_CALL(*dds_manager, SetOnStopAppRequest(_)).Times(AnyNumber());
    EXPECT_CALL(*dds_manager, SetOnUpdateListResponse(_)).Times(AnyNumber());

    //auto* start_handler = new ::testing::NiceMock<MockStartAppHandler>();
    ::testing::NiceMock<MockStartAppHandler> start_handler;
    EXPECT_CALL(*dds_manager, GetStartAppHandler())
        .WillRepeatedly(Return(&start_handler));

    EXPECT_CALL(*controller, StartService("a")).WillOnce(Return(true));
    EXPECT_CALL(*controller, StartService("b")).WillOnce(Return(true));

    StartServiceResponse captured;
    EXPECT_CALL(start_handler, PublishResponse(_))
        .WillOnce(DoAll(SaveArg<0>(&captured), Return(true)));

    ASSERT_TRUE(tx->Initialize(nullptr));

    StartServiceRequest req;
    req.os_type(OsType::AGL);
    req.service_list().push_back("a");
    req.service_list().push_back("b");

    ASSERT_TRUE(static_cast<bool>(on_start_cb));
    on_start_cb(req);

    EXPECT_EQ(captured.ret_code(), ReturnCode::SUCCESS);
}

TEST_F(MessageTransmitterTest, HandleStartAppRequest_FirstFailure_BreaksAndPublishesFailed) {
    std::function<void(const StartServiceRequest&)> on_start_cb;
    EXPECT_CALL(*dds_manager, Initialize(_)).WillOnce(Return(true));
    EXPECT_CALL(*dds_manager, SetOnStartAppRequest(_))
        .WillOnce(SaveArg<0>(&on_start_cb));
    EXPECT_CALL(*dds_manager, SetOnGetAppStateRequest(_)).Times(AnyNumber());
    EXPECT_CALL(*dds_manager, SetOnStopAppRequest(_)).Times(AnyNumber());
    EXPECT_CALL(*dds_manager, SetOnUpdateListResponse(_)).Times(AnyNumber());

    //auto* start_handler = new ::testing::NiceMock<MockStartAppHandler>();
    ::testing::NiceMock<MockStartAppHandler> start_handler;
    EXPECT_CALL(*dds_manager, GetStartAppHandler())
        .WillRepeatedly(Return(&start_handler));

    {
        InSequence seq;
        EXPECT_CALL(*controller, StartService("a")).WillOnce(Return(false));
        // 실패 후 즉시 break → "b"는 호출되지 않아야 함
        EXPECT_CALL(*controller, StartService("b")).Times(0);
        EXPECT_CALL(start_handler, PublishResponse(_))
            .WillOnce(Invoke([](const StartServiceResponse& r){
                EXPECT_EQ(r.ret_code(), ReturnCode::FAILED);
                return true;
            }));
    }

    ASSERT_TRUE(tx->Initialize(nullptr));

    StartServiceRequest req;
    req.os_type(OsType::AGL);
    req.service_list().push_back("a");
    req.service_list().push_back("b");

    ASSERT_TRUE(static_cast<bool>(on_start_cb));
    on_start_cb(req);
}

// ----------------------------- HandleStopAppRequest -----------------------------

TEST_F(MessageTransmitterTest, HandleStopAppRequest_AllSuccess_PublishesSuccess) {
    std::function<void(const StopServiceRequest&)> on_stop_cb;
    EXPECT_CALL(*dds_manager, Initialize(_)).WillOnce(Return(true));
    EXPECT_CALL(*dds_manager, SetOnStopAppRequest(_))
        .WillOnce(SaveArg<0>(&on_stop_cb));
    EXPECT_CALL(*dds_manager, SetOnGetAppStateRequest(_)).Times(AnyNumber());
    EXPECT_CALL(*dds_manager, SetOnStartAppRequest(_)).Times(AnyNumber());
    EXPECT_CALL(*dds_manager, SetOnUpdateListResponse(_)).Times(AnyNumber());

    //auto* stop_handler = new ::testing::NiceMock<MockStopAppHandler>();
    ::testing::NiceMock<MockStopAppHandler> stop_handler;
    EXPECT_CALL(*dds_manager, GetStopAppHandler())
        .WillRepeatedly(Return(&stop_handler));

    EXPECT_CALL(*controller, StopService("x")).WillOnce(Return(true));
    EXPECT_CALL(*controller, StopService("y")).WillOnce(Return(true));

    StopServiceResponse captured;
    EXPECT_CALL(stop_handler, PublishResponse(_))
        .WillOnce(DoAll(SaveArg<0>(&captured), Return(true)));

    ASSERT_TRUE(tx->Initialize(nullptr));

    StopServiceRequest req;
    req.os_type(OsType::AGL);
    req.service_list().push_back("x");
    req.service_list().push_back("y");

    ASSERT_TRUE(static_cast<bool>(on_stop_cb));
    on_stop_cb(req);

    EXPECT_EQ(captured.ret_code(), ReturnCode::SUCCESS);
}

TEST_F(MessageTransmitterTest, HandleStopAppRequest_FirstFailure_BreaksAndPublishesFailed) {
    std::function<void(const StopServiceRequest&)> on_stop_cb;
    EXPECT_CALL(*dds_manager, Initialize(_)).WillOnce(Return(true));
    EXPECT_CALL(*dds_manager, SetOnStopAppRequest(_))
        .WillOnce(SaveArg<0>(&on_stop_cb));
    EXPECT_CALL(*dds_manager, SetOnGetAppStateRequest(_)).Times(AnyNumber());
    EXPECT_CALL(*dds_manager, SetOnStartAppRequest(_)).Times(AnyNumber());
    EXPECT_CALL(*dds_manager, SetOnUpdateListResponse(_)).Times(AnyNumber());

    //auto* stop_handler = new ::testing::NiceMock<MockStopAppHandler>();
    ::testing::NiceMock<MockStopAppHandler> stop_handler;
    EXPECT_CALL(*dds_manager, GetStopAppHandler())
        .WillRepeatedly(Return(&stop_handler));

    {
        InSequence seq;
        EXPECT_CALL(*controller, StopService("x")).WillOnce(Return(false));
        EXPECT_CALL(*controller, StopService("y")).Times(0);
        EXPECT_CALL(stop_handler, PublishResponse(_))
            .WillOnce(Invoke([](const StopServiceResponse& r){
                EXPECT_EQ(r.ret_code(), ReturnCode::FAILED);
                return true;
            }));
    }

    ASSERT_TRUE(tx->Initialize(nullptr));

    StopServiceRequest req;
    req.os_type(OsType::AGL);
    req.service_list().push_back("x");
    req.service_list().push_back("y");

    ASSERT_TRUE(static_cast<bool>(on_stop_cb));
    on_stop_cb(req);
}

// ----------------------------- HandleUpdateListResponse -----------------------------

TEST_F(MessageTransmitterTest, UpdateListResponse_Callback_Path_NoCrash) {
    // 1) Initialize 중 콜백 캡처
    std::function<void(const UpdateServiceListResponse&)> on_update_cb;

    EXPECT_CALL(*dds_manager, Initialize(_)).WillOnce(Return(true));
    EXPECT_CALL(*dds_manager, SetOnUpdateListResponse(_))
        .WillOnce(SaveArg<0>(&on_update_cb));

    // 다른 콜백 등록은 테스트 목적상 AnyNumber로 허용
    EXPECT_CALL(*dds_manager, SetOnGetAppStateRequest(_)).Times(AnyNumber());
    EXPECT_CALL(*dds_manager, SetOnStartAppRequest(_)).Times(AnyNumber());
    EXPECT_CALL(*dds_manager, SetOnStopAppRequest(_)).Times(AnyNumber());

    ASSERT_TRUE(tx->Initialize(nullptr));
    ASSERT_TRUE(static_cast<bool>(on_update_cb));  // 콜백이 실제로 등록됐는지 확인

    // 2) 콜백 호출로 private 핸들러 간접 실행
    UpdateServiceListResponse res;
    res.ret_code(ReturnCode::SUCCESS);

    // 현재 구현은 로그만 찍으니, 크래시 없이 호출되면 통과
    EXPECT_NO_THROW(on_update_cb(res));
}
