#include <gtest/gtest.h>
#include "AppController.hpp"
#include "TestPrinter.hpp"

#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/stat.h>
#include <cerrno>

using namespace adsw_manager::agl;

#ifndef MOCK_SYSCTL_DIR
#define MOCK_SYSCTL_DIR "unittest/helpers"
#endif

#ifndef MOCK_SYSCTL_LOG
#define MOCK_SYSCTL_LOG "sysctl.log"
#endif

namespace {
std::string g_helpers_dir;
std::string g_log_path;
std::string g_old_path;

static std::string DirnameOf(const char* file_macro) {
    std::string p = file_macro ? file_macro : "";
    auto pos = p.find_last_of("/\\");
    return (pos == std::string::npos) ? std::string(".") : p.substr(0, pos);
}

static std::string ReadAll(const std::string& p) {
    std::ifstream ifs(p);
    std::ostringstream oss;
    oss << ifs.rdbuf();
    return oss.str();
}

static void Truncate(const std::string& p) {
    std::ofstream ofs(p, std::ios::trunc);
    (void)ofs;
}

static void EnsureExecutable(const std::string& path) {
    struct stat st{};
    if (stat(path.c_str(), &st) == 0) {
        if (!(st.st_mode & S_IXUSR)) {
            chmod(path.c_str(), st.st_mode | S_IXUSR);
        }
    }
}

struct ExitCalled {
    int code;
};

struct ScopedSysHooks {
    ScopedSysHooks() = default;
    ~ScopedSysHooks() {
#ifdef UNIT_TEST
        adsw_manager::agl::testhook::ResetAll();
#endif
    }
};

}

class AppControllerTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        g_helpers_dir = DirnameOf(__FILE__) + "/helpers";
        const std::string shim = g_helpers_dir + "/systemctl";

        EnsureExecutable(shim);

        g_log_path = "sysctl.log";
        setenv("ADSW_TEST_SYSCTL_LOG", g_log_path.c_str(), 1);

        const char *cur = getenv("PATH");
        g_old_path = cur ? cur : "";
        std::string new_path = g_helpers_dir + ":" + g_old_path;
        setenv("PATH", new_path.c_str(), 1);
    }

    static void TearDownTestSuite() {
        if (!g_old_path.empty()) ::setenv("PATH", g_old_path.c_str(), 1);
    }

    void SetUp() override {
        Truncate(g_log_path);
        ::unsetenv("ADSW_TEST_EXIT_CODE");
        ::unsetenv("ADSW_TEST_LOAD_STATE");
        ::unsetenv("ADSW_TEST_ACTIVE_STATE");
        ::unsetenv("ADSW_TEST_FAIL_LOADSTATE");
        ::unsetenv("ADSW_TEST_FAIL_ACTIVESTATE");
#ifdef UNIT_TEST
        adsw_manager::agl::testhook::ResetAll();
#endif
    }

    void TearDown() override {
#ifdef UNIT_TEST
        adsw_manager::agl::testhook::ResetAll();
#endif
    }
};

// ----------------------------- Initialize -----------------------------

TEST_F(AppControllerTest, Initialize_true) {
    AppController c;
    EXPECT_TRUE(c.Initialize());
}

// ----------------------------- LaunchApp -----------------------------

TEST_F(AppControllerTest, Launch_BuildsSystemctlStartWithServiceSuffix) {
    AppController c;
    ::setenv("ADSW_TEST_EXIT_CODE", "0", 1);

    ASSERT_TRUE(c.LaunchApp("adsw-monitor"));

    const std::string log = ReadAll(g_log_path);
    ASSERT_NE(log.find("systemctl start adsw-monitor.service"), std::string::npos)
        << "LOG:\n" << log;
}

TEST_F(AppControllerTest, Launch_KeepsDotServiceIfAlreadyGiven) {
    AppController c;
    ::setenv("ADSW_TEST_EXIT_CODE", "0", 1);

    ASSERT_TRUE(c.LaunchApp("speedometer.service"));

    const std::string log = ReadAll(g_log_path);
    ASSERT_NE(log.find("systemctl start speedometer.service"), std::string::npos)
        << "LOG:\n" << log;
}

TEST_F(AppControllerTest, Launch_ReturnsFalse_OnChildNonZeroButCommandStillCorrect) {
    AppController c;
    ::setenv("ADSW_TEST_EXIT_CODE", "3", 1);

    EXPECT_FALSE(c.LaunchApp("adsw-monitor"));

    const std::string log = ReadAll(g_log_path);
    ASSERT_NE(log.find("systemctl start adsw-monitor.service"), std::string::npos)
        << "LOG:\n" << log;
}

