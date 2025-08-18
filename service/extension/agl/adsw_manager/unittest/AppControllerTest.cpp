#include <gtest/gtest.h>

#include "AppController.hpp"

#include <string>

using namespace adsw_manager::agl;

extern "C" int __real_system(const char*);

extern "C" int __wrap_system(const char* cmd);

extern std::string g_last_cmd;
extern int g_next_status; // 테스트에서 설정: (0<<8)=성공, (1<<8)=비매칭, -1=system 실패 등
static int ExitStatus(int code){ return (code << 8); }

// ----------------------------- LaunchApp -----------------------------

TEST(AppControllerLegacyTest, LaunchBuildsExpectedCommand) {
    g_next_status = ExitStatus(0);
    AppController c;  // 기존 코드 그대로

    EXPECT_TRUE(c.LaunchApp("speedometer", "app-cont"));
    EXPECT_NE(g_last_cmd.find("docker exec -d app-cont /usr/bin/flutter-auto"), std::string::npos);
}

TEST(AppControllerLegacyTest, LaunchReturnsFalseOnNonZeroExit) {
    g_next_status = ExitStatus(2);        // 서브커맨드 실패
    AppController c;
    EXPECT_FALSE(c.LaunchApp("speedometer", "app-cont"));
}

TEST(AppControllerLegacyTest, LaunchReturnsFalseOnSystemFailure) {
    g_next_status = -1;                   // system() 자체 실패
    AppController c;
    EXPECT_FALSE(c.LaunchApp("speedometer", "app-cont"));
}

// ----------------------------- TerminateApp -----------------------------

TEST(AppControllerLegacyTest, TerminateBuildsExpectedCommandAndSucceeds) {
    g_next_status = ExitStatus(0);
    AppController c;

    EXPECT_TRUE(c.TerminateApp("speedometer", "app-cont"));
    // 현재 구현은 appId를 쓰지 않고 flutter-auto만 죽임
    EXPECT_NE(g_last_cmd.find("docker exec app-cont pkill flutter-auto"), std::string::npos);
}

TEST(AppControllerLegacyTest, TerminateReturnsFalseOnNonZeroExit) {
    g_next_status = ExitStatus(3);
    AppController c;
    EXPECT_FALSE(c.TerminateApp("speedometer", "app-cont"));
}

TEST(AppControllerLegacyTest, TerminateReturnsFalseOnSystemFailure) {
    g_next_status = -1;
    AppController c;
    EXPECT_FALSE(c.TerminateApp("speedometer", "app-cont"));
}

// ----------------------------- IsAppRunning -----------------------------

TEST(AppControllerLegacyTest, IsAppRunning_BuildsExpectedCommand) {
    g_next_status = ExitStatus(0);  // 매칭됨 → true
    AppController c;

    EXPECT_TRUE(c.IsAppRunning("speedometer", "app-cont"));
    // bash -c 'ps -a | grep -q speedometer' 패턴 확인(작은따옴표 포함)
    EXPECT_NE(g_last_cmd.find("docker exec app-cont bash -c 'ps -a | grep -q speedometer'"),
              std::string::npos);
}

TEST(AppControllerLegacyTest, IsAppRunning_ReturnsFalseOnExit1_NoMatch) {
    g_next_status = ExitStatus(1);  // grep 매칭 없음
    AppController c;
    EXPECT_FALSE(c.IsAppRunning("speedometer", "app-cont"));
}

TEST(AppControllerLegacyTest, IsAppRunning_ReturnsFalseOnOtherExitCodes) {
    g_next_status = ExitStatus(2);  // grep 에러 등
    AppController c;
    EXPECT_FALSE(c.IsAppRunning("speedometer", "app-cont"));
}

TEST(AppControllerLegacyTest, IsAppRunning_ReturnsFalseOnSystemFailure) {
    g_next_status = -1;             // system() 자체 실패
    AppController c;
    EXPECT_FALSE(c.IsAppRunning("speedometer", "app-cont"));
}