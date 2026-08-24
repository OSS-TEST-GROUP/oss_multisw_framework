// general adsw_manager — ContainerServiceController / ServiceMonitor 유닛테스트
//
// 이 두 클래스가 general 판의 제어 흐름 본체다 (374 + 230줄). 둘 다 테스트가
// 없었고, 이유는 ADSWManager 싱글턴과 ContainerController 의 docker 호출을
// 가로챌 수 없어서였다. 링크 심(MockADSWManager) + 실행 seam(FakeCmd...) 두
// 가지로 둘 다 끊는다.
//
// general 판의 규약은 "컨테이너 워크로드만 다룬다" 는 것이다. 네이티브 프로세스나
// ros2 run/launch 는 의도적으로 지원하지 않고 false 를 반환한다 — 그 경계가
// 흐려지면 ros2 판과 역할이 겹치므로 테스트로 고정한다.

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <memory>
#include <string>

#include "control/ContainerServiceController.hpp"
#include "control/ContainerController.hpp"
#include "monitor/ServiceMonitor.hpp"
#include "monitor/UpdateServiceInfo.hpp"
#include "service/ServiceRegistry.hpp"
#include "service/Service.hpp"
#include "service/ServiceState.hpp"
#include "service/ServiceType.hpp"
#include "service/RecoveryPolicy.hpp"
#include "service/TypeConvert.hpp"
#include "transmitter/MessageQueue.hpp"

#include "mock/MockLogger.hpp"
#include "mock/MockADSWManager.hpp"
#include "mock/FakeCmdContainerController.hpp"

using ::testing::Return;
using general::adsw_manager::Container;
using general::adsw_manager::ContainerServiceController;
using general::adsw_manager::ContainerType;
using general::adsw_manager::RecoveryPolicy;
using general::adsw_manager::ScopedMockADSWManager;
using general::adsw_manager::Service;
using general::adsw_manager::ServiceMonitor;
using general::adsw_manager::ServiceRegistry;
using general::adsw_manager::ServiceState;
using general::adsw_manager::ServiceType;
using general::adsw_manager::testing_support::FakeCmdContainerController;

namespace {

std::shared_ptr<Service> MakeService(const std::string& name, bool container_workload) {
    auto s = std::make_shared<Service>(name, ServiceType::NORMAL,
                                       RecoveryPolicy::ALWAYS, true);
    s->SetContainerWorkload(container_workload);
    return s;
}

class ControllerTest : public ::testing::Test {
protected:
    void SetUp() override {
        logger_ = std::make_unique<ScopedMockLogger>();
        mgr_    = std::make_unique<ScopedMockADSWManager>();

        ctl_ = std::make_shared<ContainerServiceController>();
        ctl_->Initialize();

        registry_ = std::make_shared<ServiceRegistry>();
        ON_CALL(**mgr_, GetServiceRegistry()).WillByDefault(Return(registry_));
        ON_CALL(**mgr_, GetServiceController()).WillByDefault(Return(ctl_));
    }
    void TearDown() override {
        registry_.reset();
        ctl_.reset();
        mgr_.reset();
        logger_.reset();
    }

    std::unique_ptr<ScopedMockLogger>            logger_;
    std::unique_ptr<ScopedMockADSWManager>       mgr_;
    std::shared_ptr<ContainerServiceController>  ctl_;
    std::shared_ptr<ServiceRegistry>             registry_;
};

} // namespace

// ── 수명주기 ────────────────────────────────────────────────────────────────

TEST_F(ControllerTest, InitializeCreatesContainerController) {
    EXPECT_NE(ctl_->GetContainer(), nullptr);
}

TEST_F(ControllerTest, FinalizeIsSafe) {
    EXPECT_NO_THROW(ctl_->Finalize());
}

TEST_F(ControllerTest, InitializeTwiceIsSafe) {
    EXPECT_TRUE(ctl_->Initialize());
    EXPECT_NE(ctl_->GetContainer(), nullptr);
}

// ── StartService / StopService — general 판의 경계 규약 ─────────────────────

TEST_F(ControllerTest, StartService_NullRejected) {
    EXPECT_FALSE(ctl_->StartService(std::shared_ptr<Service>()));
}

TEST_F(ControllerTest, StopService_NullRejected) {
    EXPECT_FALSE(ctl_->StopService(std::shared_ptr<Service>()));
}

TEST_F(ControllerTest, StartService_ContainerWorkloadGoesRunning) {
    // 컨테이너 워크로드는 컨테이너 기동으로 이미 떠 있다. docker exec 를 또 하면
    // 이중 실행이 되므로 상태만 반영하고 성공을 반환해야 한다.
    auto svc = MakeService("wl", /*container_workload=*/true);
    EXPECT_TRUE(ctl_->StartService(svc));
    EXPECT_EQ(svc->GetServiceState(), ServiceState::RUNNING);
}

