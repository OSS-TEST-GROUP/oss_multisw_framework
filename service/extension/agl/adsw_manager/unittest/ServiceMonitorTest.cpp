#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "TestPrinter.hpp"

#define protected public
#define private   public
#include "ServiceMonitor.hpp"
#undef private
#undef protected

#include "common/AppUtils.hpp"
#include "mock/MockServiceRegistry.hpp"
#include "mock/MockMessageTransmitter.hpp"
#include "mock/MockAppStateReader.hpp"
#include "mock/MockAppController.hpp"

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>

using namespace adsw_manager::agl;
using namespace lifecyclemgr;
using namespace eprosima::fastdds::dds;
using ::testing::_;
using ::testing::Return;
using ::testing::ByMove;
using ::testing::NiceMock;

namespace {

// 최소 필드만 세팅하는 헬퍼
static Service MakeService(const std::string& name) {
    Service s; s.name(name); return s;
}

} // namespace

class ServiceMonitorTest : public ::testing::Test {
protected:
    std::shared_ptr<MockServiceRegistry> registry;
    std::shared_ptr<MockMessageTransmitter> transmitter;
    ServiceMonitor monitor;
    DomainParticipant* participant = nullptr;

    void SetUp() override {
        registry = std::make_shared<MockServiceRegistry>();
        transmitter = std::make_shared<MockMessageTransmitter>();

        // PublishContainerState is called by StartMonitoring() and CheckAppState()
        // (F009 C-1 periodic publish).  Allow any number of calls so individual
        // test cases are not forced to add their own expectation for it.
        EXPECT_CALL(*transmitter, PublishContainerState())
            .Times(::testing::AnyNumber());

        DomainParticipantQos qos;
        participant = DomainParticipantFactory::get_instance()->create_participant(0, qos);

        ASSERT_TRUE(monitor.Initialize(participant, registry, transmitter));
        monitor.timeout_ = std::chrono::seconds(2);
    }

    void TearDown() override {
        if (participant) {
            DomainParticipantFactory::get_instance()->delete_participant(participant);
            participant = nullptr;
        }
    }

    // readers_에 직접 Mock 리더 삽입
    MockAppStateReader* EmplaceReader(const std::string& app,
                                      std::chrono::steady_clock::time_point last) {
        // MockAppStateReader는 last를 보관하고 GetLastReceivedTime에서 돌려주는 구현이라고 가정
        auto r = std::make_unique<MockAppStateReader>(last);
        auto* raw = r.get();
        monitor.readers_[app] = std::move(r);
        return raw;
    }
};

// ----------------------------- Initialize -----------------------------

TEST_F(ServiceMonitorTest, Initialize_StoresDeps_AndReturnsTrue) {
    ServiceMonitor mon;
    EXPECT_TRUE(mon.Initialize(nullptr, registry, transmitter));
    EXPECT_EQ(mon.participant_, nullptr);
    EXPECT_EQ(mon.service_registry_.get(), registry.get());
    EXPECT_EQ(mon.message_transmitter_.get(), transmitter.get());
}

// ----------------------------- StartMonitoring / StopMonitoring -----------------------------

TEST_F(ServiceMonitorTest, StartMonitoring_CreatesReadersFromRegistryList) {
    std::vector<Service> list{ MakeService("appA"), MakeService("appB") };
    EXPECT_CALL(*registry, GetAppList()).WillOnce(Return(list));
    // The background monitoring thread may call CheckAppState() before
    // StopMonitoring() joins.  Allow GetAppInfo calls from the thread without
    // requiring a specific number (the guard in CheckAppState handles nullopt).
    EXPECT_CALL(*registry, GetAppInfo(::testing::_))
        .WillRepeatedly(Return(std::optional<std::pair<Service, ServiceState>>{}));

    monitor.StartMonitoring();

    EXPECT_TRUE(monitor.running_);
    EXPECT_EQ(monitor.readers_.count("appA"), 1u);
    EXPECT_EQ(monitor.readers_.count("appB"), 1u);

    monitor.StopMonitoring();
}

