// Unit tests for ContainerController class using Google Test and Google Mock
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "control/ContainerController.hpp"
#include "service/Container.hpp"
#include "mock/MockLogger.hpp"
#include "mock/FakeCmdContainerController.hpp"

using ::testing::Return;
using ::testing::_;
using ::testing::A;
using ::testing::ReturnRef;
using ::testing::Values;
using ::testing::SetArgPointee;
using ::testing::SetArrayArgument;
using ::testing::SaveArg;
using ::testing::SaveArgPointee;
using ::testing::DoAll;
using ::testing::Args;
using ::testing::AllOf;
using ::testing::AtLeast;
using ::testing::Combine;
using ::testing::NiceMock;


class ContainerControllerTest : public ::testing::Test {
protected:
    void SetUp() override {
        Mock_Logger = new MockLogger();
        ON_CALL(*Mock_Logger, Get()).WillByDefault(Return(Mock_Logger));
           
        controller = new ros2::adsw_manager::ContainerController();
    }
    void TearDown() override {
        delete controller;
        delete Mock_Logger;
        Mock_Logger = nullptr;
    }
    ros2::adsw_manager::ContainerController* controller;
};

TEST_F(ContainerControllerTest, ConstructorDestructor) {
    // Test that constructor and destructor don't crash
    auto* test_controller = new ros2::adsw_manager::ContainerController();
    ASSERT_NE(test_controller, nullptr);
    delete test_controller;
}

TEST_F(ContainerControllerTest, InitializeAndFinalize) {
    ASSERT_TRUE(controller->Initialize());
    controller->Finalize();
    // Multiple Initialize/Finalize calls should be safe
    ASSERT_TRUE(controller->Initialize());
    controller->Finalize();
}

TEST_F(ContainerControllerTest, ContainerSupported_DefaultFalse) {
    controller->Initialize();
    EXPECT_FALSE(controller->ContainerSupported());
}

// Test GetContainer with null containers list (k3s disabled)
TEST_F(ContainerControllerTest, GetContainer_K3sDisabled) {
    controller->Initialize();
    auto container = controller->GetContainer("test-container");
    EXPECT_EQ(container, nullptr);
}

// Test GetContainer with container not found
TEST_F(ContainerControllerTest, GetContainer_NotFound) {
    controller->Initialize();
    auto container = controller->GetContainer("nonexistent-container");
    EXPECT_EQ(container, nullptr);
}

// Test AddContainer with null container
TEST_F(ContainerControllerTest, AddContainer_NullContainer) {
    controller->Initialize();
    EXPECT_FALSE(controller->AddContainer(nullptr));
}

// Test AddContainer with valid container
TEST_F(ContainerControllerTest, AddContainer_ValidContainer) {
    controller->Initialize();
    auto container = std::make_shared<ros2::adsw_manager::Container>(
        "test-container", "test-image", "/path/to/deploy.yaml", "default");
        
    EXPECT_TRUE(controller->AddContainer(container));

    EXPECT_EQ(container->GetName(), "test-container");
    EXPECT_EQ(container->GetImage(), "test-image");
    EXPECT_EQ(container->GetDeployFile(), "/path/to/deploy.yaml");
    EXPECT_EQ(container->GetNamespace(), "default");
}

// Test AddContainer with duplicate container
TEST_F(ContainerControllerTest, AddContainer_DuplicateContainer) {
    controller->Initialize();
    auto container = std::make_shared<ros2::adsw_manager::Container>(
        "test-container", "test-image", "/path/to/deploy.yaml", "default");
    EXPECT_TRUE(controller->AddContainer(container));
    // Try to add same container again
    auto duplicate = std::make_shared<ros2::adsw_manager::Container>(
        "test-container", "test-image2", "/path/to/deploy2.yaml", "default");
    EXPECT_FALSE(controller->AddContainer(duplicate));
}

// Test GetContainer after adding it
TEST_F(ContainerControllerTest, GetContainer_AfterAdding) {
    controller->Initialize();
    auto container = std::make_shared<ros2::adsw_manager::Container>(
        "test-container", "test-image", "/path/to/deploy.yaml", "default");
    EXPECT_TRUE(controller->AddContainer(container));

    auto retrieved = controller->GetContainer("test-container");
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->GetName(), "test-container");
}

// ─── BaseCommand / BaseRos2Command ────────────────────────────────────────────

TEST_F(ContainerControllerTest, BaseCommand_UnknownContainer_ReturnsEmpty) {
    controller->Initialize();
    // 등록되지 않은 컨테이너는 빈 문자열
    EXPECT_EQ(controller->BaseCommand("nonexistent"), "");
}

TEST_F(ContainerControllerTest, BaseRos2Command_UnknownContainer_ReturnsEmpty) {
    controller->Initialize();
    EXPECT_EQ(controller->BaseRos2Command("nonexistent"), "");
}