// ----------------------------- TerminateApp -----------------------------

TEST_F(AppControllerTest, Terminate_BuildsSystemctlStop) {
    AppController c;
    ::setenv("ADSW_TEST_EXIT_CODE", "0", 1);

    ASSERT_TRUE(c.TerminateApp("adsw-monitor"));

    const std::string log = ReadAll(g_log_path);
    ASSERT_NE(log.find("systemctl stop adsw-monitor.service"), std::string::npos)
        << "LOG:\n" << log;
}

TEST_F(AppControllerTest, Terminate_ReturnsFalse_WhenSystemctlStopExitNonZero) {
    AppController c;
    ::setenv("ADSW_TEST_EXIT_CODE", "5", 1);

    EXPECT_FALSE(c.TerminateApp("adsw-monitor"));

    const std::string log = ReadAll(g_log_path);
    ASSERT_NE(log.find("systemctl stop adsw-monitor.service"), std::string::npos)
        << "LOG:\n" << log;
}

TEST_F(AppControllerTest, Terminate_ReturnsFalse_WhenStopExitNonZero_StopUnitSystemFailBranch) {
    AppController c;
    ::setenv("ADSW_TEST_EXIT_CODE", "7", 1);

    EXPECT_FALSE(c.TerminateApp("adsw-monitor"));

    const std::string log = ReadAll(g_log_path);
    ASSERT_NE(log.find("systemctl stop adsw-monitor.service"), std::string::npos)
        << "LOG:\n" << log;
}

// ----------------------------- IsAppRunning -----------------------------

TEST_F(AppControllerTest, IsAppRunning_BuildsTwoShowQueriesWithValue) {
    AppController c;
    ::setenv("ADSW_TEST_LOAD_STATE", "loaded", 1);
    ::setenv("ADSW_TEST_ACTIVE_STATE", "active", 1);

    EXPECT_TRUE(c.IsAppRunning("adsw-monitor"));

    const std::string log = ReadAll(g_log_path);
    ASSERT_NE(log.find("systemctl show adsw-monitor.service -p LoadState --value"), std::string::npos)
        << "LOG:\n" << log;
    ASSERT_NE(log.find("systemctl show adsw-monitor.service -p ActiveState --value"), std::string::npos)
        << "LOG:\n" << log;
}

TEST_F(AppControllerTest, IsAppRunning_AppNameWithoutSuffix_StillQueriesServiceName) {
    AppController c;
    ::setenv("ADSW_TEST_LOAD_STATE", "loaded", 1);
    ::setenv("ADSW_TEST_ACTIVE_STATE", "inactive", 1);

    EXPECT_FALSE(c.IsAppRunning("speedometer"));

    const std::string log = ReadAll(g_log_path);
    ASSERT_NE(log.find("systemctl show speedometer.service -p LoadState --value"), std::string::npos)
        << "LOG:\n" << log;
    ASSERT_NE(log.find("systemctl show speedometer.service -p ActiveState --value"), std::string::npos)
        << "LOG:\n" << log;
}

TEST_F(AppControllerTest, IsAppRunning_ReturnsTrue_WhenActiveStateActivating) {
    AppController c;
    ::setenv("ADSW_TEST_LOAD_STATE", "loaded", 1);
    ::setenv("ADSW_TEST_ACTIVE_STATE", "activating", 1);

    EXPECT_TRUE(c.IsAppRunning("adsw-monitor"));

    const std::string log = ReadAll(g_log_path);
    ASSERT_NE(log.find("systemctl show adsw-monitor.service -p LoadState --value"), std::string::npos)
        << "LOG:\n" << log;
    ASSERT_NE(log.find("systemctl show adsw-monitor.service -p ActiveState --value"), std::string::npos)
        << "LOG:\n" << log;
}

TEST_F(AppControllerTest, IsAppRunning_ReturnsFalse_WhenActiveStateFailed) {
    AppController c;
    ::setenv("ADSW_TEST_LOAD_STATE", "loaded", 1);
    ::setenv("ADSW_TEST_ACTIVE_STATE", "failed", 1);

    EXPECT_FALSE(c.IsAppRunning("adsw-monitor"));

    const std::string log = ReadAll(g_log_path);
    ASSERT_NE(log.find("systemctl show adsw-monitor.service -p LoadState --value"), std::string::npos)
        << "LOG:\n" << log;
    ASSERT_NE(log.find("systemctl show adsw-monitor.service -p ActiveState --value"), std::string::npos)
        << "LOG:\n" << log;
}

