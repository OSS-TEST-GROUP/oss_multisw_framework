// ============================================================================
// general/adsw_manager ContainerController — Docker 통합 테스트
//
// 실제 Docker 데몬을 사용해 ContainerController 의 docker 제어(StartContainer/
// StopContainer/IsContainerRunning/GetContainerStatusStr/GetContainerExitCode/
// RestartContainer)를 검증한다. DDS·lifecycle_mgr 없이 docker 만 있으면 된다 —
// 이게 general 매니저를 Linux docker 로 확인하는 가장 격리된 방법이다.
//
// 의존: docker 데몬 + 흔한 이미지(alpine). 사전 빌드/커스텀 Dockerfile 불필요 —
//       이미지가 없으면 `docker pull alpine:latest` 를 한 번 시도하고, 그래도
//       없으면(오프라인 등) 해당 테스트를 SKIP 한다.
//
// 실행:
//   ADSWGeneralContainerDockerIntegrationTest
// ============================================================================

#include <gtest/gtest.h>

#include <chrono>
#include <cstdlib>
#include <string>
#include <thread>

#include "control/ContainerController.hpp"
#include "service/Container.hpp"

using namespace general::adsw_manager;

static bool IsDockerAvailable() {
    return system("docker info > /dev/null 2>&1") == 0;
}

// 이미지 보장: 없으면 pull 한 번 시도. 최종 존재 여부를 반환.
static bool EnsureImage(const std::string& image) {
    std::string inspect = "docker image inspect " + image + " > /dev/null 2>&1";
    if (system(inspect.c_str()) == 0) return true;
    std::string pull = "docker pull " + image + " > /dev/null 2>&1";
    system(pull.c_str());
    return system(inspect.c_str()) == 0;
}

static void ForceRemoveContainer(const std::string& name) {
    std::string cmd = "docker rm -f " + name + " > /dev/null 2>&1";
    system(cmd.c_str());
}

static void WaitMs(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

// DOCKER 타입 컨테이너 제어 fixture. keep-alive 를 위해 흔한 alpine 이미지에
// `sleep` 커맨드를 얹어(command_args) 컨테이너가 계속 running 상태를 유지하게 한다.
class GeneralContainerDockerTest : public ::testing::Test {
protected:
    static constexpr const char* kContainerName = "adsw-general-test-docker";
    static constexpr const char* kImageName     = "alpine:latest";

    void SetUp() override {
        if (!IsDockerAvailable()) {
            GTEST_SKIP() << "Docker 를 사용할 수 없습니다 — 테스트 건너뜀.";
        }
        if (!EnsureImage(kImageName)) {
            GTEST_SKIP() << "이미지 '" << kImageName << "' 를 확보할 수 없습니다 "
                         << "(오프라인일 수 있음) — 테스트 건너뜀.";
        }
        ForceRemoveContainer(kContainerName);  // 이전 잔재 정리

        controller_ = std::make_unique<ContainerController>();
        controller_->Initialize();

        auto container = std::make_shared<Container>(
            kContainerName, kImageName, "", "", ContainerType::DOCKER);
        // keep-alive: docker run ... alpine:latest sleep 3600
        container->SetCommandArgs({"sleep", "3600"});
        controller_->AddContainer(container);
    }

    void TearDown() override {
        ForceRemoveContainer(kContainerName);
    }

    std::unique_ptr<ContainerController> controller_;
};

TEST_F(GeneralContainerDockerTest, StartContainer_Success) {
    EXPECT_TRUE(controller_->StartContainer(kContainerName));
    WaitMs(500);
    EXPECT_TRUE(controller_->IsContainerRunning(kContainerName));
}

TEST_F(GeneralContainerDockerTest, StatusStr_Running) {
    ASSERT_TRUE(controller_->StartContainer(kContainerName));
    WaitMs(500);
    EXPECT_EQ(controller_->GetContainerStatusStr(kContainerName), "running");
}

TEST_F(GeneralContainerDockerTest, ExitCode_WhileRunning_IsZero) {
    ASSERT_TRUE(controller_->StartContainer(kContainerName));
    WaitMs(500);
    EXPECT_EQ(controller_->GetContainerExitCode(kContainerName), 0);
}

TEST_F(GeneralContainerDockerTest, StopContainer_Success) {
    ASSERT_TRUE(controller_->StartContainer(kContainerName));
    WaitMs(500);
    EXPECT_TRUE(controller_->StopContainer(kContainerName));
    WaitMs(500);
    EXPECT_FALSE(controller_->IsContainerRunning(kContainerName));
}

TEST_F(GeneralContainerDockerTest, StatusStr_AfterStop) {
    ASSERT_TRUE(controller_->StartContainer(kContainerName));
    WaitMs(500);
    ASSERT_TRUE(controller_->StopContainer(kContainerName));
    WaitMs(500);
    // StopDockerContainer 는 stop && rm 이므로 제거됨 → "unknown".
    // (혹시 남아있으면 "exited")
    std::string status = controller_->GetContainerStatusStr(kContainerName);
    EXPECT_TRUE(status == "unknown" || status == "exited") << "status=" << status;
}

TEST_F(GeneralContainerDockerTest, RestartContainer_StaysRunning) {
    ASSERT_TRUE(controller_->StartContainer(kContainerName));
    WaitMs(500);
    EXPECT_TRUE(controller_->RestartContainer(kContainerName));
    WaitMs(1000);
    EXPECT_TRUE(controller_->IsContainerRunning(kContainerName));
}

TEST_F(GeneralContainerDockerTest, StartContainer_AlreadyRunning_Idempotent) {
    ASSERT_TRUE(controller_->StartContainer(kContainerName));
    WaitMs(500);
    // 이미 실행 중 → docker start 가 성공 처리 (StartDockerContainer 의
    // "기존 컨테이너 먼저 start" 경로). 재기동 없이 running 유지.
    EXPECT_TRUE(controller_->StartContainer(kContainerName));
    EXPECT_TRUE(controller_->IsContainerRunning(kContainerName));
}

TEST_F(GeneralContainerDockerTest, StatusStr_BeforeStart_IsUnknown) {
    // SetUp 에서 정리했으므로 시작 전엔 존재하지 않음.
    EXPECT_EQ(controller_->GetContainerStatusStr(kContainerName), "unknown");
    EXPECT_FALSE(controller_->IsContainerRunning(kContainerName));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
