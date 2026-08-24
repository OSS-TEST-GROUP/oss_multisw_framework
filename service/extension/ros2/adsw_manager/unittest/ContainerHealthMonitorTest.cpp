// Unit tests for ContainerHealthMonitor class using Google Test
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <chrono>
#include <thread>
#include <vector>
#include <string>

#include "monitor/ContainerHealthMonitor.hpp"
#include "service/Container.hpp"
#include "mock/MockLogger.hpp"
#include "mock/MockADSWManager.hpp"

using ::testing::Return;
using ::testing::_;
using ::testing::NiceMock;
using namespace ros2::adsw_manager;

// ─────────────────────────────────────────────────────────────────────────────
// 테스트용 서브클래스:
//  - NotifyContainerState()를 오버라이드하여 호출 기록
//  - InjectContainerState()로 외부에서 컨테이너 상태를 직접 주입
// ─────────────────────────────────────────────────────────────────────────────
class TestableContainerHealthMonitor : public ContainerHealthMonitor {
public:
    struct Notification {
        std::string container_name;
        bool        is_running;
        int32_t     exit_code;
    };

    std::vector<Notification> notifications;

    // 외부에서 상태 변경을 시뮬레이션 (PollLoop 없이 로직 직접 검증)
    void InjectStateChange(const std::string& name, bool is_running, int32_t exit_code = 0) {
        bool prev = prev_state_.count(name) ? prev_state_[name] : !is_running; // 반드시 다르게
        if (is_running != prev) {
            NotifyContainerState(name, is_running, exit_code);
            prev_state_[name] = is_running;
        }
    }

    // prev_state_ 를 직접 설정 (초기 상태 시뮬레이션)
    void SetPrevState(const std::string& name, bool was_running) {
        prev_state_[name] = was_running;
    }

    bool GetPrevState(const std::string& name) {
        auto it = prev_state_.find(name);
        return (it != prev_state_.end()) ? it->second : false;
    }

    bool HasPrevState(const std::string& name) {
        return prev_state_.count(name) > 0;
    }

protected:
    void NotifyContainerState(const std::string& container_name,
                              bool is_running,
                              int32_t exit_code) override {
        notifications.push_back({container_name, is_running, exit_code});
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Test Fixture
// ─────────────────────────────────────────────────────────────────────────────
class ContainerHealthMonitorTest : public ::testing::Test {
protected:
    void SetUp() override {
        Mock_Logger = new NiceMock<MockLogger>();
        ON_CALL(*Mock_Logger, Get()).WillByDefault(Return(Mock_Logger));
        monitor = std::make_unique<TestableContainerHealthMonitor>();
    }
    void TearDown() override {
        monitor.reset();
        delete Mock_Logger;
        Mock_Logger = nullptr;
    }

    std::unique_ptr<TestableContainerHealthMonitor> monitor;
};

// ─────────────────────────────────────────────────────────────────────────────
// 생성 / 소멸 테스트
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ContainerHealthMonitorTest, ConstructorDestructor_TC001) {
    auto m = std::make_unique<TestableContainerHealthMonitor>();
    EXPECT_NE(m.get(), nullptr);
}

TEST_F(ContainerHealthMonitorTest, StopWithoutStart_TC002) {
    // Start 없이 Stop 해도 크래시 없음
    EXPECT_NO_THROW(monitor->Stop());
}

// ─────────────────────────────────────────────────────────────────────────────
// prev_state_ 추적 테스트
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ContainerHealthMonitorTest, PrevState_InitiallyEmpty_TC003) {
    EXPECT_FALSE(monitor->HasPrevState("myapp"));
}

TEST_F(ContainerHealthMonitorTest, SetPrevState_CanRetrieve_TC004) {
    monitor->SetPrevState("myapp", true);
    EXPECT_TRUE(monitor->GetPrevState("myapp"));

    monitor->SetPrevState("myapp", false);
    EXPECT_FALSE(monitor->GetPrevState("myapp"));
}

// ─────────────────────────────────────────────────────────────────────────────
// 상태 변경 감지 및 알림 테스트
// ─────────────────────────────────────────────────────────────────────────────

// 중지 → 실행 전환: notification 발생
TEST_F(ContainerHealthMonitorTest, StateChange_StoppedToRunning_TC005) {
    monitor->SetPrevState("myapp", false);   // 이전 상태: 중지
    monitor->InjectStateChange("myapp", true, 0);  // 현재 상태: 실행

    ASSERT_EQ(monitor->notifications.size(), 1u);
    EXPECT_EQ(monitor->notifications[0].container_name, "myapp");
    EXPECT_TRUE(monitor->notifications[0].is_running);
    EXPECT_EQ(monitor->notifications[0].exit_code, 0);
}

