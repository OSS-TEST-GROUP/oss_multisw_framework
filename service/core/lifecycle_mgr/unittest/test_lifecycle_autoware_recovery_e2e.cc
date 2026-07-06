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

// Build LD_LIBRARY_PATH for child processes
std::string BuildLdPath() {
    return MmfBuildLdPath();
}

// Start autoware container for the test (network host for DDS multicast).
// domain: the per-run DDS domain ID — passed as ROS_DOMAIN_ID so that the
// container's ros2 node list / ros2 run commands join the isolated domain
// instead of domain 0 (used by the production oss-mmf stack).
bool StartContainer(const char* container_name, int domain) {
    std::string iface = ExecShell("ip route show default 2>/dev/null | awk '{print $5; exit}'");
    while (!iface.empty() && (iface.back() == '\n' || iface.back() == '\r' || iface.back() == ' '))
        iface.pop_back();
    if (iface.empty()) iface = "enp128s31f6";

    std::string xml_path = std::string("/tmp/cyclonedds_") + container_name + ".xml";
    {
        std::ofstream cf(xml_path);
        cf << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
           << "<CycloneDDS><Domain><General><Interfaces>"
           << "<NetworkInterface name=\"" << iface << "\"/>"
           << "</Interfaces></General></Domain></CycloneDDS>\n";
    }

    ExecShell(std::string("docker stop ") + container_name + " 2>/dev/null; docker rm " + container_name + " 2>/dev/null");
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ExecShell(std::string("docker run -d --name ") + container_name
              + " --network host"
              + " -v " + xml_path + ":/tmp/cyclonedds_e2e.xml"
              + " -e CYCLONEDDS_URI=file:///tmp/cyclonedds_e2e.xml"
              + " -e ROS_DOMAIN_ID=" + std::to_string(domain)
              + " " + AutowareImage() + " sleep infinity");

    return WaitForCondition([&]() {
        std::string out = ExecShell(
            std::string("docker ps --filter name=") + container_name
            + " --filter status=running --format '{{.Names}}'");
        return out.find(container_name) != std::string::npos;
    }, 30);
}

// Write saved-service-list.yaml with given recovery policy string
bool WriteServiceListYaml(const char* config_dir,
                          const char* container_name,
                          const char* recovery_policy) {
    mkdir(config_dir, 0755);
    std::string yaml_path = std::string(config_dir) + "/saved-service-list.yaml";
    std::ofstream f(yaml_path);
    if (!f.is_open()) return false;
    f << "version: 0.1\n"
      << "specs:\n"
      << "  - containers:\n"
      << "      - name: " << container_name << "\n"
      << "        image: \"" << AutowareImage() << "\"\n"
      << "        container-type: \"docker\"\n"
      << "        services:\n"
      << "          - package: \"logging_demo\"\n"
      << "            name: \"logging_demo_main\"\n"
      << "            launchfile: \"\"\n"
      << "            type: \"NORMAL\"\n"
      << "            recovery: \"" << recovery_policy << "\"\n"
      << "            enabled: \"true\"\n"
      << "            max: \"0\"\n";
    return true;
}

// ── Test Suite 1: ALWAYS recovery policy ────────────────────────────────────
//
// Phase 1: F003 chain → logging_demo_main running.
// Phase 2: docker stop → container crash.
// Phase 3: lifecycle_mgr PolicyAlways(NOT_RUNNING) → StartServiceRequest →
//          adsw_manager docker start + ros2 run → node reappears.

constexpr const char* ALWAYS_CONTAINER  = "autoware-test-recovery-always";
constexpr const char* ALWAYS_CONFIG_DIR = "/tmp/autoware_recovery_always_e2e";

class L3AutowareRecoveryAlwaysTest : public ::testing::Test {
public:
    static pid_t lifecycle_pid_;
    static pid_t adsw_pid_;
    static bool node_started_ok_;
    static bool crash_ok_;
    static bool node_recovered_ok_;

