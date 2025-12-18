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
    // k3s is disabled by default, so even after adding it returns nullptr
    EXPECT_EQ(retrieved, nullptr);
}

TEST_F(ContainerControllerTest, BaseCommand_K3sDisabled) {
    controller->Initialize();
    EXPECT_EQ(controller->BaseCommand(""), "");
}

TEST_F(ContainerControllerTest, BaseRos2Command_K3sDisabled) {
    controller->Initialize();
    EXPECT_EQ(controller->BaseRos2Command(""), "");
}

TEST_F(ContainerControllerTest, BaseCommand_AdswRos2Container) {
    controller->Initialize();
    EXPECT_EQ(controller->BaseCommand("oss-mmf-ext-ros2"), "");
}

TEST_F(ContainerControllerTest, BaseRos2Command_AdswRos2Container) {
    controller->Initialize();
    EXPECT_EQ(controller->BaseRos2Command("oss-mmf-ext-ros2"), "");
}

TEST_F(ContainerControllerTest, StartContainer_K3sDisabled) {
    controller->Initialize();
    EXPECT_FALSE(controller->StartContainer("test-container"));
}

TEST_F(ContainerControllerTest, StopContainer_K3sDisabled) {
    controller->Initialize();
    EXPECT_FALSE(controller->StopContainer("test-container"));
}

TEST_F(ContainerControllerTest, GetContainerStatus_K3sDisabled) {
    controller->Initialize();
    EXPECT_FALSE(controller->GetContainerStatus("test-container"));
}

TEST_F(ContainerControllerTest, GetContainerLogs_K3sDisabled) {
    controller->Initialize();
    EXPECT_FALSE(controller->GetContainerLogs("test-container"));
}

// Test with empty container name
TEST_F(ContainerControllerTest, StartContainer_EmptyName) {
    controller->Initialize();
    EXPECT_FALSE(controller->StartContainer(""));
}

TEST_F(ContainerControllerTest, StopContainer_EmptyName) {
    controller->Initialize();
    EXPECT_FALSE(controller->StopContainer(""));
}

TEST_F(ContainerControllerTest, GetContainerStatus_EmptyName) {
    controller->Initialize();
    EXPECT_FALSE(controller->GetContainerStatus(""));
}

TEST_F(ContainerControllerTest, GetContainerLogs_EmptyName) {
    controller->Initialize();
    EXPECT_FALSE(controller->GetContainerLogs(""));
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
