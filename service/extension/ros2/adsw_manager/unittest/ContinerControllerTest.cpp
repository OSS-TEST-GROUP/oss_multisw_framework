// Unit tests for ContainerController class using Google Test and Google Mock
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "adsw_manager/control/ContainerController.hpp"
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

TEST_F(ContainerControllerTest, InitializeAndFinalize) {
    ASSERT_TRUE(controller->Initialize());
    controller->Finalize();
}

TEST_F(ContainerControllerTest, BaseCommand_K3sDisabled) {
    EXPECT_EQ(controller->BaseCommand(""), "");
}

TEST_F(ContainerControllerTest, BaseRos2Command_K3sDisabled) {
    EXPECT_EQ(controller->BaseRos2Command(""), "");
}

TEST_F(ContainerControllerTest, StartContainer_K3sDisabled) {
    EXPECT_FALSE(controller->StartContainer("test-container"));
}

TEST_F(ContainerControllerTest, StopContainer_K3sDisabled) {
    EXPECT_FALSE(controller->StopContainer("test-container"));
}

TEST_F(ContainerControllerTest, GetContainerStatus_K3sDisabled) {
    EXPECT_FALSE(controller->GetContainerStatus("test-container"));
}

TEST_F(ContainerControllerTest, GetContainerLogs_K3sDisabled) {
    EXPECT_FALSE(controller->GetContainerLogs("test-container"));
}
TEST_F(ContainerControllerTest, BaseCommand_K3sEnabled) {
    controller->Initialize(); // Assuming this sets k3s_container_ to true
    EXPECT_NE(controller->BaseCommand(""), "");
}
TEST_F(ContainerControllerTest, BaseRos2Command_K3sEnabled) {
    controller->Initialize(); // Assuming this sets k3s_container_ to true
    EXPECT_NE(controller->BaseRos2Command(""), "");
}
TEST_F(ContainerControllerTest, StartContainer_K3sEnabled) {
    controller->Initialize(); // Assuming this sets k3s_container_ to true
    EXPECT_TRUE(controller->StartContainer("test-container"));
}
TEST_F(ContainerControllerTest, StopContainer_K3sEnabled) {
    controller->Initialize(); // Assuming this sets k3s_container_ to true
    EXPECT_TRUE(controller->StopContainer("test-container"));
}
TEST_F(ContainerControllerTest, GetContainerStatus_K3sEnabled) {
    controller->Initialize(); // Assuming this sets k3s_container_ to true
    EXPECT_TRUE(controller->GetContainerStatus("test-container"));
}
TEST_F(ContainerControllerTest, GetContainerLogs_K3sEnabled) {
    controller->Initialize(); // Assuming this sets k3s_container_ to true
    EXPECT_TRUE(controller->GetContainerLogs("test-container"));
}



int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