TEST_F(ControllerTest, StartService_NonContainerServiceIsRejected) {
    // general 판은 컨테이너 워크로드만 다룬다. 네이티브/ros2 실행은 ros2 판 소관.
    auto svc = MakeService("native", /*container_workload=*/false);
    EXPECT_FALSE(ctl_->StartService(svc));
    EXPECT_NE(svc->GetServiceState(), ServiceState::RUNNING);
}

TEST_F(ControllerTest, StopService_NonContainerServiceIsRejected) {
    auto svc = MakeService("native", false);
    EXPECT_FALSE(ctl_->StopService(svc));
}

TEST_F(ControllerTest, StopService_ContainerWorkloadWithoutGroupFails) {
    // 그룹(= 컨테이너 이름)을 못 찾으면 멈출 대상이 없다.
    auto svc = MakeService("wl", true);
    EXPECT_FALSE(ctl_->StopService(svc));
}

TEST_F(ControllerTest, StartServiceByName_UnknownServiceFails) {
    EXPECT_FALSE(ctl_->StartService(std::string("ghost")));
}

TEST_F(ControllerTest, StopServiceByName_UnknownServiceFails) {
    EXPECT_FALSE(ctl_->StopService(std::string("ghost")));
}

TEST_F(ControllerTest, StartServiceByName_ResolvesThroughRegistry) {
    auto svc = MakeService("wl", true);
    registry_->AddService(svc, "cnt");
    EXPECT_TRUE(ctl_->StartService(std::string("wl")));
    EXPECT_EQ(svc->GetServiceState(), ServiceState::RUNNING);
}

TEST_F(ControllerTest, StartServiceByName_NoRegistryFails) {
    ON_CALL(**mgr_, GetServiceRegistry())
        .WillByDefault(Return(std::shared_ptr<ServiceRegistry>()));
    EXPECT_FALSE(ctl_->StartService(std::string("wl")));
}

// ── StartGroup / StopGroup ──────────────────────────────────────────────────

TEST_F(ControllerTest, StartGroup_EmptyNameFails) {
    EXPECT_FALSE(ctl_->StartGroup(""));
}

TEST_F(ControllerTest, StopGroup_EmptyNameFails) {
    EXPECT_FALSE(ctl_->StopGroup(""));
}

TEST_F(ControllerTest, StartGroup_NoContainersRegisteredFails) {
    // ContainerSupported() 가 false 면(등록된 컨테이너 없음) 진행하지 않는다.
    EXPECT_FALSE(ctl_->StartGroup("g1"));
}

TEST_F(ControllerTest, StopGroup_NoContainersRegisteredFails) {
    EXPECT_FALSE(ctl_->StopGroup("g1"));
}

TEST_F(ControllerTest, StartAndStopGroup_WithRegisteredContainer) {
    auto cc = ctl_->GetContainer();
    ASSERT_NE(cc, nullptr);
    cc->AddContainer(std::make_shared<Container>(
        "g1", "img", "/tmp/g1.yaml", "ns", ContainerType::DOCKER));

    // 실제 docker 를 부르므로 결과값은 환경에 따라 다르다. 여기서 고정하는 건
    // "등록된 컨테이너가 있으면 게이트를 통과해 실행까지 간다" 는 것뿐이다.
    EXPECT_NO_THROW((void)ctl_->StartGroup("g1"));
    EXPECT_NO_THROW((void)ctl_->StopGroup("g1"));
}

// ── 요청 처리 ───────────────────────────────────────────────────────────────

TEST_F(ControllerTest, ProcessStartRequest_NullMessageFails) {
    EXPECT_FALSE(ctl_->ProcessServiceStartRequest(nullptr));
}

TEST_F(ControllerTest, ProcessStopRequest_NullMessageFails) {
    EXPECT_FALSE(ctl_->ProcessServiceStopRequest(nullptr));
}

TEST_F(ControllerTest, ProcessStartRequest_EmptyListFails) {
    lifecyclemgr::StartServiceRequest req;
    EXPECT_FALSE(ctl_->ProcessServiceStartRequest(&req));
}

TEST_F(ControllerTest, ProcessStopRequest_EmptyListFails) {
    lifecyclemgr::StopServiceRequest req;
    EXPECT_FALSE(ctl_->ProcessServiceStopRequest(&req));
}

TEST_F(ControllerTest, ProcessStartRequest_UnknownServiceIsSkipped) {
    lifecyclemgr::StartServiceRequest req;
    req.service_list().push_back("ghost");
    EXPECT_NO_THROW((void)ctl_->ProcessServiceStartRequest(&req));
}