TEST_F(ServiceMonitorTest, StopMonitoring_StopsThreadAndJoins) {
    // 빈 리스트여도 문제없이 시작/정지 가능
    std::vector<Service> empty;
    EXPECT_CALL(*registry, GetAppList()).WillOnce(Return(empty));

    monitor.StartMonitoring();
    monitor.StopMonitoring();

    EXPECT_FALSE(monitor.running_);
    EXPECT_FALSE(monitor.monitoring_thread_.joinable());
}

// ----------------------------- CheckAppState -----------------------------

TEST_F(ServiceMonitorTest, CheckAppState_Timeout_MakesNOT_RUNNING_AndPublishes) {
    // readers_만 준비하면 CheckAppState는 StartMonitoring 호출 없이도 동작
    const std::string app = "appA";
    auto now = std::chrono::steady_clock::now();
    EmplaceReader(app, now - std::chrono::seconds(10)); // 타임아웃 초과 → 기대 상태 NOT_RUNNING

    // 레지스트리에 현재 상태 RUNNING으로 존재한다고 가정
    Service svc = MakeService(app);
    auto current_pair = std::make_pair(svc, ServiceState::RUNNING);
    EXPECT_CALL(*registry, GetAppInfo(app))
        .WillOnce(Return(std::optional<decltype(current_pair)>(current_pair)));

    EXPECT_CALL(*registry, UpdateAppState(app, ServiceState::NOT_RUNNING)).Times(1);
    EXPECT_CALL(*transmitter, PublishAppState()).Times(1);

    monitor.CheckAppState();
}

TEST_F(ServiceMonitorTest, CheckAppState_RecentMessage_KeepsRUNNING_NoPublish) {
    const std::string app = "ota_monitor";
    auto now = std::chrono::steady_clock::now();
    EmplaceReader(app, now - std::chrono::milliseconds(100)); // 타임아웃 이내 → 기대 상태 RUNNING

    // 이미 RUNNING이면 업데이트/퍼블리시 없어야 함
    Service svc = MakeService(app);
    auto current_pair = std::make_pair(svc, ServiceState::RUNNING);
    EXPECT_CALL(*registry, GetAppInfo(app))
        .WillOnce(Return(std::optional<decltype(current_pair)>(current_pair)));

    EXPECT_CALL(*registry, UpdateAppState(_, _)).Times(0);
    EXPECT_CALL(*transmitter, PublishAppState()).Times(0);

    monitor.CheckAppState();
}

TEST_F(ServiceMonitorTest, CheckAppState_Timeout_ButAlreadyNOT_RUNNING_NoPublish) {
    const std::string app = "lfc_monitor";
    auto now = std::chrono::steady_clock::now();
    EmplaceReader(app, now - std::chrono::seconds(5)); // 타임아웃 초과 → 기대 상태 NOT_RUNNING

    Service svc = MakeService(app);
    auto current_pair = std::make_pair(svc, ServiceState::NOT_RUNNING);
    EXPECT_CALL(*registry, GetAppInfo(app))
        .WillOnce(Return(std::optional<decltype(current_pair)>(current_pair)));

    // 이미 기대 상태와 동일 → 변경/퍼블리시 없음
    EXPECT_CALL(*registry, UpdateAppState(_, _)).Times(0);
    EXPECT_CALL(*transmitter, PublishAppState()).Times(0);

    monitor.CheckAppState();
}

TEST_F(ServiceMonitorTest, CheckAppState_Timeout_EmbeddedAppDoesNotUseDockerFallback) {
    NiceMock<MockAppController> app_controller;
    ASSERT_TRUE(monitor.Initialize(participant, registry, transmitter, &app_controller));

    const std::string app = "ota_monitor";
    auto now = std::chrono::steady_clock::now();
    EmplaceReader(app, now - std::chrono::seconds(5));

    EXPECT_CALL(*registry, GetDockerInfo(app))
        .WillOnce(Return(std::optional<AppDockerInfo>{}));

    Service svc = MakeService(app);
    auto current_pair = std::make_pair(svc, ServiceState::RUNNING);
    EXPECT_CALL(*registry, GetAppInfo(app))
        .WillOnce(Return(std::optional<decltype(current_pair)>(current_pair)));

    EXPECT_CALL(app_controller, IsAppRunning(_)).Times(0);
    EXPECT_CALL(*registry, UpdateAppState(app, ServiceState::NOT_RUNNING)).Times(1);
    EXPECT_CALL(*transmitter, PublishAppState()).Times(1);

    monitor.CheckAppState();
}

