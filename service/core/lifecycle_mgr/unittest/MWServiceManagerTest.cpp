// Unit tests for MWServiceManager class.
#include <cstdlib>
//
// F011: lifecycle_mgr no longer fork/exec's middleware services; the
// container's entrypoint script (docker/entrypoint/core_entrypoint.sh) does.
// As a result, RunMWServiceManagement / StartMWService / StopMWService /
// run_process / kill_process were removed.  The tests below cover the
// remaining surface: Initialize, Start, Stop, LoadMWConfig (multiple
// branches), and the k3s_mode load path.

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <fstream>
#include <memory>
#include <string>
#include <vector>
#include <cstdio>

#include "service/MWServiceManager.hpp"
#include "mock/mock_mwservice.hpp"
#include "mock/mock_argumentconfig.hpp"
#include "mock/mock_logger.hpp"

using namespace lifecycle_manager;

using ::testing::Return;
using ::testing::_;

class MWServiceManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        Mock_Logger = new MockLogger();
        Mock_MWService = nullptr;
    }
    void TearDown() override {
        delete Mock_Logger;
        Mock_Logger = nullptr;
        Mock_MWService = nullptr;
    }
};

TEST_F(MWServiceManagerTest, StartAndInitialize) {
    MWServiceManager mw;

    // Initialize returns false when MMF_CONFIG_PATH doesn't point to a real
    // file — the default test environment has no such file.  Start is now a
    // no-op (post-F011) and must always succeed.
    (void)mw.Initialize();
    EXPECT_TRUE(mw.Start());
}

TEST_F(MWServiceManagerTest, LoadMWConfigReturnsFalseOnInvalidFile) {
    MWServiceManager mgr;
    EXPECT_FALSE(mgr.LoadMWConfig("/invalid/path/to/config.yaml"));
}

TEST_F(MWServiceManagerTest, LoadMWConfigReturnsTrueOnValidFile) {
    MWServiceManager mgr;
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

TEST_F(MWServiceManagerTest, LoadConfigK3sModeTrue) {
    MWServiceManager mgr;
    const char* tmpFile = "/tmp/test_config_k3s_true.yaml";
    std::ofstream ofs(tmpFile);
    ofs << "lifecycle_mgr_config:\n"
        << "  k3s_mode: true\n";
    ofs.close();

    EXPECT_TRUE(mgr.LoadMWConfig(tmpFile));
    EXPECT_TRUE(mgr.IsK3sMode());
    std::remove(tmpFile);
}

TEST_F(MWServiceManagerTest, LoadConfigK3sModeDefaultsFalse) {
    MWServiceManager mgr;
    const char* tmpFile = "/tmp/test_config_k3s_default.yaml";
    std::ofstream ofs(tmpFile);
    ofs << "config_middleware: []\n";
    ofs.close();

    EXPECT_TRUE(mgr.LoadMWConfig(tmpFile));
    EXPECT_FALSE(mgr.IsK3sMode());
    std::remove(tmpFile);
}

TEST_F(MWServiceManagerTest, InitializeReturnsFalseWithoutConfig) {
    // run_unittest.sh exports MMF_CONFIG_PATH for the whole suite, and once
    // core install runs that dir holds a real config_middleware.yaml — so the
    // old "rely on env being unset" assumption no longer holds. Point the env
    // at a guaranteed-nonexistent dir to deterministically exercise the
    // missing-config path.
    const char* saved = std::getenv("MMF_CONFIG_PATH");
    std::string saved_str = saved ? saved : "";
    setenv("MMF_CONFIG_PATH", "/nonexistent_mmf_config_dir", 1);

    MWServiceManager mgr;
    EXPECT_FALSE(mgr.Initialize());

    if (saved) {
        setenv("MMF_CONFIG_PATH", saved_str.c_str(), 1);
    } else {
        unsetenv("MMF_CONFIG_PATH");
    }
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