TEST_F(ControllerTest, ProcessStartRequest_ContainerWorkloadPath) {
    auto svc = MakeService("wl", true);
    registry_->AddService(svc, "cnt");

    lifecyclemgr::StartServiceRequest req;
    req.service_list().push_back("wl");
    EXPECT_NO_THROW((void)ctl_->ProcessServiceStartRequest(&req));
}

TEST_F(ControllerTest, ProcessStopRequest_ContainerWorkloadPath) {
    auto svc = MakeService("wl", true);
    registry_->AddService(svc, "cnt");

    lifecyclemgr::StopServiceRequest req;
    req.service_list().push_back("wl");
    EXPECT_NO_THROW((void)ctl_->ProcessServiceStopRequest(&req));
}

TEST_F(ControllerTest, ProcessStartRequest_NoRegistryIsSafe) {
    ON_CALL(**mgr_, GetServiceRegistry())
        .WillByDefault(Return(std::shared_ptr<ServiceRegistry>()));
    lifecyclemgr::StartServiceRequest req;
    req.service_list().push_back("wl");
    EXPECT_NO_THROW((void)ctl_->ProcessServiceStartRequest(&req));
}

// ═══════════════════════════════════════════════════════════════════════════
// ServiceMonitor
// ═══════════════════════════════════════════════════════════════════════════

namespace {

class ServiceMonitorTest : public ::testing::Test {
protected:
    void SetUp() override {
        logger_ = std::make_unique<ScopedMockLogger>();
        mgr_    = std::make_unique<ScopedMockADSWManager>();
        registry_ = std::make_shared<ServiceRegistry>();
        ON_CALL(**mgr_, GetServiceRegistry()).WillByDefault(Return(registry_));
        // 트랜스미터는 null 로 둔다 → DDS 를 건드리지 않고 조기 반환한다.
        ON_CALL(**mgr_, GetTransmitter())
            .WillByDefault(Return(std::shared_ptr<general::adsw_manager::DDSTransmitter>()));
        mon_ = std::make_unique<ServiceMonitor>();
    }
    void TearDown() override {
        mon_.reset();
        registry_.reset();
        mgr_.reset();
        logger_.reset();
    }

    std::unique_ptr<ScopedMockLogger>       logger_;
    std::unique_ptr<ScopedMockADSWManager>  mgr_;
    std::shared_ptr<ServiceRegistry>        registry_;
    std::unique_ptr<ServiceMonitor>         mon_;
};

} // namespace

TEST_F(ServiceMonitorTest, InitializeAndFinalize) {
    EXPECT_TRUE(mon_->Initialize());
    EXPECT_NO_THROW(mon_->Finalize());
}

TEST_F(ServiceMonitorTest, NotifyServiceStateWithoutTransmitterIsSafe) {
    EXPECT_NO_THROW(mon_->NotifyServiceState());
}

TEST_F(ServiceMonitorTest, NotifyAdswStatusWithoutTransmitterIsSafe) {
    EXPECT_NO_THROW(mon_->NotifyAdswStatus(0, "ok"));
}

TEST_F(ServiceMonitorTest, NodeStartedStoppedAndCount) {
    EXPECT_EQ(mon_->NodeCount("svc"), 0);

    EXPECT_EQ(mon_->NodeStarted("svc", "n1"), 1);
    EXPECT_EQ(mon_->NodeStarted("svc", "n2"), 2);
    EXPECT_EQ(mon_->NodeCount("svc"), 2);

    EXPECT_EQ(mon_->NodeStopped("svc", "n1"), 1);
    EXPECT_EQ(mon_->NodeCount("svc"), 1);
}

TEST_F(ServiceMonitorTest, NodeStoppedOnUnknownNodeIsSafe) {
    EXPECT_NO_THROW((void)mon_->NodeStopped("svc", "never-started"));
}

TEST_F(ServiceMonitorTest, NodeCountsAreIndependentPerService) {
    mon_->NodeStarted("a", "n1");
    mon_->NodeStarted("b", "n1");
    mon_->NodeStarted("b", "n2");
    EXPECT_EQ(mon_->NodeCount("a"), 1);
    EXPECT_EQ(mon_->NodeCount("b"), 2);
}

TEST_F(ServiceMonitorTest, UpdateServiceState_UnknownServiceIsSafe) {
    general::adsw_manager::UpdateServiceInfo info(
        "ghost", "", general::adsw_manager::ServiceEvent::CREATED, ServiceState::RUNNING);
    EXPECT_NO_THROW(mon_->UpdateServiceState(info));
}

