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

#include "autoware_e2e_common.h"

namespace {

constexpr const char* AUTOWARE_CONTAINER = "autoware-test-polling";
constexpr const char* E2E_CONFIG_DIR     = "/tmp/autoware_polling_e2e_test";
constexpr const char* LF_LOG_PATH        = "lifecycle_mgr.log";

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
        if (line.find("PublishingLoop") != std::string::npos &&
            line.find("mmf/lifecyclemgr/GetServiceStateRequest") != std::string::npos) {
            stats.request_count++;
        }
        if (line.find("on_data_available") != std::string::npos &&
            line.find("mmf/lifecyclemgr/GetServiceStateResponse") != std::string::npos) {
            stats.response_count++;
        }
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

        // Fast-skip: check docker, mock image, and MMF binaries before any
        // blocking I/O. This exits in <1s on machines without prerequisites.
        std::string skip_reason;
        if (!AutowareE2EPrereqs(skip_reason)) {
            GTEST_SKIP() << skip_reason;
            return;
        }

        // Kill stale processes from prior runs to avoid DDS domain interference.
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
          << "        image: \"" << AutowareImage() << "\"\n"
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

        // 2. Detect default network interface for CycloneDDS
        std::string iface = ExecShell("ip route show default 2>/dev/null | awk '{print $5; exit}'");
        while (!iface.empty() && (iface.back() == '\n' || iface.back() == '\r' || iface.back() == ' '))
            iface.pop_back();
        if (iface.empty()) iface = "enp128s31f6";  // fallback

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
        ExecShell(std::string("docker stop ") + AUTOWARE_CONTAINER + " 2>/dev/null; docker rm " + AUTOWARE_CONTAINER + " 2>/dev/null");
        std::this_thread::sleep_for(std::chrono::seconds(1));
        ExecShell(std::string("docker run -d --name ") + AUTOWARE_CONTAINER
                  + " --network host"
                  + " -v " + CYCLONE_XML_PATH + ":/tmp/cyclonedds_polling_e2e.xml"
                  + " -e CYCLONEDDS_URI=file:///tmp/cyclonedds_polling_e2e.xml"
                  + " " + AutowareImage() + " sleep infinity");

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
        std::string ld_path = MmfBuildLdPath();
        std::string lf_config = MmfLfConfig();
        std::string fastdds_profile = MmfFastDdsProfile();
        setenv("FASTDDS_DEFAULT_PROFILES_FILE", fastdds_profile.c_str(), 1);

        // 5. Fork lifecycle_mgr
        lifecycle_pid_ = fork();
        if (lifecycle_pid_ == 0) {
            setenv("MMF_CONFIG_PATH", lf_config.c_str(), 1);
            setenv("LD_LIBRARY_PATH", ld_path.c_str(), 1);
            setenv("FASTDDS_DEFAULT_PROFILES_FILE", fastdds_profile.c_str(), 1);
            execl(MmfLfBin().c_str(), "lifecycle_mgr", nullptr);
            _exit(127);
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // 6. Fork adsw_manager
        adsw_pid_ = fork();
        if (adsw_pid_ == 0) {
            setenv("SAVED_REGISTRY_BACKUP", E2E_CONFIG_DIR, 1);
            setenv("LD_LIBRARY_PATH", ld_path.c_str(), 1);
            setenv("FASTDDS_DEFAULT_PROFILES_FILE", fastdds_profile.c_str(), 1);
            execl(MmfAdswBin().c_str(), "adsw_manager", nullptr);
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
        std::this_thread::sleep_for(std::chrono::seconds(25));

        // 9. Parse lifecycle_mgr.log (generated in CWD of test binary)
        polling_stats_ = ParseLifecycleLog(LF_LOG_PATH);

        setup_ok_ = true;
    }

    static void TearDownTestSuite() {
        KillAndWait(adsw_pid_);
        KillAndWait(lifecycle_pid_);
        adsw_pid_ = 0;
        lifecycle_pid_ = 0;

        ExecShell(std::string("docker stop ") + AUTOWARE_CONTAINER + " 2>/dev/null; docker rm " + AUTOWARE_CONTAINER + " 2>/dev/null");
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
    ASSERT_TRUE(setup_ok_) << "SetUpTestSuite failed — /logger_usage_demo node did not start";

    std::string cmd = std::string("docker exec ") + AUTOWARE_CONTAINER
                    + " bash -c 'source /opt/ros/humble/setup.bash && ros2 node list 2>/dev/null'";
    std::string result = ExecShell(cmd);
    EXPECT_TRUE(result.find("/logger_usage_demo") != std::string::npos)
        << "Expected /logger_usage_demo node in ROS2 node list";
}

TEST_F(L3AutowarePollingE2ETest, TC002_PollingRequestSent_AtLeastTwice) {
    ASSERT_TRUE(setup_ok_) << "SetUpTestSuite failed";

    EXPECT_GE(polling_stats_.request_count, 2)
        << "Expected at least 2 GetServiceStateRequest DDS publishes in 25 seconds. "
        << "Log shows " << polling_stats_.request_count << " requests. "
        << "Check lifecycle_mgr.log for 'PublishingLoop.*GetServiceStateRequest' entries.";
}

TEST_F(L3AutowarePollingE2ETest, TC003_PollingResponseReceived_Running) {
    ASSERT_TRUE(setup_ok_) << "SetUpTestSuite failed";

    EXPECT_GE(polling_stats_.response_count, 2)
        << "Expected at least 2 GetServiceStateResponse DDS receptions in 25 seconds. "
        << "Log shows " << polling_stats_.response_count << " responses. "
        << "Check lifecycle_mgr.log for 'on_data_available.*GetServiceStateResponse' entries.";

    EXPECT_GT(polling_stats_.running_state_count, 0)
        << "Expected at least 1 'Running' state transition in lifecycle_mgr.log";
}

TEST_F(L3AutowarePollingE2ETest, TC004_PollingCompletedWithinTimeout) {
    ASSERT_TRUE(setup_ok_) << "SetUpTestSuite failed within timeout";
    SUCCEED();
}

}  // namespace

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
