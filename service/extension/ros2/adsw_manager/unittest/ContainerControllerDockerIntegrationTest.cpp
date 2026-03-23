// ============================================================================
// ContainerController Docker 통합 테스트
//
// 실제 Docker 명령을 사용합니다.
// 사전 조건: docker build -t adsw-test-app:latest
//            unittest/docker/Dockerfile 에서 이미지 빌드 필요
//
// 실행 방법:
//   ./ADSWContainerControllerDockerIntegrationTest
//   또는 테스트 스크립트: unittest/test/run_docker_integration_test.sh
// ============================================================================

#include <gtest/gtest.h>
#include <cstdlib>
#include <chrono>
#include <thread>
#include <string>

#include "control/ContainerController.hpp"
#include "service/Container.hpp"
#include "logger/Logger.hpp"

using namespace ros2::adsw_manager;

// ─────────────────────────────────────────────────────────────────────────────
// 헬퍼: Docker 가용 여부 체크
// ─────────────────────────────────────────────────────────────────────────────
static bool IsDockerAvailable() {
    return system("docker info > /dev/null 2>&1") == 0;
}

// 테스트 이미지 존재 여부 체크
static bool IsTestImageAvailable(const std::string& image) {
    std::string cmd = "docker image inspect " + image + " > /dev/null 2>&1";
    return system(cmd.c_str()) == 0;
}

// 컨테이너 강제 제거 (테스트 전/후 정리)
static void ForceRemoveContainer(const std::string& name) {
    std::string cmd = "docker rm -f " + name + " > /dev/null 2>&1";
    system(cmd.c_str());
}

// docker-compose 로 생성된 컨테이너 정리
static void DockerComposeDown(const std::string& compose_file) {
    std::string cmd = "docker compose -f " + compose_file + " down > /dev/null 2>&1";
    system(cmd.c_str());
}

