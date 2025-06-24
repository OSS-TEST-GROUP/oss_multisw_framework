// Unit tests for MWServiceManager class using Google Test and Google Mock

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "mock/mock_mw_service_manager.hpp"
#include "mock/mock_mwservice.hpp"
#include "mock/mock_argumentconfig.hpp"

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

class MWServiceManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code if needed
    }
    void TearDown() override {
        // Cleanup code if needed
    }
};

TEST_F(MWServiceManagerTest, StartAndInitialize) {
    MockMWServiceManager mw;
    EXPECT_CALL(mw, Initialize()).WillOnce(::testing::Return(true));
    EXPECT_TRUE(mw.Initialize());
    EXPECT_CALL(mw, Start()).WillOnce(::testing::Return(true));
    EXPECT_TRUE(mw.Start());
}

// Add more tests for MWServiceManager as needed
TEST_F(MWServiceManagerTest, LoadMWConfigReturnsFalseOnInvalidFile) {
    MWServiceManager mgr;
    // Pass a non-existent file path
    EXPECT_FALSE(mgr.LoadMWConfig("/invalid/path/to/config.yaml"));
}

TEST_F(MWServiceManagerTest, LoadMWConfigReturnsTrueOnValidFile) {
    MWServiceManager mgr;
    // Create a temporary YAML file
    const char* tmpFile = "/tmp/test_config.yaml";
    std::ofstream ofs(tmpFile);
    ofs << "config_middleware:\n"
        << "  - code: 1\n"
        << "    name: test_service\n"
        << "    exepath: /bin/true\n"
        << "    active: true\n";
    ofs.close();

    EXPECT_TRUE(mgr.LoadMWConfig(tmpFile));
    std::remove(tmpFile);
}

TEST_F(MWServiceManagerTest, StartMWServiceSetsPID) {
    // Use a mock MWService with a dummy executable
    auto mw_service = std::make_shared<MWService>(1, "test", "/bin/sleep", true, std::vector<ArgumentConfig>{});
    MWServiceManager mgr;
    mgr.StartMWService(mw_service);
    EXPECT_GT(mw_service->GetPID(), 0);
    // Clean up the process
    mgr.StopMWService(mw_service);
}

TEST_F(MWServiceManagerTest, StopMWServiceResetsPID) {
    auto mw_service = std::make_shared<MWService>(1, "test", "/bin/sleep", true, std::vector<ArgumentConfig>{});
    MWServiceManager mgr;
    mgr.StartMWService(mw_service);
    pid_t pid = mw_service->GetPID();
    ASSERT_GT(pid, 0);
    mgr.StopMWService(mw_service);
    EXPECT_EQ(mw_service->GetPID(), 0);
}

TEST_F(MWServiceManagerTest, RunMWServiceManagementStartsAllActiveServices) {
    MWServiceManager mgr;
    // Add two dummy services
    auto s1 = std::make_shared<MWService>(1, "svc1", "/bin/sleep", true, std::vector<ArgumentConfig>{});
    auto s2 = std::make_shared<MWService>(2, "svc2", "/bin/sleep", true, std::vector<ArgumentConfig>{});
    mgr.mw_services_["svc1"] = s1;
    mgr.mw_services_["svc2"] = s2;
    mgr.RunMWServiceManagement();
    EXPECT_GT(s1->GetPID(), 0);
    EXPECT_GT(s2->GetPID(), 0);
    mgr.StopMWService(s1);
    mgr.StopMWService(s2);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