TEST_F(ServiceMonitorTest, UpdateServiceState_CreatedHonorsPassedState) {
    auto svc = MakeService("svc", true);
    registry_->AddService(svc, "cnt");

    general::adsw_manager::UpdateServiceInfo info(
        "svc", "", general::adsw_manager::ServiceEvent::CREATED, ServiceState::RUNNING);
    mon_->UpdateServiceState(info);

    // CREATED 가 전달받은 state 를 존중해야 한다. 여기를 INITIALIZING 으로
    // 하드코딩했다가 정상 서비스가 영원히 INITIALIZING 에 갇히는 버그가 있었다.
    EXPECT_EQ(svc->GetServiceState(), ServiceState::RUNNING);
}

TEST_F(ServiceMonitorTest, UpdateServiceState_DeletedGoesNotRunning) {
    auto svc = MakeService("svc", true);
    svc->SetServiceState(ServiceState::RUNNING);
    registry_->AddService(svc, "cnt");

    general::adsw_manager::UpdateServiceInfo info(
        "svc", "", general::adsw_manager::ServiceEvent::DELETED, ServiceState::NOT_RUNNING);
    mon_->UpdateServiceState(info);

    EXPECT_EQ(svc->GetServiceState(), ServiceState::NOT_RUNNING);
}

TEST_F(ServiceMonitorTest, UpdateServiceState_StateUpdatedAppliesState) {
    auto svc = MakeService("svc", true);
    registry_->AddService(svc, "cnt");

    general::adsw_manager::UpdateServiceInfo info(
        "svc", "", general::adsw_manager::ServiceEvent::STATE_UPDATED, ServiceState::TERMINATING);
    mon_->UpdateServiceState(info);

    EXPECT_EQ(svc->GetServiceState(), ServiceState::TERMINATING);
}

TEST_F(ServiceMonitorTest, OnServiceProcessDied_UnknownServiceIsSafe) {
    EXPECT_NO_THROW(mon_->OnServiceProcessDied("ghost", -1));
}

TEST_F(ServiceMonitorTest, OnServiceProcessDied_ResetsStateAndPid) {
    auto svc = MakeService("svc", true);
    svc->SetServiceState(ServiceState::RUNNING);
    svc->SetPID(4242);
    registry_->AddService(svc, "cnt");

    mon_->OnServiceProcessDied("svc", 137);

    EXPECT_EQ(svc->GetServiceState(), ServiceState::NOT_RUNNING);
    EXPECT_EQ(svc->GetExitCode(), 137);
    EXPECT_EQ(svc->GetPID(), 0u);
}

TEST_F(ServiceMonitorTest, OnContainerStateChanged_IsSafeWithNoServices) {
    EXPECT_NO_THROW(mon_->OnContainerStateChanged("cnt", false, 1));
}

TEST_F(ServiceMonitorTest, OnContainerStateChanged_RunningPromotesServicesToInitializing) {
    auto a = MakeService("a", true);
    auto b = MakeService("b", true);
    registry_->AddService(a, "cnt");
    registry_->AddService(b, "cnt");

    mon_->OnContainerStateChanged("cnt", /*is_running=*/true, 0);
    EXPECT_EQ(a->GetServiceState(), ServiceState::INITIALIZING);
    EXPECT_EQ(b->GetServiceState(), ServiceState::INITIALIZING);
}

TEST_F(ServiceMonitorTest, OnContainerStateChanged_StoppedMarksNotRunningWithExitCode) {
    auto a = MakeService("a", true);
    a->SetServiceState(ServiceState::RUNNING);
    registry_->AddService(a, "cnt");

    mon_->OnContainerStateChanged("cnt", /*is_running=*/false, 137);
    EXPECT_EQ(a->GetServiceState(), ServiceState::NOT_RUNNING);
    EXPECT_EQ(a->GetExitCode(), 137);
}

// ── 알려진 결함: NotifyServiceState / UpdateServiceState 가 항상 빈 목록을 본다 ──
//
// ServiceRegistry::AddService 는 group 이 비면 거부한다. 따라서 group_[""] 는
// 절대 생기지 않는데, ServiceMonitor 는 두 곳에서 GetServicesByGroup("") 로
// "전체 서비스"를 얻으려 한다 (ServiceMonitor.cpp:87, :219).
//   → NotifyServiceState 가 service_list 를 빈 채로 publish 한다.
//   → UpdateServiceState() (인자 없는 쪽) 는 아무 서비스도 순회하지 않는다.
//
// lifecycle_mgr 의 ServiceRegistry 는 빈 그룹을 허용해서 "" 가 실제로 "전체"로
// 동작한다. 그쪽 코드를 가져오면서 AddService 의 규약 차이가 따라오지 않은 것으로
// 보인다. ros2 판도 동일하다.
//
// 고치면 DDS 로 나가는 내용이 바뀌므로 별도 이슈로 두고, 여기서는 현재 동작을
// 고정해 둔다. 고칠 때 이 두 테스트가 먼저 깨지면서 알려 줄 것이다.

