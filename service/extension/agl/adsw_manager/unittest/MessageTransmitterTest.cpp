// #include <gtest/gtest.h>
// #include <gmock/gmock.h>

// #include "mock/MockServiceRegistry.hpp"
// #include "mock/MockServiceController.hpp"
// #include "mock/MockDDSManager.hpp"
// #include "mock/MockAppController.hpp"
// #include "message_transmitter/MessageTransmitter.hpp"

// using namespace adsw_manager::agl;
// using namespace lifecyclemgr;
// using ::testing::_;
// using ::testing::Return;
// using ::testing::Invoke;
// using ::testing::StrictMock;

// class MessageTransmitterTest : public ::testing::Test {
// protected:
//     std::shared_ptr<MockServiceRegistry> registry;
//     std::shared_ptr<MockServiceController> controller;
//     std::shared_ptr<MockAppController> a_controller;
//     std::shared_ptr<MockDDSManager> dds_manager;
//     std::unique_ptr<MessageTransmitter> transmitter;

//     void SetUp() override {
//         registry = std::make_shared<MockServiceRegistry>();
//         a_controller = std::make_shared<MockAppController>();
//         controller = std::make_shared<MockServiceController>(a_controller.get());
//         dds_manager = std::make_shared<MockDDSManager>();
        
//         transmitter = std::make_unique<MessageTransmitter>(registry, controller, dds_manager);
//     }
// };

// // ----------------------------- Initialize -----------------------------

// TEST_F(MessageTransmitterTest, Initialize_CallsDDSManagerInitialize) {
//     ::testing::Sequence s;

//     EXPECT_CALL(*dds_manager, Initialize(_))
//         .InSequence(s)
//         .WillOnce(Return(true));

//     EXPECT_CALL(*dds_manager, SetOnGetAppStateRequest(_))
//         .Times(1)
//         .InSequence(s);

//     EXPECT_CALL(*dds_manager, SetOnStartAppRequest(_))
//         .Times(1)
//         .InSequence(s);

//     EXPECT_CALL(*dds_manager, SetOnStopAppRequest(_))
//         .Times(1)
//         .InSequence(s);

//     bool result = transmitter->Initialize(nullptr);

//     EXPECT_TRUE(result);
// }

// // ----------------------------- PublishAppState -----------------------------

// TEST_F(MessageTransmitterTest, PublishAppState_CallsDDSManagerPublish) {

//     using ::testing::ContainerEq;

//     std::vector<ServiceStatus> apps;

//     ServiceStatus a; 
//     a.name("adsw_monitor"); 
//     a.service_state(ServiceState::NOT_RUNNING); 
//     apps.push_back(a);

//     ServiceStatus b; 
//     b.name("map");          
//     b.service_state(ServiceState::RUNNING);     
//     apps.push_back(b);

//     ServiceStatus c; 
//     c.name("radio");        
//     c.service_state(ServiceState::RUNNING_NO_RESPONSE);   
//     apps.push_back(c);

//     EXPECT_CALL(*registry, GetAppList())
//         .Times(1)
//         .WillOnce(::testing::Return(apps));

//     // 기대 벡터
//     const std::vector<std::string> expected_names = {"adsw_monitor", "map", "radio"};
//     const std::vector<ServiceState> expected_states = {
//         ServiceState::NOT_RUNNING, ServiceState::RUNNING, ServiceState::RUNNING_NO_RESPONSE
//     };

//     // 전달된 메시지의 두 벡터 검증
//     EXPECT_CALL(*dds_manager, PublishNotifyAppState(_))
//         .Times(1)
//         .WillOnce(Invoke([&](const NotifyServiceState& msg){
//             // 컨테이너 매처로 순서까지 동일하게 확인
//             EXPECT_THAT(msg.service_list(), ContainerEq(expected_names));
//             EXPECT_THAT(msg.service_state_list(), ContainerEq(expected_states));
//         }));

//     transmitter->PublishAppState();

// }

// // ----------------------------- HandleGetAppStateRequest -----------------------------

// TEST_F(MessageTransmitterTest, HandleGetAppStateRequest_PublishesStatesForGivenApps)
// {
//     using ::testing::_;
//     using ::testing::AnyNumber;
//     using ::testing::DoAll;
//     using ::testing::Return;
//     using ::testing::SaveArg;

//     // Initialize 성공 + 콜백 캡처
//     std::function<void(const GetServiceStateRequest &)> on_get_cb;
//     EXPECT_CALL(*dds_manager, Initialize(_)).WillOnce(Return(true));
//     EXPECT_CALL(*dds_manager, SetOnGetAppStateRequest(_))
//         .WillOnce(SaveArg<0>(&on_get_cb));
//     EXPECT_CALL(*dds_manager, SetOnStartAppRequest(_)).Times(AnyNumber());
//     EXPECT_CALL(*dds_manager, SetOnStopAppRequest(_)).Times(AnyNumber());