TEST_F(ContainerControllerTest, BaseCommand_AdswRos2ContainerAlwaysEmpty) {
    // ADSW_ROS2_CONTAINER_NAME 는 항상 "" 반환 (자기 자신이므로 exec 불필요)
    controller->Initialize();
    EXPECT_EQ(controller->BaseCommand("oss-mmf-ext-ros2"), "");
    EXPECT_EQ(controller->BaseRos2Command("oss-mmf-ext-ros2"), "");
}

TEST_F(ContainerControllerTest, BaseCommand_DockerContainer_ReturnsDockerExec) {
    controller->Initialize();
    auto c = std::make_shared<ros2::adsw_manager::Container>(
        "myapp", "myapp:latest", "", "", ros2::adsw_manager::ContainerType::DOCKER);
    controller->AddContainer(c);

    std::string cmd = controller->BaseCommand("myapp");
    EXPECT_NE(cmd.find("docker exec myapp"), std::string::npos);
}

TEST_F(ContainerControllerTest, BaseCommand_K3sContainer_ReturnsKubectl) {
    controller->Initialize();
    auto c = std::make_shared<ros2::adsw_manager::Container>(
        "mypod", "mypod:latest", "pod.yaml", "default", ros2::adsw_manager::ContainerType::K3S);
    controller->AddContainer(c);

    std::string cmd = controller->BaseCommand("mypod");
    EXPECT_NE(cmd.find("kubectl exec"), std::string::npos);
    EXPECT_NE(cmd.find("-n default"), std::string::npos);
}

TEST_F(ContainerControllerTest, BaseRos2Command_DockerContainer_ContainsSourceBash) {
    controller->Initialize();
    auto c = std::make_shared<ros2::adsw_manager::Container>(
        "myapp", "myapp:latest", "", "", ros2::adsw_manager::ContainerType::DOCKER);
    controller->AddContainer(c);

    std::string cmd = controller->BaseRos2Command("myapp");
    EXPECT_NE(cmd.find("source /opt/ros/humble/setup.bash"), std::string::npos);
}

// ─── 미등록 컨테이너에 대한 연산 ────────────────────────────────────────────

TEST_F(ContainerControllerTest, StartContainer_NotRegistered_ReturnsFalse) {
    controller->Initialize();
    EXPECT_FALSE(controller->StartContainer("not-registered"));
}

TEST_F(ContainerControllerTest, StopContainer_NotRegistered_ReturnsFalse) {
    controller->Initialize();
    EXPECT_FALSE(controller->StopContainer("not-registered"));
}

TEST_F(ContainerControllerTest, GetContainerStatus_NotRegistered_ReturnsFalse) {
    controller->Initialize();
    EXPECT_FALSE(controller->GetContainerStatus("not-registered"));
}

TEST_F(ContainerControllerTest, GetContainerLogs_NotRegistered_ReturnsFalse) {
    controller->Initialize();
    EXPECT_FALSE(controller->GetContainerLogs("not-registered"));
}

TEST_F(ContainerControllerTest, StartContainer_EmptyName_ReturnsFalse) {
    controller->Initialize();
    EXPECT_FALSE(controller->StartContainer(""));
}

TEST_F(ContainerControllerTest, StopContainer_EmptyName_ReturnsFalse) {
    controller->Initialize();
    EXPECT_FALSE(controller->StopContainer(""));
}

// ─── 신규 상태 조회 메서드 ────────────────────────────────────────────────────

TEST_F(ContainerControllerTest, GetContainerStatusStr_NotRegistered_ReturnsUnknown) {
    controller->Initialize();
    EXPECT_EQ(controller->GetContainerStatusStr("nonexistent"), "unknown");
}

TEST_F(ContainerControllerTest, GetContainerExitCode_NotRegistered_ReturnsMinusOne) {
    controller->Initialize();
    EXPECT_EQ(controller->GetContainerExitCode("nonexistent"), -1);
}

TEST_F(ContainerControllerTest, IsContainerRunning_NotRegistered_ReturnsFalse) {
    controller->Initialize();
    EXPECT_FALSE(controller->IsContainerRunning("nonexistent"));
}

TEST_F(ContainerControllerTest, GetAllContainers_Empty) {
    controller->Initialize();
    EXPECT_TRUE(controller->GetAllContainers().empty());
}

TEST_F(ContainerControllerTest, GetAllContainers_AfterAdd) {
    controller->Initialize();
    auto c1 = std::make_shared<ros2::adsw_manager::Container>(
        "c1", "img1", "", "", ros2::adsw_manager::ContainerType::DOCKER);
    auto c2 = std::make_shared<ros2::adsw_manager::Container>(
        "c2", "img2", "compose.yaml", "", ros2::adsw_manager::ContainerType::DOCKER_COMPOSE);
    controller->AddContainer(c1);
    controller->AddContainer(c2);

    auto all = controller->GetAllContainers();
    EXPECT_EQ(all.size(), 2u);
}