// 짧게 대기 (컨테이너 상태 안정화)
static void WaitMs(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

// ─────────────────────────────────────────────────────────────────────────────
// Test fixture: Docker run (DOCKER 타입)
// ─────────────────────────────────────────────────────────────────────────────
class ContainerDockerTest : public ::testing::Test {
protected:
    static constexpr const char* kContainerName = "adsw-test-docker";
    static constexpr const char* kImageName     = "adsw-test-app:latest";

    void SetUp() override {
        if (!IsDockerAvailable()) {
            GTEST_SKIP() << "Docker를 사용할 수 없습니다. 테스트를 건너뜁니다.";
        }
        if (!IsTestImageAvailable(kImageName)) {
            GTEST_SKIP() << "테스트 이미지 '" << kImageName
                         << "' 가 없습니다.\n"
                         << "다음 명령으로 빌드하세요:\n"
                         << "  docker build -t " << kImageName
                         << " unittest/docker/";
        }
        // 이전 테스트 잔재 정리
        ForceRemoveContainer(kContainerName);

        controller_ = std::make_unique<ContainerController>();
        controller_->Initialize();

        auto container = std::make_shared<Container>(
            kContainerName, kImageName, "", "", ContainerType::DOCKER);
        controller_->AddContainer(container);
    }

    void TearDown() override {
        // 테스트 후 컨테이너 강제 제거
        ForceRemoveContainer(kContainerName);
    }

    std::unique_ptr<ContainerController> controller_;
};

// ─── Docker 컨테이너 시작/상태/종료 통합 테스트 ──────────────────────────────

TEST_F(ContainerDockerTest, StartContainer_Success_IT001) {
    EXPECT_TRUE(controller_->StartContainer(kContainerName));
    WaitMs(500);  // 컨테이너 기동 대기
    EXPECT_TRUE(controller_->IsContainerRunning(kContainerName));
}

TEST_F(ContainerDockerTest, GetContainerStatusStr_Running_IT002) {
    ASSERT_TRUE(controller_->StartContainer(kContainerName));
    WaitMs(500);
    std::string status = controller_->GetContainerStatusStr(kContainerName);
    EXPECT_EQ(status, "running");
}

TEST_F(ContainerDockerTest, GetContainerExitCode_WhileRunning_IT003) {
    ASSERT_TRUE(controller_->StartContainer(kContainerName));
    WaitMs(500);
    // 실행 중 exit code = 0
    EXPECT_EQ(controller_->GetContainerExitCode(kContainerName), 0);
}

TEST_F(ContainerDockerTest, StopContainer_Success_IT004) {
    ASSERT_TRUE(controller_->StartContainer(kContainerName));
    WaitMs(500);
    EXPECT_TRUE(controller_->StopContainer(kContainerName));
    WaitMs(500);
    EXPECT_FALSE(controller_->IsContainerRunning(kContainerName));
}

TEST_F(ContainerDockerTest, GetContainerStatusStr_AfterStop_IT005) {
    ASSERT_TRUE(controller_->StartContainer(kContainerName));
    WaitMs(500);
    ASSERT_TRUE(controller_->StopContainer(kContainerName));
    WaitMs(500);
    std::string status = controller_->GetContainerStatusStr(kContainerName);
    // 컨테이너가 제거되면 "unknown", 남아있으면 "exited"
    EXPECT_TRUE(status == "unknown" || status == "exited");
}

TEST_F(ContainerDockerTest, RestartContainer_StaysRunning_IT006) {
    ASSERT_TRUE(controller_->StartContainer(kContainerName));
    WaitMs(500);
    EXPECT_TRUE(controller_->RestartContainer(kContainerName));
    WaitMs(1000);  // 재시작 대기
    EXPECT_TRUE(controller_->IsContainerRunning(kContainerName));
}

TEST_F(ContainerDockerTest, StartContainer_AlreadyRunning_Idempotent_IT007) {
    ASSERT_TRUE(controller_->StartContainer(kContainerName));
    WaitMs(500);
    // 이미 실행 중인 컨테이너 재시작 → docker start는 성공
    EXPECT_TRUE(controller_->StartContainer(kContainerName));
    EXPECT_TRUE(controller_->IsContainerRunning(kContainerName));
}

TEST_F(ContainerDockerTest, ContainerStatus_BeforeStart_IT008) {
    // 시작 전: 미등록 컨테이너 → unknown
    std::string status = controller_->GetContainerStatusStr(kContainerName);
    // 정리 후이므로 존재하지 않음
    EXPECT_EQ(status, "unknown");
    EXPECT_FALSE(controller_->IsContainerRunning(kContainerName));
}

// ─────────────────────────────────────────────────────────────────────────────
// Test fixture: Docker Compose (DOCKER_COMPOSE 타입)
// ─────────────────────────────────────────────────────────────────────────────
class ContainerDockerComposeTest : public ::testing::Test {
protected:
    // 빌드 후 배포 파일 경로는 실행 디렉토리 기준으로 지정
    // cmake install 후 bin/unittest/adsw_manager/ 에 위치
    static constexpr const char* kContainerName  = "adsw-test-compose";
    static constexpr const char* kImageName      = "adsw-test-app:latest";
    static constexpr const char* kComposeFile    = "docker/docker-compose.test.yaml";

    void SetUp() override {
        if (!IsDockerAvailable()) {
            GTEST_SKIP() << "Docker를 사용할 수 없습니다.";
        }
        if (!IsTestImageAvailable(kImageName)) {
            GTEST_SKIP() << "테스트 이미지 '" << kImageName << "' 가 없습니다.";
        }
        DockerComposeDown(kComposeFile);

        controller_ = std::make_unique<ContainerController>();
        controller_->Initialize();

        auto container = std::make_shared<Container>(
            kContainerName, kImageName, kComposeFile, "", ContainerType::DOCKER_COMPOSE);
        controller_->AddContainer(container);
    }

    void TearDown() override {
        DockerComposeDown(kComposeFile);
    }

    std::unique_ptr<ContainerController> controller_;
};

TEST_F(ContainerDockerComposeTest, StartComposeContainer_Success_IT010) {
    EXPECT_TRUE(controller_->StartContainer(kContainerName));
    WaitMs(1500);  // compose up 대기 시간 여유
    EXPECT_TRUE(controller_->IsContainerRunning(kContainerName));
}

TEST_F(ContainerDockerComposeTest, GetContainerStatusStr_Running_IT011) {
    ASSERT_TRUE(controller_->StartContainer(kContainerName));
    WaitMs(1500);
    std::string status = controller_->GetContainerStatusStr(kContainerName);
    EXPECT_EQ(status, "running");
}

TEST_F(ContainerDockerComposeTest, StopComposeContainer_Success_IT012) {
    ASSERT_TRUE(controller_->StartContainer(kContainerName));
    WaitMs(1500);
    EXPECT_TRUE(controller_->StopContainer(kContainerName));
    WaitMs(1000);
    EXPECT_FALSE(controller_->IsContainerRunning(kContainerName));
}

TEST_F(ContainerDockerComposeTest, RestartComposeContainer_IT013) {
    ASSERT_TRUE(controller_->StartContainer(kContainerName));
    WaitMs(1500);
    EXPECT_TRUE(controller_->RestartContainer(kContainerName));
    WaitMs(2000);
    EXPECT_TRUE(controller_->IsContainerRunning(kContainerName));
}

// ─────────────────────────────────────────────────────────────────────────────
// ContainerHealthMonitor 통합 테스트 (Docker 상태 변경 감지)
// ─────────────────────────────────────────────────────────────────────────────
#include "monitor/ContainerHealthMonitor.hpp"

class ContainerHealthMonitorDockerTest : public ::testing::Test {
protected:
    static constexpr const char* kContainerName = "adsw-test-hm";
    static constexpr const char* kImageName     = "adsw-test-app:latest";

    struct StateChange {
        std::string name;
        bool is_running;
        int32_t exit_code;
    };

    // ContainerController를 직접 주입받는 테스트용 헬스 모니터
    class TestableHealthMonitor : public ContainerHealthMonitor {
    public:
        explicit TestableHealthMonitor(ContainerController* ctrl) : ctrl_(ctrl) {}
        std::vector<StateChange> changes;

        // 단일 폴링 사이클을 직접 실행
        void RunOnePollCycle() {
            if (!ctrl_) return;
            auto containers = ctrl_->GetAllContainers();
            for (const auto& c : containers) {
                const std::string& name = c->GetName();
                bool is_running = ctrl_->IsContainerRunning(name);
                auto it = prev_state_.find(name);
                bool prev = (it != prev_state_.end()) ? it->second : !is_running;
                if (is_running != prev) {
                    int32_t exit_code = is_running ? 0 : ctrl_->GetContainerExitCode(name);
                    NotifyContainerState(name, is_running, exit_code);
                    prev_state_[name] = is_running;
                }
            }
        }

    protected:
        void NotifyContainerState(const std::string& name,
                                  bool running, int32_t code) override {
            changes.push_back({name, running, code});
        }
    private:
        ContainerController* ctrl_;
    };

    void SetUp() override {
        if (!IsDockerAvailable()) {
            GTEST_SKIP() << "Docker를 사용할 수 없습니다.";
        }
        if (!IsTestImageAvailable(kImageName)) {
            GTEST_SKIP() << "테스트 이미지 '" << kImageName << "' 가 없습니다.";
        }
        ForceRemoveContainer(kContainerName);

        controller_ = std::make_unique<ContainerController>();
        controller_->Initialize();
        auto c = std::make_shared<Container>(
            kContainerName, kImageName, "", "", ContainerType::DOCKER);
        controller_->AddContainer(c);

        monitor_ = std::make_unique<TestableHealthMonitor>(controller_.get());
        // 초기 상태: 중지
        monitor_->SetPrevState(kContainerName, false);
    }

    void TearDown() override {
        ForceRemoveContainer(kContainerName);
    }

    std::unique_ptr<ContainerController> controller_;
    std::unique_ptr<TestableHealthMonitor> monitor_;
};

// 시작 시 "중지→실행" 변경 감지
TEST_F(ContainerHealthMonitorDockerTest, DetectsContainerStart_IT020) {
    // 폴링: 아직 시작 전 → 변경 없음
    monitor_->RunOnePollCycle();
    EXPECT_EQ(monitor_->changes.size(), 0u);

    // 컨테이너 시작
    ASSERT_TRUE(controller_->StartContainer(kContainerName));
    WaitMs(700);

    // 폴링: 중지→실행 감지
    monitor_->RunOnePollCycle();
    ASSERT_EQ(monitor_->changes.size(), 1u);
    EXPECT_EQ(monitor_->changes[0].name, kContainerName);
    EXPECT_TRUE(monitor_->changes[0].is_running);
}

// 종료 시 "실행→중지" 변경 감지
TEST_F(ContainerHealthMonitorDockerTest, DetectsContainerStop_IT021) {
    ASSERT_TRUE(controller_->StartContainer(kContainerName));
    WaitMs(700);

    // 시작 감지 (prev=false → running=true)
    monitor_->RunOnePollCycle();
    ASSERT_GE(monitor_->changes.size(), 1u);
    monitor_->changes.clear();

    // 컨테이너 중지
    ASSERT_TRUE(controller_->StopContainer(kContainerName));
    WaitMs(700);

    // 폴링: 실행→중지 감지
    monitor_->RunOnePollCycle();
    ASSERT_EQ(monitor_->changes.size(), 1u);
    EXPECT_EQ(monitor_->changes[0].name, kContainerName);
    EXPECT_FALSE(monitor_->changes[0].is_running);
}

// 상태 변화 없을 때 알림 없음
TEST_F(ContainerHealthMonitorDockerTest, NoNotification_WhenStateSame_IT022) {
    // prev=false, 실제로도 중지 → 변경 없음
    monitor_->RunOnePollCycle();
    EXPECT_EQ(monitor_->changes.size(), 0u);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