//     // Handler 목 주입
//     auto get_handler = std::make_unique<::testing::NiceMock<MockAppStateHandler>>();
//     auto *get_handler_raw = get_handler.get();
//     EXPECT_CALL(*dds_manager, GetAppStateHandler())
//         .WillRepeatedly(Return(get_handler_raw));

//     // Registry 응답 준비: "app_ok"는 RESUMED, "missing"은 nullptr
//     ServiceStatus ok_info;
//     ok_info.name() = "app_ok";
//     ok_info.service_state() = ServiceState::RUNNING;

//     EXPECT_CALL(*registry, GetAppInfo("app_ok"))
//         .WillOnce(Return(&ok_info));
//     EXPECT_CALL(*registry, GetAppInfo("missing"))
//         .WillOnce(Return(nullptr));

//     // PublishResponse로 넘어가는 응답 캡처
//     GetServiceStateResponse captured;
//     EXPECT_CALL(*get_handler_raw, PublishResponse(_))
//         .WillOnce(DoAll(SaveArg<0>(&captured), Return(true)));

//     ASSERT_TRUE(transmitter->Initialize(nullptr));

//     // 콜백 실행 → 내부에서 HandleGetAppStateRequest 호출
//     GetServiceStateRequest req;
//     req.service_list().push_back("app_ok");
//     req.service_list().push_back("missing");
//     ASSERT_TRUE(static_cast<bool>(on_get_cb));
//     on_get_cb(req);

//     ASSERT_EQ(captured.service_list().size(), 2u);
//     ASSERT_EQ(captured.service_state_list().size(), 2u);
//     EXPECT_EQ(captured.service_list()[0], "app_ok");
//     EXPECT_EQ(captured.service_list()[1], "missing");
//     EXPECT_EQ(captured.service_state_list()[0], ServiceState::RUNNING);
//     EXPECT_EQ(captured.service_state_list()[1], ServiceState::NOT_RUNNING);

// }

// // ----------------------------- HandleStartAppRequest -----------------------------

// TEST_F(MessageTransmitterTest, HandleStartAppRequest_CallsControllerAndPublishesSuccess)
// {
//     using ::testing::_;
//     using ::testing::AnyNumber;
//     using ::testing::DoAll;
//     using ::testing::Return;
//     using ::testing::SaveArg;

//     // Initialize 성공 + Start 콜백만 캡처 (나머지는 허용)
//     std::function<void(const StartServiceRequest &)> on_start_cb;
//     EXPECT_CALL(*dds_manager, Initialize(_)).WillOnce(Return(true));
//     EXPECT_CALL(*dds_manager, SetOnStartAppRequest(_))
//         .WillOnce(SaveArg<0>(&on_start_cb));
//     EXPECT_CALL(*dds_manager, SetOnGetAppStateRequest(_)).Times(AnyNumber());
//     EXPECT_CALL(*dds_manager, SetOnStopAppRequest(_)).Times(AnyNumber());

//     // Handler 목 주입
//     auto start_handler = std::make_unique<::testing::NiceMock<MockStartAppHandler>>();
//     auto *start_handler_raw = start_handler.get();
//     EXPECT_CALL(*dds_manager, GetStartAppHandler())
//         .WillRepeatedly(Return(start_handler_raw));

//     // Controller가 성공 리턴 → 응답은 SUCCESS여야 함
//     EXPECT_CALL(*controller, StartService("test_app"))
//         .WillOnce(Return(true));

//     StartServiceResponse captured;
//     EXPECT_CALL(*start_handler_raw, PublishResponse(_))
//         .WillOnce(DoAll(SaveArg<0>(&captured), Return(true)));

//     ASSERT_TRUE(transmitter->Initialize(nullptr));

//     StartServiceRequest req;
//     req.service_list().push_back("test_app");
//     ASSERT_TRUE(static_cast<bool>(on_start_cb));
//     on_start_cb(req);

//     EXPECT_EQ(captured.ret_code(), ReturnCode::SUCCESS);

// }

// TEST_F(MessageTransmitterTest, HandleStartAppRequest_Failure_PublishesFailedResponse)
// {
//     using ::testing::_;
//     using ::testing::AnyNumber;
//     using ::testing::InSequence; // 순서: StartService → PublishResponse
//     using ::testing::Return;
//     using ::testing::SaveArg;

//     // Initialize 중 Start 콜백 캡처
//     std::function<void(const StartServiceRequest &)> on_start_cb;
//     EXPECT_CALL(*dds_manager, Initialize(_)).WillOnce(Return(true));
//     EXPECT_CALL(*dds_manager, SetOnStartAppRequest(_))
//         .WillOnce(SaveArg<0>(&on_start_cb));
//     EXPECT_CALL(*dds_manager, SetOnGetAppStateRequest(_)).Times(AnyNumber());
//     EXPECT_CALL(*dds_manager, SetOnStopAppRequest(_)).Times(AnyNumber());