TEST_F(ContainerControllerTest, ContainerSupported_TrueAfterAdd) {
    controller->Initialize();
    EXPECT_FALSE(controller->ContainerSupported());
    auto c = std::make_shared<ros2::adsw_manager::Container>(
        "c1", "img1", "", "", ros2::adsw_manager::ContainerType::DOCKER);
    controller->AddContainer(c);
    EXPECT_TRUE(controller->ContainerSupported());
}

// Additional tests for better coverage

// Test multiple AddContainer calls
TEST_F(ContainerControllerTest, AddContainer_MultipleContainers) {
    controller->Initialize();
    
    for (int i = 0; i < 5; i++) {
        auto container = std::make_shared<ros2::adsw_manager::Container>(
            "container" + std::to_string(i), 
            "image" + std::to_string(i), 
            "/path/deploy" + std::to_string(i) + ".yaml", 
            "ns" + std::to_string(i));
        EXPECT_TRUE(controller->AddContainer(container));
    }
}

// Test various container names
TEST_F(ContainerControllerTest, StartContainer_VariousNames) {
    controller->Initialize();
    
    // Test with various container names (all should fail as k3s is disabled)
    EXPECT_FALSE(controller->StartContainer("my-container"));
    EXPECT_FALSE(controller->StartContainer("container-with-dashes"));
    EXPECT_FALSE(controller->StartContainer("container_with_underscores"));
    EXPECT_FALSE(controller->StartContainer("ContainerCamelCase"));
}

TEST_F(ContainerControllerTest, StopContainer_VariousNames) {
    controller->Initialize();
    
    EXPECT_FALSE(controller->StopContainer("my-container"));
    EXPECT_FALSE(controller->StopContainer("container-with-dashes"));
    EXPECT_FALSE(controller->StopContainer("container_with_underscores"));
}

TEST_F(ContainerControllerTest, GetContainerStatus_VariousNames) {
    controller->Initialize();
    
    EXPECT_FALSE(controller->GetContainerStatus("my-container"));
    EXPECT_FALSE(controller->GetContainerStatus("container-with-dashes"));
}

TEST_F(ContainerControllerTest, GetContainerLogs_VariousNames) {
    controller->Initialize();
    
    EXPECT_FALSE(controller->GetContainerLogs("my-container"));
    EXPECT_FALSE(controller->GetContainerLogs("container-with-dashes"));
}

// Test Finalize without Initialize
TEST_F(ContainerControllerTest, FinalizeWithoutInitialize) {
    auto* fresh_controller = new ros2::adsw_manager::ContainerController();
    EXPECT_NO_THROW(fresh_controller->Finalize());
    delete fresh_controller;
}

// Test Initialize-Finalize cycles
TEST_F(ContainerControllerTest, InitializeFinalize_MultipleCycles) {
    for (int i = 0; i < 3; i++) {
        EXPECT_TRUE(controller->Initialize());
        auto container = std::make_shared<ros2::adsw_manager::Container>(
            "container-cycle-" + std::to_string(i), 
            "image", 
            "/path/deploy.yaml", 
            "ns");
        EXPECT_TRUE(controller->AddContainer(container));
        controller->Finalize();
    }
}

// Test BaseCommand with various names
TEST_F(ContainerControllerTest, BaseCommand_VariousNames) {
    controller->Initialize();
    
    EXPECT_EQ(controller->BaseCommand(""), "");
    EXPECT_EQ(controller->BaseCommand("some-container"), "");
    EXPECT_EQ(controller->BaseCommand("oss-mmf-ext-ros2"), "");
}

// Test BaseRos2Command with various names
TEST_F(ContainerControllerTest, BaseRos2Command_VariousNames) {
    controller->Initialize();
    
    EXPECT_EQ(controller->BaseRos2Command(""), "");
    EXPECT_EQ(controller->BaseRos2Command("some-container"), "");
    EXPECT_EQ(controller->BaseRos2Command("oss-mmf-ext-ros2"), "");
}

// Test GetContainer with various container names 
TEST_F(ContainerControllerTest, GetContainer_VariousNames) {
    controller->Initialize();
    
    EXPECT_EQ(controller->GetContainer(""), nullptr);
    EXPECT_EQ(controller->GetContainer("nonexistent"), nullptr);
    EXPECT_EQ(controller->GetContainer("test-container-123"), nullptr);
}

// Test ContainerSupported called multiple times
TEST_F(ContainerControllerTest, ContainerSupported_MultipleCalls) {
    controller->Initialize();
    
    EXPECT_FALSE(controller->ContainerSupported());
    EXPECT_FALSE(controller->ContainerSupported());
    EXPECT_FALSE(controller->ContainerSupported());
}


