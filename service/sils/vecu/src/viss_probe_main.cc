// SiLS vECU Phase 1 Vertical PoC -- VISS /VIN/Vehicle probe.
//
// Publishes VissRequestTopic samples on the real MMF endpoint documented in
// config/sils/contracts/mmf-endpoints.yaml (contract id "viss-request"),
// following the same DDS setup pattern as the SUT itself
// (service/core/vehicle_info_mgr/src/transport/dds_transport.cpp) and its
// interactive test client (service/core/vehicle_info_mgr/tests/dds_client_test.cpp):
// domain id from VISS_DDS_DOMAIN_ID (not MMF_DDS_DOMAIN_ID -- architecture v3 Final
// §3.4/§18-B), plain TOPIC_QOS_DEFAULT + RELIABLE reliability.
//
// Scope is intentionally narrow per architecture v3 Final §17 Phase 1 Scope Gate:
// WallClock only, one endpoint, no Relay/lockstep/replay/fault/FMI. Behavior mirrors
// service/sils/vecu/descriptors/viss_probe.yaml (values are inlined here rather than
// YAML-loaded -- a generic Descriptor loader is explicitly out of Phase 1 scope).

#include <algorithm>
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/qos/DataWriterQos.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

#include "VissRequestPubSubTypes.hpp"
#include "clock_source.hpp"

namespace {

// contract id: viss-request (config/sils/contracts/mmf-endpoints.yaml)
const std::string kTopic("/VIN/Vehicle");

std::atomic<bool> g_stop{false};
std::atomic<int> g_matched_count{0};

// vECU identity (Descriptor's instance_id, e.g. viss_probe.yaml's "viss-probe-1")
// -- distinct from run_id (which identifies one Coordinator run, not one vECU).
// Only used for the participant name and sidecar trace metadata, never the DDS
// payload itself: the wire contract is whatever the real VISS server parses,
// and inventing extra payload fields would break that (architecture v3 Final
// §7.3 -- identity goes in sidecar trace, not payload).
std::string g_instance_id = "unknown";

void OnSignal(int /*signum*/) {
    g_stop.store(true);
}

long long WallTimeMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

std::string GetEnvOr(const char* name, const std::string& fallback) {
    const char* v = std::getenv(name);
    return v ? std::string(v) : fallback;
}

int GetEnvIntOr(const char* name, int fallback) {
    const char* v = std::getenv(name);
    if (!v) return fallback;
    try {
        return std::stoi(v);
    } catch (const std::exception&) {
        std::cerr << "Invalid integer for " << name << "=\"" << v << "\", using default "
                  << fallback << std::endl;
        return fallback;
    }
}

// Trace line per architecture v3 Final §7.1/§12.2 (publish_intent/publish_result,
// run_id, wall time) -- deliberately minimal JSONL, no tick sequence since Phase 1
// is WallClock only (no TickGrant/TickComplete).
void TraceEvent(const std::string& run_id, const std::string& event, int seq,
                const std::string& extra_json_fields = "") {
    std::cout << "{\"run_id\":\"" << run_id << "\",\"instance_id\":\"" << g_instance_id
              << "\",\"event\":\"" << event << "\",\"seq\":" << seq
              << ",\"wall_time_ms\":" << WallTimeMs() << ",\"topic\":\"" << kTopic << "\""
              << (extra_json_fields.empty() ? "" : ("," + extra_json_fields)) << "}"
              << std::endl;
}

std::string EscapeJsonString(const std::string& in) {
    std::string out;
    out.reserve(in.size());
    for (char c : in) {
        if (c == '"' || c == '\\') out += '\\';
        out += c;
    }
    return out;
}

// Builds the envelope the real VISS server's RequestListener expects:
// {"topic": <response topic>, "request": "<escaped inner JSON>"}.
// A bare inner JSON without this envelope is rejected by the SUT as
// "not a valid JSON or has missing fields" -- see dds_transport.cpp.
std::string BuildRequestPayload(int seq, const std::string& request_action,
                                 const std::string& request_path,
                                 const std::string& response_topic_prefix) {
    std::ostringstream response_topic;
    response_topic << response_topic_prefix << "/" << seq;

    std::ostringstream inner;
    inner << R"({"action":")" << request_action << R"(","path":")" << request_path
          << R"(","requestId":"sils-)" << seq << R"("})";

    std::ostringstream outer;
    outer << R"({"topic":")" << response_topic.str() << R"(","request":")"
          << EscapeJsonString(inner.str()) << R"("})";
    return outer.str();
}

class ProbeWriterListener : public eprosima::fastdds::dds::DataWriterListener {
public:
    explicit ProbeWriterListener(const std::string& run_id) : run_id_(run_id) {}

    void on_publication_matched(
        eprosima::fastdds::dds::DataWriter* /*writer*/,
        const eprosima::fastdds::dds::PublicationMatchedStatus& info) override {
        g_matched_count.store(info.current_count);
        TraceEvent(run_id_, info.current_count_change > 0 ? "endpoint_matched" : "endpoint_unmatched",
                   0, "\"matched_count\":" + std::to_string(info.current_count));
    }

private:
    std::string run_id_;
};

}  // namespace