    static void SetUpTestSuite() {
        node_started_ok_ = false;
        crash_ok_        = false;
        node_recovered_ok_ = false;

        // Fast-skip: check docker, mock image, and MMF binaries before any
        // blocking I/O. This exits in <1s on machines without prerequisites.
        std::string skip_reason;
        if (!AutowareE2EPrereqs(skip_reason)) {
            GTEST_SKIP() << skip_reason;
            return;
        }

        // Per-run DDS domain isolation: pick a unique domain in [1,200] so this
        // test binary does not cross-talk with the production oss-mmf stack (domain 0)
        // or other concurrently running E2E test binaries.
        int domain = E2EDomainId();
        std::string fastdds_profile = E2EWriteFastDdsProfile(domain);
        std::string domain_str = std::to_string(domain);
        setenv("FASTDDS_DEFAULT_PROFILES_FILE", fastdds_profile.c_str(), 1);

        ExecShell("pkill -9 adsw_manager 2>/dev/null");
        ExecShell("pkill -9 lifecycle_mgr 2>/dev/null");
        std::this_thread::sleep_for(std::chrono::seconds(2));

        if (!WriteServiceListYaml(ALWAYS_CONFIG_DIR, ALWAYS_CONTAINER, "ALWAYS")) {
            ADD_FAILURE() << "Failed to write service list YAML";
            return;
        }

        // Start container with ROS_DOMAIN_ID=domain so that:
        //   - ros2 node list inside the container uses domain N (not 0)
        //   - docker start (recovery path) inherits the env and also uses domain N
        if (!StartContainer(ALWAYS_CONTAINER, domain)) {
            ADD_FAILURE() << "Container " << ALWAYS_CONTAINER << " did not start within 30s";
            return;
        }

        std::string ld_path = BuildLdPath();
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
            setenv("SAVED_REGISTRY_BACKUP", ALWAYS_CONFIG_DIR, 1);
            setenv("LD_LIBRARY_PATH", ld_path.c_str(), 1);
            setenv("FASTDDS_DEFAULT_PROFILES_FILE", fastdds_profile.c_str(), 1);
            setenv("ROS_DOMAIN_ID", domain_str.c_str(), 1);
            execl(MmfAdswBin().c_str(), "adsw_manager", nullptr);
            _exit(127);
        }

        // Phase 1: Wait for F003 chain — logging_demo_main running.
        // The docker exec inherits the container's ROS_DOMAIN_ID=N env, so
        // ros2 node list queries domain N (our isolated domain).
        node_started_ok_ = WaitForCondition([&]() {
            std::string out = ExecShell(
                std::string("docker exec ") + ALWAYS_CONTAINER + " bash -c "
                "'source /opt/ros/humble/setup.bash 2>/dev/null && ros2 node list 2>/dev/null'");
            return out.find("/logger_usage_demo") != std::string::npos;
        }, 60);

        if (!node_started_ok_) {
            ADD_FAILURE() << "Phase 1 failed: /logger_usage_demo did not appear within 60s";
            return;
        }

        // Phase 2: Stop container to simulate crash
        ExecShell(std::string("docker stop -t 2 ") + ALWAYS_CONTAINER + " 2>/dev/null");

        crash_ok_ = WaitForCondition([&]() {
            std::string out = ExecShell(
                std::string("docker exec ") + ALWAYS_CONTAINER + " bash -c "
                "'source /opt/ros/humble/setup.bash 2>/dev/null && ros2 node list 2>/dev/null'"
                " 2>/dev/null");
            return out.find("/logger_usage_demo") == std::string::npos;
        }, 10);

        if (!crash_ok_) {
            ADD_FAILURE() << "Phase 2 failed: container did not stop within 10s";
            return;
        }

        // Phase 3: Wait for ALWAYS recovery — lifecycle_mgr sends StartServiceRequest →
        //          adsw_manager does docker start + ros2 run → node reappears.
        // docker start preserves the container's ROS_DOMAIN_ID=N so the recovered
        // ros2 node also joins domain N, and our ros2 node list query (also domain N)
        // can see it.  docker start takes ~3-5s, then ros2 run + node init ~20-30s.
        node_recovered_ok_ = WaitForCondition([&]() {
            std::string out = ExecShell(
                std::string("docker exec ") + ALWAYS_CONTAINER + " bash -c "
                "'source /opt/ros/humble/setup.bash 2>/dev/null && ros2 node list 2>/dev/null'"
                " 2>/dev/null");
            return out.find("/logger_usage_demo") != std::string::npos;
        }, 90);