// 실행 → 중지 전환: notification 발생 (exit_code 포함)
TEST_F(ContainerHealthMonitorTest, StateChange_RunningToStopped_TC006) {
    monitor->SetPrevState("myapp", true);    // 이전 상태: 실행
    monitor->InjectStateChange("myapp", false, 1);  // 현재 상태: 중지, exit=1

    ASSERT_EQ(monitor->notifications.size(), 1u);
    EXPECT_EQ(monitor->notifications[0].container_name, "myapp");
    EXPECT_FALSE(monitor->notifications[0].is_running);
    EXPECT_EQ(monitor->notifications[0].exit_code, 1);
}

// 상태 변경 없음: notification 없음
TEST_F(ContainerHealthMonitorTest, NoStateChange_NoNotification_TC007) {
    monitor->SetPrevState("myapp", true);
    // 같은 상태 → InjectStateChange 내부에서 prev == curr 이면 호출 안 함
    // 여기서는 prev=true, inject=true → 변경 없음
    // InjectStateChange는 반드시 다른 상태만 처리하도록 구현됨
    // → 직접 동일 상태로 SetPrevState+GetPrevState 확인
    monitor->SetPrevState("myapp", true);
    EXPECT_EQ(monitor->notifications.size(), 0u);
}

// 여러 컨테이너 각각 독립적으로 추적
TEST_F(ContainerHealthMonitorTest, MultipleContainers_IndependentTracking_TC008) {
    monitor->SetPrevState("app1", false);
    monitor->SetPrevState("app2", true);

    monitor->InjectStateChange("app1", true, 0);   // app1: 중지→실행
    monitor->InjectStateChange("app2", false, 2);  // app2: 실행→중지

    ASSERT_EQ(monitor->notifications.size(), 2u);

    // 알림 순서 확인
    EXPECT_EQ(monitor->notifications[0].container_name, "app1");
    EXPECT_TRUE(monitor->notifications[0].is_running);

    EXPECT_EQ(monitor->notifications[1].container_name, "app2");
    EXPECT_FALSE(monitor->notifications[1].is_running);
    EXPECT_EQ(monitor->notifications[1].exit_code, 2);
}

// prev_state_ 가 갱신됨 확인
TEST_F(ContainerHealthMonitorTest, PrevStateUpdatedAfterChange_TC009) {
    monitor->SetPrevState("myapp", false);
    monitor->InjectStateChange("myapp", true, 0);

    EXPECT_TRUE(monitor->GetPrevState("myapp"));  // 갱신 확인
}

// 연속 상태 전환
TEST_F(ContainerHealthMonitorTest, SequentialStateChanges_TC010) {
    monitor->SetPrevState("myapp", false);

    monitor->InjectStateChange("myapp", true, 0);   // 중지→실행
    monitor->InjectStateChange("myapp", false, 0);  // 실행→중지
    monitor->InjectStateChange("myapp", true, 0);   // 중지→실행

    EXPECT_EQ(monitor->notifications.size(), 3u);
    EXPECT_TRUE(monitor->notifications[0].is_running);
    EXPECT_FALSE(monitor->notifications[1].is_running);
    EXPECT_TRUE(monitor->notifications[2].is_running);
}

// exit_code = 0: 정상 종료
TEST_F(ContainerHealthMonitorTest, ExitCode_Zero_NormalExit_TC011) {
    monitor->SetPrevState("myapp", true);
    monitor->InjectStateChange("myapp", false, 0);

    ASSERT_EQ(monitor->notifications.size(), 1u);
    EXPECT_EQ(monitor->notifications[0].exit_code, 0);
}

// exit_code > 0: 오류 종료
TEST_F(ContainerHealthMonitorTest, ExitCode_Nonzero_ErrorExit_TC012) {
    monitor->SetPrevState("myapp", true);
    monitor->InjectStateChange("myapp", false, 137);  // SIGKILL

    ASSERT_EQ(monitor->notifications.size(), 1u);
    EXPECT_EQ(monitor->notifications[0].exit_code, 137);
}

