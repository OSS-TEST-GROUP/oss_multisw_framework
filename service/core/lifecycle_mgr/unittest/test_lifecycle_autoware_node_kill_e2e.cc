// F009 L3 E2E: ROS2 node-level crash detection via PID polling.
//
// F006 used `docker stop` because ContainerHealthMonitor only watches the
// container itself. F009 adds per-service PID polling (kill -0 inside the
// container) so SIGKILL of an individual node also fires NotifyServiceState.
//
// Phase 1: F003 chain with ALWAYS policy → /logger_usage_demo running.
// Phase 2: docker exec <container> pkill -9 logging_demo_main
// Phase 3: Within ~10s, adsw_manager must publish NotifyServiceState and
//          lifecycle_mgr must receive it (verified via log scraping).
// Phase 4: ALWAYS policy must restart the node within 60s (full chain).

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

constexpr const char* AUTOWARE_CONTAINER = "autoware-test-nodekill";
constexpr const char* CONFIG_DIR = "/tmp/autoware_nodekill_e2e_test";

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

size_t CountLines(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return 0;
    size_t n = 0;
    std::string line;
    while (std::getline(f, line)) n++;
    return n;
}

// Count occurrences of `needle` in file starting from `skip_lines` (so we can
// distinguish events that fired BEFORE pkill from events that fired AFTER).
int CountLinesContaining(const std::string& path, const std::string& needle, size_t skip_lines) {
    std::ifstream f(path);
    if (!f.is_open()) return -1;
    std::string line;
    size_t idx = 0;
    int count = 0;
    while (std::getline(f, line)) {
        if (idx++ < skip_lines) continue;
        if (line.find(needle) != std::string::npos) count++;
    }
    return count;
}

class L3AutowareNodeKillE2ETest : public ::testing::Test {
public:
    static pid_t lifecycle_pid_;
    static pid_t adsw_pid_;
    static bool  node_started_ok_;
    static bool  process_died_logged_;
    static bool  notify_published_;
    static bool  notify_received_;
    static bool  node_recovered_;

    static void SetUpTestSuite() {
        node_started_ok_     = false;
        process_died_logged_ = false;
        notify_published_    = false;
        notify_received_     = false;
        node_recovered_      = false;

        // Fast-skip: check docker, mock image, and MMF binaries before any
        // blocking I/O. This exits in <1s on machines without prerequisites.
        std::string skip_reason;
        if (!AutowareE2EPrereqs(skip_reason)) {
            GTEST_SKIP() << skip_reason;
            return;
        }

        std::string fastdds_profile = MmfFastDdsProfile();
        setenv("FASTDDS_DEFAULT_PROFILES_FILE", fastdds_profile.c_str(), 1);

        ExecShell("pkill -9 adsw_manager 2>/dev/null");
        ExecShell("pkill -9 lifecycle_mgr 2>/dev/null");
        std::this_thread::sleep_for(std::chrono::seconds(2));

        mkdir(CONFIG_DIR, 0755);
        {
            std::ofstream f(std::string(CONFIG_DIR) + "/saved-service-list.yaml");
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
        }

        std::string iface = ExecShell("ip route show default 2>/dev/null | awk '{print $5; exit}'");
        while (!iface.empty() && (iface.back() == '\n' || iface.back() == '\r' || iface.back() == ' '))
            iface.pop_back();
        if (iface.empty()) iface = "enp128s31f6";

        const char* CYCLONE_XML = "/tmp/cyclonedds_nodekill_e2e.xml";
        {
            std::ofstream cf(CYCLONE_XML);
            cf << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
               << "<CycloneDDS><Domain><General><Interfaces>"
               << "<NetworkInterface name=\"" << iface << "\"/>"
               << "</Interfaces></General></Domain></CycloneDDS>\n";
        }

        ExecShell(std::string("docker stop ") + AUTOWARE_CONTAINER + " 2>/dev/null; docker rm " + AUTOWARE_CONTAINER + " 2>/dev/null");
        std::this_thread::sleep_for(std::chrono::seconds(1));
        ExecShell(std::string("docker run -d --name ") + AUTOWARE_CONTAINER
                  + " --network host"
                  + " -v " + CYCLONE_XML + ":/tmp/cyclonedds_nodekill.xml"
                  + " -e CYCLONEDDS_URI=file:///tmp/cyclonedds_nodekill.xml"
                  + " " + AutowareImage() + " sleep infinity");

        bool container_ok = WaitForCondition([]() {
            std::string out = ExecShell(
                std::string("docker ps --filter name=") + AUTOWARE_CONTAINER
                + " --filter status=running --format '{{.Names}}'");
            return out.find(AUTOWARE_CONTAINER) != std::string::npos;
        }, 30);
        if (!container_ok) {
            ADD_FAILURE() << "Container did not start within 30s";
            return;
        }

        std::string ld_path = MmfBuildLdPath();
        std::string lf_config = MmfLfConfig();

        lifecycle_pid_ = fork();
        if (lifecycle_pid_ == 0) {
            setenv("MMF_CONFIG_PATH", lf_config.c_str(), 1);
            setenv("LD_LIBRARY_PATH", ld_path.c_str(), 1);
            setenv("FASTDDS_DEFAULT_PROFILES_FILE", fastdds_profile.c_str(), 1);
            execl(MmfLfBin().c_str(), "lifecycle_mgr", nullptr);
            _exit(127);
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));

        adsw_pid_ = fork();
        if (adsw_pid_ == 0) {
            setenv("SAVED_REGISTRY_BACKUP", CONFIG_DIR, 1);
            setenv("LD_LIBRARY_PATH", ld_path.c_str(), 1);
            setenv("FASTDDS_DEFAULT_PROFILES_FILE", fastdds_profile.c_str(), 1);
            execl(MmfAdswBin().c_str(), "adsw_manager", nullptr);
            _exit(127);
        }

