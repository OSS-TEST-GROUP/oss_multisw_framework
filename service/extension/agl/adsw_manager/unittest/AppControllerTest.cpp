#include <gtest/gtest.h>

#define private public
#include "AppController.hpp"
#undef private

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <string>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>

namespace {

static std::string g_helpers_dir;
static std::string g_old_path;
static std::string g_log_path;
static std::string g_state_dir;

std::string DirnameOf(const std::string& path)
{
    auto pos = path.find_last_of("/\\");
    if (pos == std::string::npos) return ".";
    return path.substr(0, pos);
}

void EnsureExecutable(const std::string& path)
{
    // chmod +x
    ::chmod(path.c_str(), 0755);
}

void TruncateFile(const std::string& path)
{
    std::ofstream ofs(path, std::ios::trunc);
}

void RmRf(const std::string& path)
{
    std::string cmd = "rm -rf '" + path + "' >/dev/null 2>&1";
    std::system(cmd.c_str());
}

void MkdirP(const std::string& path)
{
    std::string cmd = "mkdir -p '" + path + "' >/dev/null 2>&1";
    std::system(cmd.c_str());
}

void Touch(const std::string& path)
{
    std::ofstream ofs(path, std::ios::app);
}

std::string ExistsFile(const std::string& name)   { return g_state_dir + "/exists_" + name; }
std::string RunningFile(const std::string& name)  { return g_state_dir + "/running_" + name; }

void SetFail(const std::string& what)
{
    // ADSW_TEST_DOCKER_FAIL="start|run|stop|inspect|container_inspect|version"
    if (what.empty()) {
        ::unsetenv("ADSW_TEST_DOCKER_FAIL");
    } else {
        ::setenv("ADSW_TEST_DOCKER_FAIL", what.c_str(), 1);
    }
}

} // namespace

namespace adsw_manager::agl {

class AppControllerTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        g_helpers_dir = DirnameOf(__FILE__) + "/helpers";
        EnsureExecutable(g_helpers_dir + "/docker");
    }

    void SetUp() override {
        // 원래 PATH 저장
        const char* cur = ::getenv("PATH");
        saved_path_ = cur ? cur : "";

        std::string new_path = g_helpers_dir + ":" + saved_path_;
        ::setenv("PATH", new_path.c_str(), 1);

        g_log_path = "docker_stub.log";
        g_state_dir = "docker_stub_state";
        ::setenv("ADSW_TEST_DOCKER_LOG", g_log_path.c_str(), 1);
        ::setenv("ADSW_TEST_DOCKER_STATE_DIR", g_state_dir.c_str(), 1);

        TruncateFile(g_log_path);
        RmRf(g_state_dir);
        MkdirP(g_state_dir);

        ::unsetenv("ADSW_TEST_DOCKER_FAIL");
    }

    void TearDown() override {
        // env 정리
        ::unsetenv("ADSW_TEST_DOCKER_FAIL");
        ::unsetenv("ADSW_TEST_DOCKER_LOG");
        ::unsetenv("ADSW_TEST_DOCKER_STATE_DIR");

        // PATH 원복
        ::setenv("PATH", saved_path_.c_str(), 1);
    }

private:
    std::string saved_path_;
};

TEST_F(AppControllerTest, Initialize_Called)
{
    AppController c;
    EXPECT_TRUE(c.Initialize());
}

TEST_F(AppControllerTest, RunDocker_VersionSuccessAndFail)
{
    AppController c;

    int st_ok = c.RunDocker({"--version"});
    ASSERT_GE(st_ok, 0);
    ASSERT_TRUE(WIFEXITED(st_ok));
    EXPECT_EQ(WEXITSTATUS(st_ok), 0);

    SetFail("version");
    int st_fail = c.RunDocker({"--version"});
    ASSERT_GE(st_fail, 0);
    ASSERT_TRUE(WIFEXITED(st_fail));
    EXPECT_NE(WEXITSTATUS(st_fail), 0);
}

