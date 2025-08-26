#include <gtest/gtest.h>
#include "AppController.hpp"

#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/stat.h>

using namespace adsw_manager::agl;

#ifndef MOCK_SYSCTL_DIR
#define MOCK_SYSCTL_DIR "unittest/helpers"
#endif

#ifndef MOCK_SYSCTL_LOG
#define MOCK_SYSCTL_LOG "sysctl.log"  // 빌드 디렉토리에 생성
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

// 파일 전체 읽기
static std::string ReadAll(const std::string& p) {
    std::ifstream ifs(p);
    std::ostringstream oss;
    oss << ifs.rdbuf();
    return oss.str();
}

// 파일 비우기
static void Truncate(const std::string& p) {
    std::ofstream ofs(p, std::ios::trunc);
    (void)ofs;
}

static void EnsureExecutable(const std::string& path) {
    struct stat st{};
    if (stat(path.c_str(), &st) == 0) {
        // 소유자 실행권한 없으면 추가
        if (!(st.st_mode & S_IXUSR)) {
            chmod(path.c_str(), st.st_mode | S_IXUSR);
        }
    }
}

} // namespace

// -------------------- 테스트 픽스처 --------------------
class AppControllerTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        // helpers 절대경로: <이 파일>/helpers
        g_helpers_dir = DirnameOf(__FILE__) + "/helpers";
        const std::string shim = g_helpers_dir + "/systemctl";

        // 실행권한 보정(깃 권한이 안 먹힌 환경 대비)
        EnsureExecutable(shim);

        // 로그는 빌드/실행 디렉토리에 생성
        g_log_path = "sysctl.log";
        setenv("ADSW_TEST_SYSCTL_LOG", g_log_path.c_str(), 1);

        // PATH 앞에 helpers 추가
        const char *cur = getenv("PATH");
        g_old_path = cur ? cur : "";
        std::string new_path = g_helpers_dir + ":" + g_old_path;
        setenv("PATH", new_path.c_str(), 1);

        // ::setenv("ADSW_TEST_SYSCTL_LOG", MOCK_SYSCTL_LOG, 1);

        // const char* cur = ::getenv("PATH");
        // g_old_path = cur ? cur : "";
        // std::string new_path = std::string(MOCK_SYSCTL_DIR) + ":" + g_old_path;
        // ::setenv("PATH", new_path.c_str(), 1);
    }

    static void TearDownTestSuite() {
        if (!g_old_path.empty()) ::setenv("PATH", g_old_path.c_str(), 1);
    }

    void SetUp() override {
        Truncate(g_log_path);
        ::unsetenv("ADSW_TEST_EXIT_CODE");
        ::unsetenv("ADSW_TEST_LOAD_STATE");
        ::unsetenv("ADSW_TEST_ACTIVE_STATE");
    }
};

// ----------------------------- LaunchApp -----------------------------

TEST_F(AppControllerTest, Launch_BuildsSystemctlStartWithServiceSuffix) {
    AppController c;
    ::setenv("ADSW_TEST_EXIT_CODE", "0", 1); // 성공

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
    ::setenv("ADSW_TEST_EXIT_CODE", "3", 1); // 실패로 처리

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