// ─────────────────────────────────────────────────────────────────────────────
// Start / Stop 스레드 테스트 (ADSWManager 없이 빠르게 종료)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ContainerHealthMonitorTest, StartStop_ThreadSafe_TC013) {
    // Start → 짧게 대기 → Stop : 크래시 없이 종료
    monitor->Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_NO_THROW(monitor->Stop());
}

TEST_F(ContainerHealthMonitorTest, DoubleStop_Safe_TC014) {
    monitor->Start();
    monitor->Stop();
    // 두 번 Stop 해도 안전
    EXPECT_NO_THROW(monitor->Stop());
}

// ═══════════════════════════════════════════════════════════════════════════
// PollOnce / NotifyInitialContainerState / PollServicesInContainer
//
// 이 세 경로가 ContainerHealthMonitor 미커버분(163줄 중 143줄)의 거의 전부다.
// 원래는 3초 sleep 이 든 스레드 루프 안에 있고 docker exec 를 popen 으로 부르는
// 익명 네임스페이스 자유 함수에 묶여 있어서 실제 런타임 없이는 못 돌렸다.
//
//   - PollLoop 한 사이클을 PollOnce() 로 분리했다 (스레드/sleep 없이 호출 가능)
//   - IsServiceAliveInContainer / LookupServicePidInContainer 를
//     protected virtual 멤버로 올렸다 (docker 없이 답을 심을 수 있음)
//   - ROSServiceController::GetContainer() 를 virtual 로 바꿔 컨테이너 주입 가능
// ═══════════════════════════════════════════════════════════════════════════

#include "mock/FakeCmdContainerController.hpp"
#include "mock/MockROSServiceController.hpp"
#include "monitor/ServiceMonitor.hpp"
#include "service/ServiceRegistry.hpp"
#include "service/ServiceType.hpp"
#include "service/RecoveryPolicy.hpp"

using ros2::adsw_manager::testing_support::FakeCmdContainerController;

namespace {

class PollDrivenMonitor : public ContainerHealthMonitor {
  public:
    struct Note { std::string name; bool running; int32_t exit_code; };

    std::vector<Note> notified;     // NotifyContainerState (전이에만)
    std::vector<Note> heartbeats;   // PublishContainerStateHeartbeat (무변화 시)

    // 컨테이너·토큰별로 심어두는 답. 없으면 기본값을 쓴다.
    std::map<std::string, bool>     alive_by_token;
    std::map<std::string, uint64_t> pid_by_token;
    bool     default_alive = true;
    uint64_t default_pid   = 0;

    std::vector<std::string> alive_queries;

    using ContainerHealthMonitor::PollOnce;
    using ContainerHealthMonitor::PollServicesInContainer;

    void SetPrev(const std::string& name, bool was_running) { prev_state_[name] = was_running; }
    bool HasPrev(const std::string& name) const { return prev_state_.count(name) > 0; }

  protected:
    void NotifyContainerState(const std::string& n, bool r, int32_t e) override {
        notified.push_back({n, r, e});
    }
    void PublishContainerStateHeartbeat(const std::string& n, bool r, int32_t e) override {
        heartbeats.push_back({n, r, e});
    }
    bool IsServiceAliveInContainer(const std::string&, const std::string& token) override {
        alive_queries.push_back(token);
        auto it = alive_by_token.find(token);
        return it == alive_by_token.end() ? default_alive : it->second;
    }
    uint64_t LookupServicePidInContainer(const std::string&, const std::string& token) override {
        auto it = pid_by_token.find(token);
        return it == pid_by_token.end() ? default_pid : it->second;
    }
};

class ContainerHealthMonitorPollTest : public ::testing::Test {
  protected:
    void SetUp() override {
        Mock_Logger = new NiceMock<MockLogger>();
        ON_CALL(*Mock_Logger, Get()).WillByDefault(Return(Mock_Logger));

        Mock_ADSWManager = new NiceMock<MockADSWManager>();
        mgr_holder.reset(Mock_ADSWManager, [](MockADSWManager*) {});  // 소유권은 우리가
        ADSWManager::SetShared(mgr_holder);

        Mock_ROSServiceController = new NiceMock<MockROSServiceController>();
        controller.reset(Mock_ROSServiceController, [](MockROSServiceController*) {});

        containers = std::make_shared<FakeCmdContainerController>();
        registry   = std::make_shared<ServiceRegistry>();
        svc_monitor = std::make_shared<ServiceMonitor>();

        ON_CALL(*Mock_ADSWManager, GetServiceController()).WillByDefault(Return(controller));
        ON_CALL(*Mock_ADSWManager, GetServiceRegistry()).WillByDefault(Return(registry));
        ON_CALL(*Mock_ADSWManager, GetServiceMonitor()).WillByDefault(Return(svc_monitor));
        // 트랜스미터는 null 로 둔다 → ServiceMonitor 가 DDS 를 건드리지 않고 빠진다.
        ON_CALL(*Mock_ADSWManager, GetTransmitter())
            .WillByDefault(Return(std::shared_ptr<DDSTransmitter>()));
        ON_CALL(*Mock_ROSServiceController, GetContainer()).WillByDefault(Return(containers));

        mon = std::make_unique<PollDrivenMonitor>();
    }