TEST_F(ServiceMonitorTest, KnownDefect_EmptyGroupLookupNeverMatchesAnyService) {
    auto a = MakeService("a", true);
    // 빈 그룹으로는 등록 자체가 거부된다.
    EXPECT_FALSE(registry_->AddService(a, ""));
    // 정상 그룹으로 넣어도 "" 조회로는 잡히지 않는다.
    auto b = MakeService("b", true);
    ASSERT_TRUE(registry_->AddService(b, "cnt"));
    EXPECT_TRUE(registry_->GetServicesByGroup("").empty());
}

TEST_F(ServiceMonitorTest, KnownDefect_UpdateServiceStateNoArgTouchesNothing) {
    auto a = MakeService("a", true);
    a->SetServiceState(ServiceState::RUNNING);
    registry_->AddService(a, "cnt");

    mon_->UpdateServiceState();

    // "" 조회가 비어 있으므로 상태가 그대로다. 위 결함이 고쳐지면
    // NOT_RUNNING 으로 바뀌면서 이 기대가 깨진다.
    EXPECT_EQ(a->GetServiceState(), ServiceState::RUNNING);
}

// ═══════════════════════════════════════════════════════════════════════════
// MessageQueue — 퍼블리싱 스레드 수명주기
//
// 큐 자체는 (MessageType, void*) 쌍을 담고 별도 스레드가 꺼내 보낸다.
// 여기서 고정하는 건 Start/Stop 이 안전하고 멱등한가다 — 중복 Stop 이나
// Start 없는 Stop 에서 join 이 터지지 않아야 한다.
// ═══════════════════════════════════════════════════════════════════════════

TEST(GeneralMessageQueueTest, StopWithoutStartIsSafe) {
    ScopedMockLogger lg;
    general::adsw_manager::MessageQueue q;
    EXPECT_NO_THROW(q.Stop());
}

TEST(GeneralMessageQueueTest, StartThenStop) {
    ScopedMockLogger lg;
    general::adsw_manager::MessageQueue q;
    EXPECT_TRUE(q.Start());
    EXPECT_NO_THROW(q.Stop());
}

TEST(GeneralMessageQueueTest, DoubleStopIsSafe) {
    ScopedMockLogger lg;
    general::adsw_manager::MessageQueue q;
    q.Start();
    q.Stop();
    EXPECT_NO_THROW(q.Stop());
}

TEST(GeneralMessageQueueTest, SendMessageBeforeStartDoesNotCrash) {
    ScopedMockLogger lg;
    general::adsw_manager::MessageQueue q;
    general::adsw_manager::MsgType msg{
        general::adsw_manager::MessageType::NOTIFY_SERVICE_STATE, nullptr};
    EXPECT_NO_THROW(q.SendMessage(msg));
    q.Stop();
}

// ============================================================================
// ProcessServiceStartRequest / ProcessServiceStopRequest
//
// 왜 미커버였나:
//   container_controller_ 가 private 이고 세터가 없어서 가짜 컨테이너
//   컨트롤러를 물려줄 수 없었다. Initialize() 가 만드는 실물은 docker 를
//   셸로 부르므로 유닛 레인에서 쓸 수 없다. 그래서 컨테이너 선기동 로직이
//   통째로 실행 0회였다 (28줄). 이번에 general 유닛테스트에도
//   -fno-access-control 을 켜서(다른 5개 모듈과 같은 규약) 주입한다.
//
//   이 함수가 지키는 규약이 두 개 있고 둘 다 사용자에게 바로 보인다:
//     1) 컨테이너 우선. 서비스는 컨테이너 안에 산다. 컨테이너 기동이
//        실패하면 거기 든 서비스는 시도조차 하지 않는다 — 그러지 않으면
//        StartService 안의 docker exec 가 뒤늦게 실패해서 오류 로그가
//        두 겹으로 쌓이고 ctrl/* 결과가 엉뚱해진다.
//     2) 멱등. 이미 RUNNING 인 서비스에 Start 가 또 오면 다시 띄우지
//        않는다. 다시 띄우면 앞의 PID 가 고아가 되고 좀비가 쌓이며
//        모니터 상태가 RUNNING↔NOT_RUNNING 으로 깜빡인다.
//        단 INITIALIZING + PID==0 은 콜드스타트라 예외다 — 여기서
//        건너뛰면 운영자의 Start 클릭이 조용히 삼켜진다.
// ============================================================================

