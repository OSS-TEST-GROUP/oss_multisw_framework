#include <gtest/gtest.h>
#include "AppController.hpp"

#include <string>
#include <vector>
#include <cstdio>
#include <cstring>
#include <unistd.h>

// ====== 캡처용 전역 ======
static int g_exec_child_exit_code = 0; // 자식 프로세스(=execvp 래퍼) 종료 코드
static std::string g_last_exec_file;
static std::vector<std::string> g_last_exec_argv;
static int g_exec_call_count = 0;

static std::vector<std::string> g_popen_cmds;  // popen에 들어온 커맨드 기록
static std::string g_mock_load_state   = "loaded";
static std::string g_mock_active_state = "active";

// ====== real 심볼 선언 ======
extern "C" {
int  __real_execvp(const char* file, char* const argv[]);
FILE* __real_popen(const char* cmd, const char* mode);
int   __real_pclose(FILE* stream);
}

// ====== wrap 구현 ======
// -Wl,--wrap=execvp
extern "C" int __wrap_execvp(const char* file, char* const argv[]) {
    // argv 캡처
    g_last_exec_file = file ? file : "";
    g_last_exec_argv.clear();
    if (argv) {
        for (char* const* p = argv; *p; ++p) {
            g_last_exec_argv.emplace_back(*p ? *p : "");
        }
    }
    g_exec_call_count++;

    // 실제 exec는 절대 하지 않고, 바로 종료
    _exit(g_exec_child_exit_code);
    return -1; // 도달 불가(컴파일러 경고 억제)
}

// -Wl,--wrap=popen
extern "C" FILE* __wrap_popen(const char* cmd, const char* mode) {
    g_popen_cmds.emplace_back(cmd ? cmd : "");

    // 요청된 show 항목에 따라 가짜 출력 생성
    std::string s = cmd ? cmd : "";
    std::string out;
    if (s.find("LoadState") != std::string::npos) {
        out = g_mock_load_state + "\n";
    } else if (s.find("ActiveState") != std::string::npos) {
        out = g_mock_active_state + "\n";
    } else {
        out = "";
    }

    FILE* f = tmpfile();
    if (!f) return nullptr;
    if (!out.empty()) {
        fwrite(out.data(), 1, out.size(), f);
        rewind(f);
    }
    return f;
}

// -Wl,--wrap=pclose
extern "C" int __wrap_pclose(FILE* stream) {
    if (stream) fclose(stream);
    return 0;
}

// ====== 테스트 ======
using namespace adsw_manager::agl;

struct AppControllerCmdTest : ::testing::Test {
    void SetUp() override {
        g_exec_child_exit_code = 0;
        g_last_exec_file.clear();
        g_last_exec_argv.clear();
        g_exec_call_count = 0;

        g_popen_cmds.clear();
        g_mock_load_state   = "loaded";
        g_mock_active_state = "active";
    }
};

// -------- LaunchApp(Command) --------
TEST_F(AppControllerCmdTest, Launch_BuildsSystemctlStartWithServiceSuffix) {
    AppController c;
    ASSERT_TRUE(c.LaunchApp("adsw-monitor")); // child exit=0 -> 성공 취급

    ASSERT_EQ(g_exec_call_count, 1);
    EXPECT_EQ(g_last_exec_file, "systemctl");
    ASSERT_EQ(g_last_exec_argv.size(), 3u);
    EXPECT_EQ(g_last_exec_argv[0], "systemctl");
    EXPECT_EQ(g_last_exec_argv[1], "start");
    EXPECT_EQ(g_last_exec_argv[2], "adsw-monitor.service"); // 자동 .service 부착 확인
}

TEST_F(AppControllerCmdTest, Launch_KeepsDotServiceIfAlreadyGiven) {
    AppController c;
    ASSERT_TRUE(c.LaunchApp("speedometer.service"));

    ASSERT_EQ(g_exec_call_count, 1);
    EXPECT_EQ(g_last_exec_argv[1], "start");
    EXPECT_EQ(g_last_exec_argv[2], "speedometer.service");
}

// -------- TerminateApp(Command) --------
TEST_F(AppControllerCmdTest, Terminate_BuildsSystemctlStop) {
    AppController c;
    ASSERT_TRUE(c.TerminateApp("adsw-monitor"));

    ASSERT_EQ(g_exec_call_count, 1);
    EXPECT_EQ(g_last_exec_file, "systemctl");
    ASSERT_EQ(g_last_exec_argv.size(), 3u);
    EXPECT_EQ(g_last_exec_argv[1], "stop");
    EXPECT_EQ(g_last_exec_argv[2], "adsw-monitor.service");
}

// -------- IsAppRunning(Command) --------
TEST_F(AppControllerCmdTest, IsAppRunning_BuildsTwoShowQueriesWithValue) {
    AppController c;

    // true를 반환하게끔 가짜 상태 지정
    g_mock_load_state   = "loaded";
    g_mock_active_state = "active";

    EXPECT_TRUE(c.IsAppRunning("adsw-monitor"));

    // popen이 두 번 호출되어야 함: LoadState, ActiveState
    ASSERT_GE(g_popen_cmds.size(), 2u);
    EXPECT_EQ(g_popen_cmds[0],
              "systemctl show adsw-monitor.service -p LoadState --value");
    EXPECT_EQ(g_popen_cmds[1],
              "systemctl show adsw-monitor.service -p ActiveState --value");
}

TEST_F(AppControllerCmdTest, IsAppRunning_AppNameWithoutSuffix_StillQueriesServiceName) {
    AppController c;
    g_mock_load_state   = "loaded";
    g_mock_active_state = "inactive"; // 반환값은 false지만 커맨드만 검증

    (void)c.IsAppRunning("speedometer");
    ASSERT_GE(g_popen_cmds.size(), 2u);
    EXPECT_NE(g_popen_cmds[0].find("speedometer.service"), std::string::npos);
    EXPECT_NE(g_popen_cmds[1].find("speedometer.service"), std::string::npos);
}

// -------- 실패 코드도 커맨드만 검증(선택) --------
TEST_F(AppControllerCmdTest, Launch_ReturnsFalse_OnChildNonZeroButCommandStillCorrect) {
    AppController c;
    g_exec_child_exit_code = 3; // 자식 종료코드 3으로 설정 -> 실패 반환

    EXPECT_FALSE(c.LaunchApp("adsw-monitor"));
    ASSERT_EQ(g_exec_call_count, 1);
    EXPECT_EQ(g_last_exec_argv[1], "start");
    EXPECT_EQ(g_last_exec_argv[2], "adsw-monitor.service");
}
