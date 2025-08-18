#include <gtest/gtest.h>
#include "ContainerController.hpp"

#include <string>
#include <deque>
#include <vector>

using namespace adsw_manager::agl;

extern "C" int __real_system(const char*);

std::string g_last_cmd;
std::vector<std::string> g_cmds;     // 모든 호출 기록
int g_next_status = 0;                // 큐가 비었을 때 기본 반환
std::deque<int> g_statuses;           // 호출 순서대로 꺼내 쓸 상태값

extern "C" int __wrap_system(const char* cmd) {
    g_last_cmd = cmd ? cmd : "";
    g_cmds.push_back(g_last_cmd);
    if (!g_statuses.empty()) {
        int s = g_statuses.front();
        g_statuses.pop_front();
        return s;
    }
    return g_next_status;
}

static int ExitStatus(int code) { return (code << 8); } // 정상 종료 인코딩
static void ResetRecorder() {
    g_last_cmd.clear();
    g_cmds.clear();
    g_statuses.clear();
    g_next_status = ExitStatus(0);
}
static void QueueStatuses(std::initializer_list<int> sts) {
    g_statuses.clear();
    for (int s : sts) g_statuses.push_back(s);
}

// ----------------------------- StartContainer -----------------------------

TEST(ContainerControllerTest, StartContainer_Exists_StartSuccess) {
    ResetRecorder();
    // 1) docker ps -a | grep -q name -> exit 0 (존재)
    // 2) docker start name -> exit 0 (성공)
    QueueStatuses({ ExitStatus(0), ExitStatus(0) });

    ContainerController c;
    EXPECT_TRUE(c.StartContainer("mycont"));

    ASSERT_EQ(g_cmds.size(), 2u);
    EXPECT_NE(g_cmds[0].find("docker ps -a | grep -q mycont"), std::string::npos);
    EXPECT_NE(g_cmds[1].find("docker start mycont"), std::string::npos);
}

TEST(ContainerControllerTest, StartContainer_Exists_StartFail) {
    ResetRecorder();
    QueueStatuses({ ExitStatus(0), ExitStatus(2) }); // 존재, start 실패

    ContainerController c;
    EXPECT_FALSE(c.StartContainer("mycont"));

    ASSERT_EQ(g_cmds.size(), 2u);
    EXPECT_NE(g_cmds[0].find("docker ps -a | grep -q mycont"), std::string::npos);
    EXPECT_NE(g_cmds[1].find("docker start mycont"), std::string::npos);
}

TEST(ContainerControllerTest, StartContainer_NotExist_RunSuccess) {
    ResetRecorder();
    QueueStatuses({ ExitStatus(1), ExitStatus(0) }); // 미존재, run 성공

    ContainerController c;
    EXPECT_TRUE(c.StartContainer("mycont"));

    ASSERT_EQ(g_cmds.size(), 2u);
    EXPECT_NE(g_cmds[0].find("docker ps -a | grep -q mycont"), std::string::npos);
    // 구현 문자열 일부만 확인(-it, --name, 이미지명 등)
    EXPECT_NE(g_cmds[1].find("docker run -it --net=host"), std::string::npos);
    EXPECT_NE(g_cmds[1].find("--name mycont"), std::string::npos);
    EXPECT_NE(g_cmds[1].find("adsw_monitor_image"), std::string::npos);
}

TEST(ContainerControllerTest, StartContainer_NotExist_RunFail) {
    ResetRecorder();
    QueueStatuses({ ExitStatus(1), ExitStatus(3) }); // 미존재, run 실패

    ContainerController c;
    EXPECT_FALSE(c.StartContainer("mycont"));

    ASSERT_EQ(g_cmds.size(), 2u);
    EXPECT_NE(g_cmds[1].find("docker run -it"), std::string::npos);
}

TEST(ContainerControllerTest, StartContainer_SystemFailureOnGrep) {
    ResetRecorder();
    QueueStatuses({ -1 });  // system() 자체 실패

    ContainerController c;
    EXPECT_FALSE(c.StartContainer("mycont"));

    ASSERT_EQ(g_cmds.size(), 1u);
    EXPECT_NE(g_cmds[0].find("docker ps -a | grep -q mycont"), std::string::npos);
}

// ----------------------------- StopContainer -----------------------------

TEST(ContainerControllerTest, StopContainer_Success) {
    ResetRecorder();
    g_next_status = ExitStatus(0);

    ContainerController c;
    EXPECT_TRUE(c.StopContainer("mycont"));
    EXPECT_NE(g_last_cmd.find("docker stop mycont"), std::string::npos);
}

TEST(ContainerControllerTest, StopContainer_Fail_NonZeroExit) {
    ResetRecorder();
    g_next_status = ExitStatus(5);

    ContainerController c;
    EXPECT_FALSE(c.StopContainer("mycont"));
}

TEST(ContainerControllerTest, StopContainer_SystemFailure) {
    ResetRecorder();
    g_next_status = -1;

    ContainerController c;
    EXPECT_FALSE(c.StopContainer("mycont"));
}

// ----------------------------- IsContainerRunning -----------------------------

TEST(ContainerControllerTest, IsContainerRunning_FoundTrue) {
    ResetRecorder();
    g_next_status = ExitStatus(0);  // grep 매칭

    ContainerController c;
    EXPECT_TRUE(c.IsContainerRunning("mycont"));
    EXPECT_NE(g_last_cmd.find("docker ps --filter name=mycont | grep -q mycont"), std::string::npos);
}

TEST(ContainerControllerTest, IsContainerRunning_NotRunningExit1) {
    ResetRecorder();
    g_next_status = ExitStatus(1);  // grep 미매칭

    ContainerController c;
    EXPECT_FALSE(c.IsContainerRunning("mycont"));
}

TEST(ContainerControllerTest, IsContainerRunning_OtherExitCodesAreFalse) {
    ResetRecorder();
    g_next_status = ExitStatus(2);  // grep 에러 등

    ContainerController c;
    EXPECT_FALSE(c.IsContainerRunning("mycont"));
}

TEST(ContainerControllerTest, IsContainerRunning_SystemFailure) {
    ResetRecorder();
    g_next_status = -1;             // system() 자체 실패

    ContainerController c;
    EXPECT_FALSE(c.IsContainerRunning("mycont"));
}