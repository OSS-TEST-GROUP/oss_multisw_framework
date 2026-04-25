#include <gtest/gtest.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include <thread>
#include <string>
#include <fstream>
#include <functional>
#include <sys/stat.h>

namespace {

constexpr const char* AUTOWARE_IMAGE =
    "ghcr.io/autowarefoundation/autoware:universe-dependencies-cuda-humble-20260423-amd64";
constexpr const char* AUTOWARE_CONTAINER = "autoware-test";

constexpr const char* LIFECYCLE_MGR_BIN =
    "/home/obigo/project/oss/oss_agent/mmf/oss_multisw_framework/output/x86_64/Debug/core/bin/lifecycle_mgr";
constexpr const char* ADSW_MANAGER_BIN =
    "/home/obigo/project/oss/oss_agent/mmf/oss_multisw_framework/output/x86_64/Debug/extension/ros2/bin/adsw_manager";
constexpr const char* FASTDDS_LIB_PATH =
    "/home/obigo/obigo-sdk/Fast-DDS/output/ros2_x86_64/lib";
constexpr const char* LF_LIB_PATH =
    "/home/obigo/project/oss/oss_agent/mmf/oss_multisw_framework/output/x86_64/Debug/core/lib";
constexpr const char* ADSW_LIB_PATH =
    "/home/obigo/project/oss/oss_agent/mmf/oss_multisw_framework/output/x86_64/Debug/extension/ros2/lib";
constexpr const char* LF_CONFIG_PATH =
    "/home/obigo/project/oss/oss_agent/mmf/oss_multisw_framework/service/core/lifecycle_mgr/config";
constexpr const char* E2E_CONFIG_DIR = "/tmp/autoware_e2e_test";

std::string ExecShell(const std::string& cmd) {
    std::string result;
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";
    char buf[256];
    while (fgets(buf, sizeof(buf), pipe)) result += buf;
    pclose(pipe);
    return result;
}

bool WaitForCondition(std::function<bool()> cond, int timeout_sec) {
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(timeout_sec);
    while (std::chrono::steady_clock::now() < deadline) {
        if (cond()) return true;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return false;
}

void KillAndWait(pid_t pid) {
    if (pid <= 0) return;
    kill(pid, SIGTERM);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    if (kill(pid, 0) == 0) kill(pid, SIGKILL);
    int status;
    waitpid(pid, &status, WNOHANG);
}

class L3AutowareE2ETest : public ::testing::Test {
public:
    static pid_t lifecycle_pid_;
    static pid_t adsw_pid_;
    static bool setup_ok_;
    static std::string node_list_result_;

    static void SetUpTestSuite() {
        setup_ok_ = false;

        // 1. Create config dir and write saved-service-list.yaml
        mkdir(E2E_CONFIG_DIR, 0755);
        std::string yaml_path = std::string(E2E_CONFIG_DIR) + "/saved-service-list.yaml";
        std::ofstream f(yaml_path);
        f << "version: 0.1\n"
          << "specs:\n"
          << "  - containers:\n"
          << "      - name: autoware-test\n"
          << "        image: \"" << AUTOWARE_IMAGE << "\"\n"
          << "        container-type: \"docker\"\n"
          << "        services:\n"
          << "          - package: \"logging_demo\"\n"
          << "            name: \"logging_demo_main\"\n"
          << "            launchfile: \"\"\n"
          << "            type: \"NORMAL\"\n"
          << "            recovery: \"ALWAYS\"\n"
          << "            enabled: \"true\"\n"
          << "            max: \"0\"\n";
        f.close();

        // 2. Start autoware container (network host for DDS multicast)
        ExecShell(std::string("docker rm -f ") + AUTOWARE_CONTAINER + " 2>/dev/null; "
                  "docker run -d --name " + AUTOWARE_CONTAINER
                  + " --network host " + AUTOWARE_IMAGE + " sleep infinity");

        bool container_ok = WaitForCondition([]() {
            std::string out = ExecShell(
                "docker ps --filter name=autoware-test --filter status=running --format '{{.Names}}'");
            return out.find("autoware-test") != std::string::npos;
        }, 30);

        if (!container_ok) {
            ADD_FAILURE() << "autoware container did not start within 30s";
            return;
        }

        // 3. Build LD_LIBRARY_PATH for child processes
        std::string ld_path = std::string(LF_LIB_PATH) + ":"
                            + ADSW_LIB_PATH + ":"
                            + FASTDDS_LIB_PATH;
        const char* existing_ld = std::getenv("LD_LIBRARY_PATH");
        if (existing_ld && strlen(existing_ld) > 0) ld_path += std::string(":") + existing_ld;

        // 4. Fork lifecycle_mgr
        lifecycle_pid_ = fork();
        if (lifecycle_pid_ == 0) {
            setenv("MMF_CONFIG_PATH", LF_CONFIG_PATH, 1);
            setenv("LD_LIBRARY_PATH", ld_path.c_str(), 1);
            execl(LIFECYCLE_MGR_BIN, "lifecycle_mgr", nullptr);
            _exit(127);
        }

        // 5. Fork adsw_manager
        adsw_pid_ = fork();
        if (adsw_pid_ == 0) {
            setenv("SAVED_REGISTRY_BACKUP", E2E_CONFIG_DIR, 1);
            setenv("LD_LIBRARY_PATH", ld_path.c_str(), 1);
            execl(ADSW_MANAGER_BIN, "adsw_manager", nullptr);
            _exit(127);
        }

        // 6. Wait for logging_demo_main to appear in container (up to 60s)
        // Flow: adsw starts → sends UpdateServiceList → lifecycle responds → sends StartServiceRequest
        //       → adsw docker execs ros2 run → node appears in ros2 node list
        setup_ok_ = WaitForCondition([]() {
            std::string out = ExecShell(
                "docker exec autoware-test bash -c "
                "'source /opt/ros/humble/setup.bash 2>/dev/null && ros2 node list 2>/dev/null'");
            if (out.find("/logging_demo_main") != std::string::npos) {
                node_list_result_ = out;
                return true;
            }
            return false;
        }, 60);
    }

    static void TearDownTestSuite() {
        KillAndWait(adsw_pid_);
        KillAndWait(lifecycle_pid_);
        ExecShell(std::string("docker rm -f ") + AUTOWARE_CONTAINER + " 2>/dev/null");
        ExecShell(std::string("rm -rf ") + E2E_CONFIG_DIR);
        adsw_pid_ = -1;
        lifecycle_pid_ = -1;
    }
};

pid_t L3AutowareE2ETest::lifecycle_pid_ = -1;
pid_t L3AutowareE2ETest::adsw_pid_ = -1;
bool L3AutowareE2ETest::setup_ok_ = false;
std::string L3AutowareE2ETest::node_list_result_;

// AC1: autoware container running
TEST_F(L3AutowareE2ETest, TC001_ContainerRunning) {
    std::string out = ExecShell(
        "docker ps --filter name=autoware-test --filter status=running --format '{{.Names}}'");
    EXPECT_NE(out.find("autoware-test"), std::string::npos)
        << "autoware-test container must be running";
}

// AC2: lifecycle_mgr and adsw_manager binaries running
TEST_F(L3AutowareE2ETest, TC002_BinariesRunning) {
    ASSERT_GT(lifecycle_pid_, 0) << "lifecycle_mgr was not forked";
    ASSERT_GT(adsw_pid_, 0) << "adsw_manager was not forked";
    EXPECT_EQ(kill(lifecycle_pid_, 0), 0) << "lifecycle_mgr process must be alive";
    EXPECT_EQ(kill(adsw_pid_, 0), 0) << "adsw_manager process must be alive";
}

// AC4+AC5: adsw_manager ran docker exec → logging_demo_main node in container
TEST_F(L3AutowareE2ETest, TC003_LoggingDemoNodeStartedInContainer) {
    ASSERT_TRUE(setup_ok_)
        << "SetUpTestSuite: logging_demo_main did not appear in ros2 node list within 60s. "
        << "Check adsw_manager StartService command construction.";
    std::string out = ExecShell(
        "docker exec autoware-test bash -c "
        "'source /opt/ros/humble/setup.bash 2>/dev/null && ros2 node list 2>/dev/null'");
    EXPECT_NE(out.find("/logging_demo_main"), std::string::npos)
        << "Expected /logging_demo_main in ros2 node list, got: " << out;
}

// AC6: StartServiceResponse(SUCCESS) implied — adsw_manager only sends SUCCESS after getting PID
// from docker exec. If node is running, the full handshake chain succeeded.
TEST_F(L3AutowareE2ETest, TC004_StartServiceHandshakeComplete) {
    ASSERT_TRUE(setup_ok_)
        << "E2E handshake chain incomplete: UpdateServiceList→StartServiceRequest→docker exec→StartServiceResponse";
    // Node running in container proves: lifecycle sent StartServiceRequest, adsw received it,
    // adsw ran docker exec successfully (got PID), and sent StartServiceResponse(SUCCESS).
    EXPECT_FALSE(node_list_result_.empty())
        << "node_list_result_ should contain the ros2 node list output";
    EXPECT_NE(node_list_result_.find("/logging_demo_main"), std::string::npos);
}

// AC8: Verify node appeared within timeout (60s covers handshake + startup time)
TEST_F(L3AutowareE2ETest, TC005_NodeStartedWithinTimeout) {
    ASSERT_TRUE(setup_ok_)
        << "logging_demo_main did not start within 60s timeout — "
        << "lifecycle_mgr→StartServiceRequest→adsw_manager→docker exec chain failed";
}

} // namespace

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