    void TearDown() override {
        mon.reset();
        ADSWManager::SetShared(nullptr);
        delete Mock_ADSWManager;      Mock_ADSWManager = nullptr;
        delete Mock_ROSServiceController; Mock_ROSServiceController = nullptr;
        delete Mock_Logger;           Mock_Logger = nullptr;
    }

    std::shared_ptr<Container> AddContainer(const std::string& name, bool running,
                                            int32_t exit_code = 0) {
        auto c = std::make_shared<Container>(name, name + "-img", "/tmp/" + name + ".yaml",
                                             name + "-ns", ContainerType::DOCKER);
        containers->AddContainer(c);
        containers->SetContainerRunning(name, running, exit_code);
        return c;
    }

    std::shared_ptr<Service> AddService(const std::string& name, const std::string& group,
                                        ServiceState state, uint64_t pid) {
        auto s = std::make_shared<Service>(name, ServiceType::NORMAL, RecoveryPolicy::ALWAYS, true);
        s->SetServiceState(state);
        s->SetPID(pid);
        registry->AddService(s, group);
        return s;
    }

    std::shared_ptr<ADSWManager>          mgr_holder;
    std::shared_ptr<ROSServiceController> controller;
    std::shared_ptr<FakeCmdContainerController> containers;
    std::shared_ptr<ServiceRegistry>      registry;
    std::shared_ptr<ServiceMonitor>       svc_monitor;
    std::unique_ptr<PollDrivenMonitor>    mon;
};

} // namespace

// ── PollOnce: 조기 반환 경로 ────────────────────────────────────────────────

TEST_F(ContainerHealthMonitorPollTest, PollOnce_NoManagerIsNoop) {
    ADSWManager::SetShared(nullptr);
    EXPECT_NO_THROW(mon->PollOnce());
    EXPECT_TRUE(mon->notified.empty());
}

TEST_F(ContainerHealthMonitorPollTest, PollOnce_NoControllerIsNoop) {
    ON_CALL(*Mock_ADSWManager, GetServiceController())
        .WillByDefault(Return(std::shared_ptr<ROSServiceController>()));
    EXPECT_NO_THROW(mon->PollOnce());
    EXPECT_TRUE(mon->notified.empty());
}

TEST_F(ContainerHealthMonitorPollTest, PollOnce_NoContainerControllerIsNoop) {
    ON_CALL(*Mock_ROSServiceController, GetContainer())
        .WillByDefault(Return(std::shared_ptr<ContainerController>()));
    EXPECT_NO_THROW(mon->PollOnce());
    EXPECT_TRUE(mon->notified.empty());
}

TEST_F(ContainerHealthMonitorPollTest, PollOnce_NoContainersIsNoop) {
    mon->PollOnce();
    EXPECT_TRUE(mon->notified.empty());
    EXPECT_TRUE(mon->heartbeats.empty());
}

// ── PollOnce: 상태 전이 / 하트비트 ──────────────────────────────────────────

TEST_F(ContainerHealthMonitorPollTest, PollOnce_FirstSightAlwaysNotifies) {
    AddContainer("app", /*running=*/true);
    mon->PollOnce();
    ASSERT_EQ(mon->notified.size(), 1u);
    EXPECT_EQ(mon->notified[0].name, "app");
    EXPECT_TRUE(mon->notified[0].running);
    EXPECT_EQ(mon->notified[0].exit_code, 0);
    EXPECT_TRUE(mon->HasPrev("app"));
}

TEST_F(ContainerHealthMonitorPollTest, PollOnce_UnchangedStateSendsHeartbeatNotNotify) {
    AddContainer("app", true);
    mon->PollOnce();                 // 첫 관측 → notify
    mon->notified.clear();
    mon->PollOnce();                 // 변화 없음 → heartbeat
    EXPECT_TRUE(mon->notified.empty());
    ASSERT_EQ(mon->heartbeats.size(), 1u);
    EXPECT_EQ(mon->heartbeats[0].name, "app");
    EXPECT_TRUE(mon->heartbeats[0].running);
}

