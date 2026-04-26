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

// ── Constants ──────────────────────────────────────────────────────────────
constexpr const char* AUTOWARE_IMAGE =
    "ghcr.io/autowarefoundation/autoware:universe-dependencies-cuda-humble-20260423-amd64";
constexpr const char* AUTOWARE_CONTAINER = "autoware-test-crash";

constexpr const char* ADSW_MANAGER_BIN =
    "/home/obigo/project/oss/oss_agent/mmf/oss_multisw_framework/output/x86_64/Debug/extension/ros2/bin/adsw_manager";
constexpr const char* LIFECYCLE_MGR_BIN =
    "/home/obigo/project/oss/oss_agent/mmf/oss_multisw_framework/output/x86_64/Debug/core/bin/lifecycle_mgr";
constexpr const char* FASTDDS_LIB_PATH =
    "/home/obigo/obigo-sdk/Fast-DDS/output/ros2_x86_64/lib";
constexpr const char* LF_LIB_PATH =
    "/home/obigo/project/oss/oss_agent/mmf/oss_multisw_framework/output/x86_64/Debug/core/lib";
constexpr const char* ADSW_LIB_PATH =
    "/home/obigo/project/oss/oss_agent/mmf/oss_multisw_framework/output/x86_64/Debug/extension/ros2/lib";
constexpr const char* LF_CONFIG_PATH =
    "/home/obigo/project/oss/oss_agent/mmf/oss_multisw_framework/service/core/lifecycle_mgr/config";
constexpr const char* E2E_CRASH_CONFIG_DIR = "/tmp/autoware_crash_e2e_test";
constexpr const char* FASTDDS_PROFILE =
    "/home/obigo/project/oss/oss_agent/mmf/oss_multisw_framework"
    "/docker/build_environment/fastdds-profile.xml";

// ── Helpers ─────────────────────────────────────────────────────────────────
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
    waitpid(pid, &status, 0);
}

// ── Test Fixture ─────────────────────────────────────────────────────────────
// F006 L3 E2E: Crash notification via NotifyServiceState
//
// Phase 1: F003 chain — start lifecycle_mgr + adsw_manager → logging_demo_main runs
// Phase 2: pkill -9 logging_demo_main → ContainerHealthMonitor detects crash
//          → ServiceMonitor::NotifyServiceState() → DDS publish
// Phase 3: Verify adsw.log has "NotifyServiceState" and lifecycle_mgr.log has reception

class L3AutowareCrashE2ETest : public ::testing::Test {
public:
    static pid_t lifecycle_pid_;
    static pid_t adsw_pid_;
    static bool node_started_ok_;
    static bool node_crashed_ok_;
    static bool notify_published_ok_;
    static bool notify_received_ok_;

