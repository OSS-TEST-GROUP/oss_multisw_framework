// Unit tests for MWServiceManager class using Google Test and Google Mock

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <fstream>
#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include <sys/wait.h>
#include <signal.h>

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

TEST_F(MWServiceManagerTest, LoadConfigWithMissingFields) {
    MWServiceManager mgr;
    const char* tmpFile = "/tmp/test_config_missing_fields.yaml";
    std::ofstream ofs(tmpFile);
    ofs << "config_middleware:\n"
        << "  - code: 1\n"
        << "    name: test_service\n"
        << "    # missing exepath\n"
        << "    active: true\n";
    ofs.close();

    EXPECT_TRUE(mgr.LoadMWConfig(tmpFile));
    std::remove(tmpFile);
}

TEST_F(MWServiceManagerTest, LoadConfigWithArguments) {
    MWServiceManager mgr;
    const char* tmpFile = "/tmp/test_config_with_args.yaml";
    std::ofstream ofs(tmpFile);
    ofs << "config_middleware:\n"
        << "  - code: 1\n"
        << "    name: test_service\n"
        << "    exepath: /bin/true\n"
        << "    active: true\n"
        << "    args:\n"
        << "      - arg_name: arg1\n"
        << "        description: First argument\n"
        << "        arg_type: string\n"
        << "        arg_default: value1\n"
        << "      - arg_name: arg2\n"
        << "        description: Second argument\n"
        << "        arg_type: int\n"
        << "        arg_default: 42\n";
    ofs.close();

    EXPECT_TRUE(mgr.LoadMWConfig(tmpFile));
    std::remove(tmpFile);
}

TEST_F(MWServiceManagerTest, LoadConfigWithInactiveService) {
    MWServiceManager mgr;
    const char* tmpFile = "/tmp/test_config_inactive.yaml";
    std::ofstream ofs(tmpFile);
    ofs << "config_middleware:\n"
        << "  - code: 1\n"
        << "    name: inactive_service\n"
        << "    exepath: /bin/true\n"
        << "    active: false\n";
    ofs.close();

    EXPECT_TRUE(mgr.LoadMWConfig(tmpFile));
    std::remove(tmpFile);
}

TEST_F(MWServiceManagerTest, LoadConfigWithMissingActiveField) {
    MWServiceManager mgr;
    const char* tmpFile = "/tmp/test_config_missing_active.yaml";
    std::ofstream ofs(tmpFile);
    ofs << "config_middleware:\n"
        << "  - code: 1\n"
        << "    name: test_service\n"
        << "    exepath: /bin/true\n";
    ofs.close();

    EXPECT_TRUE(mgr.LoadMWConfig(tmpFile));
    std::remove(tmpFile);
}

TEST_F(MWServiceManagerTest, LoadConfigWithInvalidYAML) {
    MWServiceManager mgr;
    const char* tmpFile = "/tmp/test_config_invalid.yaml";
    std::ofstream ofs(tmpFile);
    ofs << "config_middleware:\n"
        << "  - code: 1\n"
        << "    name: test_service\n"
        << "    exepath: /bin/true\n"
        << "    active: [invalid yaml structure";
    ofs.close();

    EXPECT_FALSE(mgr.LoadMWConfig(tmpFile));
    std::remove(tmpFile);
}

TEST_F(MWServiceManagerTest, LoadConfigWithEmptyFile) {
    MWServiceManager mgr;
    const char* tmpFile = "/tmp/test_config_empty.yaml";
    std::ofstream ofs(tmpFile);
    ofs << "";
    ofs.close();

    EXPECT_TRUE(mgr.LoadMWConfig(tmpFile));
    std::remove(tmpFile);
}

TEST_F(MWServiceManagerTest, LoadConfigWithNoMiddlewareSection) {
    MWServiceManager mgr;
    const char* tmpFile = "/tmp/test_config_no_middleware.yaml";
    std::ofstream ofs(tmpFile);
    ofs << "some_other_section:\n"
        << "  - key: value\n";
    ofs.close();

    EXPECT_TRUE(mgr.LoadMWConfig(tmpFile));
    std::remove(tmpFile);
}

TEST_F(MWServiceManagerTest, RunProcessWithInvalidExecutable) {
    MWServiceManager mgr;
    std::vector<std::string> args;
    pid_t pid = mgr.run_process("invalid", "/invalid/path/to/executable", args);
    
    if (pid > 0) {
        // If fork succeeded, child will fail on execvp
        int status;
        waitpid(pid, &status, 0);
        EXPECT_NE(WIFEXITED(status) && WEXITSTATUS(status), 0);
    }
}

TEST_F(MWServiceManagerTest, RunProcessWithArguments) {
    MWServiceManager mgr;
    std::vector<std::string> args = {"1"};  // sleep for 1 second
    pid_t pid = mgr.run_process("sleep", "/bin/sleep", args);
    
    EXPECT_GT(pid, 0);
    
    // Kill the process
    kill(pid, SIGKILL);
    int status;
    waitpid(pid, &status, 0);
}

TEST_F(MWServiceManagerTest, RunProcessWithWaitTime) {
    MWServiceManager mgr;
    std::vector<std::string> args = {"10"};
    
    auto start = std::chrono::steady_clock::now();
    pid_t pid = mgr.run_process("sleep", "/bin/sleep", args);
    auto end = std::chrono::steady_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();
    EXPECT_GE(duration, 0);
    EXPECT_GT(pid, 0);
    
    // Kill the process
    kill(pid, SIGKILL);
    int status;
    waitpid(pid, &status, 0);
}

TEST_F(MWServiceManagerTest, KillProcessInvalidPID) {
    MWServiceManager mgr;
    EXPECT_FALSE(mgr.kill_process(-1));
}

TEST_F(MWServiceManagerTest, StopMWServiceWithNoPID) {
    auto mw_service = std::make_shared<MWService>(1, "test", "/bin/sleep", true, std::vector<ArgumentConfig>{});
    MWServiceManager mgr;
    
    // Don't start the service, just try to stop it
    EXPECT_NO_THROW(mgr.StopMWService(mw_service));
    EXPECT_EQ(mw_service->GetPID(), -1);
}

TEST_F(MWServiceManagerTest, SkipLifecycleMgrInRunMWServiceManagement) {
    MWServiceManager mgr;
    auto lifecycle_svc = std::make_shared<MWService>(1, "lifecycle_mgr", "/bin/sleep", true, std::vector<ArgumentConfig>{});
    auto other_svc = std::make_shared<MWService>(2, "other", "/bin/sleep", true, std::vector<ArgumentConfig>{});
    
    mgr.mw_services_["lifecycle_mgr"] = lifecycle_svc;
    mgr.mw_services_["other"] = other_svc;
    
    mgr.RunMWServiceManagement();
    
    // lifecycle_mgr should not be started
    EXPECT_EQ(lifecycle_svc->GetPID(), -1);
    // other should be started
    EXPECT_GT(other_svc->GetPID(), 0);
    
    mgr.StopMWService(other_svc);
}

TEST_F(MWServiceManagerTest, InitializeReturnsFalse) {
    MWServiceManager mgr;
    EXPECT_FALSE(mgr.Initialize());
}

TEST_F(MWServiceManagerTest, StartReturnsTrue) {
    MWServiceManager mgr;
    EXPECT_TRUE(mgr.Start());
}

TEST_F(MWServiceManagerTest, StopDoesNotThrow) {
    MWServiceManager mgr;
    EXPECT_NO_THROW(mgr.Stop());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