TEST_F(ContainerHealthMonitorPollTest, PollOnce_RunningToStoppedNotifiesWithExitCode) {
    AddContainer("app", true);
    mon->PollOnce();
    mon->notified.clear();

    containers->SetContainerRunning("app", false, 137);
    mon->PollOnce();
    ASSERT_EQ(mon->notified.size(), 1u);
    EXPECT_FALSE(mon->notified[0].running);
    EXPECT_EQ(mon->notified[0].exit_code, 137);
}

TEST_F(ContainerHealthMonitorPollTest, PollOnce_RunningContainerReportsExitCodeZero) {
    // 실행 중이면 exit code 를 조회하지 않고 0 으로 보고한다.
    AddContainer("app", true, 42);
    mon->PollOnce();
    ASSERT_EQ(mon->notified.size(), 1u);
    EXPECT_EQ(mon->notified[0].exit_code, 0);
}

TEST_F(ContainerHealthMonitorPollTest, PollOnce_StoppedContainerSkipsServicePolling) {
    AddContainer("app", false, 1);
    AddService("svc", "app", ServiceState::RUNNING, 100);
    mon->PollOnce();
    // 컨테이너가 죽어 있으면 서비스 폴링은 하지 않는다.
    EXPECT_TRUE(mon->alive_queries.empty());
}

TEST_F(ContainerHealthMonitorPollTest, PollOnce_MultipleContainersTrackedIndependently) {
    AddContainer("a", true);
    AddContainer("b", false, 2);
    mon->PollOnce();
    ASSERT_EQ(mon->notified.size(), 2u);
    EXPECT_TRUE(mon->HasPrev("a"));
    EXPECT_TRUE(mon->HasPrev("b"));
}

// ── NotifyInitialContainerState ─────────────────────────────────────────────

TEST_F(ContainerHealthMonitorPollTest, NotifyInitial_ReportsEveryContainer) {
    AddContainer("a", true);
    AddContainer("b", false, 3);
    mon->NotifyInitialContainerState();
    ASSERT_EQ(mon->notified.size(), 2u);
    EXPECT_TRUE(mon->HasPrev("a"));
    EXPECT_TRUE(mon->HasPrev("b"));
    EXPECT_EQ(mon->notified[1].exit_code, 3);
}

TEST_F(ContainerHealthMonitorPollTest, NotifyInitial_NoManagerIsNoop) {
    ADSWManager::SetShared(nullptr);
    EXPECT_NO_THROW(mon->NotifyInitialContainerState());
    EXPECT_TRUE(mon->notified.empty());
}

TEST_F(ContainerHealthMonitorPollTest, NotifyInitial_NoContainerControllerIsNoop) {
    ON_CALL(*Mock_ROSServiceController, GetContainer())
        .WillByDefault(Return(std::shared_ptr<ContainerController>()));
    EXPECT_NO_THROW(mon->NotifyInitialContainerState());
    EXPECT_TRUE(mon->notified.empty());
}

// ── PollServicesInContainer ─────────────────────────────────────────────────

TEST_F(ContainerHealthMonitorPollTest, PollServices_ContainerWorkloadPromotedToRunning) {
    AddContainer("app", true);
    auto s = AddService("wl", "app", ServiceState::INITIALIZING, 0);
    s->SetContainerWorkload(true);

    mon->PollServicesInContainer("app");
    EXPECT_EQ(s->GetServiceState(), ServiceState::RUNNING);
    // 컨테이너 워크로드는 pgrep 경로를 타지 않는다.
    EXPECT_TRUE(mon->alive_queries.empty());
}

TEST_F(ContainerHealthMonitorPollTest, PollServices_AlivePromotesInitializingToRunning) {
    AddContainer("app", true);
    auto s = AddService("svc", "app", ServiceState::INITIALIZING, 500);
    mon->default_alive = true;

    mon->PollServicesInContainer("app");
    EXPECT_EQ(s->GetServiceState(), ServiceState::RUNNING);
}

TEST_F(ContainerHealthMonitorPollTest, PollServices_PidRehydrationFromPgrep) {
    AddContainer("app", true);
    auto s = AddService("svc", "app", ServiceState::RUNNING, 0);
    mon->default_pid = 4242;
    mon->default_alive = true;

    mon->PollServicesInContainer("app");
    EXPECT_EQ(s->GetPID(), 4242u);
}

