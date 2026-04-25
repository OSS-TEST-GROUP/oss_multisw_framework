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
#include <mutex>
#include <atomic>

// DDS
#include "transmitter/DDSMessageEndPoint.hpp"
#include "lifecycle/StopServiceRequestPubSubTypes.hpp"
#include "lifecycle/StopServiceRequest.hpp"

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/rtps/transport/UDPv4TransportDescriptor.hpp>

using namespace eprosima::fastdds::dds;
using namespace eprosima::fastdds::rtps;
using namespace lifecycle_manager;

namespace {

// ── Constants ──────────────────────────────────────────────────────────────
constexpr const char* AUTOWARE_IMAGE =
    "ghcr.io/autowarefoundation/autoware:universe-dependencies-cuda-humble-20260423-amd64";
constexpr const char* AUTOWARE_CONTAINER = "autoware-test-stop";

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
constexpr const char* E2E_STOP_CONFIG_DIR = "/tmp/autoware_stop_e2e_test";
constexpr const char* FASTDDS_PROFILE =
    "/home/obigo/project/oss/oss_agent/mmf/oss_multisw_framework"
    "/docker/build_environment/fastdds-profile.xml";
constexpr const char* STOP_DDS_TOPIC = "mmf/lifecyclemgr/StopServiceRequest";

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

// ── DDS Helpers ─────────────────────────────────────────────────────────────
// UDP-only transport avoids SHM futex deadlocks with stale segments from prior runs.
static DomainParticipant* CreateUdpOnlyParticipant(int domain_id, const char* name) {
    DomainParticipantQos qos;
    qos.name(name);
    qos.transport().use_builtin_transports = false;
    auto udp = std::make_shared<UDPv4TransportDescriptor>();
    qos.transport().user_transports.push_back(udp);
    return DomainParticipantFactory::get_instance()->create_participant(domain_id, qos);
}

// ── DDS Stop Request Publisher ───────────────────────────────────────────────
// Publishes StopServiceRequest on mmf/lifecyclemgr/StopServiceRequest.
// lifecycle_mgr is killed before this publisher is created (after Phase 1 completes),
// so this publisher safely takes over the stop command role in Phase 2.
//
// Uses base-class protected members directly — do NOT add private participant_/publisher_
// here; that would shadow the base members and cause ~DDSMessageEndPoint() to access
// freed pointers (segfault).
class StopRequestPublisher : public DDSMessageEndPoint {
public:
    StopRequestPublisher() : DDSMessageEndPoint(STOP_DDS_TOPIC) {
        SetTypeSupport(std::make_shared<TypeSupport>(
            new lifecyclemgr::StopServiceRequestPubSubType()));
    }

    ~StopRequestPublisher() { Cleanup(); }

    bool Initialize() {
        participant_ = CreateUdpOnlyParticipant(0, "StopTestPublisher");
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

    bool Send(const std::string& service_name) {
        lifecyclemgr::StopServiceRequest req;
        req.service_list().push_back(service_name);
        req.os_type() = lifecyclemgr::OsType::ROS2;
        req.is_group() = false;
        return Write(&req);
    }

    void OnDataAvailable() override {}
    void OnDataPublished(bool) override {}
};

// ── Test Fixture ─────────────────────────────────────────────────────────────
class L3AutowareStopE2ETest : public ::testing::Test {
public:
    static pid_t lifecycle_pid_;
    static pid_t adsw_pid_;
    static bool node_started_ok_;
    static bool node_stopped_ok_;

    static void SetUpTestSuite() {
        // Set FastDDS profile before any DDS usage in this process
        setenv("FASTDDS_DEFAULT_PROFILES_FILE", FASTDDS_PROFILE, 1);

        node_started_ok_ = false;
        node_stopped_ok_ = false;

        // Kill stale processes from prior runs to avoid DDS domain interference.
        // Use basename only — execl(path, "argv0") sets cmdline to argv0, not the full path,
        // so pkill -f with a full path would not match running processes.
        ExecShell("pkill -9 adsw_manager 2>/dev/null");
        ExecShell("pkill -9 lifecycle_mgr 2>/dev/null");
        std::this_thread::sleep_for(std::chrono::seconds(2)); // allow stale DDS participants to deregister

        // 1. Write config: logging_demo_main in autoware-test-stop container
        mkdir(E2E_STOP_CONFIG_DIR, 0755);
        std::string yaml_path = std::string(E2E_STOP_CONFIG_DIR) + "/saved-service-list.yaml";
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

        // 2. Detect network interface and write CycloneDDS config for container
        std::string iface = ExecShell("ip route show default 2>/dev/null | awk '{print $5; exit}'");
        while (!iface.empty() && (iface.back() == '\n' || iface.back() == '\r' || iface.back() == ' '))
            iface.pop_back();
        if (iface.empty()) iface = "enp128s31f6";

        const char* CYCLONE_XML_PATH = "/tmp/cyclonedds_stop_e2e.xml";
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
                  + " -v " + CYCLONE_XML_PATH + ":/tmp/cyclonedds_stop_e2e.xml"
                  + " -e CYCLONEDDS_URI=file:///tmp/cyclonedds_stop_e2e.xml"
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
            setenv("SAVED_REGISTRY_BACKUP", E2E_STOP_CONFIG_DIR, 1);
            setenv("LD_LIBRARY_PATH", ld_path.c_str(), 1);
            setenv("FASTDDS_DEFAULT_PROFILES_FILE", FASTDDS_PROFILE, 1);
            execl(ADSW_MANAGER_BIN, "adsw_manager", nullptr);
            _exit(127);
        }