//     // StartAppHandler 목을 dds_manager가 돌려주도록 함
//     auto start_handler = std::make_unique<::testing::NiceMock<MockStartAppHandler>>();
//     auto *start_handler_raw = start_handler.get();
//     EXPECT_CALL(*dds_manager, GetStartAppHandler())
//         .WillRepeatedly(Return(start_handler_raw));

//     // StartService가 false → PublishResponse는 FAILED
//     {
//         InSequence seq;
//         EXPECT_CALL(*controller, StartService("test_app"))
//             .WillOnce(Return(false));
//         EXPECT_CALL(*start_handler_raw, PublishResponse(_))
//             .WillOnce(Invoke([](const StartServiceResponse &r)
//                              {
//                                  EXPECT_EQ(r.ret_code(), ReturnCode::FAILED);
//                                  return true; // PublishResponse가 bool이면 필요
//                              }));
//     }

//     // 초기화하고 콜백 실행
//     ASSERT_TRUE(transmitter->Initialize(nullptr));
//     StartServiceRequest req;
//     req.service_list().push_back("test_app");
//     ASSERT_TRUE(static_cast<bool>(on_start_cb));
//     on_start_cb(req);
// }

// // ----------------------------- HandleStopAppRequest -----------------------------

// TEST_F(MessageTransmitterTest, HandleStopAppRequest_CallsControllerAndPublishesSuccess)
// {
//     using ::testing::_;
//     using ::testing::AnyNumber;
//     using ::testing::DoAll;
//     using ::testing::Return;
//     using ::testing::SaveArg;

//     // Initialize 성공 + Stop 콜백 캡처
//     std::function<void(const StopServiceRequest &)> on_stop_cb;
//     EXPECT_CALL(*dds_manager, Initialize(_)).WillOnce(Return(true));
//     EXPECT_CALL(*dds_manager, SetOnStopAppRequest(_))
//         .WillOnce(SaveArg<0>(&on_stop_cb));
//     EXPECT_CALL(*dds_manager, SetOnGetAppStateRequest(_)).Times(AnyNumber());
//     EXPECT_CALL(*dds_manager, SetOnStartAppRequest(_)).Times(AnyNumber());

//     // Handler 목 주입
//     auto stop_handler = std::make_unique<::testing::NiceMock<MockStopAppHandler>>();
//     auto *stop_handler_raw = stop_handler.get();
//     EXPECT_CALL(*dds_manager, GetStopAppHandler())
//         .WillRepeatedly(Return(stop_handler_raw));

//     // Controller가 성공 리턴 → 응답은 SUCCESS여야 함
//     EXPECT_CALL(*controller, StopService("test_app"))
//         .WillOnce(Return(true));

//     StopServiceResponse captured;
//     EXPECT_CALL(*stop_handler_raw, PublishResponse(_))
//         .WillOnce(DoAll(SaveArg<0>(&captured), Return(true)));

//     ASSERT_TRUE(transmitter->Initialize(nullptr));

//     StopServiceRequest req;
//     // Start와 동일한 구조라고 가정 (app_info().app_name())
//     req.service_list().push_back("test_app");
//     ASSERT_TRUE(static_cast<bool>(on_stop_cb));
//     on_stop_cb(req);

//     EXPECT_EQ(captured.ret_code(), ReturnCode::SUCCESS);
// }

// TEST_F(MessageTransmitterTest, HandleStopAppRequest_Failure_PublishesFailedResponse)
// {
//     using ::testing::_;
//     using ::testing::AnyNumber;
//     using ::testing::InSequence;
//     using ::testing::Return;
//     using ::testing::SaveArg;

//     // Initialize 동안 Stop 콜백 캡처, 나머지 콜백은 허용
//     std::function<void(const StopServiceRequest &)> on_stop_cb;
//     EXPECT_CALL(*dds_manager, Initialize(_)).WillOnce(Return(true));
//     EXPECT_CALL(*dds_manager, SetOnStopAppRequest(_))
//         .WillOnce(SaveArg<0>(&on_stop_cb));
//     EXPECT_CALL(*dds_manager, SetOnGetAppStateRequest(_)).Times(AnyNumber());
//     EXPECT_CALL(*dds_manager, SetOnStartAppRequest(_)).Times(AnyNumber());

//     // dds_manager가 돌려줄 Stop 핸들러 목 준비
//     auto stop_handler = std::make_unique<::testing::NiceMock<MockStopAppHandler>>();
//     auto *stop_handler_raw = stop_handler.get();
//     EXPECT_CALL(*dds_manager, GetStopAppHandler())
//         .WillRepeatedly(Return(stop_handler_raw));

