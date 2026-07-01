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

// DDS
#include "transmitter/DDSMessageEndPoint.hpp"
#include "lifecycle/SetServiceModeRequestPubSubTypes.hpp"
#include "lifecycle/SetServiceModeRequest.hpp"

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/rtps/transport/UDPv4TransportDescriptor.hpp>

using namespace eprosima::fastdds::dds;
using namespace eprosima::fastdds::rtps;
using namespace lifecycle_manager;

#include "autoware_e2e_common.h"

namespace {

// ── Constants ──────────────────────────────────────────────────────────────
constexpr const char* AUTOWARE_CONTAINER  = "autoware-test-ota";
constexpr const char* E2E_OTA_CONFIG_DIR  = "/tmp/autoware_ota_e2e_test";
constexpr const char* SET_MODE_TOPIC      = "mmf/lifecyclemgr/SetServiceModeRequest";

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

static DomainParticipant* CreateUdpOnlyParticipant(int domain_id, const char* name) {
    DomainParticipantQos qos;
    qos.name(name);
    qos.transport().use_builtin_transports = false;
    auto udp = std::make_shared<UDPv4TransportDescriptor>();
    qos.transport().user_transports.push_back(udp);
    return DomainParticipantFactory::get_instance()->create_participant(domain_id, qos);
}

// ── DDS SetServiceMode Request Publisher ─────────────────────────────────────
class SetModePublisher : public DDSMessageEndPoint {
public:
    SetModePublisher() : DDSMessageEndPoint(SET_MODE_TOPIC) {
        SetTypeSupport(std::make_shared<TypeSupport>(
            new lifecyclemgr::SetServiceModeRequestPubSubType()));
    }

    ~SetModePublisher() { Cleanup(); }

    bool Initialize() {
        participant_ = CreateUdpOnlyParticipant(0, "OtaTestModePublisher");
        if (!participant_) return false;
        publisher_ = participant_->create_publisher(PUBLISHER_QOS_DEFAULT);
        if (!publisher_) return false;
        return DDSMessageEndPoint::Initialize(participant_, publisher_, nullptr);
    }

    void Cleanup() {
        if (participant_) {
            participant_->delete_contained_entities();
            DomainParticipantFactory::get_instance()->delete_participant(participant_);
            participant_ = nullptr;
            publisher_ = nullptr;
            writer_ = nullptr;
            dds_topic_ = nullptr;
        }
    }

    bool SendOtaMode(const std::string& service_name) {
        lifecyclemgr::SetServiceModeRequest req;
        req.service_list().push_back(service_name);
        req.os_type() = lifecyclemgr::OsType::ROS2;
        req.service_mode() = lifecyclemgr::ServiceMode::OTA_MODE;
        return Write(&req);
    }

    bool SendNormalMode(const std::string& service_name) {
        lifecyclemgr::SetServiceModeRequest req;
        req.service_list().push_back(service_name);
        req.os_type() = lifecyclemgr::OsType::ROS2;
        req.service_mode() = lifecyclemgr::ServiceMode::NORMAL;
        return Write(&req);
    }

    void OnDataAvailable() override {}
    void OnDataPublished(bool) override {}
};

// ── Test Fixture ─────────────────────────────────────────────────────────────
// F008 L3 E2E: OTA mode transition test
//
// Phase 1: F003 chain — node running (ALWAYS policy).
// Phase 2: SetServiceModeRequest(OTA_MODE) → lifecycle_mgr updates service mode.
//          Crash container (docker stop) → lifecycle_mgr PolicyAlways skipped (OTA_MODE) →
//          node does NOT restart.
// Phase 3: SetServiceModeRequest(NORMAL) → lifecycle_mgr restores NORMAL mode →
//          next GetServiceStateResponse(NOT_RUNNING) → PolicyAlways → START → node restarts.

class L3AutowareOtaE2ETest : public ::testing::Test {
public:
    static pid_t lifecycle_pid_;
    static pid_t adsw_pid_;
    static bool node_started_ok_;
    static bool ota_mode_ok_;      // node stayed down in OTA_MODE after crash
    static bool normal_mode_ok_;   // node restarted after NORMAL mode restored

