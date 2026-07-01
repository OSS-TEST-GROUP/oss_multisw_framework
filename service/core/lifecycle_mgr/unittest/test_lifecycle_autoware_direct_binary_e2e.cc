// F010 L3 E2E: direct binary execution inside a container.
//
// Verifies the third execution mode for adsw_manager:
//   exec_type: "binary" + launchfile empty  →  docker exec runs the binary
//   referenced by `package` directly, with no ros2 run/launch indirection.
//
// Phase 1: F003 chain with ALWAYS policy + exec_type=binary →
//   docker exec autoware-test-binary bash -c "/usr/bin/yes >/dev/null & echo $!"
//   starts /usr/bin/yes inside the container.
// Phase 2: docker exec pkill -9 -f /usr/bin/yes$ kills the binary.
// Phase 3: PID polling sees alive=no (token resolved to `/usr/bin/yes`),
//   OnServiceProcessDied fires, NotifyServiceState publishes,
//   lifecycle_mgr receives.
// Phase 4: ALWAYS recovery → second StartService for the same package.

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

constexpr const char* AUTOWARE_CONTAINER = "autoware-test-binary";
constexpr const char* CONFIG_DIR = "/tmp/autoware_direct_binary_e2e_test";

// /usr/bin/yes is shipped in coreutils on every Linux image; it loops forever
// printing 'y\n' until stdout closes (or it's killed). Bash with a single
// background command exec-replaces the subshell into yes, so the PID echoed
// by `& echo $!` IS the yes process — no intermediate wrapper.
constexpr const char* SVC_PACKAGE = "/usr/bin/yes";
constexpr const char* SVC_NAME    = "yes_daemon";  // arbitrary id

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

class L3AutowareDirectBinaryE2ETest : public ::testing::Test {
public:
    static pid_t lifecycle_pid_;
    static pid_t adsw_pid_;
    static bool  binary_started_ok_;
    static bool  process_died_logged_;
    static bool  notify_published_;
    static bool  notify_received_;
    static bool  binary_recovered_;

    static void SetUpTestSuite() {
        binary_started_ok_   = false;
        process_died_logged_ = false;
        notify_published_    = false;
        notify_received_     = false;
        binary_recovered_    = false;

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
              << "          - package: \"" << SVC_PACKAGE << "\"\n"
              << "            name: \"" << SVC_NAME << "\"\n"
              << "            launchfile: \"\"\n"
              << "            exec_type: \"binary\"\n"
              << "            type: \"NORMAL\"\n"
              << "            recovery: \"ALWAYS\"\n"
              << "            enabled: \"true\"\n"
              << "            max: \"0\"\n";
        }

        std::string iface = ExecShell("ip route show default 2>/dev/null | awk '{print $5; exit}'");
        while (!iface.empty() && (iface.back() == '\n' || iface.back() == '\r' || iface.back() == ' '))
            iface.pop_back();
        if (iface.empty()) iface = "enp128s31f6";

        const char* CYCLONE_XML = "/tmp/cyclonedds_direct_binary_e2e.xml";
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
                  + " -v " + CYCLONE_XML + ":/tmp/cyclonedds_direct_binary.xml"
                  + " -e CYCLONEDDS_URI=file:///tmp/cyclonedds_direct_binary.xml"
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

        // Phase 1: StartService for the binary fires.  The `package` field
        // appears in the log line; we use that as our success marker since
        // there's no ros2 daemon involvement for a plain /usr/bin/yes.
        binary_started_ok_ = WaitForCondition([]() {
            return CountLinesContaining("adsw.log",
                std::string("StartService for :: ") + SVC_PACKAGE, 0) >= 1;
        }, 60);
        if (!binary_started_ok_) {
            ADD_FAILURE() << "Phase 1 failed: StartService for " << SVC_PACKAGE
                          << " never logged within 60s.";
            return;
        }

        // Let polling observe alive=yes for at least one cycle first.
        std::this_thread::sleep_for(std::chrono::seconds(5));
        size_t adsw_baseline = CountLines("adsw.log");
        size_t lf_baseline   = CountLines("lifecycle_mgr.log");

        // Phase 2: kill the binary inside the container.  pgrep matches on
        // full cmdline; /usr/bin/yes has nothing after the path, so `$`
        // anchored pkill -f /usr/bin/yes$ targets only this process.
        ExecShell(std::string("docker exec ") + AUTOWARE_CONTAINER
                  + " pkill -9 -f '" + SVC_PACKAGE + "$' 2>/dev/null");

        // Phase 3: detection chain within ~15s.
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

        // Phase 4: ALWAYS recovery → second StartService for the package.
        binary_recovered_ = WaitForCondition([adsw_baseline]() {
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
        ExecShell("rm -f /tmp/cyclonedds_direct_binary_e2e.xml");
    }
};

pid_t L3AutowareDirectBinaryE2ETest::lifecycle_pid_ = -1;
pid_t L3AutowareDirectBinaryE2ETest::adsw_pid_      = -1;
bool  L3AutowareDirectBinaryE2ETest::binary_started_ok_   = false;
bool  L3AutowareDirectBinaryE2ETest::process_died_logged_ = false;
bool  L3AutowareDirectBinaryE2ETest::notify_published_    = false;
bool  L3AutowareDirectBinaryE2ETest::notify_received_     = false;
bool  L3AutowareDirectBinaryE2ETest::binary_recovered_    = false;

TEST_F(L3AutowareDirectBinaryE2ETest, TC001_Phase1_BinaryStarted) {
    ASSERT_TRUE(binary_started_ok_)
        << "F003 chain failed: StartService for " << SVC_PACKAGE
        << " never logged. exec_type=binary may not be recognized, or "
        << "lifecycle_mgr didn't issue StartServiceRequest.";
}

TEST_F(L3AutowareDirectBinaryE2ETest, TC002_OnServiceProcessDied_Fired) {
    ASSERT_TRUE(binary_started_ok_) << "Phase 1 prerequisite failed";
    EXPECT_TRUE(process_died_logged_)
        << "adsw.log missing 'OnServiceProcessDied service=" << SVC_NAME
        << "' within 15s after killing /usr/bin/yes. "
        << "Service::ResolveProcessMatchToken likely returned the wrong token "
        << "for the binary mode, so pgrep -f <token>$ never matched the process.";
}

TEST_F(L3AutowareDirectBinaryE2ETest, TC003_NotifyServiceStatePublished) {
    ASSERT_TRUE(process_died_logged_) << "PID death detection prerequisite failed";
    EXPECT_TRUE(notify_published_)
        << "adsw.log missing 'write message. NotifyServiceState' post-baseline.";
}

TEST_F(L3AutowareDirectBinaryE2ETest, TC004_LifecycleMgrReceivedNotification) {
    ASSERT_TRUE(notify_published_) << "DDS publish prerequisite failed";
    EXPECT_TRUE(notify_received_)
        << "lifecycle_mgr.log does not show NotifyServiceState delivery.";
}

TEST_F(L3AutowareDirectBinaryE2ETest, TC005_BinaryRecoveredViaAlwaysPolicy) {
    ASSERT_TRUE(notify_received_) << "Notification receipt prerequisite failed";
    EXPECT_TRUE(binary_recovered_)
        << "Second StartService for " << SVC_PACKAGE
        << " did not fire within 60s. ALWAYS recovery chain broken.";
}

} // namespace

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