int main() {
    std::signal(SIGINT, OnSignal);
    std::signal(SIGTERM, OnSignal);

    const std::string run_id = GetEnvOr("SILS_RUN_ID", "local");
    g_instance_id = GetEnvOr("SILS_VECU_INSTANCE_ID", "viss-probe-1");
    const int period_ms = GetEnvIntOr("SILS_VISS_PROBE_PERIOD_MS", 500);
    const std::string response_topic_prefix =
        GetEnvOr("SILS_VISS_PROBE_RESPONSE_TOPIC_PREFIX", "sils/viss_probe/response");
    const std::string request_action = GetEnvOr("SILS_VISS_PROBE_REQUEST_ACTION", "get");
    const std::string request_path = GetEnvOr("SILS_VISS_PROBE_REQUEST_PATH", "Vehicle.Speed");

    int domain_id = 0;
    const std::string domain_env = GetEnvOr("VISS_DDS_DOMAIN_ID", "");
    if (!domain_env.empty()) {
        try {
            domain_id = std::stoi(domain_env);
        } catch (const std::exception& e) {
            std::cerr << "Invalid VISS_DDS_DOMAIN_ID=\"" << domain_env
                      << "\": " << e.what() << ". Using default domain 0." << std::endl;
        }
    }

    using namespace eprosima::fastdds::dds;

    TraceEvent(run_id, "run_start", 0, "\"domain_id\":" + std::to_string(domain_id));

    DomainParticipantQos pqos;
    // Suffixed with instance_id so multiple vECU instances (e.g. a future
    // wheelspeed_ecu ×4 fleet, architecture v3 Final §11) show up as distinct,
    // greppable participants instead of colliding under one fixed name
    // (§9.5's "동일 domain 내 participant/instance 이름 중복 금지" check).
    pqos.name("sils_vecu_viss_probe:" + g_instance_id);
    DomainParticipant* participant =
        DomainParticipantFactory::get_instance()->create_participant(
            domain_id, pqos, nullptr, StatusMask::none());
    if (participant == nullptr) {
        std::cerr << "Failed to create DDS participant on domain " << domain_id << std::endl;
        return 1;
    }

    TypeSupport request_type(new VissRequestTopicPubSubType());
    request_type.register_type(participant);

    Publisher* publisher = participant->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr);
    Topic* topic = participant->create_topic(kTopic, request_type.get_type_name(), TOPIC_QOS_DEFAULT);

    DataWriterQos writer_qos = DATAWRITER_QOS_DEFAULT;
    // Matches config/sils/qos/qos-profiles.yaml "viss-default" and the SUT's own
    // reader QoS (dds_transport.cpp DataReaderQos.reliability = RELIABLE) -- a
    // BEST_EFFORT writer would fail RxO matching against a RELIABLE reader.
    writer_qos.reliability().kind = RELIABLE_RELIABILITY_QOS;

    ProbeWriterListener listener(run_id);
    DataWriter* writer = publisher->create_datawriter(topic, writer_qos, &listener);
    if (writer == nullptr) {
        std::cerr << "Failed to create DataWriter for topic " << kTopic << std::endl;
        return 1;
    }

    TraceEvent(run_id, "ready", 0);

    // Clock port (§8): WallClock (default) or LockstepClock, selected by
    // SILS_CLOCK_MODE. The tick loop below is identical for both -- the mode
    // lives entirely behind IClockSource (WallClock preserves the Phase 1
    // free-running cadence; Lockstep blocks on a ClockMaster TickGrant and
    // replies TickComplete via clock->TickDone).
    const std::string clock_mode = GetEnvOr("SILS_CLOCK_MODE", "wall");
    std::unique_ptr<sils::IClockSource> clock = sils::MakeClockSource(
        clock_mode, period_ms, &g_stop, participant, run_id, g_instance_id);
    TraceEvent(run_id, "clock_start", 0, "\"mode\":\"" + std::string(clock->mode()) + "\"");

    // Publish one VISS request sample for a tick (identical wire behavior in both
    // clock modes -- only WHEN it fires differs).
    auto publish_one = [&](uint64_t seq) {
        VissRequestTopic sample;
        sample.payload(BuildRequestPayload(static_cast<int>(seq), request_action,
                                           request_path, response_topic_prefix));
        TraceEvent(run_id, "publish_intent", static_cast<int>(seq),
                   "\"matched_count\":" + std::to_string(g_matched_count.load()));
        writer->write(&sample);
        TraceEvent(run_id, "publish_result", static_cast<int>(seq));
    };

    sils::TickInfo tick;
    uint64_t last_seq = 0;
    while (clock->WaitForNextTick(tick)) {
        publish_one(tick.seq);
        clock->TickDone(tick);  // lockstep: sends TickComplete; wall: no-op
        last_seq = tick.seq;
    }

    TraceEvent(run_id, "run_stopping", static_cast<int>(last_seq));

    // In lockstep the clock source created its own tick-control reader/writer on
    // this participant, so tear ALL contained entities down in one shot (the
    // probe's own writer/publisher/topic + the clock's endpoints) before deleting
    // the participant -- delete_participant fails with a precondition error if any
    // contained entity survives. delete_contained_entities first also stops the
    // grant reader's listener before we free the clock object.
    (void)writer; (void)publisher; (void)topic;  // owned by participant; freed below
    participant->delete_contained_entities();
    clock.reset();
    DomainParticipantFactory::get_instance()->delete_participant(participant);

    TraceEvent(run_id, "run_stopped", static_cast<int>(last_seq));
    return 0;
}
