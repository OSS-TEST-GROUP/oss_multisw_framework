// general adsw_manager — ContainerController 유닛테스트
//
// 이 클래스의 start/stop/restart 와 status/exit-code 경로는 전부
// RunCommand/CaptureCommand 를 지나 docker · kubectl 을 부른다. 그래서 기존에는
// 실제 docker 가 있어야만 도는 ContainerDockerTest 8건에만 의존했고, 그 8건은
// CI 에서 전부 skip 되어 363줄이 통째로 미커버였다.
//
// 두 seam 을 서브클래싱해 명령을 가로채면 docker 없이 같은 로직을 전부 돌릴 수
// 있다. 검증 대상은 "무엇을 실행했는가"(명령 문자열)와 "반환값을 어떻게
// 해석하는가" 두 가지다.

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <memory>
#include <string>

#include "control/ContainerController.hpp"
#include "service/Container.hpp"
#include "mock/MockLogger.hpp"
#include "mock/FakeCmdContainerController.hpp"

using ::testing::HasSubstr;
using general::adsw_manager::Container;
using general::adsw_manager::ContainerType;
using general::adsw_manager::testing_support::FakeCmdContainerController;

namespace {

std::shared_ptr<Container> MakeContainer(const std::string& name, ContainerType type) {
    return std::make_shared<Container>(name, name + "-image", "/tmp/" + name + ".yaml",
                                       name + "-ns", type);
}

class ContainerControllerTest : public ::testing::Test {
  protected:
    void SetUp() override {
        logger_ = std::make_unique<ScopedMockLogger>();
        ctl = new FakeCmdContainerController();
    }
    void TearDown() override {
        delete ctl;
        logger_.reset();
    }

    std::shared_ptr<Container> Add(const std::string& name, ContainerType type) {
        auto c = MakeContainer(name, type);
        EXPECT_TRUE(ctl->AddContainer(c));
        return c;
    }

    std::unique_ptr<ScopedMockLogger> logger_;
    FakeCmdContainerController* ctl;
};

} // namespace

// ── 생성 / 등록 ─────────────────────────────────────────────────────────────

TEST_F(ContainerControllerTest, InitializeAndFinalizeAreSafeRepeatedly) {
    EXPECT_TRUE(ctl->Initialize());
    ctl->Finalize();
    EXPECT_TRUE(ctl->Initialize());
    ctl->Finalize();
}

TEST_F(ContainerControllerTest, AddContainer_NullRejected) {
    EXPECT_FALSE(ctl->AddContainer(nullptr));
    EXPECT_TRUE(ctl->GetAllContainers().empty());
}

TEST_F(ContainerControllerTest, AddContainer_DuplicateNameRejected) {
    Add("dup", ContainerType::DOCKER);
    EXPECT_FALSE(ctl->AddContainer(MakeContainer("dup", ContainerType::DOCKER)));
    EXPECT_EQ(ctl->GetAllContainers().size(), 1u);
}

TEST_F(ContainerControllerTest, ContainerSupported_FalseUntilAdded) {
    EXPECT_FALSE(ctl->ContainerSupported());
    Add("c1", ContainerType::DOCKER);
    EXPECT_TRUE(ctl->ContainerSupported());
}

TEST_F(ContainerControllerTest, GetContainer_NotFoundReturnsNull) {
    EXPECT_EQ(ctl->GetContainer("nope"), nullptr);
}

TEST_F(ContainerControllerTest, GetAllContainers_ReturnsEveryEntry) {
    Add("a", ContainerType::DOCKER);
    Add("b", ContainerType::K3S);
    Add("c", ContainerType::DOCKER_COMPOSE);
    EXPECT_EQ(ctl->GetAllContainers().size(), 3u);
}

// ── K3S ─────────────────────────────────────────────────────────────────────

