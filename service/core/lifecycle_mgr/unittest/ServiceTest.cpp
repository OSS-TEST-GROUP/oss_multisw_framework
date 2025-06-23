// Unit tests for Service class using Google Test and Google Mock
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <string>
#include <memory>

#include "service/Service.hpp"

#include "unittest/mock/mock_service.hpp"

using namespace lifecycle_manager;

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

class ServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code if needed
    }
    void TearDown() override {
        // Cleanup code if needed
    }
};


TEST_F(ServiceTest, DefaultConstructor) {
    Service s;
    EXPECT_EQ(s.GetName(), "");
    EXPECT_FALSE(s.isEnabled());
}

TEST_F(ServiceTest, SetAndGetServiceMode) {
    Service s;
    s.SetServiceMode(ServiceMode::NORMAL);
    EXPECT_EQ(s.GetServiceMode(), ServiceMode::NORMAL);
}

TEST_F(ServiceTest, SetAndGetServiceState) {
    Service s;
    s.SetServiceState(ServiceState::RUNNING);
    EXPECT_EQ(s.GetServiceState(), ServiceState::RUNNING);
}

// Add more tests for Service as needed
TEST_F(ServiceTest, ParameterizedConstructor) {
    Service s("TestService", ServiceType::NORMAL, RecoveryPolicy::ALWAYS, true);
    EXPECT_EQ(s.GetName(), "TestService");
    EXPECT_EQ(s.GetServiceType(), ServiceType::NORMAL);
    EXPECT_EQ(s.GetRecoveryPolicy(), RecoveryPolicy::ALWAYS);
    EXPECT_TRUE(s.isEnabled());
    EXPECT_EQ(s.GetServiceMode(), ServiceMode::NORMAL);
    EXPECT_EQ(s.GetServiceState(), ServiceState::NOT_RUNNING);
    EXPECT_EQ(s.GetLastServiceState(), ServiceState::NOT_RUNNING);
    EXPECT_EQ(s.GetRecoverdCount(), 0);
}

TEST_F(ServiceTest, ServiceStateTransition) {
    Service s;
    s.SetServiceState(ServiceState::RUNNING);
    EXPECT_EQ(s.GetServiceState(), ServiceState::RUNNING);
    EXPECT_EQ(s.GetLastServiceState(), ServiceState::NOT_RUNNING);
    s.SetServiceState(ServiceState::RUNNING);
    EXPECT_EQ(s.GetServiceState(), ServiceState::RUNNING);
    EXPECT_EQ(s.GetLastServiceState(), ServiceState::RUNNING);
}

TEST_F(ServiceTest, RecoverdCountOperations) {
    Service s;
    EXPECT_EQ(s.GetRecoverdCount(), 0);
    s.IncreateRecoverdCount();
    EXPECT_EQ(s.GetRecoverdCount(), 1);
    s.IncreateRecoverdCount();
    EXPECT_EQ(s.GetRecoverdCount(), 2);
    s.ClearRecoverdCount();
    EXPECT_EQ(s.GetRecoverdCount(), 0);
}

TEST_F(ServiceTest, PrintDoesNotThrow) {
    Service s("PrintService", ServiceType::NORMAL, RecoveryPolicy::ALWAYS, false);
    EXPECT_NO_THROW(s.Print());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
