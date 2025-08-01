// Unit tests for DDSTransmitter class using Google Test and Google Mock
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "service/Service.cpp"  // Include actual implementation

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

using namespace ros2::adsw_manager;

class ServiceTest : public ::testing::Test {
protected:
    virtual void SetUp() override {
    }

    virtual void TearDown() override {
    }

    Service ins{"TestService", ServiceType::ONESHOT, RecoveryPolicy::ALWAYS, true};
};

TEST_F(ServiceTest, Constructor_InitialValues_TC001) {
    EXPECT_EQ(ins.GetName(), "TestService");
    EXPECT_EQ(ins.GetServiceType(), ServiceType::ONESHOT);
    EXPECT_EQ(ins.GetRecoveryPolicy(), RecoveryPolicy::ALWAYS);
    EXPECT_TRUE(ins.IsEnabled());
    EXPECT_EQ(ins.GetServiceState(), ServiceState::NOT_RUNNING);
}

TEST_F(ServiceTest, SetAndGetServiceState_TC002) {
    ins.SetServiceState(ServiceState::RUNNING);
    EXPECT_EQ(ins.GetServiceState(), ServiceState::RUNNING);

    ins.SetServiceState(ServiceState::TERMINATING);
    EXPECT_EQ(ins.GetServiceState(), ServiceState::TERMINATING);
}

TEST_F(ServiceTest, IsEnabled_TC003) {
    EXPECT_TRUE(ins.IsEnabled());

    // Reconstruct with false
    Service ins2("AnotherService", ServiceType::NORMAL, RecoveryPolicy::ALWAYS, false);
    EXPECT_FALSE(ins2.IsEnabled());
}

TEST_F(ServiceTest, GetName_TC005) {
    EXPECT_EQ(ins.GetName(), "TestService");
}
TEST_F(ServiceTest, GetPackageAndLaunchFile_TC006) {
    ins.SetPackage("test_package");
    EXPECT_EQ(ins.GetPackage(), "test_package");

    ins.SetLaunchFile("test_launch_file");
    EXPECT_EQ(ins.GetLaunchFile(), "test_launch_file");
}

TEST_F(ServiceTest, SetAndGetPID_TC007) {
    ins.SetPID(12345);
    EXPECT_EQ(ins.GetPID(), 12345);

    ins.SetPID(67890);
    EXPECT_EQ(ins.GetPID(), 67890);
}
TEST_F(ServiceTest, SetAndGetRunningNodes_TC008) {
    ins.SetRunningNodes(5);
    EXPECT_EQ(ins.GetMaxNodes(), 5);

    ins.SetRunningNodes(10);
    EXPECT_EQ(ins.GetMaxNodes(), 10);
}

TEST_F(ServiceTest, SetAndGetServiceState_TC012) {
    ins.SetServiceState(ServiceState::INITIALIZING);
    EXPECT_EQ(ins.GetServiceState(), ServiceState::INITIALIZING);

    ins.SetServiceState(ServiceState::RUNNING);
    EXPECT_EQ(ins.GetServiceState(), ServiceState::RUNNING);
}


int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}