// ═══════════════════════════════════════════════════════════════════════════
// 외부 프로세스 실행 경로 (start/stop/restart, status, exit-code)
//
// 이 경로들은 전부 RunCommand/CaptureCommand 를 지나 docker · kubectl 을 부른다.
// 그래서 실제 런타임이 있어야만 도는 ContainerDockerTest(8) /
// ContainerDockerComposeTest(4) 에만 의존했고, 유닛 레인에서는 한 줄도
// 커버되지 않았다 (262줄 중 184줄 미커버).
//
// ContainerController 의 두 seam 을 서브클래싱해 명령을 가로채면 docker 없이
// 같은 로직을 전부 돌릴 수 있다. 검증 대상은 "무엇을 실행했는가"(명령 문자열)와
// "반환값을 어떻게 해석하는가" 두 가지다.
// ═══════════════════════════════════════════════════════════════════════════

namespace {

using ros2::adsw_manager::Container;
using ros2::adsw_manager::ContainerType;
using ros2::adsw_manager::testing_support::FakeCmdContainerController;

std::shared_ptr<Container> MakeContainer(const std::string& name, ContainerType type) {
    return std::make_shared<Container>(name, name + "-image", "/tmp/" + name + ".yaml",
                                       name + "-ns", type);
}

class ContainerControllerCmdTest : public ::testing::Test {
  protected:
    void SetUp() override {
        Mock_Logger = new MockLogger();
        ON_CALL(*Mock_Logger, Get()).WillByDefault(Return(Mock_Logger));
        ctl = new FakeCmdContainerController();
    }
    void TearDown() override {
        delete ctl;
        delete Mock_Logger;
        Mock_Logger = nullptr;
    }

    std::shared_ptr<Container> Add(const std::string& name, ContainerType type) {
        auto c = MakeContainer(name, type);
        EXPECT_TRUE(ctl->AddContainer(c));
        return c;
    }

    FakeCmdContainerController* ctl;
};

} // namespace

// ── 컨테이너 등록 ───────────────────────────────────────────────────────────

TEST_F(ContainerControllerCmdTest, AddContainer_NullRejected) {
    EXPECT_FALSE(ctl->AddContainer(nullptr));
    EXPECT_TRUE(ctl->GetAllContainers().empty());
}

TEST_F(ContainerControllerCmdTest, AddContainer_DuplicateNameRejected) {
    Add("dup", ContainerType::DOCKER);
    EXPECT_FALSE(ctl->AddContainer(MakeContainer("dup", ContainerType::DOCKER)));
    EXPECT_EQ(ctl->GetAllContainers().size(), 1u);
}

TEST_F(ContainerControllerCmdTest, ContainerSupported_TrueAfterAdd) {
    EXPECT_FALSE(ctl->ContainerSupported());
    Add("c1", ContainerType::DOCKER);
    EXPECT_TRUE(ctl->ContainerSupported());
}

TEST_F(ContainerControllerCmdTest, GetContainer_NotFoundReturnsNull) {
    EXPECT_EQ(ctl->GetContainer("nope"), nullptr);
}

// ── K3S ─────────────────────────────────────────────────────────────────────

TEST_F(ContainerControllerCmdTest, StartK3s_Success) {
    Add("k1", ContainerType::K3S);
    EXPECT_TRUE(ctl->StartContainer("k1"));
    ASSERT_EQ(ctl->run_cmds.size(), 1u);
    EXPECT_EQ(ctl->run_cmds[0], "kubectl apply -f /tmp/k1.yaml");
}

TEST_F(ContainerControllerCmdTest, StartK3s_Failure) {
    Add("k1", ContainerType::K3S);
    ctl->default_rc = 1;
    EXPECT_FALSE(ctl->StartContainer("k1"));
}

TEST_F(ContainerControllerCmdTest, StopK3s_Success) {
    Add("k1", ContainerType::K3S);
    EXPECT_TRUE(ctl->StopContainer("k1"));
    ASSERT_EQ(ctl->run_cmds.size(), 1u);
    EXPECT_EQ(ctl->run_cmds[0], "kubectl delete pod k1 -n k1-ns --ignore-not-found");
}

TEST_F(ContainerControllerCmdTest, StopK3s_Failure) {
    Add("k1", ContainerType::K3S);
    ctl->default_rc = 1;
    EXPECT_FALSE(ctl->StopContainer("k1"));
}

TEST_F(ContainerControllerCmdTest, RestartK3s_DeletesThenApplies) {
    Add("k1", ContainerType::K3S);
    EXPECT_TRUE(ctl->RestartContainer("k1"));
    ASSERT_EQ(ctl->run_cmds.size(), 2u);
    EXPECT_THAT(ctl->run_cmds[0], ::testing::HasSubstr("kubectl delete pod"));
    EXPECT_THAT(ctl->run_cmds[1], ::testing::HasSubstr("kubectl apply -f"));
}