TEST_F(ContainerControllerTest, StartK3s_Success) {
    Add("k1", ContainerType::K3S);
    EXPECT_TRUE(ctl->StartContainer("k1"));
    ASSERT_EQ(ctl->run_cmds.size(), 1u);
    EXPECT_EQ(ctl->run_cmds[0], "kubectl apply -f /tmp/k1.yaml");
}

TEST_F(ContainerControllerTest, StartK3s_Failure) {
    Add("k1", ContainerType::K3S);
    ctl->default_rc = 1;
    EXPECT_FALSE(ctl->StartContainer("k1"));
}

TEST_F(ContainerControllerTest, StopK3s_Success) {
    Add("k1", ContainerType::K3S);
    EXPECT_TRUE(ctl->StopContainer("k1"));
    ASSERT_EQ(ctl->run_cmds.size(), 1u);
    EXPECT_EQ(ctl->run_cmds[0], "kubectl delete pod k1 -n k1-ns --ignore-not-found");
}

TEST_F(ContainerControllerTest, StopK3s_Failure) {
    Add("k1", ContainerType::K3S);
    ctl->default_rc = 1;
    EXPECT_FALSE(ctl->StopContainer("k1"));
}

TEST_F(ContainerControllerTest, RestartK3s_DeletesThenApplies) {
    Add("k1", ContainerType::K3S);
    EXPECT_TRUE(ctl->RestartContainer("k1"));
    ASSERT_EQ(ctl->run_cmds.size(), 2u);
    EXPECT_THAT(ctl->run_cmds[0], HasSubstr("kubectl delete pod"));
    EXPECT_THAT(ctl->run_cmds[1], HasSubstr("kubectl apply -f"));
}

TEST_F(ContainerControllerTest, RestartK3s_StopFailsSkipsApply) {
    Add("k1", ContainerType::K3S);
    ctl->PushRc({1});
    EXPECT_FALSE(ctl->RestartContainer("k1"));
    EXPECT_EQ(ctl->run_cmds.size(), 1u);
}

// ── DOCKER ──────────────────────────────────────────────────────────────────

TEST_F(ContainerControllerTest, StartDocker_ExistingContainerStarts) {
    Add("d1", ContainerType::DOCKER);
    EXPECT_TRUE(ctl->StartContainer("d1"));
    ASSERT_EQ(ctl->run_cmds.size(), 1u);
    EXPECT_EQ(ctl->run_cmds[0], "docker start d1 2>/dev/null");
}

TEST_F(ContainerControllerTest, StartDocker_FallsBackToRun) {
    Add("d1", ContainerType::DOCKER);
    ctl->PushRc({1, 0});   // docker start 실패 → docker run 성공
    EXPECT_TRUE(ctl->StartContainer("d1"));
    ASSERT_EQ(ctl->run_cmds.size(), 2u);
    EXPECT_EQ(ctl->run_cmds[1], "docker run -d --init --name d1 d1-image");
}

TEST_F(ContainerControllerTest, StartDocker_RunKeepsExtraAndCommandArgOrder) {
    auto c = MakeContainer("d1", ContainerType::DOCKER);
    c->SetExtraArgs({"--network", "host", "-e", "FOO=1"});
    c->SetCommandArgs({"/opt/app/run.sh", "--role", "sils"});
    ASSERT_TRUE(ctl->AddContainer(c));

    ctl->PushRc({1, 0});
    EXPECT_TRUE(ctl->StartContainer("d1"));
    ASSERT_EQ(ctl->run_cmds.size(), 2u);
    // extra_args 는 이미지 앞, command_args 는 이미지 뒤여야 한다.
    EXPECT_EQ(ctl->run_cmds[1],
              "docker run -d --init --name d1 --network host -e FOO=1 "
              "d1-image /opt/app/run.sh --role sils");
}

TEST_F(ContainerControllerTest, StartDocker_BothStartAndRunFail) {
    Add("d1", ContainerType::DOCKER);
    ctl->default_rc = 1;
    EXPECT_FALSE(ctl->StartContainer("d1"));
    EXPECT_EQ(ctl->run_cmds.size(), 2u);
}