    static void SetUpTestSuite() {
        node_started_ok_ = false;
        ota_mode_ok_     = false;
        normal_mode_ok_  = false;

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

        // Write YAML with ALWAYS policy so initial F003 chain works
        mkdir(E2E_OTA_CONFIG_DIR, 0755);
        std::string yaml_path = std::string(E2E_OTA_CONFIG_DIR) + "/saved-service-list.yaml";
        {
            std::ofstream f(yaml_path);
            if (!f.is_open()) {
                ADD_FAILURE() << "Failed to create YAML: " << yaml_path;
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
        }

        // Detect network interface for CycloneDDS
        std::string iface = ExecShell("ip route show default 2>/dev/null | awk '{print $5; exit}'");
        while (!iface.empty() && (iface.back() == '\n' || iface.back() == '\r' || iface.back() == ' '))
            iface.pop_back();
        if (iface.empty()) iface = "enp128s31f6";

        const char* CYCLONE_XML = "/tmp/cyclonedds_ota_e2e.xml";
        {
            std::ofstream cf(CYCLONE_XML);
            cf << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
               << "<CycloneDDS><Domain><General><Interfaces>"
               << "<NetworkInterface name=\"" << iface << "\"/>"
               << "</Interfaces></General></Domain></CycloneDDS>\n";
        }

        // Start container
        ExecShell(std::string("docker stop ") + AUTOWARE_CONTAINER + " 2>/dev/null; docker rm " + AUTOWARE_CONTAINER + " 2>/dev/null");
        std::this_thread::sleep_for(std::chrono::seconds(1));
        ExecShell(std::string("docker run -d --name ") + AUTOWARE_CONTAINER
                  + " --network host"
                  + " -v " + CYCLONE_XML + ":/tmp/cyclonedds_ota_e2e.xml"
                  + " -e CYCLONEDDS_URI=file:///tmp/cyclonedds_ota_e2e.xml"
                  + " " + AutowareImage() + " sleep infinity");

        bool container_ok = WaitForCondition([]() {
            std::string out = ExecShell(
                std::string("docker ps --filter name=") + AUTOWARE_CONTAINER
                + " --filter status=running --format '{{.Names}}'");
            return out.find(AUTOWARE_CONTAINER) != std::string::npos;
        }, 30);

        if (!container_ok) {
            ADD_FAILURE() << "Container " << AUTOWARE_CONTAINER << " did not start within 30s";
            return;
        }

        // Build LD_LIBRARY_PATH
        std::string ld_path = MmfBuildLdPath();
        std::string lf_config = MmfLfConfig();

        // Fork lifecycle_mgr
        lifecycle_pid_ = fork();
        if (lifecycle_pid_ == 0) {
            setenv("MMF_CONFIG_PATH", lf_config.c_str(), 1);
            setenv("LD_LIBRARY_PATH", ld_path.c_str(), 1);
            setenv("FASTDDS_DEFAULT_PROFILES_FILE", fastdds_profile.c_str(), 1);
            execl(MmfLfBin().c_str(), "lifecycle_mgr", nullptr);
            _exit(127);
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // Fork adsw_manager
        adsw_pid_ = fork();
        if (adsw_pid_ == 0) {
            setenv("SAVED_REGISTRY_BACKUP", E2E_OTA_CONFIG_DIR, 1);
            setenv("LD_LIBRARY_PATH", ld_path.c_str(), 1);
            setenv("FASTDDS_DEFAULT_PROFILES_FILE", fastdds_profile.c_str(), 1);
            execl(MmfAdswBin().c_str(), "adsw_manager", nullptr);
            _exit(127);
        }

        // Phase 1: Wait for F003 chain — /logger_usage_demo appears
        node_started_ok_ = WaitForCondition([]() {
            std::string out = ExecShell(
                std::string("docker exec ") + AUTOWARE_CONTAINER + " bash -c "
                "'source /opt/ros/humble/setup.bash 2>/dev/null && ros2 node list 2>/dev/null'");
            return out.find("/logger_usage_demo") != std::string::npos;
        }, 60);

        if (!node_started_ok_) {
            ADD_FAILURE() << "Phase 1 failed: /logger_usage_demo did not appear within 60s";
            return;
        }

        // Phase 2: Switch to OTA_MODE, then crash container.
        auto mode_pub = std::make_shared<SetModePublisher>();
        if (!mode_pub->Initialize()) {
            ADD_FAILURE() << "Failed to initialize SetModePublisher";
            return;
        }
        std::this_thread::sleep_for(std::chrono::seconds(2));  // allow DDS discovery

        // Send SetServiceModeRequest(OTA_MODE) to lifecycle_mgr
        mode_pub->SendOtaMode("logging_demo_main");
        std::this_thread::sleep_for(std::chrono::seconds(1));  // allow lifecycle_mgr to process

        // Crash the container
        ExecShell(std::string("docker stop -t 2 ") + AUTOWARE_CONTAINER + " 2>/dev/null");

        // In OTA_MODE, lifecycle_mgr PolicyAlways skips restart (returns NONE).
        // Wait 30s and verify node does NOT come back.
        bool node_absent = WaitForCondition([]() {
            std::string out = ExecShell(
                std::string("docker exec ") + AUTOWARE_CONTAINER + " bash -c "
                "'source /opt/ros/humble/setup.bash 2>/dev/null && ros2 node list 2>/dev/null'"
                " 2>/dev/null");
            return out.find("/logger_usage_demo") != std::string::npos;
        }, 30);
        // node_absent_after_crash = !node_appeared_in_30s
        ota_mode_ok_ = !node_absent;

        // Phase 3: Restore NORMAL mode → lifecycle_mgr PolicyAlways resumes.
        mode_pub->SendNormalMode("logging_demo_main");
        std::this_thread::sleep_for(std::chrono::seconds(1));

        normal_mode_ok_ = WaitForCondition([]() {
            std::string out = ExecShell(
                std::string("docker exec ") + AUTOWARE_CONTAINER + " bash -c "
                "'source /opt/ros/humble/setup.bash 2>/dev/null && ros2 node list 2>/dev/null'"
                " 2>/dev/null");
            return out.find("/logger_usage_demo") != std::string::npos;
        }, 90);

        mode_pub->Cleanup();

        if (!normal_mode_ok_) {
            ADD_FAILURE() << "Phase 3 failed: /logger_usage_demo did not reappear within 90s "
                          << "after NORMAL mode was restored. "
                          << "Check lifecycle_mgr.log for StartServiceRequest after SetServiceModeRequest(NORMAL).";
        }
    }

    static void TearDownTestSuite() {
        KillAndWait(adsw_pid_);
        KillAndWait(lifecycle_pid_);
        ExecShell(std::string("docker stop ") + AUTOWARE_CONTAINER + " 2>/dev/null; docker rm " + AUTOWARE_CONTAINER + " 2>/dev/null");
        ExecShell(std::string("rm -rf ") + E2E_OTA_CONFIG_DIR);
        ExecShell("rm -f /tmp/cyclonedds_ota_e2e.xml");
        lifecycle_pid_ = -1;
        adsw_pid_      = -1;
    }
};

pid_t L3AutowareOtaE2ETest::lifecycle_pid_ = -1;
pid_t L3AutowareOtaE2ETest::adsw_pid_      = -1;
bool  L3AutowareOtaE2ETest::node_started_ok_ = false;
bool  L3AutowareOtaE2ETest::ota_mode_ok_     = false;
bool  L3AutowareOtaE2ETest::normal_mode_ok_  = false;

TEST_F(L3AutowareOtaE2ETest, TC001_Phase1_NodeRunning) {
    ASSERT_TRUE(node_started_ok_)
        << "F003 chain failed: /logger_usage_demo did not appear with ALWAYS policy.";
}

TEST_F(L3AutowareOtaE2ETest, TC002_OtaMode_NodeNotRestartedAfterCrash) {
    ASSERT_TRUE(node_started_ok_) << "Phase 1 prerequisite failed";
    EXPECT_TRUE(ota_mode_ok_)
        << "In OTA_MODE, the node restarted within 30s after crash. "
        << "Expected: PolicyAlways returns NONE when ServiceMode=OTA_MODE — "
        << "no StartServiceRequest should be sent while in OTA_MODE. "
        << "Check: Service::GetServiceMode() returned NORMAL instead of OTA_MODE, "
        << "or SetServiceModeRequest was not received by lifecycle_mgr.";
}

TEST_F(L3AutowareOtaE2ETest, TC003_NormalMode_NodeRestartsAfterModeRestore) {
    ASSERT_TRUE(node_started_ok_) << "Phase 1 prerequisite failed";
    EXPECT_TRUE(normal_mode_ok_)
        << "/logger_usage_demo did not reappear within 90s after NORMAL mode was restored. "
        << "Expected: SetServiceModeRequest(NORMAL) → lifecycle_mgr restores mode → "
        << "next GetServiceStateResponse(NOT_RUNNING) → PolicyAlways(NORMAL) → START → "
        << "adsw_manager docker start + ros2 run → node reappears.";
}

} // namespace

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