        // 7. Phase 1: Wait for F003 chain to complete — adsw starts, UpdateServiceList
        //    handshake, lifecycle_mgr sends StartServiceRequest, adsw docker execs ros2 run,
        //    /logger_usage_demo appears in ros2 node list. Allow up to 60s.
        node_started_ok_ = WaitForCondition([&]() {
            std::string out = ExecShell(
                std::string("docker exec ") + AUTOWARE_CONTAINER + " bash -c "
                "'source /opt/ros/humble/setup.bash 2>/dev/null && ros2 node list 2>/dev/null'");
            return out.find("/logger_usage_demo") != std::string::npos;
        }, 60);

        if (!node_started_ok_) {
            ADD_FAILURE() << "Phase 1 failed: /logger_usage_demo did not appear within 60s. "
                          << "Check adsw_manager StartService command construction.";
            return;
        }

        // 8. Kill lifecycle_mgr to prevent it from re-issuing StartServiceRequest
        //    due to ALWAYS recovery policy after Phase 2 stops the node.
        KillAndWait(lifecycle_pid_);
        lifecycle_pid_ = -1;

        // 9. Phase 2: test binary acts as the stop initiator.
        //    Create StopRequestPublisher and allow DDS discovery with adsw_manager.
        auto stop_pub = std::make_shared<StopRequestPublisher>();
        if (!stop_pub->Initialize()) {
            ADD_FAILURE() << "Failed to initialize DDS publisher for StopServiceRequest";
            return;
        }
        std::this_thread::sleep_for(std::chrono::seconds(2)); // DDS discovery

        stop_pub->Send("logging_demo_main");
        std::this_thread::sleep_for(std::chrono::seconds(1)); // Allow delivery and processing

        // Wait for node to disappear (AC7: 5s timeout)
        node_stopped_ok_ = WaitForCondition([&]() {
            std::string out = ExecShell(
                std::string("docker exec ") + AUTOWARE_CONTAINER + " bash -c "
                "'source /opt/ros/humble/setup.bash 2>/dev/null && ros2 node list 2>/dev/null'");
            return out.find("/logger_usage_demo") == std::string::npos
                && out.find("/logger_config") == std::string::npos;
        }, 5);

        stop_pub->Cleanup();
    }

    static void TearDownTestSuite() {
        KillAndWait(adsw_pid_);
        KillAndWait(lifecycle_pid_);
        ExecShell(std::string("docker rm -f ") + AUTOWARE_CONTAINER + " 2>/dev/null");
        ExecShell(std::string("rm -rf ") + E2E_STOP_CONFIG_DIR);
        ExecShell("rm -f /tmp/cyclonedds_stop_e2e.xml");
        adsw_pid_ = -1;
        lifecycle_pid_ = -1;
    }
};

pid_t L3AutowareStopE2ETest::lifecycle_pid_ = -1;
pid_t L3AutowareStopE2ETest::adsw_pid_ = -1;
bool L3AutowareStopE2ETest::node_started_ok_ = false;
bool L3AutowareStopE2ETest::node_stopped_ok_ = false;

// AC1: node was running before stop request (lifecycle_mgr → StartServiceRequest chain)
TEST_F(L3AutowareStopE2ETest, TC001_NodeWasRunningBeforeStop) {
    ASSERT_TRUE(node_started_ok_)
        << "Phase 1 failed: /logger_usage_demo was not running after lifecycle_mgr+adsw_manager "
        << "F003 chain (UpdateServiceList handshake → StartServiceRequest → docker exec).";
}

// AC2+AC3: StopServiceRequest received; adsw_manager stopped the process
TEST_F(L3AutowareStopE2ETest, TC002_NodeStoppedAfterStopRequest) {
    ASSERT_TRUE(node_started_ok_) << "Phase 1 prerequisite failed";
    ASSERT_TRUE(node_stopped_ok_)
        << "StopServiceRequest was published but /logger_usage_demo did not disappear within 5s. "
        << "Check ROSServiceController::StopService command construction.";
}

// AC4+AC5: node is gone from ros2 node list
TEST_F(L3AutowareStopE2ETest, TC003_NodeGoneFromRos2NodeList) {
    ASSERT_TRUE(node_stopped_ok_) << "Node was not stopped";
    std::string out = ExecShell(
        std::string("docker exec ") + AUTOWARE_CONTAINER + " bash -c "
        "'source /opt/ros/humble/setup.bash 2>/dev/null && ros2 node list 2>/dev/null'");
    EXPECT_EQ(out.find("/logger_usage_demo"), std::string::npos)
        << "/logger_usage_demo should be gone from ros2 node list, got: " << out;
    EXPECT_EQ(out.find("/logger_config"), std::string::npos)
        << "/logger_config should be gone from ros2 node list, got: " << out;
}

// AC6+AC7: full stop sequence completed within timeout
TEST_F(L3AutowareStopE2ETest, TC004_StopSequenceCompletedWithinTimeout) {
    ASSERT_TRUE(node_started_ok_) << "Phase 1 prerequisite failed";
    ASSERT_TRUE(node_stopped_ok_)
        << "Stop sequence did not complete within 5s timeout";
}

} // namespace

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
