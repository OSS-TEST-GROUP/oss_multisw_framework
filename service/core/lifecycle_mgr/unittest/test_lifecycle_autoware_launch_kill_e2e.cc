// F009 follow-up L3 E2E: node-process death detection for `ros2 launch` mode.
//
// F009's first test exercised the `ros2 run` path. This one exercises the
// `ros2 launch` path: Service::ResolveProcessMatchToken() returns the launch
// file name (instead of the service name), so `pgrep -f <launchfile>$` is
// what monitors liveness.
//
// Phase 1: F003 chain with ALWAYS policy → diagnostic_aggregator launch up.
// Phase 2: docker exec pkill -9 -f example.launch.py → launch supervisor dies.
// Phase 3: PID polling detects the loss, OnServiceProcessDied fires,
//          NotifyServiceState is published.
// Phase 4: ALWAYS recovery → StartService for diagnostic_aggregator (count ≥ 1
//          after baseline).

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

constexpr const char* AUTOWARE_CONTAINER = "autoware-test-launchkill";
constexpr const char* CONFIG_DIR = "/tmp/autoware_launchkill_e2e_test";

// The launch file we use is shipped with ROS2 humble's diagnostic_aggregator
// (small example available out of the box). Its launch supervisor's cmdline
// ends with the launch file name, so Service::ResolveProcessMatchToken picks
// up `<launchfile>$` as the pgrep pattern.
constexpr const char* SVC_PACKAGE    = "diagnostic_aggregator";
constexpr const char* SVC_NAME       = "example_launcher";          // arbitrary id
constexpr const char* SVC_LAUNCHFILE = "example.launch.py";

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

class L3AutowareLaunchKillE2ETest : public ::testing::Test {
public:
    static pid_t lifecycle_pid_;
    static pid_t adsw_pid_;
    static bool  launch_started_ok_;
    static bool  process_died_logged_;
    static bool  notify_published_;
    static bool  notify_received_;
    static bool  launch_recovered_;

    static void SetUpTestSuite() {
        launch_started_ok_   = false;
        process_died_logged_ = false;
        notify_published_    = false;
        notify_received_     = false;
        launch_recovered_    = false;

        // Fast-skip: check docker, mock image, and MMF binaries before any
        // blocking I/O. This exits in <1s on machines without prerequisites.
        std::string skip_reason;
        if (!AutowareE2EPrereqs(skip_reason)) {
            GTEST_SKIP() << skip_reason;
            return;
        }

        // Per-run DDS domain isolation
        int domain = E2EDomainId();
        std::string fastdds_profile = E2EWriteFastDdsProfile(domain);
        std::string domain_str = std::to_string(domain);
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
              << "          - package: \"" << SVC_PACKAGE << "\"\n"
              << "            name: \"" << SVC_NAME << "\"\n"
              << "            launchfile: \"" << SVC_LAUNCHFILE << "\"\n"
              << "            type: \"NORMAL\"\n"
              << "            recovery: \"ALWAYS\"\n"
              << "            enabled: \"true\"\n"
              << "            max: \"0\"\n";
        }

        std::string iface = ExecShell("ip route show default 2>/dev/null | awk '{print $5; exit}'");
        while (!iface.empty() && (iface.back() == '\n' || iface.back() == '\r' || iface.back() == ' '))
            iface.pop_back();
        if (iface.empty()) iface = "enp128s31f6";