TEST_F(AppControllerTest, ReadDockerOutput_CapturesStdout)
{
    AppController c;
    std::string out;

    EXPECT_TRUE(c.ReadDockerOutput("docker --version 2>/dev/null", out));
    EXPECT_NE(out.find("Docker version"), std::string::npos);
}

TEST_F(AppControllerTest, IsAppRunning_TrueWhenExistsAndRunningFilePresent)
{
    AppController c;
    const std::string name = "app_run_true";

    Touch(ExistsFile(name));
    Touch(RunningFile(name));

    EXPECT_TRUE(c.IsAppRunning(name));
}

TEST_F(AppControllerTest, IsAppRunning_FalseWhenExistsButNotRunning)
{
    AppController c;
    const std::string name = "app_run_false";

    Touch(ExistsFile(name));

    EXPECT_FALSE(c.IsAppRunning(name));
}

TEST_F(AppControllerTest, IsAppRunning_FalseWhenInspectFails)
{
    AppController c;
    const std::string name = "app_inspect_fail";

    Touch(ExistsFile(name));
    Touch(RunningFile(name));

    SetFail("inspect");

    EXPECT_FALSE(c.IsAppRunning(name));
}

TEST_F(AppControllerTest, LaunchApp_WhenNotExists_RunSuccess_CreatesStateAndRunning)
{
    AppController c;
    const std::string name = "app_launch_run_ok";

    EXPECT_TRUE(c.LaunchApp(name));

    std::ifstream f1(ExistsFile(name));
    std::ifstream f2(RunningFile(name));
    EXPECT_TRUE(f1.good());
    EXPECT_TRUE(f2.good());

    EXPECT_TRUE(c.IsAppRunning(name));
}

TEST_F(AppControllerTest, LaunchApp_WhenExists_StartSuccess_SetsRunning)
{
    AppController c;
    const std::string name = "app_launch_start_ok";

    Touch(ExistsFile(name));

    EXPECT_TRUE(c.LaunchApp(name));

    std::ifstream f2(RunningFile(name));
    EXPECT_TRUE(f2.good());
    EXPECT_TRUE(c.IsAppRunning(name));
}

TEST_F(AppControllerTest, LaunchApp_WhenExists_StartFail_ReturnsFalse)
{
    AppController c;
    const std::string name = "app_launch_start_fail";

    Touch(ExistsFile(name));
    SetFail("start");

    EXPECT_FALSE(c.LaunchApp(name));
}

TEST_F(AppControllerTest, LaunchApp_WhenNotExists_RunFail_ReturnsFalse)
{
    AppController c;
    const std::string name = "app_launch_run_fail";

    SetFail("run");

    EXPECT_FALSE(c.LaunchApp(name));
}

TEST_F(AppControllerTest, LaunchApp_WhenContainerInspectFails_FallsBackToRun)
{
    AppController c;
    const std::string name = "app_inspect_fail_fallback_run";

    SetFail("container_inspect");

    EXPECT_TRUE(c.LaunchApp(name));

    std::ifstream f1(ExistsFile(name));
    std::ifstream f2(RunningFile(name));
    EXPECT_TRUE(f1.good());
    EXPECT_TRUE(f2.good());
}

TEST_F(AppControllerTest, TerminateApp_StopSuccessAndFail)
{
    AppController c;
    const std::string name = "app_stop_ok_then_missing";

    Touch(ExistsFile(name));
    Touch(RunningFile(name));

    EXPECT_TRUE(c.TerminateApp(name));
    std::ifstream f2(RunningFile(name));
    EXPECT_FALSE(f2.good());

}

TEST_F(AppControllerTest, TerminateApp_StopFail_WhenNotExists)
{
    AppController c;
    const std::string name = "app_stop_missing";

    EXPECT_FALSE(c.TerminateApp(name));
}

TEST_F(AppControllerTest, TerminateApp_StopFail_ByEnv)
{
    AppController c;
    const std::string name = "app_stop_env_fail";

    Touch(ExistsFile(name));
    Touch(RunningFile(name));

    SetFail("stop");
    EXPECT_FALSE(c.TerminateApp(name));
}

} // namespace adsw_manager::agl