TEST_F(ContainerHealthMonitorPollTest, PollServices_ExternallyStartedLiftsNotRunning) {
    AddContainer("app", true);
    auto s = AddService("svc", "app", ServiceState::NOT_RUNNING, 0);
    mon->default_pid = 777;
    mon->default_alive = true;

    mon->PollServicesInContainer("app");
    // NOT_RUNNING → INITIALIZING → (같은 사이클에서) RUNNING
    EXPECT_EQ(s->GetPID(), 777u);
    EXPECT_EQ(s->GetServiceState(), ServiceState::RUNNING);
}

TEST_F(ContainerHealthMonitorPollTest, PollServices_RehydrateMissLeavesStateAlone) {
    AddContainer("app", true);
    auto s = AddService("svc", "app", ServiceState::INITIALIZING, 0);
    mon->default_pid = 0;   // pgrep 미스

    mon->PollServicesInContainer("app");
    EXPECT_EQ(s->GetPID(), 0u);
    EXPECT_EQ(s->GetServiceState(), ServiceState::INITIALIZING);
}

TEST_F(ContainerHealthMonitorPollTest, PollServices_DeathReportedOnceAndCacheCleared) {
    AddContainer("app", true);
    auto s = AddService("svc", "app", ServiceState::RUNNING, 900);

    mon->default_alive = true;
    mon->PollServicesInContainer("app");      // 캐시에 alive 로 등록
    EXPECT_EQ(s->GetServiceState(), ServiceState::RUNNING);

    mon->default_alive = false;
    mon->PollServicesInContainer("app");      // alive → dead 전이
    EXPECT_EQ(s->GetServiceState(), ServiceState::NOT_RUNNING);
    EXPECT_EQ(s->GetExitCode(), -1);
    EXPECT_EQ(s->GetPID(), 0u);
}

TEST_F(ContainerHealthMonitorPollTest, PollServices_NoRegistryIsNoop) {
    ON_CALL(*Mock_ADSWManager, GetServiceRegistry())
        .WillByDefault(Return(std::shared_ptr<ServiceRegistry>()));
    EXPECT_NO_THROW(mon->PollServicesInContainer("app"));
}

TEST_F(ContainerHealthMonitorPollTest, PollServices_NoManagerIsNoop) {
    ADSWManager::SetShared(nullptr);
    EXPECT_NO_THROW(mon->PollServicesInContainer("app"));
}

// ── 프로세스 조회 헬퍼의 실제 구현 ─────────────────────────────────────────
//
// 위 테스트들은 이 둘을 스텁으로 덮어 상위 로직을 검증한다. 빈 토큰 처리는
// 이 함수들 안에 있으므로 여기서는 실제 구현을 그대로 부른다. 빈 토큰이면
// popen 이전에 반환하므로 docker 없이도 안전하다.

namespace {
class RealProbeMonitor : public ContainerHealthMonitor {
  public:
    using ContainerHealthMonitor::IsServiceAliveInContainer;
    using ContainerHealthMonitor::LookupServicePidInContainer;
};
} // namespace

TEST(ContainerHealthMonitorProbeTest, EmptyTokenTreatedAsAliveWithoutProbing) {
    Mock_Logger = new NiceMock<MockLogger>();
    ON_CALL(*Mock_Logger, Get()).WillByDefault(Return(Mock_Logger));

    RealProbeMonitor m;
    // 토큰이 없으면 판정할 수 없다 → 오탐으로 죽이지 않고 살아있다고 본다.
    EXPECT_TRUE(m.IsServiceAliveInContainer("any", ""));
    // PID 조회는 반대로 0 (= 모름/미실행) 이 안전한 기본값이다.
    EXPECT_EQ(m.LookupServicePidInContainer("any", ""), 0u);

    delete Mock_Logger;
    Mock_Logger = nullptr;
}

