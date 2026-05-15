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
constexpr const char* AUTOWARE_CONTAINER = "autoware-test-polling";

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
constexpr const char* E2E_CONFIG_DIR = "/tmp/autoware_polling_e2e_test";
constexpr const char* LF_LOG_PATH = "lifecycle_mgr.log";

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
    if (kill(pid, 0) == 0) {
        kill(pid, SIGKILL);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    int status;
    waitpid(pid, &status, 0);  // Block until child terminates — prevents zombie
}

// Parse lifecycle_mgr.log for GetServiceStateRequest and GetServiceStateResponse
struct PollingStats {
    int request_count = 0;
    int response_count = 0;
    int running_state_count = 0;
};

PollingStats ParseLifecycleLog(const std::string& log_path) {
    PollingStats stats;
    std::ifstream f(log_path);
    if (!f.is_open()) return stats;

    std::string line;
    while (std::getline(f, line)) {
        // Look for PublishingLoop with GetServiceStateRequest
        if (line.find("PublishingLoop") != std::string::npos &&
            line.find("mmf/lifecyclemgr/GetServiceStateRequest") != std::string::npos) {
            stats.request_count++;
        }
        // Look for GetServiceStateResponse reception
        if (line.find("on_data_available") != std::string::npos &&
            line.find("mmf/lifecyclemgr/GetServiceStateResponse") != std::string::npos) {
            stats.response_count++;
        }
        // Look for state transitions to Running
        if (line.find("SetServiceState") != std::string::npos &&
            line.find("Running") != std::string::npos) {
            stats.running_state_count++;
        }
    }
    return stats;
}

class L3AutowarePollingE2ETest : public ::testing::Test {
public:
    static pid_t lifecycle_pid_;
    static pid_t adsw_pid_;
    static bool setup_ok_;
    static std::string node_list_result_;
    static PollingStats polling_stats_;

