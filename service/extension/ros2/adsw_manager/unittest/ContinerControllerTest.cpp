// Unit tests for ContainerController class using Google Test and Google Mock
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "control/ContainerController.hpp"
#include "service/Container.hpp"
#include "mock/MockLogger.hpp"

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


int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