TEST_F(ContainerControllerTest, StopDocker_StopsAndRemoves) {
    Add("d1", ContainerType::DOCKER);
    EXPECT_TRUE(ctl->StopContainer("d1"));
    ASSERT_EQ(ctl->run_cmds.size(), 1u);
    EXPECT_EQ(ctl->run_cmds[0], "docker stop d1 && docker rm d1");
}

TEST_F(ContainerControllerTest, StopDocker_Failure) {
    Add("d1", ContainerType::DOCKER);
    ctl->default_rc = 1;
    EXPECT_FALSE(ctl->StopContainer("d1"));
}

TEST_F(ContainerControllerTest, RestartDocker_Success) {
    Add("d1", ContainerType::DOCKER);
    EXPECT_TRUE(ctl->RestartContainer("d1"));
    EXPECT_EQ(ctl->run_cmds[0], "docker restart d1");
}

TEST_F(ContainerControllerTest, RestartDocker_Failure) {
    Add("d1", ContainerType::DOCKER);
    ctl->default_rc = 1;
    EXPECT_FALSE(ctl->RestartContainer("d1"));
}

// ── DOCKER_COMPOSE ──────────────────────────────────────────────────────────

TEST_F(ContainerControllerTest, StartCompose_Success) {
    Add("c1", ContainerType::DOCKER_COMPOSE);
    EXPECT_TRUE(ctl->StartContainer("c1"));
    EXPECT_EQ(ctl->run_cmds[0], "docker compose -f /tmp/c1.yaml up -d");
}

TEST_F(ContainerControllerTest, StartCompose_Failure) {
    Add("c1", ContainerType::DOCKER_COMPOSE);
    ctl->default_rc = 1;
    EXPECT_FALSE(ctl->StartContainer("c1"));
}

TEST_F(ContainerControllerTest, StopCompose_Success) {
    Add("c1", ContainerType::DOCKER_COMPOSE);
    EXPECT_TRUE(ctl->StopContainer("c1"));
    EXPECT_EQ(ctl->run_cmds[0], "docker compose -f /tmp/c1.yaml down");
}

TEST_F(ContainerControllerTest, StopCompose_Failure) {
    Add("c1", ContainerType::DOCKER_COMPOSE);
    ctl->default_rc = 1;
    EXPECT_FALSE(ctl->StopContainer("c1"));
}

TEST_F(ContainerControllerTest, RestartCompose_Success) {
    Add("c1", ContainerType::DOCKER_COMPOSE);
    EXPECT_TRUE(ctl->RestartContainer("c1"));
    EXPECT_EQ(ctl->run_cmds[0], "docker compose -f /tmp/c1.yaml restart");
}

TEST_F(ContainerControllerTest, RestartCompose_Failure) {
    Add("c1", ContainerType::DOCKER_COMPOSE);
    ctl->default_rc = 1;
    EXPECT_FALSE(ctl->RestartContainer("c1"));
}

// ── 알 수 없는 타입 / 미등록 이름 ───────────────────────────────────────────

TEST_F(ContainerControllerTest, UnknownType_AllOperationsFail) {
    Add("u1", ContainerType::UNKNOWN);
    EXPECT_FALSE(ctl->StartContainer("u1"));
    EXPECT_FALSE(ctl->StopContainer("u1"));
    EXPECT_FALSE(ctl->RestartContainer("u1"));
    EXPECT_FALSE(ctl->GetContainerStatus("u1"));
    EXPECT_FALSE(ctl->GetContainerLogs("u1"));
    EXPECT_EQ(ctl->GetContainerStatusStr("u1"), "unknown");
    EXPECT_EQ(ctl->GetContainerExitCode("u1"), -1);
    EXPECT_TRUE(ctl->run_cmds.empty());
}