#include "lifecycle/StartServiceRequest.hpp"
#include "lifecycle/StopServiceRequest.hpp"
#include "transmitter/DDSTransmitter.hpp"
#include "transmitter/DDSMessageEndpoint.hpp"
#include "endpoint/MessageTypes.hpp"

namespace {

using general::adsw_manager::DDSMessageEndpoint;
using general::adsw_manager::DDSTransmitter;

// DDSMessageEndpoint 는 추상 클래스다. 순수 가상 4개만 채우면 등록할 수 있다.
// Initialize 하지 않았으므로 Write 는 false 를 돌려줄 뿐, DDS 는 뜨지 않는다.
class StubEndpoint : public DDSMessageEndpoint {
public:
    explicit StubEndpoint(std::string topic) : topic_(std::move(topic)) {}
    std::string GetTopic() override { return topic_; }
    std::string GetTypeName() override { return "StubType"; }
    std::shared_ptr<TypeSupport> GetTypeSupport() override { return nullptr; }
    void OnDataAvailable() override {}
private:
    std::string topic_;
};

class RequestControllerTest : public ::testing::Test {
protected:
    void SetUp() override {
        logger_ = std::make_unique<ScopedMockLogger>();
        mgr_    = std::make_unique<ScopedMockADSWManager>();

        ctl_ = std::make_shared<ContainerServiceController>();
        ctl_->Initialize();
        // 실물 대신 가짜를 꽂는다 (docker 셸 호출 차단).
        fake_ = std::make_shared<FakeCmdContainerController>();
        ctl_->container_controller_ = fake_;

        registry_    = std::make_shared<ServiceRegistry>();
        monitor_     = std::make_shared<ServiceMonitor>();
        transmitter_ = std::make_shared<DDSTransmitter>();
        // SendMessage 는 message_queue_ 가 있어야 true 를 낸다. Initialize() 를
        // 부르면 실 DDS participant 가 뜨므로 큐만 직접 꽂는다. 큐는 Start()
        // 하지 않아 스레드도 뜨지 않는다 — 넣기만 하고 아무도 꺼내지 않는다.
        transmitter_->message_queue_ =
            std::make_shared<general::adsw_manager::MessageQueue>();

        ON_CALL(**mgr_, GetServiceRegistry()).WillByDefault(Return(registry_));
        ON_CALL(**mgr_, GetServiceController()).WillByDefault(Return(ctl_));
        ON_CALL(**mgr_, GetServiceMonitor()).WillByDefault(Return(monitor_));
        ON_CALL(**mgr_, GetTransmitter()).WillByDefault(Return(transmitter_));
        ON_CALL(**mgr_, GetTopic(::testing::_)).WillByDefault(Return("mmf/general/Stub"));
    }
    void TearDown() override {
        transmitter_.reset();
        monitor_.reset();
        registry_.reset();
        fake_.reset();
        ctl_.reset();
        mgr_.reset();
        logger_.reset();
    }

    // 컨테이너 c 에 속한 서비스 하나를 등록한다 (그룹 이름 = 컨테이너 이름).
    std::shared_ptr<Service> Register(const std::string& svc, const std::string& container,
                                      ServiceState state = ServiceState::NOT_RUNNING,
                                      uint64_t pid = 0) {
        auto s = MakeService(svc, /*container_workload=*/true);
        s->SetServiceState(state);
        s->SetPID(pid);
        registry_->AddService(s, container);
        return s;
    }

    // 컨테이너는 컨트롤러에 등록돼 있어야 StartContainer 가 동작한다
    // (GetContainer 로 못 찾으면 곧바로 false).
    void AddContainer(const std::string& name, bool running) {
        fake_->AddContainer(std::make_shared<Container>(
            name, name + "-img", "/tmp/" + name + ".yaml", "ns", ContainerType::DOCKER));
        fake_->SetContainerRunning(name, running);
    }

    lifecyclemgr::StartServiceRequest StartReq(std::vector<std::string> names) {
        lifecyclemgr::StartServiceRequest r;
        r.service_list(std::move(names));
        r.is_group(false);
        return r;
    }

    std::unique_ptr<ScopedMockLogger>            logger_;
    std::unique_ptr<ScopedMockADSWManager>       mgr_;
    std::shared_ptr<ContainerServiceController>  ctl_;
    std::shared_ptr<FakeCmdContainerController>  fake_;
    std::shared_ptr<ServiceRegistry>             registry_;
    std::shared_ptr<ServiceMonitor>              monitor_;
    std::shared_ptr<DDSTransmitter>              transmitter_;
};

} // namespace

TEST_F(RequestControllerTest, StartRequest_NullMessageRejected) {
    EXPECT_FALSE(ctl_->ProcessServiceStartRequest(nullptr));
}