TEST_F(ContainerControllerCmdTest, RestartK3s_StopFailsSkipsApply) {
    Add("k1", ContainerType::K3S);
    ctl->PushRc({1});
    EXPECT_FALSE(ctl->RestartContainer("k1"));
    EXPECT_EQ(ctl->run_cmds.size(), 1u);
}

// ── DOCKER ──────────────────────────────────────────────────────────────────

TEST_F(ContainerControllerCmdTest, StartDocker_ExistingContainerStarts) {
    Add("d1", ContainerType::DOCKER);
    EXPECT_TRUE(ctl->StartContainer("d1"));
    ASSERT_EQ(ctl->run_cmds.size(), 1u);
    EXPECT_EQ(ctl->run_cmds[0], "docker start d1 2>/dev/null");
}

TEST_F(ContainerControllerCmdTest, StartDocker_FallsBackToRun) {
    Add("d1", ContainerType::DOCKER);
    ctl->PushRc({1, 0});   // docker start 실패 → docker run 성공
    EXPECT_TRUE(ctl->StartContainer("d1"));
    ASSERT_EQ(ctl->run_cmds.size(), 2u);
    EXPECT_EQ(ctl->run_cmds[1], "docker run -d --init --name d1 d1-image");
}

TEST_F(ContainerControllerCmdTest, StartDocker_RunIncludesExtraAndCommandArgs) {
    auto c = MakeContainer("d1", ContainerType::DOCKER);
    c->SetExtraArgs({"--network", "host", "-e", "FOO=1"});
    c->SetCommandArgs({"ros2", "launch", "pkg", "app.launch.py"});
    ASSERT_TRUE(ctl->AddContainer(c));

    ctl->PushRc({1, 0});
    EXPECT_TRUE(ctl->StartContainer("d1"));
    ASSERT_EQ(ctl->run_cmds.size(), 2u);
    // extra_args 는 이미지 앞, command_args 는 이미지 뒤에 와야 한다.
    EXPECT_EQ(ctl->run_cmds[1],
              "docker run -d --init --name d1 --network host -e FOO=1 "
              "d1-image ros2 launch pkg app.launch.py");
}

TEST_F(ContainerControllerCmdTest, StartDocker_BothStartAndRunFail) {
    Add("d1", ContainerType::DOCKER);
    ctl->default_rc = 1;
    EXPECT_FALSE(ctl->StartContainer("d1"));
    EXPECT_EQ(ctl->run_cmds.size(), 2u);
}

TEST_F(ContainerControllerCmdTest, StopDocker_StopsAndRemoves) {
    Add("d1", ContainerType::DOCKER);
    EXPECT_TRUE(ctl->StopContainer("d1"));
    ASSERT_EQ(ctl->run_cmds.size(), 1u);
    EXPECT_EQ(ctl->run_cmds[0], "docker stop d1 && docker rm d1");
}

TEST_F(ContainerControllerCmdTest, StopDocker_Failure) {
    Add("d1", ContainerType::DOCKER);
    ctl->default_rc = 1;
    EXPECT_FALSE(ctl->StopContainer("d1"));
}

TEST_F(ContainerControllerCmdTest, RestartDocker_Success) {
    Add("d1", ContainerType::DOCKER);
    EXPECT_TRUE(ctl->RestartContainer("d1"));
    ASSERT_EQ(ctl->run_cmds.size(), 1u);
    EXPECT_EQ(ctl->run_cmds[0], "docker restart d1");
}

TEST_F(ContainerControllerCmdTest, RestartDocker_Failure) {
    Add("d1", ContainerType::DOCKER);
    ctl->default_rc = 1;
    EXPECT_FALSE(ctl->RestartContainer("d1"));
}

// ── DOCKER_COMPOSE ──────────────────────────────────────────────────────────

TEST_F(ContainerControllerCmdTest, StartCompose_Success) {
    Add("c1", ContainerType::DOCKER_COMPOSE);
    EXPECT_TRUE(ctl->StartContainer("c1"));
    ASSERT_EQ(ctl->run_cmds.size(), 1u);
    EXPECT_EQ(ctl->run_cmds[0], "docker compose -f /tmp/c1.yaml up -d");
}

TEST_F(ContainerControllerCmdTest, StartCompose_Failure) {
    Add("c1", ContainerType::DOCKER_COMPOSE);
    ctl->default_rc = 1;
    EXPECT_FALSE(ctl->StartContainer("c1"));
}

TEST_F(ContainerControllerCmdTest, StopCompose_Success) {
    Add("c1", ContainerType::DOCKER_COMPOSE);
    EXPECT_TRUE(ctl->StopContainer("c1"));
    EXPECT_EQ(ctl->run_cmds[0], "docker compose -f /tmp/c1.yaml down");
}

