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
// Route stop through lifecycle_mgr ctrl channel (trusted path).
// Sending StopServiceRequest directly to adsw_manager is blocked by its
// phantom-publisher guard (only lifecycle_mgr is trusted). Instead we publish
// CtrlStopServiceRequest to lifecycle_mgr's ctrl/StopService subscriber, which
// then forwards a trusted StopServiceRequest to adsw_manager.
#include "transmitter/DDSMessageEndPoint.hpp"
#include "lifecycle/LifecycleCtrlRequestPubSubTypes.hpp"
#include "lifecycle/LifecycleCtrlRequest.hpp"

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/rtps/transport/UDPv4TransportDescriptor.hpp>

using namespace eprosima::fastdds::dds;
using namespace eprosima::fastdds::rtps;
using namespace lifecycle_manager;

#include "autoware_e2e_common.h"

namespace {

// ── Constants ──────────────────────────────────────────────────────────────
constexpr const char* AUTOWARE_CONTAINER   = "autoware-test-stop";
constexpr const char* E2E_STOP_CONFIG_DIR  = "/tmp/autoware_stop_e2e_test";
// ctrl/StopService is subscribed by lifecycle_mgr; it relays a trusted
// StopServiceRequest to adsw_manager, bypassing the phantom-publisher guard.
constexpr const char* STOP_DDS_TOPIC       = "mmf/lifecyclemgr/ctrl/StopService";

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
static DomainParticipant* CreateUdpOnlyParticipant(int domain_id, const char* name) {
    DomainParticipantQos qos;
    qos.name(name);
    qos.transport().use_builtin_transports = false;
    auto udp = std::make_shared<UDPv4TransportDescriptor>();
    qos.transport().user_transports.push_back(udp);
    return DomainParticipantFactory::get_instance()->create_participant(domain_id, qos);
}

// ── DDS Ctrl Stop Request Publisher ──────────────────────────────────────────
// Publishes CtrlStopServiceRequest to lifecycle_mgr's ctrl/StopService topic.
// lifecycle_mgr subscribes there, applies its own publishing-participant guard,
// then forwards a trusted StopServiceRequest to adsw_manager. This avoids the
// phantom-publisher guard that blocks direct StopServiceRequest from the test.
//
// domain_id is E2EDomainId() — the per-run isolated domain so the publisher
// reaches the test's lifecycle_mgr (on domain N) rather than the production
// stack (on domain 0).
class StopRequestPublisher : public DDSMessageEndPoint {
public:
    explicit StopRequestPublisher(int domain_id)
        : DDSMessageEndPoint(STOP_DDS_TOPIC), domain_id_(domain_id) {
        // Must match the PubSubType lifecycle_mgr registers on ctrl/StopService.
        // "lifecyclemgr::CtrlStopServiceRequest" — type name must match exactly.
        SetTypeSupport(std::make_shared<TypeSupport>(
            new lifecyclemgr::CtrlStopServiceRequestPubSubType()));
    }

    ~StopRequestPublisher() { Cleanup(); }

    bool Initialize() {
        participant_ = CreateUdpOnlyParticipant(domain_id_, "StopTestPublisher");
        if (!participant_) return false;
        publisher_ = participant_->create_publisher(PUBLISHER_QOS_DEFAULT);
        if (!publisher_) return false;
        return DDSMessageEndPoint::Initialize(participant_, publisher_, nullptr);
    }

private:
    int domain_id_;

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
        lifecyclemgr::CtrlStopServiceRequest req;
        req.service_list().push_back(service_name);
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
        node_started_ok_ = false;
        node_stopped_ok_ = false;

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
        ExecShell(std::string("docker stop ") + AUTOWARE_CONTAINER + " 2>/dev/null; docker rm " + AUTOWARE_CONTAINER + " 2>/dev/null");
        std::this_thread::sleep_for(std::chrono::seconds(1));
        ExecShell(std::string("docker run -d --name ") + AUTOWARE_CONTAINER
                  + " --network host"
                  + " -v " + CYCLONE_XML_PATH + ":/tmp/cyclonedds_stop_e2e.xml"
                  + " -e CYCLONEDDS_URI=file:///tmp/cyclonedds_stop_e2e.xml"
                  + " -e ROS_DOMAIN_ID=" + domain_str
                  + " " + AutowareImage() + " sleep infinity");

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
        std::string ld_path = MmfBuildLdPath();
        std::string lf_config = MmfLfConfig();

        // 5. Fork lifecycle_mgr
        lifecycle_pid_ = fork();
        if (lifecycle_pid_ == 0) {
            setenv("MMF_CONFIG_PATH", lf_config.c_str(), 1);
            setenv("LD_LIBRARY_PATH", ld_path.c_str(), 1);
            setenv("FASTDDS_DEFAULT_PROFILES_FILE", fastdds_profile.c_str(), 1);
            setenv("ROS_DOMAIN_ID", domain_str.c_str(), 1);
            execl(MmfLfBin().c_str(), "lifecycle_mgr", nullptr);
            _exit(127);
        }

        // 6. Fork adsw_manager
        adsw_pid_ = fork();
        if (adsw_pid_ == 0) {
            setenv("SAVED_REGISTRY_BACKUP", E2E_STOP_CONFIG_DIR, 1);
            setenv("LD_LIBRARY_PATH", ld_path.c_str(), 1);
            setenv("FASTDDS_DEFAULT_PROFILES_FILE", fastdds_profile.c_str(), 1);
            setenv("ROS_DOMAIN_ID", domain_str.c_str(), 1);
            execl(MmfAdswBin().c_str(), "adsw_manager", nullptr);
            _exit(127);
        }

        // 7. Phase 1: Wait for F003 chain to complete
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

        // 8. Phase 2: test binary sends CtrlStopServiceRequest to lifecycle_mgr's
        //    ctrl/StopService topic. lifecycle_mgr relays it as a trusted
        //    StopServiceRequest to adsw_manager (bypassing the phantom guard).
        //    We intentionally keep lifecycle_mgr running so it can relay.
        //    Note: ALWAYS recovery policy means lifecycle_mgr will issue a new
        //    StartServiceRequest ~5s after the stop. TC001-TC004 are checking
        //    that the stop was processed; we verify node disappearance within 5s,
        //    which is before recovery fires.
        //
        //    The publisher uses the per-run domain so it reaches our isolated
        //    lifecycle_mgr (on domain N) rather than the production stack.
        auto stop_pub = std::make_shared<StopRequestPublisher>(domain);
        if (!stop_pub->Initialize()) {
            ADD_FAILURE() << "Failed to initialize DDS publisher for CtrlStopServiceRequest";
            return;
        }
        std::this_thread::sleep_for(std::chrono::seconds(3)); // DDS discovery

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
        ExecShell(std::string("docker stop ") + AUTOWARE_CONTAINER + " 2>/dev/null; docker rm " + AUTOWARE_CONTAINER + " 2>/dev/null");
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

// AC1: node was running before stop request
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
