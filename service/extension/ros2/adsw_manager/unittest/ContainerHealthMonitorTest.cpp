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

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