TEST_F(ServiceMonitorTest, CheckAppState_Timeout_DockerBackedAppCanUseDockerFallback) {
    NiceMock<MockAppController> app_controller;
    ASSERT_TRUE(monitor.Initialize(participant, registry, transmitter, &app_controller));

    const std::string app = "hmi_launcher";
    auto now = std::chrono::steady_clock::now();
    EmplaceReader(app, now - std::chrono::seconds(5));

    AppDockerInfo docker;
    docker.image = "hmi-launcher:latest";
    EXPECT_CALL(*registry, GetDockerInfo(app))
        .WillOnce(Return(std::optional<AppDockerInfo>{docker}));
    EXPECT_CALL(app_controller, IsAppRunning(app)).WillOnce(Return(true));

    Service svc = MakeService(app);
    auto current_pair = std::make_pair(svc, ServiceState::NOT_RUNNING);
    EXPECT_CALL(*registry, GetAppInfo(app))
        .WillOnce(Return(std::optional<decltype(current_pair)>(current_pair)));

    EXPECT_CALL(*registry, UpdateAppState(app, ServiceState::RUNNING)).Times(1);
    EXPECT_CALL(*transmitter, PublishAppState()).Times(1);

    monitor.CheckAppState();
}

TEST_F(ServiceMonitorTest, CheckAppState_Timeout_DockerBackedAppRestartsWhenDown) {
    NiceMock<MockAppController> app_controller;
    ASSERT_TRUE(monitor.Initialize(participant, registry, transmitter, &app_controller));

    const std::string app = "hmi_launcher";
    auto now = std::chrono::steady_clock::now();
    EmplaceReader(app, now - std::chrono::seconds(5));

    AppDockerInfo docker;
    docker.image = "hmi-launcher:latest";
    EXPECT_CALL(*registry, GetDockerInfo(app))
        .WillOnce(Return(std::optional<AppDockerInfo>{docker}));
    EXPECT_CALL(app_controller, IsAppRunning(app)).WillOnce(Return(false));
    EXPECT_CALL(app_controller, ShouldAutoRestartApp(app)).WillOnce(Return(true));
    EXPECT_CALL(app_controller, LaunchApp(app)).WillOnce(Return(true));

    Service svc = MakeService(app);
    auto current_pair = std::make_pair(svc, ServiceState::NOT_RUNNING);
    EXPECT_CALL(*registry, GetAppInfo(app))
        .WillOnce(Return(std::optional<decltype(current_pair)>(current_pair)));

    EXPECT_CALL(*registry, UpdateAppState(app, ServiceState::RUNNING)).Times(1);
    EXPECT_CALL(*transmitter, PublishAppState()).Times(1);

    monitor.CheckAppState();
}

TEST_F(ServiceMonitorTest, CheckAppState_Timeout_ManualStopDoesNotRestartDockerBackedApp) {
    NiceMock<MockAppController> app_controller;
    ASSERT_TRUE(monitor.Initialize(participant, registry, transmitter, &app_controller));

    const std::string app = "hmi_launcher";
    auto now = std::chrono::steady_clock::now();
    EmplaceReader(app, now - std::chrono::seconds(5));

    AppDockerInfo docker;
    docker.image = "hmi-launcher:latest";
    EXPECT_CALL(*registry, GetDockerInfo(app))
        .WillOnce(Return(std::optional<AppDockerInfo>{docker}));
    EXPECT_CALL(app_controller, IsAppRunning(app)).WillOnce(Return(false));
    EXPECT_CALL(app_controller, ShouldAutoRestartApp(app)).WillOnce(Return(false));
    EXPECT_CALL(app_controller, LaunchApp(app)).Times(0);

    Service svc = MakeService(app);
    auto current_pair = std::make_pair(svc, ServiceState::RUNNING);
    EXPECT_CALL(*registry, GetAppInfo(app))
        .WillOnce(Return(std::optional<decltype(current_pair)>(current_pair)));

    EXPECT_CALL(*registry, UpdateAppState(app, ServiceState::NOT_RUNNING)).Times(1);
    EXPECT_CALL(*transmitter, PublishAppState()).Times(1);

    monitor.CheckAppState();
}