TEST_F(ContainerControllerTest, UnregisteredName_AllOperationsFail) {
    EXPECT_FALSE(ctl->StartContainer("ghost"));
    EXPECT_FALSE(ctl->StopContainer("ghost"));
    EXPECT_FALSE(ctl->RestartContainer("ghost"));
    EXPECT_FALSE(ctl->GetContainerStatus("ghost"));
    EXPECT_FALSE(ctl->GetContainerLogs("ghost"));
    EXPECT_EQ(ctl->GetContainerStatusStr("ghost"), "unknown");
    EXPECT_EQ(ctl->GetContainerExitCode("ghost"), -1);
}

// ── status / logs (RunCommand 반환값 해석) ──────────────────────────────────

TEST_F(ContainerControllerTest, GetContainerStatus_K3sCommandAndResult) {
    Add("k1", ContainerType::K3S);
    EXPECT_TRUE(ctl->GetContainerStatus("k1"));
    EXPECT_EQ(ctl->run_cmds[0],
              "kubectl get pod k1 -n k1-ns -o jsonpath='{.status.phase}'");
}

TEST_F(ContainerControllerTest, GetContainerStatus_DockerFailureRc) {
    Add("d1", ContainerType::DOCKER);
    ctl->default_rc = 1;
    EXPECT_FALSE(ctl->GetContainerStatus("d1"));
    EXPECT_EQ(ctl->run_cmds[0], "docker inspect --format='{{.State.Status}}' d1");
}

TEST_F(ContainerControllerTest, GetContainerLogs_K3sAndDocker) {
    Add("k1", ContainerType::K3S);
    Add("d1", ContainerType::DOCKER);
    EXPECT_TRUE(ctl->GetContainerLogs("k1"));
    EXPECT_EQ(ctl->run_cmds[0], "kubectl logs k1 -n k1-ns -c k1");
    EXPECT_TRUE(ctl->GetContainerLogs("d1"));
    EXPECT_EQ(ctl->run_cmds[1], "docker logs d1");
}

TEST_F(ContainerControllerTest, GetContainerLogs_FailureRc) {
    Add("d1", ContainerType::DOCKER);
    ctl->default_rc = 1;
    EXPECT_FALSE(ctl->GetContainerLogs("d1"));
}

// ── GetContainerStatusStr (CaptureCommand 출력 가공) ────────────────────────

TEST_F(ContainerControllerTest, StatusStr_DockerStripsQuotesAndNewline) {
    Add("d1", ContainerType::DOCKER);
    ctl->capture_out = "'running'\n";
    EXPECT_EQ(ctl->GetContainerStatusStr("d1"), "running");
    EXPECT_EQ(ctl->capture_cmds[0],
              "docker inspect --format='{{.State.Status}}' d1 2>/dev/null");
}

TEST_F(ContainerControllerTest, StatusStr_StripsCarriageReturn) {
    Add("d1", ContainerType::DOCKER);
    ctl->capture_out = "exited\r\n";
    EXPECT_EQ(ctl->GetContainerStatusStr("d1"), "exited");
}

TEST_F(ContainerControllerTest, StatusStr_EmptyOutputBecomesUnknown) {
    Add("d1", ContainerType::DOCKER);
    ctl->capture_out = "";
    EXPECT_EQ(ctl->GetContainerStatusStr("d1"), "unknown");
}

TEST_F(ContainerControllerTest, StatusStr_OnlyQuotesBecomesUnknown) {
    Add("d1", ContainerType::DOCKER);
    ctl->capture_out = "''\n";
    EXPECT_EQ(ctl->GetContainerStatusStr("d1"), "unknown");
}

TEST_F(ContainerControllerTest, StatusStr_K3sUsesJsonpath) {
    Add("k1", ContainerType::K3S);
    ctl->capture_out = "Running\n";
    EXPECT_EQ(ctl->GetContainerStatusStr("k1"), "Running");
    EXPECT_EQ(ctl->capture_cmds[0],
              "kubectl get pod k1 -n k1-ns -o jsonpath='{.status.phase}' 2>/dev/null");
}