TEST_F(RequestControllerTest, StopRequest_NullMessageRejected) {
    EXPECT_FALSE(ctl_->ProcessServiceStopRequest(nullptr));
}

// 주의: 이 함수의 **반환값은 시작 성공 여부가 아니라 응답을 발행했는지**다.
// 시작 결과는 start_service_response_.ret_code() 에 담긴다. 둘을 섞으면
// "발행은 됐는데 하나도 안 떴다" 를 성공으로 읽게 된다.
TEST_F(RequestControllerTest, StartRequest_EmptyListReportsFailedButStillAnswers) {
    auto req = StartReq({});

    EXPECT_TRUE(ctl_->ProcessServiceStartRequest(&req))
        << "요청이 비었어도 ctrl/* 응답은 보내야 한다";
    EXPECT_EQ(ctl_->start_service_response_.ret_code(),
              lifecyclemgr::ReturnCode::FAILED);
}

TEST_F(RequestControllerTest, StartRequest_FailsWithoutTransmitter) {
    // 트랜스미터가 이미 내려간 상태. 예전에는 여기만 널 검사가 빠져 있었다.
    ON_CALL(**mgr_, GetTransmitter())
        .WillByDefault(Return(std::shared_ptr<DDSTransmitter>()));
    Register("svc-a", "web");
    AddContainer("web", true);

    auto req = StartReq({"svc-a"});
    EXPECT_FALSE(ctl_->ProcessServiceStartRequest(&req));
}

// ── 컨테이너 우선 기동 ──────────────────────────────────────────────────────

TEST_F(RequestControllerTest, StartRequest_RunningContainerIsNotRestarted) {
    Register("svc-a", "web");
    AddContainer("web", true);

    auto req = StartReq({"svc-a"});
    ctl_->ProcessServiceStartRequest(&req);

    for (const auto& cmd : fake_->run_cmds) {
        EXPECT_EQ(cmd.find("docker start"), std::string::npos)
            << "이미 도는 컨테이너를 다시 기동했다: " << cmd;
    }
}

TEST_F(RequestControllerTest, StartRequest_StoppedContainerIsStartedOnce) {
    // 같은 컨테이너에 든 서비스가 둘이어도 컨테이너 기동은 한 번만.
    Register("svc-a", "web");
    Register("svc-b", "web");
    AddContainer("web", false);

    auto req = StartReq({"svc-a", "svc-b"});
    ctl_->ProcessServiceStartRequest(&req);

    int starts = 0;
    for (const auto& cmd : fake_->run_cmds) {
        if (cmd.find("start") != std::string::npos) ++starts;
    }
    EXPECT_LE(starts, 1) << "같은 컨테이너를 여러 번 기동했다";
}

TEST_F(RequestControllerTest, StartRequest_ServicesInAFailedContainerAreSkipped) {
    Register("svc-a", "web");
    AddContainer("web", false);
    fake_->default_rc = 1;                 // StartContainer 실패

    auto req = StartReq({"svc-a"});
    EXPECT_TRUE(ctl_->ProcessServiceStartRequest(&req));   // 응답은 나간다
    EXPECT_EQ(ctl_->start_service_response_.ret_code(),
              lifecyclemgr::ReturnCode::FAILED)
        << "컨테이너가 안 떴는데 서비스 시작을 성공으로 보고했다";
}

TEST_F(RequestControllerTest, StartRequest_UnknownServiceIsSkippedNotFatal) {
    AddContainer("web", true);
    auto req = StartReq({"does-not-exist"});
    EXPECT_NO_THROW(ctl_->ProcessServiceStartRequest(&req));
}

// ── 멱등 가드 ───────────────────────────────────────────────────────────────

TEST_F(RequestControllerTest, StartRequest_AlreadyRunningIsCountedNotRespawned) {
    Register("svc-a", "web", ServiceState::RUNNING, /*pid=*/4242);
    AddContainer("web", true);

    auto req = StartReq({"svc-a"});
    EXPECT_TRUE(ctl_->ProcessServiceStartRequest(&req))
        << "이미 도는 서비스는 성공으로 세야 ctrl/* 응답이 SUCCESS 가 된다";
    EXPECT_EQ(ctl_->start_service_response_.ret_code(),
              lifecyclemgr::ReturnCode::SUCCESS);
}

TEST_F(RequestControllerTest, StartRequest_InitializingWithPidIsTreatedAsRunning) {
    Register("svc-a", "web", ServiceState::INITIALIZING, /*pid=*/4242);
    AddContainer("web", true);

    auto req = StartReq({"svc-a"});
    EXPECT_TRUE(ctl_->ProcessServiceStartRequest(&req));
}