TEST_F(AppControllerTest, IsAppRunning_ReturnsFalse_WhenUnitNotLoaded) {
    AppController c;
    ::setenv("ADSW_TEST_LOAD_STATE", "not-found", 1);

    EXPECT_FALSE(c.IsAppRunning("adsw-monitor"));

    const std::string log = ReadAll(g_log_path);
    ASSERT_NE(log.find("systemctl show adsw-monitor.service -p LoadState --value"), std::string::npos)
        << "LOG:\n" << log;
    ASSERT_EQ(log.find("systemctl show adsw-monitor.service -p ActiveState --value"), std::string::npos)
        << "LOG:\n" << log;
}

TEST_F(AppControllerTest, IsAppRunning_ReturnsFalse_WhenLoadStateQueryFails_RunAndCaptureFalse) {
    AppController c;

    ::setenv("ADSW_TEST_FAIL_LOADSTATE", "1", 1);

    EXPECT_FALSE(c.IsAppRunning("adsw-monitor"));

    const std::string log = ReadAll(g_log_path);
    ASSERT_NE(log.find("systemctl show adsw-monitor.service -p LoadState --value"), std::string::npos)
        << "LOG:\n" << log;
    ASSERT_EQ(log.find("systemctl show adsw-monitor.service -p ActiveState --value"), std::string::npos)
        << "LOG:\n" << log;
}

TEST_F(AppControllerTest, IsAppRunning_ReturnsFalse_WhenActiveStateQueryFails_RunAndCaptureFalse) {
    AppController c;

    ::setenv("ADSW_TEST_LOAD_STATE", "loaded", 1);
    ::setenv("ADSW_TEST_FAIL_ACTIVESTATE", "1", 1);

    EXPECT_FALSE(c.IsAppRunning("adsw-monitor"));

    const std::string log = ReadAll(g_log_path);
    ASSERT_NE(log.find("systemctl show adsw-monitor.service -p LoadState --value"), std::string::npos)
        << "LOG:\n" << log;
    ASSERT_NE(log.find("systemctl show adsw-monitor.service -p ActiveState --value"), std::string::npos)
        << "LOG:\n" << log;
}

#ifdef UNIT_TEST

TEST_F(AppControllerTest, RunSystemctl_ForkFails_pidLessThan0Branch) {
    ScopedSysHooks _;

    AppController c;

    adsw_manager::agl::testhook::SetFork([]() -> pid_t {
        errno = EAGAIN;
        return -1; // pid < 0
    });

    EXPECT_FALSE(c.LaunchApp("adsw-monitor"));
}

TEST_F(AppControllerTest, RunSystemctl_ChildBranch_ExecvpFails_Exit127Covered) {
    ScopedSysHooks _;

    AppController c;

    adsw_manager::agl::testhook::SetFork([]() -> pid_t {
        return 0; // pid == 0
    });

    adsw_manager::agl::testhook::SetExecvp([](const char*, char* const*) -> int {
        errno = ENOENT;
        return -1;
    });

    adsw_manager::agl::testhook::SetExit([](int code) {
        throw ExitCalled{code};
    });

    try {
        (void)c.LaunchApp("adsw-monitor");
        FAIL() << "Expected ExitCalled";
    } catch (const ExitCalled& e) {
        EXPECT_EQ(e.code, 127);
    }
}

TEST_F(AppControllerTest, RunSystemctl_WaitpidFails_waitpidErrorBranch) {
    ScopedSysHooks _;

    AppController c;

    adsw_manager::agl::testhook::SetFork([]() -> pid_t {
        return 1234;
    });

    adsw_manager::agl::testhook::SetWaitpid([](pid_t, int*, int) -> pid_t {
        errno = ECHILD;
        return -1;
    });

    EXPECT_FALSE(c.LaunchApp("adsw-monitor"));
}

TEST_F(AppControllerTest, RunSystemctl_ParentSuccess_StatusExit0Branch) {
    ScopedSysHooks _;

    AppController c;

    adsw_manager::agl::testhook::SetFork([]() -> pid_t {
        return 1234;
    });

    adsw_manager::agl::testhook::SetWaitpid([](pid_t pid, int* status, int) -> pid_t {
        *status = 0;
        return pid;
    });

    EXPECT_TRUE(c.LaunchApp("adsw-monitor"));
}

#endif  // UNIT_TEST