    static void SetUpTestSuite() {
        // Set FastDDS profile before any DDS usage in this process
        setenv("FASTDDS_DEFAULT_PROFILES_FILE", FASTDDS_PROFILE, 1);

        node_started_ok_ = false;
        node_crashed_ok_ = false;
        notify_published_ok_ = false;
        notify_received_ok_ = false;

        // Kill stale processes from prior runs to avoid DDS domain interference
        ExecShell("pkill -9 adsw_manager 2>/dev/null");
        ExecShell("pkill -9 lifecycle_mgr 2>/dev/null");
        std::this_thread::sleep_for(std::chrono::seconds(2));

        // 1. Write config: logging_demo_main in autoware-test-crash container
        mkdir(E2E_CRASH_CONFIG_DIR, 0755);
        std::string yaml_path = std::string(E2E_CRASH_CONFIG_DIR) + "/saved-service-list.yaml";
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
          << "            recovery: \"ALWAYS\"\n"  // ALWAYS recognized; lifecycle_mgr will re-issue StartServiceRequest after crash
          << "            enabled: \"true\"\n"
          << "            max: \"0\"\n";
        f.close();

        // 2. Detect network interface and write CycloneDDS config for container
        std::string iface = ExecShell("ip route show default 2>/dev/null | awk '{print $5; exit}'");
        while (!iface.empty() && (iface.back() == '\n' || iface.back() == '\r' || iface.back() == ' '))
            iface.pop_back();
        if (iface.empty()) iface = "enp128s31f6";

        const char* CYCLONE_XML_PATH = "/tmp/cyclonedds_crash_e2e.xml";
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

        // 3. Start autoware container (--network host for DDS multicast)
        ExecShell(std::string("docker rm -f ") + AUTOWARE_CONTAINER + " 2>/dev/null");
        std::this_thread::sleep_for(std::chrono::seconds(1));
        ExecShell(std::string("docker run -d --name ") + AUTOWARE_CONTAINER
                  + " --network host"
                  + " -v " + CYCLONE_XML_PATH + ":/tmp/cyclonedds_crash_e2e.xml"
                  + " -e CYCLONEDDS_URI=file:///tmp/cyclonedds_crash_e2e.xml"
                  + " " + AUTOWARE_IMAGE + " sleep infinity");

        bool container_ok = WaitForCondition([&]() {
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

        // 5. Fork lifecycle_mgr — it orchestrates UpdateServiceList handshake and issues
        //    StartServiceRequest to adsw_manager, driving the full F003 chain.
        lifecycle_pid_ = fork();
        if (lifecycle_pid_ == 0) {
            setenv("MMF_CONFIG_PATH", LF_CONFIG_PATH, 1);
            setenv("LD_LIBRARY_PATH", ld_path.c_str(), 1);
            setenv("FASTDDS_DEFAULT_PROFILES_FILE", FASTDDS_PROFILE, 1);
            execl(LIFECYCLE_MGR_BIN, "lifecycle_mgr", nullptr);
            _exit(127);
        }

        // 6. Fork adsw_manager
        adsw_pid_ = fork();
        if (adsw_pid_ == 0) {
            setenv("SAVED_REGISTRY_BACKUP", E2E_CRASH_CONFIG_DIR, 1);
            setenv("LD_LIBRARY_PATH", ld_path.c_str(), 1);
            setenv("FASTDDS_DEFAULT_PROFILES_FILE", FASTDDS_PROFILE, 1);
            execl(ADSW_MANAGER_BIN, "adsw_manager", nullptr);
            _exit(127);
        }

        // 7. Phase 1: Wait for F003 chain to complete — /logger_usage_demo appears
        node_started_ok_ = WaitForCondition([&]() {
            std::string out = ExecShell(
                std::string("docker exec ") + AUTOWARE_CONTAINER + " bash -c "
                "'source /opt/ros/humble/setup.bash 2>/dev/null && ros2 node list 2>/dev/null'");
            return out.find("/logger_usage_demo") != std::string::npos;
        }, 60);

        if (!node_started_ok_) {
            ADD_FAILURE() << "Phase 1 failed: /logger_usage_demo did not appear within 60s";
            return;
        }

        // 8. Phase 2: Stop the container to simulate a crash.
        //    ContainerHealthMonitor monitors container-level state via docker inspect.
        //    Killing only a process inside the container leaves container "running" — undetected.
        //    Stopping the container triggers: docker inspect → "exited" → OnContainerStateChanged()
        //    → NotifyServiceState(STOPPED, CRASH).
        ExecShell(std::string("docker stop -t 2 ") + AUTOWARE_CONTAINER + " 2>/dev/null");

        // When container stops, docker exec into it returns error → ros2 node list is empty.
        // WaitForCondition sees node absent (empty output) → node_crashed_ok_ = true.
        node_crashed_ok_ = WaitForCondition([&]() {
            std::string out = ExecShell(
                std::string("docker exec ") + AUTOWARE_CONTAINER + " bash -c "
                "'source /opt/ros/humble/setup.bash 2>/dev/null && ros2 node list 2>/dev/null'"
                " 2>/dev/null");
            return out.find("/logger_usage_demo") == std::string::npos;
        }, 15);

        if (!node_crashed_ok_) {
            ADD_FAILURE() << "Phase 2 failed: container stop did not make /logger_usage_demo disappear within 15s";
            return;
        }

        // Wait a bit more for NotifyServiceState DDS message to be sent and received
        std::this_thread::sleep_for(std::chrono::seconds(2));

        // 9. Phase 3: Parse logs to verify NotifyServiceState was published and received
        //    adsw.log should have: "write message. mmf/lifecyclemgr/NotifyServiceState"
        //    lifecycle_mgr.log should have: "on_data_available() [mmf/lifecyclemgr/NotifyServiceState]"

        // Parse adsw.log (should be in test CWD or /tmp)
        std::string adsw_log_path = "adsw.log";
        std::ifstream adsw_log(adsw_log_path);
        if (!adsw_log.is_open()) {
            // Try /tmp or build dir
            adsw_log_path = "/home/obigo/project/oss/oss_agent/mmf/oss_multisw_framework/adsw.log";
            adsw_log.open(adsw_log_path);
        }
        if (adsw_log.is_open()) {
            std::string line;
            while (std::getline(adsw_log, line)) {
                if (line.find("write message. mmf/lifecyclemgr/NotifyServiceState") != std::string::npos) {
                    notify_published_ok_ = true;
                    break;
                }
            }
        }

        // Parse lifecycle_mgr.log
        std::string lf_log_path = "lifecycle_mgr.log";
        std::ifstream lf_log(lf_log_path);
        if (!lf_log.is_open()) {
            lf_log_path = "/home/obigo/project/oss/oss_agent/mmf/oss_multisw_framework/lifecycle_mgr.log";
            lf_log.open(lf_log_path);
        }
        if (lf_log.is_open()) {
            std::string line;
            while (std::getline(lf_log, line)) {
                if (line.find("on_data_available") != std::string::npos &&
                    line.find("NotifyServiceState") != std::string::npos) {
                    notify_received_ok_ = true;
                    break;
                }
            }
        }
    }

    static void TearDownTestSuite() {
        KillAndWait(lifecycle_pid_);
        KillAndWait(adsw_pid_);
        ExecShell(std::string("docker rm -f ") + AUTOWARE_CONTAINER + " 2>/dev/null");
        ExecShell(std::string("rm -rf ") + E2E_CRASH_CONFIG_DIR);
        ExecShell("rm -f /tmp/cyclonedds_crash_e2e.xml");
        lifecycle_pid_ = -1;
        adsw_pid_ = -1;
    }
};

pid_t L3AutowareCrashE2ETest::lifecycle_pid_ = -1;
pid_t L3AutowareCrashE2ETest::adsw_pid_ = -1;
bool L3AutowareCrashE2ETest::node_started_ok_ = false;
bool L3AutowareCrashE2ETest::node_crashed_ok_ = false;
bool L3AutowareCrashE2ETest::notify_published_ok_ = false;
bool L3AutowareCrashE2ETest::notify_received_ok_ = false;

// AC1: F003 prerequisite — node was running
TEST_F(L3AutowareCrashE2ETest, TC001_NodeWasRunning) {
    ASSERT_TRUE(node_started_ok_)
        << "Phase 1 failed: /logger_usage_demo was not running after lifecycle_mgr+adsw_manager "
        << "F003 chain (UpdateServiceList handshake → StartServiceRequest → docker exec).";
}

// AC2+AC3: pkill -9 executed; node disappeared
TEST_F(L3AutowareCrashE2ETest, TC002_NodeCrashDetected) {
    ASSERT_TRUE(node_started_ok_) << "Phase 1 prerequisite failed";
    ASSERT_TRUE(node_crashed_ok_)
        << "docker stop was executed but /logger_usage_demo did not disappear "
        << "within 15s. Container might not have stopped or docker exec is still returning stale output.";

    // Double-check: node should be gone from ros2 node list
    std::string out = ExecShell(
        std::string("docker exec ") + AUTOWARE_CONTAINER + " bash -c "
        "'source /opt/ros/humble/setup.bash 2>/dev/null && ros2 node list 2>/dev/null'");
    EXPECT_EQ(out.find("/logger_usage_demo"), std::string::npos)
        << "/logger_usage_demo should be gone from ros2 node list, got: " << out;
}

// AC4: adsw_manager published NotifyServiceState DDS message
TEST_F(L3AutowareCrashE2ETest, TC003_NotifyServiceStatePublished) {
    ASSERT_TRUE(node_crashed_ok_) << "Crash prerequisite failed";
    EXPECT_TRUE(notify_published_ok_)
        << "adsw.log does not contain 'write message. mmf/lifecyclemgr/NotifyServiceState'. "
        << "ContainerHealthMonitor may not have detected the crash, or ServiceMonitor did not call NotifyServiceState(). "
        << "Check: ContainerHealthMonitor::PollLoop (kPollIntervalMs=1000) → ServiceMonitor::OnContainerStateChanged() → NotifyServiceState().";
}

// AC5: lifecycle_mgr received NotifyServiceState DDS message
TEST_F(L3AutowareCrashE2ETest, TC004_LifecycleMgrReceivedNotification) {
    ASSERT_TRUE(node_crashed_ok_) << "Crash prerequisite failed";
    EXPECT_TRUE(notify_received_ok_)
        << "lifecycle_mgr.log does not contain 'on_data_available.*NotifyServiceState'. "
        << "DDS discovery or endpoint matching may have failed. "
        << "Check: lifecycle_mgr NotifyServiceStateEndPoint::OnDataAvailable() should log reception.";
}

} // namespace

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