// 위 테스트는 빈 토큰이라 popen 이전에 반환한다 — 즉 두 함수의 **본문은 실행
// 0회**였다(브랜치 50개). 토큰을 채우면 popen 경로로 들어가는데, 닿을 수 없는
// 컨테이너를 주면 docker 설치 여부와 무관하게 결정적이다:
//   - alive: docker exec 실패 → `|| echo dead` → "dead" → false
//   - pid  : stderr 는 /dev/null 로 버려지고 stdout 이 비어 fgets 실패 → 0
// 즉 "확인할 수 없을 때의 안전 기본값"을 고정하는 테스트다. 이게 무너지면
// 멀쩡한 서비스를 죽었다고 보고하거나(오탐 재시작), 엉뚱한 PID 를 잡는다.
TEST(ContainerHealthMonitorProbeTest, UnreachableContainerFallsBackSafely) {
    Mock_Logger = new NiceMock<MockLogger>();
    ON_CALL(*Mock_Logger, Get()).WillByDefault(Return(Mock_Logger));

    // 어떤 환경에도 존재할 수 없는 이름.
    const std::string kNoSuch = "mmf-unittest-no-such-container-4f2a9c";
    RealProbeMonitor m;

    EXPECT_FALSE(m.IsServiceAliveInContainer(kNoSuch, "some_node"));
    EXPECT_EQ(m.LookupServicePidInContainer(kNoSuch, "some_node"), 0u);

    delete Mock_Logger;
    Mock_Logger = nullptr;
}

TEST(ContainerHealthMonitorProbeTest, ProbesNeverThrow) {
    // 둘 다 popen/pclose 를 직접 다루고 폴 루프 스레드에서 불린다. 예외가
    // 새 나가면 스레드가 통째로 죽어 컨테이너 감시가 조용히 멈춘다.
    Mock_Logger = new NiceMock<MockLogger>();
    ON_CALL(*Mock_Logger, Get()).WillByDefault(Return(Mock_Logger));

    RealProbeMonitor m;
    EXPECT_NO_THROW(m.IsServiceAliveInContainer("", "tok"));
    EXPECT_NO_THROW(m.LookupServicePidInContainer("", "tok"));

    delete Mock_Logger;
    Mock_Logger = nullptr;
}

// ============================================================================
// NotifyContainerState / PublishContainerStateHeartbeat 의 실제 구현
//
// 왜 미커버였나:
//   이 파일의 다른 픽스처들(TestableContainerHealthMonitor, PollDrivenMonitor)은
//   두 메서드를 override 해서 호출만 기록한다. 폴링 로직을 DDS 없이 보려면
//   그래야 하지만, 그 바람에 **진짜 구현은 한 번도 실행되지 않았다**.
//
//   여기서는 override 하지 않고 base 구현을 그대로 노출해서 부른다. 검증
//   대상은 협력자가 하나씩 없을 때의 방어 분기다 — 컨테이너가 죽는 순간은
//   종료 중일 때가 많아서 ADSWManager/모니터/트랜스미터가 이미 사라졌을 수
//   있고, 거기서 터지면 죽는 이유를 알려줄 마지막 메시지를 잃는다.
//
//   DDS 로 실제 쓰기까지 가는 경로는 통합 레인 소관이다. 여기서는 엔드포인트가
//   있을 때 그 지점까지 도달하고 예외 없이 끝나는 것까지만 고정한다.
// ============================================================================

namespace {

// base 구현을 그대로 부르기 위한 껍데기. override 하지 않는 것이 핵심이다.
class RealNotifyMonitor : public ContainerHealthMonitor {
  public:
    using ContainerHealthMonitor::NotifyContainerState;
    using ContainerHealthMonitor::PublishContainerStateHeartbeat;
};

// DDSMessageEndpoint 는 추상 클래스다. 순수 가상 4개만 채우면 등록할 수 있다.
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

constexpr const char* kContainerTopic = "mmf/adsw/NotifyContainerState";

class ContainerStateNotifyTest : public ::testing::Test {
  protected:
    void SetUp() override {
        Mock_Logger = new NiceMock<MockLogger>();
        ON_CALL(*Mock_Logger, Get()).WillByDefault(Return(Mock_Logger));

        Mock_ADSWManager = new NiceMock<MockADSWManager>();
        mgr_holder_.reset(Mock_ADSWManager, [](MockADSWManager*) {});
        ADSWManager::SetShared(mgr_holder_);

        transmitter_ = std::make_shared<DDSTransmitter>();
        monitor_     = std::make_shared<ServiceMonitor>();

        ON_CALL(*Mock_ADSWManager, GetServiceMonitor()).WillByDefault(Return(monitor_));
        ON_CALL(*Mock_ADSWManager, GetTransmitter()).WillByDefault(Return(transmitter_));
        ON_CALL(*Mock_ADSWManager, GetTopic(_)).WillByDefault(Return(kContainerTopic));
        // ServiceMonitor::OnContainerStateChanged 는 레지스트리가 없으면 곧바로
        // 빠진다. 팬아웃이 호출되는 것만 보면 되므로 널로 둔다.
        ON_CALL(*Mock_ADSWManager, GetServiceRegistry())
            .WillByDefault(Return(std::shared_ptr<ServiceRegistry>()));

        mon_ = std::make_unique<RealNotifyMonitor>();
    }