        // Phase 1: F003 chain — node running
        node_started_ok_ = WaitForCondition([]() {
            std::string out = ExecShell(
                std::string("docker exec ") + AUTOWARE_CONTAINER + " bash -c "
                "'source /opt/ros/humble/setup.bash 2>/dev/null && ros2 node list 2>/dev/null'");
            return out.find("/logger_usage_demo") != std::string::npos;
        }, 60);
        if (!node_started_ok_) {
            ADD_FAILURE() << "Phase 1 failed: node did not start within 60s";
            return;
        }

        // Settle, then snapshot log baselines so post-pkill events are isolated
        std::this_thread::sleep_for(std::chrono::seconds(3));
        size_t adsw_baseline = CountLines("adsw.log");
        size_t lf_baseline   = CountLines("lifecycle_mgr.log");

        // Phase 2: pkill -9 -f the node inside the container.
        // -f matches against the full cmdline (not /proc/<pid>/comm, which is
        // truncated to 15 chars on Linux — "logging_demo_main" is 17 chars and
        // would never match without -f). This kills the actual C++ binary and
        // its ros2-run wrapper. ContainerHealthMonitor's PID-polling path
        // (kPollIntervalMs=3000) should detect this within one polling cycle.
        ExecShell(std::string("docker exec ") + AUTOWARE_CONTAINER
                  + " pkill -9 -f logging_demo_main 2>/dev/null");

        // Phase 3: Verify the detection chain fires within 15s.
        process_died_logged_ = WaitForCondition([adsw_baseline]() {
            return CountLinesContaining("adsw.log",
                "OnServiceProcessDied service=logging_demo_main",
                adsw_baseline) > 0;
        }, 15);

        notify_published_ = (CountLinesContaining("adsw.log",
                "write message. mmf/lifecyclemgr/NotifyServiceState",
                adsw_baseline) > 0);

        notify_received_ = WaitForCondition([lf_baseline]() {
            // lifecycle_mgr endpoint logs message delivery on its subscription.
            return CountLinesContaining("lifecycle_mgr.log",
                "mmf/lifecyclemgr/NotifyServiceState",
                lf_baseline) > 0;
        }, 5);

        // Phase 4: Full recovery chain — ALWAYS policy → StartServiceRequest →
        // adsw_manager StartService for logging_demo (a SECOND time after the baseline).
        node_recovered_ = WaitForCondition([adsw_baseline]() {
            return CountLinesContaining("adsw.log",
                "StartService for :: logging_demo", adsw_baseline) >= 1;
        }, 60);
    }

    static void TearDownTestSuite() {
        KillAndWait(adsw_pid_);
        KillAndWait(lifecycle_pid_);
        ExecShell(std::string("docker stop ") + AUTOWARE_CONTAINER + " 2>/dev/null; docker rm " + AUTOWARE_CONTAINER + " 2>/dev/null");
        ExecShell(std::string("rm -rf ") + CONFIG_DIR);
        ExecShell("rm -f /tmp/cyclonedds_nodekill_e2e.xml");
    }
};

pid_t L3AutowareNodeKillE2ETest::lifecycle_pid_ = -1;
pid_t L3AutowareNodeKillE2ETest::adsw_pid_      = -1;
bool  L3AutowareNodeKillE2ETest::node_started_ok_     = false;
bool  L3AutowareNodeKillE2ETest::process_died_logged_ = false;
bool  L3AutowareNodeKillE2ETest::notify_published_    = false;
bool  L3AutowareNodeKillE2ETest::notify_received_     = false;
bool  L3AutowareNodeKillE2ETest::node_recovered_      = false;

TEST_F(L3AutowareNodeKillE2ETest, TC001_Phase1_NodeRunning) {
    ASSERT_TRUE(node_started_ok_)
        << "F003 chain failed — /logger_usage_demo never appeared with ALWAYS policy.";
}

TEST_F(L3AutowareNodeKillE2ETest, TC002_OnServiceProcessDied_Fired) {
    ASSERT_TRUE(node_started_ok_) << "Phase 1 prerequisite failed";
    EXPECT_TRUE(process_died_logged_)
        << "adsw.log did not contain 'OnServiceProcessDied service=logging_demo_main' "
        << "within 15s after pkill. ContainerHealthMonitor PID polling did NOT detect "
        << "the SIGKILL — verify `docker exec kill -0 <pid>` works against the stored PID.";
}

TEST_F(L3AutowareNodeKillE2ETest, TC003_NotifyServiceStatePublished) {
    ASSERT_TRUE(process_died_logged_) << "PID death-detection prerequisite failed";
    EXPECT_TRUE(notify_published_)
        << "adsw.log did not log 'write message. NotifyServiceState' after the death event. "
        << "OnServiceProcessDied fired but DDS publish did not — check ServiceMonitor::NotifyServiceState.";
}

TEST_F(L3AutowareNodeKillE2ETest, TC004_LifecycleMgrReceivedNotification) {
    ASSERT_TRUE(notify_published_) << "DDS publish prerequisite failed";
    EXPECT_TRUE(notify_received_)
        << "lifecycle_mgr.log does not reference NotifyServiceState after the death event. "
        << "DDS subscription/discovery may be broken.";
}

TEST_F(L3AutowareNodeKillE2ETest, TC005_NodeRecoveredViaAlwaysPolicy) {
    ASSERT_TRUE(notify_received_) << "Notification receipt prerequisite failed";
    EXPECT_TRUE(node_recovered_)
        << "Node did not reappear within 60s after kill. ALWAYS policy chain "
        << "(NotifyServiceState → PolicyAlways → StartServiceRequest → docker exec ros2 run) failed.";
}

} // namespace

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