TEST_F(ContainerControllerCmdTest, StopCompose_Failure) {
    Add("c1", ContainerType::DOCKER_COMPOSE);
    ctl->default_rc = 1;
    EXPECT_FALSE(ctl->StopContainer("c1"));
}

TEST_F(ContainerControllerCmdTest, RestartCompose_Success) {
    Add("c1", ContainerType::DOCKER_COMPOSE);
    EXPECT_TRUE(ctl->RestartContainer("c1"));
    EXPECT_EQ(ctl->run_cmds[0], "docker compose -f /tmp/c1.yaml restart");
}

TEST_F(ContainerControllerCmdTest, RestartCompose_Failure) {
    Add("c1", ContainerType::DOCKER_COMPOSE);
    ctl->default_rc = 1;
    EXPECT_FALSE(ctl->RestartContainer("c1"));
}

// ── 알 수 없는 타입 / 미등록 이름 ───────────────────────────────────────────

TEST_F(ContainerControllerCmdTest, UnknownType_AllOperationsFail) {
    Add("u1", ContainerType::UNKNOWN);
    EXPECT_FALSE(ctl->StartContainer("u1"));
    EXPECT_FALSE(ctl->StopContainer("u1"));
    EXPECT_FALSE(ctl->RestartContainer("u1"));
    EXPECT_FALSE(ctl->GetContainerStatus("u1"));
    EXPECT_FALSE(ctl->GetContainerLogs("u1"));
    EXPECT_EQ(ctl->GetContainerStatusStr("u1"), "unknown");
    EXPECT_EQ(ctl->GetContainerExitCode("u1"), -1);
    EXPECT_EQ(ctl->BaseCommand("u1"), "");
    EXPECT_EQ(ctl->BaseRos2Command("u1"), "");
    EXPECT_TRUE(ctl->run_cmds.empty());
}

TEST_F(ContainerControllerCmdTest, UnregisteredName_AllOperationsFail) {
    EXPECT_FALSE(ctl->StartContainer("ghost"));
    EXPECT_FALSE(ctl->StopContainer("ghost"));
    EXPECT_FALSE(ctl->RestartContainer("ghost"));
    EXPECT_FALSE(ctl->GetContainerStatus("ghost"));
    EXPECT_FALSE(ctl->GetContainerLogs("ghost"));
    EXPECT_EQ(ctl->GetContainerStatusStr("ghost"), "unknown");
    EXPECT_EQ(ctl->GetContainerExitCode("ghost"), -1);
    EXPECT_EQ(ctl->BaseCommand("ghost"), "");
    EXPECT_EQ(ctl->BaseRos2Command("ghost"), "");
}

// ── status / logs (RunCommand 반환값 해석) ──────────────────────────────────

TEST_F(ContainerControllerCmdTest, GetContainerStatus_K3sCommandAndResult) {
    Add("k1", ContainerType::K3S);
    EXPECT_TRUE(ctl->GetContainerStatus("k1"));
    EXPECT_EQ(ctl->run_cmds[0],
              "kubectl get pod k1 -n k1-ns -o jsonpath='{.status.phase}'");
}

TEST_F(ContainerControllerCmdTest, GetContainerStatus_DockerFailureRc) {
    Add("d1", ContainerType::DOCKER);
    ctl->default_rc = 1;
    EXPECT_FALSE(ctl->GetContainerStatus("d1"));
    EXPECT_EQ(ctl->run_cmds[0], "docker inspect --format='{{.State.Status}}' d1");
}

TEST_F(ContainerControllerCmdTest, GetContainerLogs_K3sAndDocker) {
    Add("k1", ContainerType::K3S);
    Add("d1", ContainerType::DOCKER);
    EXPECT_TRUE(ctl->GetContainerLogs("k1"));
    EXPECT_EQ(ctl->run_cmds[0], "kubectl logs k1 -n k1-ns -c k1");
    EXPECT_TRUE(ctl->GetContainerLogs("d1"));
    EXPECT_EQ(ctl->run_cmds[1], "docker logs d1");
}

TEST_F(ContainerControllerCmdTest, GetContainerLogs_FailureRc) {
    Add("d1", ContainerType::DOCKER);
    ctl->default_rc = 1;
    EXPECT_FALSE(ctl->GetContainerLogs("d1"));
}

// ── GetContainerStatusStr (CaptureCommand 출력 가공) ────────────────────────

TEST_F(ContainerControllerCmdTest, StatusStr_DockerStripsQuotesAndNewline) {
    Add("d1", ContainerType::DOCKER);
    ctl->capture_out = "'running'\n";
    EXPECT_EQ(ctl->GetContainerStatusStr("d1"), "running");
    EXPECT_EQ(ctl->capture_cmds[0],
              "docker inspect --format='{{.State.Status}}' d1 2>/dev/null");
}