//     // StopService(false) → PublishResponse(FAILED)
//     {
//         InSequence seq;
//         EXPECT_CALL(*controller, StopService("test_app"))
//             .WillOnce(Return(false));
//         EXPECT_CALL(*stop_handler_raw, PublishResponse(_))
//             .WillOnce(::testing::Invoke([](const StopServiceResponse &r)
//                                         {
//                                             EXPECT_EQ(r.ret_code(), ReturnCode::FAILED);
//                                             return true; // PublishResponse가 bool 반환인 경우
//                                         }));
//     }

//     // 초기화 후 콜백 실행 (→ 내부에서 HandleStopAppRequest 호출)
//     ASSERT_TRUE(transmitter->Initialize(nullptr));

//     StopServiceRequest req;
//     req.service_list().push_back("test_app");

//     ASSERT_TRUE(static_cast<bool>(on_stop_cb));
//     on_stop_cb(req);
// }

#include <gtest/gtest.h>
#include <gmock/gmock.h>

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
        {"map",          ServiceState::RUNNING},
        {"radio",        ServiceState::RUNNING_NO_RESPONSE}
    };

    EXPECT_CALL(*registry, GetNameStatePairs())
        .WillOnce(Return(pairs));

    EXPECT_CALL(*dds_manager, PublishNotifyAppState(_))
        .WillOnce(Invoke([&](const NotifyServiceState& msg){
            EXPECT_THAT(msg.service_list(), 
                        ContainerEq(std::vector<std::string>{"adsw_monitor","map","radio"}));
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
    Service b; b.name("map");
    apps.push_back(a);
    apps.push_back(b);

    EXPECT_CALL(*registry, GetAppList())
        .WillOnce(Return(apps));

    EXPECT_CALL(*dds_manager, PublishUpdateAppList(_))
        .WillOnce(Invoke([&](const UpdateServiceListRequest& msg){
            ASSERT_EQ(msg.service_list().size(), 2u);
            EXPECT_EQ(msg.service_list()[0].name(), "adsw_monitor");
            EXPECT_EQ(msg.service_list()[1].name(), "map");
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
    auto* get_handler = new ::testing::NiceMock<MockAppStateHandler>();
    EXPECT_CALL(*dds_manager, GetAppStateHandler())
        .WillRepeatedly(Return(get_handler));

    // Registry: 이름 -> 상태
    EXPECT_CALL(*registry, GetAppState("ok_a")).WillOnce(Return(ServiceState::RUNNING));
    EXPECT_CALL(*registry, GetAppState("ok_b")).WillOnce(Return(ServiceState::NOT_RUNNING));

    GetServiceStateResponse captured;
    EXPECT_CALL(*get_handler, PublishResponse(_))
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

    auto* start_handler = new ::testing::NiceMock<MockStartAppHandler>();
    EXPECT_CALL(*dds_manager, GetStartAppHandler())
        .WillRepeatedly(Return(start_handler));

    EXPECT_CALL(*controller, StartService("a")).WillOnce(Return(true));
    EXPECT_CALL(*controller, StartService("b")).WillOnce(Return(true));

    StartServiceResponse captured;
    EXPECT_CALL(*start_handler, PublishResponse(_))
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

    auto* start_handler = new ::testing::NiceMock<MockStartAppHandler>();
    EXPECT_CALL(*dds_manager, GetStartAppHandler())
        .WillRepeatedly(Return(start_handler));

    {
        InSequence seq;
        EXPECT_CALL(*controller, StartService("a")).WillOnce(Return(false));
        // 실패 후 즉시 break → "b"는 호출되지 않아야 함
        EXPECT_CALL(*controller, StartService("b")).Times(0);
        EXPECT_CALL(*start_handler, PublishResponse(_))
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

    auto* stop_handler = new ::testing::NiceMock<MockStopAppHandler>();
    EXPECT_CALL(*dds_manager, GetStopAppHandler())
        .WillRepeatedly(Return(stop_handler));

    EXPECT_CALL(*controller, StopService("x")).WillOnce(Return(true));
    EXPECT_CALL(*controller, StopService("y")).WillOnce(Return(true));

    StopServiceResponse captured;
    EXPECT_CALL(*stop_handler, PublishResponse(_))
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

    auto* stop_handler = new ::testing::NiceMock<MockStopAppHandler>();
    EXPECT_CALL(*dds_manager, GetStopAppHandler())
        .WillRepeatedly(Return(stop_handler));

    {
        InSequence seq;
        EXPECT_CALL(*controller, StopService("x")).WillOnce(Return(false));
        EXPECT_CALL(*controller, StopService("y")).Times(0);
        EXPECT_CALL(*stop_handler, PublishResponse(_))
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
