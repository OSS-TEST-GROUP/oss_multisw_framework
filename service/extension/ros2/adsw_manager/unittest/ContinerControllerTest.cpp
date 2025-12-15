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


class ContainerControllerTest : public ::testing::Test {
protected:
    void SetUp() override {
        Mock_Logger = new MockLogger();
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
    EXPECT_NE(retrieved, nullptr);
    if (retrieved) {
        EXPECT_EQ(retrieved->GetName(), "test-container");
    }
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



int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