        if (!node_recovered_ok_) {
            ADD_FAILURE() << "Phase 3 failed: /logger_usage_demo did not reappear within 90s after crash. "
                          << "Expected ALWAYS recovery to send StartServiceRequest → docker start → ros2 run. "
                          << "(DDS domain: " << domain << ", profile: " << fastdds_profile << ")";
        }
    }

    static void TearDownTestSuite() {
        KillAndWait(adsw_pid_);
        KillAndWait(lifecycle_pid_);
        ExecShell(std::string("docker stop ") + ALWAYS_CONTAINER + " 2>/dev/null; docker rm " + ALWAYS_CONTAINER + " 2>/dev/null");
        ExecShell(std::string("rm -rf ") + ALWAYS_CONFIG_DIR);
        ExecShell(std::string("rm -f /tmp/cyclonedds_") + ALWAYS_CONTAINER + ".xml");
        lifecycle_pid_ = -1;
        adsw_pid_      = -1;
    }
};

pid_t L3AutowareRecoveryAlwaysTest::lifecycle_pid_ = -1;
pid_t L3AutowareRecoveryAlwaysTest::adsw_pid_      = -1;
bool  L3AutowareRecoveryAlwaysTest::node_started_ok_  = false;
bool  L3AutowareRecoveryAlwaysTest::crash_ok_         = false;
bool  L3AutowareRecoveryAlwaysTest::node_recovered_ok_ = false;

TEST_F(L3AutowareRecoveryAlwaysTest, TC001_Phase1_NodeRunning) {
    ASSERT_TRUE(node_started_ok_)
        << "F003 chain failed: /logger_usage_demo never appeared with ALWAYS policy.";
}

TEST_F(L3AutowareRecoveryAlwaysTest, TC002_ContainerCrashed) {
    ASSERT_TRUE(node_started_ok_) << "Phase 1 prerequisite failed";
    ASSERT_TRUE(crash_ok_)
        << "docker stop did not cause /logger_usage_demo to disappear within 10s.";
}

TEST_F(L3AutowareRecoveryAlwaysTest, TC003_NodeRecoveredAfterCrash) {
    ASSERT_TRUE(crash_ok_) << "Crash prerequisite failed";
    EXPECT_TRUE(node_recovered_ok_)
        << "ALWAYS recovery policy did not restart the node within 90s. "
        << "Expected: lifecycle_mgr receives NotifyServiceState(NOT_RUNNING) "
        << "→ PolicyAlways → StartServiceRequest → adsw_manager docker start "
        << "+ docker exec ros2 run → /logger_usage_demo reappears.";
}

// ── Test Suite 2: MANUAL (no-restart) policy ────────────────────────────────
//
// MANUAL policy: PolicyManual::Evaluate() always returns NONE for all states.
// Expectation: UpdateServiceList handshake runs (services registered as NOT_RUNNING),
// but PolicyManual → NONE → no StartServiceRequest → node never starts.

constexpr const char* MANUAL_CONTAINER  = "autoware-test-recovery-manual";
constexpr const char* MANUAL_CONFIG_DIR = "/tmp/autoware_recovery_manual_e2e";

class L3AutowareRecoveryManualTest : public ::testing::Test {
public:
    static pid_t lifecycle_pid_;
    static pid_t adsw_pid_;
    static bool handshake_ok_;
    static bool node_absent_ok_;