TEST_F(ContainerControllerCmdTest, StatusStr_StripsCarriageReturn) {
    Add("d1", ContainerType::DOCKER);
    ctl->capture_out = "exited\r\n";
    EXPECT_EQ(ctl->GetContainerStatusStr("d1"), "exited");
}

TEST_F(ContainerControllerCmdTest, StatusStr_EmptyOutputBecomesUnknown) {
    Add("d1", ContainerType::DOCKER);
    ctl->capture_out = "";
    EXPECT_EQ(ctl->GetContainerStatusStr("d1"), "unknown");
}

TEST_F(ContainerControllerCmdTest, StatusStr_OnlyQuotesBecomesUnknown) {
    Add("d1", ContainerType::DOCKER);
    ctl->capture_out = "''\n";
    EXPECT_EQ(ctl->GetContainerStatusStr("d1"), "unknown");
}

TEST_F(ContainerControllerCmdTest, StatusStr_K3sUsesJsonpath) {
    Add("k1", ContainerType::K3S);
    ctl->capture_out = "Running\n";
    EXPECT_EQ(ctl->GetContainerStatusStr("k1"), "Running");
    EXPECT_EQ(ctl->capture_cmds[0],
              "kubectl get pod k1 -n k1-ns -o jsonpath='{.status.phase}' 2>/dev/null");
}