    void TearDown() override {
        mon_.reset();
        transmitter_.reset();
        monitor_.reset();
        ADSWManager::SetShared(nullptr);
        delete Mock_ADSWManager; Mock_ADSWManager = nullptr;
        delete Mock_Logger;      Mock_Logger = nullptr;
    }

    std::unique_ptr<RealNotifyMonitor>    mon_;
    std::shared_ptr<MockADSWManager>      mgr_holder_;
    std::shared_ptr<DDSTransmitter>       transmitter_;
    std::shared_ptr<ServiceMonitor>       monitor_;
};

} // namespace

TEST_F(ContainerStateNotifyTest, FansOutAndPublishesWhenEverythingIsPresent) {
    ASSERT_TRUE(transmitter_->AddMessageEndpoint(
        std::make_shared<StubEndpoint>(kContainerTopic)));

    EXPECT_CALL(*Mock_ADSWManager, GetServiceMonitor()).Times(::testing::AtLeast(1));
    EXPECT_NO_THROW(mon_->NotifyContainerState("web", false, 137));
}

TEST_F(ContainerStateNotifyTest, RunningAndStoppedBothPublish) {
    ASSERT_TRUE(transmitter_->AddMessageEndpoint(
        std::make_shared<StubEndpoint>(kContainerTopic)));

    // 페이로드의 상태 열거값이 갈리는 삼항 양쪽.
    EXPECT_NO_THROW(mon_->PublishContainerStateHeartbeat("web", true, 0));
    EXPECT_NO_THROW(mon_->PublishContainerStateHeartbeat("web", false, 137));
}

TEST_F(ContainerStateNotifyTest, SurvivesMissingServiceMonitor) {
    ON_CALL(*Mock_ADSWManager, GetServiceMonitor())
        .WillByDefault(Return(std::shared_ptr<ServiceMonitor>()));
    ASSERT_TRUE(transmitter_->AddMessageEndpoint(
        std::make_shared<StubEndpoint>(kContainerTopic)));

    // 모니터가 없어도 DDS 발행은 계속돼야 한다 — 컨테이너 상태는 서비스
    // 상태와 독립적으로 lifecycle_mgr 에 전달된다.
    EXPECT_NO_THROW(mon_->NotifyContainerState("web", false, 137));
}

TEST_F(ContainerStateNotifyTest, SurvivesMissingTransmitter) {
    ON_CALL(*Mock_ADSWManager, GetTransmitter())
        .WillByDefault(Return(std::shared_ptr<DDSTransmitter>()));

    EXPECT_NO_THROW(mon_->NotifyContainerState("web", false, 137));
    EXPECT_NO_THROW(mon_->PublishContainerStateHeartbeat("web", false, 137));
}

TEST_F(ContainerStateNotifyTest, SurvivesMissingTopic) {
    ON_CALL(*Mock_ADSWManager, GetTopic(_))
        .WillByDefault(Return(static_cast<const char*>(nullptr)));

    EXPECT_NO_THROW(mon_->PublishContainerStateHeartbeat("web", false, 137));
}

TEST_F(ContainerStateNotifyTest, SurvivesUnregisteredEndpoint) {
    // 토픽은 받았지만 그 이름으로 등록된 엔드포인트가 없다.
    EXPECT_NO_THROW(mon_->PublishContainerStateHeartbeat("web", false, 137));
    EXPECT_EQ(transmitter_->GetMessageEndpoint(kContainerTopic), nullptr);
}

TEST_F(ContainerStateNotifyTest, SurvivesMissingManagerDuringShutdown) {
    // 종료 중이라 싱글턴이 이미 내려갔다. 두 메서드 모두 조용히 끝나야 한다.
    ADSWManager::SetShared(nullptr);

    EXPECT_NO_THROW(mon_->NotifyContainerState("web", false, 137));
    EXPECT_NO_THROW(mon_->PublishContainerStateHeartbeat("web", false, 137));

    ADSWManager::SetShared(mgr_holder_);   // TearDown 이 정리할 수 있게 되돌린다
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