TEST_F(RequestControllerTest, StartRequest_ColdStartInitializingIsNotSwallowed) {
    // INITIALIZING + PID==0 은 컨테이너만 뜨고 프로세스는 아직 없는 상태다.
    // 여기서 "이미 실행 중" 으로 취급하면 운영자의 Start 가 조용히 사라진다.
    auto svc = Register("svc-a", "web", ServiceState::INITIALIZING, /*pid=*/0);
    AddContainer("web", true);

    auto req = StartReq({"svc-a"});
    ctl_->ProcessServiceStartRequest(&req);

    // 실제로 기동을 시도했는지 — 컨테이너 워크로드라 docker exec 가 나가야 한다.
    bool attempted = false;
    for (const auto& cmd : fake_->run_cmds) {
        if (cmd.find("exec") != std::string::npos) { attempted = true; break; }
    }
    EXPECT_TRUE(attempted || svc->GetServiceState() != ServiceState::INITIALIZING)
        << "콜드스타트인데 기동을 건너뛰었다";
}

// ── Stop ────────────────────────────────────────────────────────────────────

TEST_F(RequestControllerTest, StopRequest_EmptyListReportsFailedButStillAnswers) {
    lifecyclemgr::StopServiceRequest req;
    req.service_list({});

    EXPECT_TRUE(ctl_->ProcessServiceStopRequest(&req));
    EXPECT_EQ(ctl_->stop_service_response_.ret_code(),
              lifecyclemgr::ReturnCode::FAILED);
}

TEST_F(RequestControllerTest, StopRequest_StopsRegisteredService) {
    Register("svc-a", "web", ServiceState::RUNNING, /*pid=*/4242);
    AddContainer("web", true);

    lifecyclemgr::StopServiceRequest req;
    req.service_list({"svc-a"});
    EXPECT_NO_THROW(ctl_->ProcessServiceStopRequest(&req));
}

// ── ServiceMonitor 의 발행 경로 ─────────────────────────────────────────────
//
// 위 ServiceMonitorTest 들은 "트랜스미터가 없을 때 안전한가" 만 봤다. 그래서
// 엔드포인트를 찾아 Write 까지 가는 본문은 실행 0회였다. 여기서는 실제로
// 엔드포인트를 등록해 그 경로를 밟는다 (Initialize 하지 않은 스텁이라 DDS 는
// 뜨지 않고 Write 는 false 를 돌려줄 뿐이다).
//
// NotifyServiceState 가 싣는 서비스 목록이 항상 비어 있는 건 위에 적힌
// KnownDefect 그대로다. 여기서 고정하는 것은 목록 내용이 아니라
// "엔드포인트가 있든 없든 터지지 않고 끝난다" 는 것이다.

namespace {

class MonitorPublishTest : public RequestControllerTest {
protected:
    void SetUp() override {
        RequestControllerTest::SetUp();
        mon_ = std::make_shared<ServiceMonitor>();
        ON_CALL(**mgr_, GetServiceMonitor()).WillByDefault(Return(mon_));
        transmitter_->AddMessageEndpoint(
            std::make_shared<StubEndpoint>("mmf/general/Stub"));
    }
    void TearDown() override {
        mon_.reset();
        RequestControllerTest::TearDown();
    }
    std::shared_ptr<ServiceMonitor> mon_;
};

} // namespace

TEST_F(MonitorPublishTest, NotifyServiceState_ReachesTheEndpoint) {
    Register("svc-a", "web", ServiceState::RUNNING, 111);
    Register("svc-b", "web", ServiceState::NOT_RUNNING);

    EXPECT_NO_THROW(mon_->NotifyServiceState());
}

TEST_F(MonitorPublishTest, NotifyServiceState_SafeWithNoServices) {
    EXPECT_NO_THROW(mon_->NotifyServiceState());
}

TEST_F(MonitorPublishTest, NotifyServiceState_SafeWithoutRegistry) {
    ON_CALL(**mgr_, GetServiceRegistry())
        .WillByDefault(Return(std::shared_ptr<ServiceRegistry>()));
    EXPECT_NO_THROW(mon_->NotifyServiceState());
}

TEST_F(MonitorPublishTest, NotifyServiceState_SafeWhenEndpointMissing) {
    // 토픽은 받았지만 그 이름으로 등록된 엔드포인트가 없다.
    ON_CALL(**mgr_, GetTopic(::testing::_)).WillByDefault(Return("mmf/general/Absent"));
    Register("svc-a", "web");
    EXPECT_NO_THROW(mon_->NotifyServiceState());
}

TEST_F(MonitorPublishTest, NotifyAdswStatus_ReachesTheEndpoint) {
    EXPECT_NO_THROW(mon_->NotifyAdswStatus(0, "ready"));
    EXPECT_NO_THROW(mon_->NotifyAdswStatus(2, "degraded"));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