TEST_F(ContainerControllerCmdTest, IsContainerRunning_BothCasings) {
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

TEST_F(ContainerControllerCmdTest, ExitCode_ParsesNumber) {
    Add("d1", ContainerType::DOCKER);
    ctl->capture_out = "137\n";
    EXPECT_EQ(ctl->GetContainerExitCode("d1"), 137);
    EXPECT_EQ(ctl->capture_cmds[0],
              "docker inspect --format='{{.State.ExitCode}}' d1 2>/dev/null");
}

TEST_F(ContainerControllerCmdTest, ExitCode_ZeroIsValid) {
    Add("d1", ContainerType::DOCKER);
    ctl->capture_out = "0\n";
    EXPECT_EQ(ctl->GetContainerExitCode("d1"), 0);
}

TEST_F(ContainerControllerCmdTest, ExitCode_NonNumericBecomesMinusOne) {
    Add("d1", ContainerType::DOCKER);
    ctl->capture_out = "not-a-number\n";
    EXPECT_EQ(ctl->GetContainerExitCode("d1"), -1);
}

TEST_F(ContainerControllerCmdTest, ExitCode_EmptyBecomesMinusOne) {
    Add("d1", ContainerType::DOCKER);
    ctl->capture_out = "";
    EXPECT_EQ(ctl->GetContainerExitCode("d1"), -1);
}

TEST_F(ContainerControllerCmdTest, ExitCode_K3sUsesTerminatedJsonpath) {
    Add("k1", ContainerType::K3S);
    ctl->capture_out = "2\n";
    EXPECT_EQ(ctl->GetContainerExitCode("k1"), 2);
    EXPECT_THAT(ctl->capture_cmds[0],
                ::testing::HasSubstr("state.terminated.exitCode"));
}

// ── BaseCommand / BaseRos2Command ───────────────────────────────────────────

TEST_F(ContainerControllerCmdTest, BaseCommand_Docker) {
    Add("d1", ContainerType::DOCKER);
    EXPECT_EQ(ctl->BaseCommand("d1"), "docker exec d1 ");
}

TEST_F(ContainerControllerCmdTest, BaseCommand_Compose) {
    Add("c1", ContainerType::DOCKER_COMPOSE);
    EXPECT_EQ(ctl->BaseCommand("c1"), "docker exec c1 ");
}

TEST_F(ContainerControllerCmdTest, BaseCommand_K3s) {
    Add("k1", ContainerType::K3S);
    EXPECT_EQ(ctl->BaseCommand("k1"), "kubectl exec -n k1-ns k1 -- ");
}

TEST_F(ContainerControllerCmdTest, BaseCommand_SelfContainerIsEmpty) {
    // adsw_manager 자신이 도는 컨테이너로는 exec 하지 않는다.
    EXPECT_EQ(ctl->BaseCommand("oss-mmf-ext-ros2"), "");
    EXPECT_EQ(ctl->BaseRos2Command("oss-mmf-ext-ros2"), "");
}

TEST_F(ContainerControllerCmdTest, BaseRos2Command_SourcesRosSetup) {
    Add("d1", ContainerType::DOCKER);
    Add("k1", ContainerType::K3S);
    EXPECT_EQ(ctl->BaseRos2Command("d1"),
              "docker exec d1 bash -c \"source /opt/ros/humble/setup.bash && ");
    EXPECT_EQ(ctl->BaseRos2Command("k1"),
              "kubectl exec -n k1-ns k1 -- bash -c \"source /opt/ros/humble/setup.bash && ");
}

// ── CaptureCommand 절단 규약 ────────────────────────────────────────────────

TEST_F(ContainerControllerCmdTest, StatusStr_TruncatedAtBufferSize) {
    Add("d1", ContainerType::DOCKER);
    // 실제 구현은 64바이트 버퍼(fgets 로 63자)를 쓴다. 긴 출력이 잘려도
    // 크래시 없이 잘린 문자열이 그대로 나와야 한다.
    ctl->capture_out = std::string(200, 'x');
    EXPECT_EQ(ctl->GetContainerStatusStr("d1"), std::string(63, 'x'));
}

// ═══════════════════════════════════════════════════════════════════════════
// RunCommand / CaptureCommand 의 *실제* 본문
//
// 위 ContainerControllerCmdTest 는 이 둘을 override 해 명령을 가로챈다 —
// docker 없이 상위 로직을 돌리려는 의도된 설계다. 그 부작용으로 두 seam 의
// 진짜 구현(system() / popen())은 실행 0회였다. 특히 CaptureCommand 는
// popen 실패·빈 출력·길이 절단을 전부 자기 안에서 처리하는데, 그 처리 결과가
// GetContainerStatusStr("unknown") / GetContainerExitCode(-1) 의 근거다.
// 가짜로 답을 심으면 그 변환 규약은 검증되지만 규약을 만들어내는 쪽은 안 된다.
//
// docker 는 필요 없다. seam 은 셸 명령 문자열을 그대로 받으므로 셸 내장
// 명령으로 계약을 확인할 수 있다.
// ═══════════════════════════════════════════════════════════════════════════

namespace {

// override 하지 않고 base 구현을 그대로 노출하는 껍데기.
class RealCmdContainerController : public ros2::adsw_manager::ContainerController {
  public:
    using ContainerController::RunCommand;
    using ContainerController::CaptureCommand;
};

class ContainerControllerRealCmdTest : public ::testing::Test {
  protected:
    void SetUp() override {
        Mock_Logger = new MockLogger();
        ON_CALL(*Mock_Logger, Get()).WillByDefault(Return(Mock_Logger));
        ctl = std::make_unique<RealCmdContainerController>();
    }
    void TearDown() override {
        ctl.reset();
        delete Mock_Logger;
        Mock_Logger = nullptr;
    }
    std::unique_ptr<RealCmdContainerController> ctl;
};

TEST_F(ContainerControllerRealCmdTest, RunCommandReportsZeroOnSuccess) {
    // 호출부는 전부 `RunCommand(cmd) == 0` 으로 성공을 판정한다.
    EXPECT_EQ(ctl->RunCommand("true"), 0);
}

TEST_F(ContainerControllerRealCmdTest, RunCommandReportsNonZeroOnFailure) {
    EXPECT_NE(ctl->RunCommand("false"), 0);
}

TEST_F(ContainerControllerRealCmdTest, CaptureCommandReturnsFirstLine) {
    // popen 은 stdout 첫 줄만 읽는다. 개행이 붙어 오는 것도 계약의 일부다 —
    // GetContainerStatusStr 이 이 문자열을 그대로 넘긴다.
    EXPECT_EQ(ctl->CaptureCommand("echo running", 64), "running\n");
}

TEST_F(ContainerControllerRealCmdTest, CaptureCommandReadsOnlyFirstLine) {
    EXPECT_EQ(ctl->CaptureCommand("printf 'first\\nsecond\\n'", 64), "first\n");
}

TEST_F(ContainerControllerRealCmdTest, CaptureCommandEmptyOutputIsEmptyString) {
    // fgets 가 nullptr 을 내는 경로. 호출부가 이걸 "unknown" / -1 로 접는다.
    EXPECT_EQ(ctl->CaptureCommand("true", 64), "");
}

TEST_F(ContainerControllerRealCmdTest, CaptureCommandTruncatesToMaxLen) {
    // max_len 은 널 종료 포함이라 실제로 담기는 건 max_len-1 바이트다.
    // 상태 문자열 버퍼가 넘치지 않는 근거가 여기다.
    const std::string out = ctl->CaptureCommand("printf 'aaaaaaaaaaaaaaaaaaaa'", 8);
    EXPECT_EQ(out, std::string(7, 'a'));
}

TEST_F(ContainerControllerRealCmdTest, CaptureCommandFailedCommandYieldsEmpty) {
    // 명령 자체가 실패해도 popen 은 성공한다(셸은 떴다). stdout 이 비어
    // 빈 문자열로 접히는 게 정상 — 여기서 던지면 상태 폴링이 통째로 멈춘다.
    EXPECT_EQ(ctl->CaptureCommand("exit 3", 64), "");
}

TEST_F(ContainerControllerRealCmdTest, SeamsNeverThrow) {
    EXPECT_NO_THROW({
        (void)ctl->RunCommand("");
        (void)ctl->CaptureCommand("", 16);
    });
}

} // namespace

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