TEST_F(ContainerControllerTest, StatusStr_TruncatedAtBufferSize) {
    Add("d1", ContainerType::DOCKER);
    // 실제 구현은 64바이트 버퍼(fgets 로 63자)를 쓴다. 잘려도 크래시 없이 나와야 한다.
    ctl->capture_out = std::string(200, 'x');
    EXPECT_EQ(ctl->GetContainerStatusStr("d1"), std::string(63, 'x'));
}

TEST_F(ContainerControllerTest, IsContainerRunning_BothCasings) {
    Add("d1", ContainerType::DOCKER);
    Add("k1", ContainerType::K3S);
    ctl->capture_out = "running\n";
    EXPECT_TRUE(ctl->IsContainerRunning("d1"));
    ctl->capture_out = "Running\n";
    EXPECT_TRUE(ctl->IsContainerRunning("k1"));
    ctl->capture_out = "exited\n";
    EXPECT_FALSE(ctl->IsContainerRunning("d1"));
}

// ── GetContainerExitCode ────────────────────────────────────────────────────

TEST_F(ContainerControllerTest, ExitCode_ParsesNumber) {
    Add("d1", ContainerType::DOCKER);
    ctl->capture_out = "137\n";
    EXPECT_EQ(ctl->GetContainerExitCode("d1"), 137);
    EXPECT_EQ(ctl->capture_cmds[0],
              "docker inspect --format='{{.State.ExitCode}}' d1 2>/dev/null");
}

TEST_F(ContainerControllerTest, ExitCode_ZeroIsValid) {
    Add("d1", ContainerType::DOCKER);
    ctl->capture_out = "0\n";
    EXPECT_EQ(ctl->GetContainerExitCode("d1"), 0);
}

TEST_F(ContainerControllerTest, ExitCode_NonNumericBecomesMinusOne) {
    Add("d1", ContainerType::DOCKER);
    ctl->capture_out = "not-a-number\n";
    EXPECT_EQ(ctl->GetContainerExitCode("d1"), -1);
}

TEST_F(ContainerControllerTest, ExitCode_EmptyBecomesMinusOne) {
    Add("d1", ContainerType::DOCKER);
    ctl->capture_out = "";
    EXPECT_EQ(ctl->GetContainerExitCode("d1"), -1);
}

TEST_F(ContainerControllerTest, ExitCode_K3sUsesTerminatedJsonpath) {
    Add("k1", ContainerType::K3S);
    ctl->capture_out = "2\n";
    EXPECT_EQ(ctl->GetContainerExitCode("k1"), 2);
    EXPECT_THAT(ctl->capture_cmds[0], HasSubstr("state.terminated.exitCode"));
}

// ── Container 값 객체 ───────────────────────────────────────────────────────

TEST_F(ContainerControllerTest, ContainerAccessorsRoundTrip) {
    auto c = MakeContainer("cx", ContainerType::DOCKER);
    EXPECT_EQ(c->GetName(), "cx");
    EXPECT_EQ(c->GetImage(), "cx-image");
    EXPECT_EQ(c->GetDeployFile(), "/tmp/cx.yaml");
    EXPECT_EQ(c->GetNamespace(), "cx-ns");
    EXPECT_EQ(c->GetPodName(), "cx");
    EXPECT_EQ(c->GetContainerType(), ContainerType::DOCKER);

    c->SetContainerType(ContainerType::K3S);
    EXPECT_EQ(c->GetContainerType(), ContainerType::K3S);

    EXPECT_TRUE(c->GetExtraArgs().empty());
    EXPECT_TRUE(c->GetCommandArgs().empty());
    c->SetExtraArgs({"-v", "/a:/b"});
    c->SetCommandArgs({"run"});
    EXPECT_EQ(c->GetExtraArgs().size(), 2u);
    EXPECT_EQ(c->GetCommandArgs().size(), 1u);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