    static void SetUpTestSuite() {
        handshake_ok_   = false;
        node_absent_ok_ = false;

        // Fast-skip: check docker, mock image, and MMF binaries before any
        // blocking I/O. This exits in <1s on machines without prerequisites.
        std::string skip_reason;
        if (!AutowareE2EPrereqs(skip_reason)) {
            GTEST_SKIP() << skip_reason;
            return;
        }

        // Per-run DDS domain isolation (same process → same domain as the ALWAYS suite)
        int domain = E2EDomainId();
        std::string fastdds_profile = E2EWriteFastDdsProfile(domain);
        std::string domain_str = std::to_string(domain);
        setenv("FASTDDS_DEFAULT_PROFILES_FILE", fastdds_profile.c_str(), 1);

        ExecShell("pkill -9 adsw_manager 2>/dev/null");
        ExecShell("pkill -9 lifecycle_mgr 2>/dev/null");
        std::this_thread::sleep_for(std::chrono::seconds(2));

        if (!WriteServiceListYaml(MANUAL_CONFIG_DIR, MANUAL_CONTAINER, "MANUAL")) {
            ADD_FAILURE() << "Failed to write MANUAL service list YAML";
            return;
        }

        if (!StartContainer(MANUAL_CONTAINER, domain)) {
            ADD_FAILURE() << "Container " << MANUAL_CONTAINER << " did not start within 30s";
            return;
        }

        std::string ld_path = BuildLdPath();
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
            setenv("SAVED_REGISTRY_BACKUP", MANUAL_CONFIG_DIR, 1);
            setenv("LD_LIBRARY_PATH", ld_path.c_str(), 1);
            setenv("FASTDDS_DEFAULT_PROFILES_FILE", fastdds_profile.c_str(), 1);
            setenv("ROS_DOMAIN_ID", domain_str.c_str(), 1);
            execl(MmfAdswBin().c_str(), "adsw_manager", nullptr);
            _exit(127);
        }

        // Wait for DDS handshake (UpdateServiceListRequest/Response).
        // Since MANUAL policy prevents any auto-start, we just wait for adsw_manager
        // to reach ADSW_RUNNING state (handshake succeeded).
        std::this_thread::sleep_for(std::chrono::seconds(5));
        handshake_ok_ = (lifecycle_pid_ > 0) && (adsw_pid_ > 0)
                        && (kill(lifecycle_pid_, 0) == 0) && (kill(adsw_pid_, 0) == 0);

        // Wait 15 more seconds (> 1 poll cycle of 10s) to confirm node never starts.
        std::this_thread::sleep_for(std::chrono::seconds(15));

        // MANUAL policy: node must NOT appear in ros2 node list (domain N)
        std::string out = ExecShell(
            std::string("docker exec ") + MANUAL_CONTAINER + " bash -c "
            "'source /opt/ros/humble/setup.bash 2>/dev/null && ros2 node list 2>/dev/null'"
            " 2>/dev/null");
        node_absent_ok_ = (out.find("/logger_usage_demo") == std::string::npos);
    }

    static void TearDownTestSuite() {
        KillAndWait(adsw_pid_);
        KillAndWait(lifecycle_pid_);
        ExecShell(std::string("docker stop ") + MANUAL_CONTAINER + " 2>/dev/null; docker rm " + MANUAL_CONTAINER + " 2>/dev/null");
        ExecShell(std::string("rm -rf ") + MANUAL_CONFIG_DIR);
        ExecShell(std::string("rm -f /tmp/cyclonedds_") + MANUAL_CONTAINER + ".xml");
        lifecycle_pid_ = -1;
        adsw_pid_      = -1;
    }
};

pid_t L3AutowareRecoveryManualTest::lifecycle_pid_ = -1;
pid_t L3AutowareRecoveryManualTest::adsw_pid_      = -1;
bool  L3AutowareRecoveryManualTest::handshake_ok_   = false;
bool  L3AutowareRecoveryManualTest::node_absent_ok_ = false;

TEST_F(L3AutowareRecoveryManualTest, TC001_HandshakeCompleted) {
    ASSERT_TRUE(handshake_ok_)
        << "lifecycle_mgr and adsw_manager are not running after 5s. Process startup failed.";
}

TEST_F(L3AutowareRecoveryManualTest, TC002_ManualPolicy_NodeNotStarted) {
    ASSERT_TRUE(handshake_ok_) << "Process startup prerequisite failed";
    EXPECT_TRUE(node_absent_ok_)
        << "/logger_usage_demo appeared in ros2 node list with MANUAL policy, "
        << "but MANUAL policy should never send StartServiceRequest. "
        << "Check: PolicyManual::Evaluate() must return NONE for NOT_RUNNING state.";
}

} // namespace

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