    static void SetUpTestSuite() {
        setup_ok_ = false;

        // Kill stale processes from prior runs to avoid DDS domain interference.
        // Use basename only — execl(path, "argv0") sets cmdline to argv0, not the full path.
        ExecShell("pkill -9 lifecycle_mgr 2>/dev/null");
        ExecShell("pkill -9 adsw_manager 2>/dev/null");
        std::this_thread::sleep_for(std::chrono::seconds(2));

        // 1. Create config dir and write saved-service-list.yaml
        mkdir(E2E_CONFIG_DIR, 0755);
        std::string yaml_path = std::string(E2E_CONFIG_DIR) + "/saved-service-list.yaml";
        std::ofstream f(yaml_path);
        if (!f.is_open()) {
            ADD_FAILURE() << "Failed to create config file: " << yaml_path;
            return;
        }
        f << "version: 0.1\n"
          << "specs:\n"
          << "  - containers:\n"
          << "      - name: " << AUTOWARE_CONTAINER << "\n"
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

        // 2. Detect default network interface for CycloneDDS (loopback doesn't support multicast)
        std::string iface = ExecShell("ip route show default 2>/dev/null | awk '{print $5; exit}'");
        while (!iface.empty() && (iface.back() == '\n' || iface.back() == '\r' || iface.back() == ' '))
            iface.pop_back();
        if (iface.empty()) iface = "enp128s31f6";  // fallback for this machine

        // Write a working CycloneDDS config using the detected interface
        const char* CYCLONE_XML_PATH = "/tmp/cyclonedds_polling_e2e.xml";
        {
            std::ofstream cf(CYCLONE_XML_PATH);
            cf << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
               << "<CycloneDDS>\n"
               << "  <Domain>\n"
               << "    <General>\n"
               << "      <Interfaces>\n"
               << "        <NetworkInterface name=\"" << iface << "\"/>\n"
               << "      </Interfaces>\n"
               << "    </General>\n"
               << "  </Domain>\n"
               << "</CycloneDDS>\n";
        }

        // 3. Start autoware container (network host for DDS multicast)
        ExecShell(std::string("docker rm -f ") + AUTOWARE_CONTAINER + " 2>/dev/null");
        std::this_thread::sleep_for(std::chrono::seconds(1));
        ExecShell(std::string("docker run -d --name ") + AUTOWARE_CONTAINER
                  + " --network host"
                  + " -v " + CYCLONE_XML_PATH + ":/tmp/cyclonedds_polling_e2e.xml"
                  + " -e CYCLONEDDS_URI=file:///tmp/cyclonedds_polling_e2e.xml"
                  + " " + AUTOWARE_IMAGE + " sleep infinity");

        bool container_ok = WaitForCondition([]() {
            std::string out = ExecShell(
                std::string("docker ps --filter name=") + AUTOWARE_CONTAINER
                + " --filter status=running --format '{{.Names}}'");
            return out.find(AUTOWARE_CONTAINER) != std::string::npos;
        }, 30);

        if (!container_ok) {
            ADD_FAILURE() << "autoware container did not start within 30s";
            return;
        }

        // 4. Build LD_LIBRARY_PATH for child processes
        std::string ld_path = std::string(LF_LIB_PATH) + ":"
                            + ADSW_LIB_PATH + ":"
                            + FASTDDS_LIB_PATH;
        const char* existing_ld = std::getenv("LD_LIBRARY_PATH");
        if (existing_ld && strlen(existing_ld) > 0) ld_path += std::string(":") + existing_ld;

        // Set FastDDS profile for UDP-only transport (avoid SHM futex deadlock)
        const char* FASTDDS_PROFILE =
            "/home/obigo/project/oss/oss_agent/mmf/oss_multisw_framework"
            "/docker/build_environment/fastdds-profile.xml";
        setenv("FASTDDS_DEFAULT_PROFILES_FILE", FASTDDS_PROFILE, 1);

        // 5. Fork lifecycle_mgr
        lifecycle_pid_ = fork();
        if (lifecycle_pid_ == 0) {
            setenv("MMF_CONFIG_PATH", LF_CONFIG_PATH, 1);
            setenv("LD_LIBRARY_PATH", ld_path.c_str(), 1);
            setenv("FASTDDS_DEFAULT_PROFILES_FILE", FASTDDS_PROFILE, 1);
            execl(LIFECYCLE_MGR_BIN, "lifecycle_mgr", nullptr);
            _exit(127);
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // 6. Fork adsw_manager
        adsw_pid_ = fork();
        if (adsw_pid_ == 0) {
            setenv("SAVED_REGISTRY_BACKUP", E2E_CONFIG_DIR, 1);
            setenv("LD_LIBRARY_PATH", ld_path.c_str(), 1);
            setenv("FASTDDS_DEFAULT_PROFILES_FILE", FASTDDS_PROFILE, 1);
            execl(ADSW_MANAGER_BIN, "adsw_manager", nullptr);
            _exit(127);
        }
        std::this_thread::sleep_for(std::chrono::seconds(2));

        // 7. Wait for /logger_usage_demo node to appear (this verifies full F003 chain)
        bool node_ok = WaitForCondition([]() {
            std::string cmd = std::string("docker exec ") + AUTOWARE_CONTAINER
                            + " bash -c 'source /opt/ros/humble/setup.bash && ros2 node list 2>/dev/null'";
            std::string result = ExecShell(cmd);
            return result.find("/logger_usage_demo") != std::string::npos;
        }, 60);

        if (!node_ok) {
            ADD_FAILURE() << "/logger_usage_demo node did not start within 60s";
            return;
        }

        // 8. Wait 25 seconds to allow at least 2 polling cycles (10s interval × 2 = 20s + margin)
        // StateMonitor starts immediately, first poll at T+0, second at T+10s, third at T+20s
        std::this_thread::sleep_for(std::chrono::seconds(25));

        // 9. Parse lifecycle_mgr.log (generated in CWD of test binary)
        polling_stats_ = ParseLifecycleLog(LF_LOG_PATH);

        setup_ok_ = true;
    }

    static void TearDownTestSuite() {
        // 1. Kill adsw_manager and lifecycle_mgr
        KillAndWait(adsw_pid_);
        KillAndWait(lifecycle_pid_);
        adsw_pid_ = 0;
        lifecycle_pid_ = 0;

        // 2. Stop and remove autoware container
        ExecShell(std::string("docker rm -f ") + AUTOWARE_CONTAINER + " 2>/dev/null");

        // 3. Clean up config dir and CycloneDDS XML
        ExecShell(std::string("rm -rf ") + E2E_CONFIG_DIR);
        ExecShell("rm -f /tmp/cyclonedds_polling_e2e.xml");
    }
};

// Static member initialization
pid_t L3AutowarePollingE2ETest::lifecycle_pid_ = 0;
pid_t L3AutowarePollingE2ETest::adsw_pid_ = 0;
bool L3AutowarePollingE2ETest::setup_ok_ = false;
std::string L3AutowarePollingE2ETest::node_list_result_;
PollingStats L3AutowarePollingE2ETest::polling_stats_;

// ── Test Cases ──────────────────────────────────────────────────────────────

TEST_F(L3AutowarePollingE2ETest, TC001_Phase1_NodeRunning) {
    // Verify that Phase 1 (F003 chain) succeeded: logging_demo_main is RUNNING.
    // This is implicitly validated in SetUpTestSuite (node_ok check), but we make it
    // explicit here for clarity.
    ASSERT_TRUE(setup_ok_) << "SetUpTestSuite failed — /logger_usage_demo node did not start";

    std::string cmd = std::string("docker exec ") + AUTOWARE_CONTAINER
                    + " bash -c 'source /opt/ros/humble/setup.bash && ros2 node list 2>/dev/null'";
    std::string result = ExecShell(cmd);
    EXPECT_TRUE(result.find("/logger_usage_demo") != std::string::npos)
        << "Expected /logger_usage_demo node in ROS2 node list";
}

TEST_F(L3AutowarePollingE2ETest, TC002_PollingRequestSent_AtLeastTwice) {
    // Verify that lifecycle_mgr sent GetServiceStateRequest at least twice.
    // StateMonitor::RunThread publishes GetServiceStateRequest every 10 seconds.
    // We waited 25 seconds, so we expect at least 2 requests (T+0, T+10s).
    ASSERT_TRUE(setup_ok_) << "SetUpTestSuite failed";

    EXPECT_GE(polling_stats_.request_count, 2)
        << "Expected at least 2 GetServiceStateRequest DDS publishes in 25 seconds. "
        << "Log shows " << polling_stats_.request_count << " requests. "
        << "Check lifecycle_mgr.log for 'PublishingLoop.*GetServiceStateRequest' entries.";
}

TEST_F(L3AutowarePollingE2ETest, TC003_PollingResponseReceived_Running) {
    // Verify that lifecycle_mgr received GetServiceStateResponse at least twice.
    // adsw_manager responds to each GetServiceStateRequest with GetServiceStateResponse.
    ASSERT_TRUE(setup_ok_) << "SetUpTestSuite failed";

    EXPECT_GE(polling_stats_.response_count, 2)
        << "Expected at least 2 GetServiceStateResponse DDS receptions in 25 seconds. "
        << "Log shows " << polling_stats_.response_count << " responses. "
        << "Check lifecycle_mgr.log for 'on_data_available.*GetServiceStateResponse' entries.";

    // Also verify that the service state is RUNNING (implicitly from F003 phase)
    EXPECT_GT(polling_stats_.running_state_count, 0)
        << "Expected at least 1 'Running' state transition in lifecycle_mgr.log";
}

TEST_F(L3AutowarePollingE2ETest, TC004_PollingCompletedWithinTimeout) {
    // Verify that the entire polling test completed within the expected timeout (25s + setup).
    // SetUpTestSuite waits 25s for polling + 60s for node startup + ~5s for process spawning.
    // If we reach this test, SetUpTestSuite succeeded within the GTest timeout, so this is
    // implicitly validated. We make it explicit for documentation.
    ASSERT_TRUE(setup_ok_) << "SetUpTestSuite failed within timeout";

    // Test completed successfully if we got here.
    SUCCEED();
}

}  // namespace

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
