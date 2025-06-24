#include <gtest/gtest.h>

#include "service/Service.cpp"  // Include actual implementation

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

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}