        const char* CYCLONE_XML = "/tmp/cyclonedds_launchkill_e2e.xml";
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
                  + " -v " + CYCLONE_XML + ":/tmp/cyclonedds_launchkill.xml"
                  + " -e CYCLONEDDS_URI=file:///tmp/cyclonedds_launchkill.xml"
                  + " -e ROS_DOMAIN_ID=" + domain_str
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
            setenv("ROS_DOMAIN_ID", domain_str.c_str(), 1);
            execl(MmfLfBin().c_str(), "lifecycle_mgr", nullptr);
            _exit(127);
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));

        adsw_pid_ = fork();
        if (adsw_pid_ == 0) {
            setenv("SAVED_REGISTRY_BACKUP", CONFIG_DIR, 1);
            setenv("LD_LIBRARY_PATH", ld_path.c_str(), 1);
            setenv("FASTDDS_DEFAULT_PROFILES_FILE", fastdds_profile.c_str(), 1);
            setenv("ROS_DOMAIN_ID", domain_str.c_str(), 1);
            execl(MmfAdswBin().c_str(), "adsw_manager", nullptr);
            _exit(127);
        }

        // Phase 1: F003 chain — StartService for diagnostic_aggregator fires
        launch_started_ok_ = WaitForCondition([]() {
            return CountLinesContaining("adsw.log",
                std::string("StartService for :: ") + SVC_PACKAGE, 0) >= 1;
        }, 60);
        if (!launch_started_ok_) {
            ADD_FAILURE() << "Phase 1 failed: StartService for "
                          << SVC_PACKAGE << " never logged within 60s.";
            return;
        }

        // Let the launch supervisor settle so PollServicesInContainer sees alive=yes
        // for at least one full cycle before we kill it.
        std::this_thread::sleep_for(std::chrono::seconds(5));
        size_t adsw_baseline = CountLines("adsw.log");
        size_t lf_baseline   = CountLines("lifecycle_mgr.log");

        // Phase 2: pkill the launch supervisor.
        ExecShell(std::string("docker exec ") + AUTOWARE_CONTAINER
                  + " pkill -9 -f " + SVC_LAUNCHFILE + " 2>/dev/null");

        // Phase 3: detection chain — within 15s
        process_died_logged_ = WaitForCondition([adsw_baseline]() {
            return CountLinesContaining("adsw.log",
                std::string("OnServiceProcessDied service=") + SVC_NAME,
                adsw_baseline) > 0;
        }, 15);

        notify_published_ = (CountLinesContaining("adsw.log",
                "write message. mmf/lifecyclemgr/NotifyServiceState",
                adsw_baseline) > 0);

        notify_received_ = WaitForCondition([lf_baseline]() {
            return CountLinesContaining("lifecycle_mgr.log",
                "mmf/lifecyclemgr/NotifyServiceState",
                lf_baseline) > 0;
        }, 5);

        // Phase 4: ALWAYS recovery — second StartService for the package
        launch_recovered_ = WaitForCondition([adsw_baseline]() {
            return CountLinesContaining("adsw.log",
                std::string("StartService for :: ") + SVC_PACKAGE,
                adsw_baseline) >= 1;
        }, 60);
    }

    static void TearDownTestSuite() {
        KillAndWait(adsw_pid_);
        KillAndWait(lifecycle_pid_);
        ExecShell(std::string("docker stop ") + AUTOWARE_CONTAINER + " 2>/dev/null; docker rm " + AUTOWARE_CONTAINER + " 2>/dev/null");
        ExecShell(std::string("rm -rf ") + CONFIG_DIR);
        ExecShell("rm -f /tmp/cyclonedds_launchkill_e2e.xml");
    }
};

pid_t L3AutowareLaunchKillE2ETest::lifecycle_pid_ = -1;
pid_t L3AutowareLaunchKillE2ETest::adsw_pid_      = -1;
bool  L3AutowareLaunchKillE2ETest::launch_started_ok_   = false;
bool  L3AutowareLaunchKillE2ETest::process_died_logged_ = false;
bool  L3AutowareLaunchKillE2ETest::notify_published_    = false;
bool  L3AutowareLaunchKillE2ETest::notify_received_     = false;
bool  L3AutowareLaunchKillE2ETest::launch_recovered_    = false;

TEST_F(L3AutowareLaunchKillE2ETest, TC001_Phase1_LaunchStarted) {
    ASSERT_TRUE(launch_started_ok_)
        << "F003 chain failed: StartService for " << SVC_PACKAGE
        << " never logged. lifecycle_mgr didn't issue StartServiceRequest, "
        << "or adsw_manager refused to run ros2 launch.";
}

TEST_F(L3AutowareLaunchKillE2ETest, TC002_OnServiceProcessDied_Fired) {
    ASSERT_TRUE(launch_started_ok_) << "Phase 1 prerequisite failed";
    EXPECT_TRUE(process_died_logged_)
        << "adsw.log did not contain 'OnServiceProcessDied service=" << SVC_NAME
        << "' within 15s after killing the launch supervisor. "
        << "Likely Service::ResolveProcessMatchToken did not return the "
        << "launchfile, or pgrep -f <launchfile>$ matched nothing.";
}

TEST_F(L3AutowareLaunchKillE2ETest, TC003_NotifyServiceStatePublished) {
    ASSERT_TRUE(process_died_logged_) << "PID death detection prerequisite failed";
    EXPECT_TRUE(notify_published_)
        << "adsw.log missing 'write message. NotifyServiceState' post-baseline.";
}

TEST_F(L3AutowareLaunchKillE2ETest, TC004_LifecycleMgrReceivedNotification) {
    ASSERT_TRUE(notify_published_) << "DDS publish prerequisite failed";
    EXPECT_TRUE(notify_received_)
        << "lifecycle_mgr.log does not show NotifyServiceState delivery.";
}

TEST_F(L3AutowareLaunchKillE2ETest, TC005_LaunchRecoveredViaAlwaysPolicy) {
    ASSERT_TRUE(notify_received_) << "Notification receipt prerequisite failed";
    EXPECT_TRUE(launch_recovered_)
        << "Second StartService for " << SVC_PACKAGE
        << " did not fire within 60s. ALWAYS recovery chain broken.";
}

} // namespace

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
