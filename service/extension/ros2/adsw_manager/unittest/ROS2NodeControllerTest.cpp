// Unit tests for DDSTransmitter class using Google Test and Google Mock
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "adsw_manager/control/ROS2NodeController.hpp"

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

// Stub system() for testing
static int system_stub_result = 0;
extern "C" int system(const char* cmd) {
    (void)cmd;
    return system_stub_result;
}

class ROS2NodeControllerTest : public ::testing::Test {
protected:
    ros2::adsw_manager::ROS2NodeController controller;
    std::string basecommand = "kubectl exec -n test testpod -- ";
    std::string package = "testpkg";
    std::string nodename = "testnode";
};

TEST_F(ROS2NodeControllerTest, StartNodeSuccess) {
    system_stub_result = 0;
    EXPECT_TRUE(controller.StartNode(basecommand, package, nodename));
}

TEST_F(ROS2NodeControllerTest, StartNodeFailure) {
    system_stub_result = 1;
    EXPECT_FALSE(controller.StartNode(basecommand, package, nodename));
}

TEST_F(ROS2NodeControllerTest, StopNodeSuccess) {
    system_stub_result = 0;
    EXPECT_TRUE(controller.StopNode(basecommand, package, nodename));
}

TEST_F(ROS2NodeControllerTest, StopNodeFailure) {
    system_stub_result = 1;
    EXPECT_FALSE(controller.StopNode(basecommand, package, nodename));
}

TEST_F(ROS2NodeControllerTest, GetStateSuccess) {
    system_stub_result = 0;
    EXPECT_TRUE(controller.GetState(basecommand, package, nodename));
}

TEST_F(ROS2NodeControllerTest, GetStateFailure) {
    system_stub_result = 1;
    EXPECT_FALSE(controller.GetState(basecommand, package, nodename));
}

TEST_F(ROS2NodeControllerTest, SetStateAlwaysFalse) {
    EXPECT_FALSE(controller.SetState(basecommand, package, nodename, "activate"));
}

TEST_F(ROS2NodeControllerTest, RestartNodeSuccess) {
    system_stub_result = 0;
    EXPECT_TRUE(controller.RestartNode(basecommand, package, nodename));
}

TEST_F(ROS2NodeControllerTest, RestartNodeFailureOnStop) {
    system_stub_result = 1;
    EXPECT_FALSE(controller.RestartNode(basecommand, package, nodename));
}



int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
