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

TEST_F(ServiceTest, AllServiceStates_TC013) {
    ins.SetServiceState(ServiceState::NOT_RUNNING);
    EXPECT_EQ(ins.GetServiceState(), ServiceState::NOT_RUNNING);
    
    ins.SetServiceState(ServiceState::INITIALIZING);
    EXPECT_EQ(ins.GetServiceState(), ServiceState::INITIALIZING);
    
    ins.SetServiceState(ServiceState::RUNNING);
    EXPECT_EQ(ins.GetServiceState(), ServiceState::RUNNING);
    
    ins.SetServiceState(ServiceState::TERMINATING);
    EXPECT_EQ(ins.GetServiceState(), ServiceState::TERMINATING);
}

TEST_F(ServiceTest, AllServiceTypes_TC014) {
    Service normal("normal", ServiceType::NORMAL, RecoveryPolicy::ALWAYS, true);
    EXPECT_EQ(normal.GetServiceType(), ServiceType::NORMAL);
    
    Service oneshot("oneshot", ServiceType::ONESHOT, RecoveryPolicy::ALWAYS, true);
    EXPECT_EQ(oneshot.GetServiceType(), ServiceType::ONESHOT);
}

TEST_F(ServiceTest, AllRecoveryPolicies_TC015) {
    Service always("svc1", ServiceType::NORMAL, RecoveryPolicy::ALWAYS, true);
    EXPECT_EQ(always.GetRecoveryPolicy(), RecoveryPolicy::ALWAYS);
    
    Service manual("svc2", ServiceType::NORMAL, RecoveryPolicy::MANUAL, true);
    EXPECT_EQ(manual.GetRecoveryPolicy(), RecoveryPolicy::MANUAL);
    
    Service on_failure("svc3", ServiceType::NORMAL, RecoveryPolicy::ON_FAILURE, true);
    EXPECT_EQ(on_failure.GetRecoveryPolicy(), RecoveryPolicy::ON_FAILURE);
}

TEST_F(ServiceTest, EmptyServiceName_TC016) {
    Service empty("", ServiceType::NORMAL, RecoveryPolicy::ALWAYS, true);
    EXPECT_EQ(empty.GetName(), "");
}

TEST_F(ServiceTest, LongServiceName_TC017) {
    std::string long_name(1000, 'x');
    Service long_svc(long_name, ServiceType::NORMAL, RecoveryPolicy::ALWAYS, true);
    EXPECT_EQ(long_svc.GetName(), long_name);
}

TEST_F(ServiceTest, SpecialCharactersInName_TC018) {
    Service special("service-name_123.test", ServiceType::NORMAL, RecoveryPolicy::ALWAYS, true);
    EXPECT_EQ(special.GetName(), "service-name_123.test");
}

TEST_F(ServiceTest, GetPackage_DefaultValue_TC019) {
    Service svc("my_service", ServiceType::NORMAL, RecoveryPolicy::ALWAYS, true);
    // When package is empty, GetPackage should return service name
    EXPECT_EQ(svc.GetPackage(), "my_service");
}

TEST_F(ServiceTest, GetPackage_WithSetPackage_TC020) {
    ins.SetPackage("custom_package");
    EXPECT_EQ(ins.GetPackage(), "custom_package");
}

TEST_F(ServiceTest, EmptyPackageName_TC021) {
    ins.SetPackage("");
    EXPECT_EQ(ins.GetPackage(), "TestService"); // Should return service name when package is empty
}

TEST_F(ServiceTest, EmptyLaunchFile_TC022) {
    ins.SetLaunchFile("");
    EXPECT_EQ(ins.GetLaunchFile(), "");
}

TEST_F(ServiceTest, LongLaunchFile_TC023) {
    std::string long_file(500, 'y');
    ins.SetLaunchFile(long_file);
    EXPECT_EQ(ins.GetLaunchFile(), long_file);
}

TEST_F(ServiceTest, SetRunningNodes_NodeIncrease_TC024) {
    ins.SetRunningNodes(5);
    EXPECT_EQ(ins.GetRunningNodes(), 5);
    EXPECT_EQ(ins.GetMaxNodes(), 5);
    
    ins.SetRunningNodes(10);
    EXPECT_EQ(ins.GetRunningNodes(), 10);
    EXPECT_EQ(ins.GetMaxNodes(), 10);
}

TEST_F(ServiceTest, SetRunningNodes_NodeDecrease_TC025) {
    ins.SetRunningNodes(10);
    EXPECT_EQ(ins.GetRunningNodes(), 10);
    EXPECT_EQ(ins.GetMaxNodes(), 10);
    
    ins.SetRunningNodes(5);
    EXPECT_EQ(ins.GetRunningNodes(), 5);
    EXPECT_EQ(ins.GetMaxNodes(), 10); // Max should stay at 10
}

TEST_F(ServiceTest, SetRunningNodes_Zero_TC026) {
    ins.SetRunningNodes(0);
    EXPECT_EQ(ins.GetRunningNodes(), 0);
    EXPECT_EQ(ins.GetMaxNodes(), 0);
}

TEST_F(ServiceTest, SetRunningNodes_NegativeValue_TC027) {
    ins.SetRunningNodes(-5);
    EXPECT_EQ(ins.GetRunningNodes(), -5);
}

TEST_F(ServiceTest, SetRunningNodes_SameValue_TC028) {
    ins.SetRunningNodes(5);
    ins.SetRunningNodes(5);
    EXPECT_EQ(ins.GetRunningNodes(), 5);
    EXPECT_EQ(ins.GetMaxNodes(), 5);
}

TEST_F(ServiceTest, PID_ZeroValue_TC029) {
    ins.SetPID(0);
    EXPECT_EQ(ins.GetPID(), 0);
}

TEST_F(ServiceTest, PID_LargeValue_TC030) {
    uint64_t large_pid = 0xFFFFFFFFFFFFFFFF;
    ins.SetPID(large_pid);
    EXPECT_EQ(ins.GetPID(), large_pid);
}

TEST_F(ServiceTest, PID_SequentialUpdates_TC031) {
    ins.SetPID(100);
    EXPECT_EQ(ins.GetPID(), 100);
    
    ins.SetPID(200);
    EXPECT_EQ(ins.GetPID(), 200);
    
    ins.SetPID(300);
    EXPECT_EQ(ins.GetPID(), 300);
}

TEST_F(ServiceTest, DisabledService_TC032) {
    Service disabled("disabled_svc", ServiceType::NORMAL, RecoveryPolicy::ALWAYS, false);
    EXPECT_FALSE(disabled.IsEnabled());
}

TEST_F(ServiceTest, SpecialPathInLaunchFile_TC033) {
    ins.SetLaunchFile("/path/to/launch/file.launch.py");
    EXPECT_EQ(ins.GetLaunchFile(), "/path/to/launch/file.launch.py");
}

TEST_F(ServiceTest, InitialPIDValue_TC034) {
    Service new_svc("new", ServiceType::NORMAL, RecoveryPolicy::ALWAYS, true);
    EXPECT_EQ(new_svc.GetPID(), 0);
}

TEST_F(ServiceTest, InitialNodeCounts_TC035) {
    Service new_svc("new", ServiceType::NORMAL, RecoveryPolicy::ALWAYS, true);
    EXPECT_EQ(new_svc.GetRunningNodes(), 0);
    EXPECT_EQ(new_svc.GetMaxNodes(), 0);
}


int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}