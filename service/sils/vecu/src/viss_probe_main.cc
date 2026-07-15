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
#include <iomanip>
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
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

#include "VissRequestPubSubTypes.hpp"
#include "behavior.hpp"
#include "clock_source.hpp"
#include "json_min.hpp"
#include "plant.hpp"
#include "replay_source.hpp"
#include "tick_inbox.hpp"

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

// Wraps a behavior's inner request JSON in the envelope the real VISS server's
// RequestListener expects: {"topic": <response topic>, "request": "<escaped inner>"}.
// A bare inner JSON without this envelope is rejected by the SUT as "not a valid
// JSON or has missing fields" -- see dds_transport.cpp. The inner request now comes
// from IBehavior (behavior.hpp); this only owns the wire envelope.
std::string RequestId(uint64_t seq) { return "sils-" + std::to_string(seq); }

std::string WrapRequestEnvelope(const std::string& inner_request, uint64_t seq,
                                const std::string& response_topic_prefix) {
    std::ostringstream response_topic;
    response_topic << response_topic_prefix << "/" << seq;
    std::ostringstream outer;
    // requestId at the envelope top level (real quotes) is the L1 correlation key --
    // JsonFindString on the responder/probe finds it, not the escaped inner one.
    outer << R"({"requestId":")" << RequestId(seq) << R"(","topic":")" << response_topic.str()
          << R"(","request":")" << EscapeJsonString(inner_request) << R"("})";
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

// L1 (§11): receives correlated responses on the response topic and traces
// response_received{request_id, status, error_code}. evaluate.py correlates these
// against request_sent by request_id -- this side just reports each arrival.
class ResponseListener : public eprosima::fastdds::dds::DataReaderListener {
public:
    explicit ResponseListener(std::string run_id) : run_id_(std::move(run_id)) {}

    void on_subscription_matched(
        eprosima::fastdds::dds::DataReader* /*reader*/,
        const eprosima::fastdds::dds::SubscriptionMatchedStatus& info) override {
        matched_.store(info.current_count);
        if (info.current_count_change > 0) {
            TraceEvent(run_id_, "endpoint_matched", 0,
                       "\"role\":\"probe-response-reader\",\"matched_count\":" +
                           std::to_string(info.current_count));
        }
    }

    int matched() const { return matched_.load(); }

    void on_data_available(eprosima::fastdds::dds::DataReader* reader) override {
        VissRequestTopic sample;
        eprosima::fastdds::dds::SampleInfo info;
        while (reader->take_next_sample(&sample, &info) == eprosima::fastdds::dds::RETCODE_OK) {
            if (!info.valid_data) continue;
            const std::string& p = sample.payload();
            const std::string rid = sils::JsonFindString(p, "requestId", "");
            const std::string status = sils::JsonFindString(p, "status", "");
            const std::string code = sils::JsonFindString(p, "errorCode", "");
            std::ostringstream extra;
            extra << "\"request_id\":\"" << rid << "\",\"status\":\"" << status << "\"";
            if (!code.empty()) extra << ",\"error_code\":\"" << code << "\"";
            TraceEvent(run_id_, "response_received", 0, extra.str());
        }
    }

private:
    std::string run_id_;
    std::atomic<int> matched_{0};
};

// Stages SUT input arrivals into a TickInbox (§7.3). Runs on the reader thread;
// assigns each arrival a reception sequence and traces input_received. The tick
// loop later freezes the pending set per tick (tick_input_frozen).
class InputStagingListener : public eprosima::fastdds::dds::DataReaderListener {
public:
    InputStagingListener(sils::TickInbox* inbox, std::string run_id, std::string topic)
        : inbox_(inbox), run_id_(std::move(run_id)), topic_(std::move(topic)) {}

    void on_data_available(eprosima::fastdds::dds::DataReader* reader) override {
        VissRequestTopic sample;
        eprosima::fastdds::dds::SampleInfo info;
        while (reader->take_next_sample(&sample, &info) == eprosima::fastdds::dds::RETCODE_OK) {
            if (!info.valid_data) continue;
            uint64_t rs = inbox_->OnSample();
            std::cout << "{\"run_id\":\"" << run_id_ << "\",\"instance_id\":\"" << g_instance_id
                      << "\",\"event\":\"input_received\",\"recv_seq\":" << rs
                      << ",\"topic\":\"" << topic_ << "\"}" << std::endl;
        }
    }

private:
    sils::TickInbox* inbox_;
    std::string run_id_;
    std::string topic_;
};

std::string RecvSeqsJson(const std::vector<uint64_t>& seqs) {
    std::ostringstream os;
    os << "[";
    for (size_t i = 0; i < seqs.size(); ++i) {
        if (i) os << ",";
        os << seqs[i];
    }
    os << "]";
    return os.str();
}

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
    // Matches config/sils/qos/qos-profiles.yaml "viss-default"/"viss-sils-verifiable"
    // and the SUT's own reader QoS (dds_transport.cpp DataReaderQos.reliability =
    // RELIABLE) -- a BEST_EFFORT writer would fail RxO matching against a RELIABLE reader.
    writer_qos.reliability().kind = RELIABLE_RELIABILITY_QOS;
    // Late-joiner robustness (SiLS run verifiability): a lockstep/plant run emits its
    // bounded tick set in a burst, then idles. An independently-started verifier may
    // only match AFTER the last sample was written -- with VOLATILE it then receives
    // nothing (matched-but-no-data). Offer TRANSIENT_LOCAL + KEEP_LAST history so a
    // verifier joining while the probe is still alive gets the run's samples. A
    // TRANSIENT_LOCAL offer is a superset (RxO-compatible with VOLATILE readers too).
    // Opt out with SILS_DURABILITY=volatile to exercise pure volatile wire behavior;
    // the verifier must use the SAME setting (durability RxO is strict).
    if (GetEnvOr("SILS_DURABILITY", "transient-local") != "volatile") {
        writer_qos.durability().kind = TRANSIENT_LOCAL_DURABILITY_QOS;
        writer_qos.history().kind = KEEP_LAST_HISTORY_QOS;
        writer_qos.history().depth = 256;
    }

    ProbeWriterListener listener(run_id);
    DataWriter* writer = publisher->create_datawriter(topic, writer_qos, &listener);
    if (writer == nullptr) {
        std::cerr << "Failed to create DataWriter for topic " << kTopic << std::endl;
        return 1;
    }
    // Report the negotiated request-writer QoS for L1 protocol conformance (§11):
    // analyze_protocol checks reliability == contracted RELIABLE.
    TraceEvent(run_id, "qos_negotiated", 0,
               "\"role\":\"probe-writer\",\"reliability\":\"RELIABLE\",\"durability\":\"" +
                   std::string(GetEnvOr("SILS_DURABILITY", "transient-local") != "volatile"
                                   ? "TRANSIENT_LOCAL" : "VOLATILE") + "\"");

    // L1 protocol mode (§11): when SILS_PROTOCOL=reqresp, subscribe to the response
    // topic so a responder's correlated replies are received (request/response
    // round-trip). Default "oneway" preserves the Phase 1 fire-and-forget probe.
    const bool reqresp = GetEnvOr("SILS_PROTOCOL", "oneway") == "reqresp";
    const std::string response_topic = GetEnvOr("SILS_RESPONSE_TOPIC", "sils/viss/response");
    ResponseListener response_listener(run_id);
    Topic* resp_topic_obj = nullptr;
    Subscriber* resp_sub = nullptr;
    if (reqresp) {
        resp_sub = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr);
        resp_topic_obj = participant->create_topic(response_topic, request_type.get_type_name(),
                                                   TOPIC_QOS_DEFAULT);
        DataReaderQos rrq = DATAREADER_QOS_DEFAULT;
        rrq.reliability().kind = RELIABLE_RELIABILITY_QOS;
        if (GetEnvOr("SILS_DURABILITY", "transient-local") != "volatile") {
            rrq.durability().kind = TRANSIENT_LOCAL_DURABILITY_QOS;
            rrq.history().kind = KEEP_LAST_HISTORY_QOS;
            rrq.history().depth = 256;
        }
        resp_sub->create_datareader(resp_topic_obj, rrq, &response_listener);
        TraceEvent(run_id, "protocol_on", 0, "\"mode\":\"reqresp\",\"response_topic\":\"" +
                                                 response_topic + "\"");

        // First-response race (§11 L1): the response reader may match the responder's
        // writer with an EMPTY history (before any reply), so TRANSIENT_LOCAL backfill
        // delivers nothing; then the very first live reply races the reliable
        // handshake and can be dropped (the first sample warms the channel). Wait for
        // the match, then settle so idle heartbeats establish the reliable channel
        // BEFORE the first request flows -- so no response (incl. the first) is missed.
        const int match_timeout_ms = GetEnvIntOr("SILS_PROTOCOL_MATCH_TIMEOUT_MS", 3000);
        const int settle_ms = GetEnvIntOr("SILS_PROTOCOL_SETTLE_MS", 300);
        int waited = 0;
        while (response_listener.matched() < 1 && waited < match_timeout_ms && !g_stop.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            waited += 20;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(settle_ms));
        TraceEvent(run_id, "protocol_ready", 0,
                   "\"response_matched\":" + std::to_string(response_listener.matched()) +
                       ",\"waited_ms\":" + std::to_string(waited + settle_ms));
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

    // Behavior port (§11 L2): what the vECU emits each tick. Default "viss-request"
    // reproduces the Phase 1 wire payload; "accumulate" is a stateful example that
    // consumes staged input. Selected by SILS_BEHAVIOR.
    const double ctrl_kp = std::stod(GetEnvOr("SILS_CTRL_KP", "0.8"));
    const double ctrl_setpoint = std::stod(GetEnvOr("SILS_CTRL_SETPOINT", "10.0"));
    std::unique_ptr<sils::IBehavior> behavior =
        sils::MakeBehavior(GetEnvOr("SILS_BEHAVIOR", "viss-request"), request_action, request_path,
                           ctrl_kp, ctrl_setpoint);
    TraceEvent(run_id, "behavior_start", 0, "\"name\":\"" + std::string(behavior->name()) + "\"");

    // Plant port (§9 L3): the physical model the controller closes the loop around.
    // SILS_PLANT selects "first-order" (live, drag=SILS_PLANT_DRAG) or "replay"
    // (playback from SILS_PLANT_REPLAY_FILE); unset/other = no plant (open loop, L2).
    const std::string plant_kind = GetEnvOr("SILS_PLANT", "none");
    const double plant_drag = std::stod(GetEnvOr("SILS_PLANT_DRAG", "0.5"));
    const double plant_init = std::stod(GetEnvOr("SILS_PLANT_INIT", "0.0"));
    const std::string plant_replay_file = GetEnvOr("SILS_PLANT_REPLAY_FILE", "");
    std::string plant_err;
    std::unique_ptr<sils::IPlant> plant =
        sils::MakePlant(plant_kind, plant_drag, plant_replay_file, plant_err);
    if (plant_kind != "none" && !plant) {
        std::cerr << "probe: " << (plant_err.empty() ? "unknown plant: " + plant_kind : plant_err)
                  << std::endl;
        participant->delete_contained_entities();
        DomainParticipantFactory::get_instance()->delete_participant(participant);
        return 2;
    }
    if (plant) {
        plant->Reset(plant_init);
        std::ostringstream pr;
        pr << "\"model\":\"" << plant->name() << "\",\"mode\":\"" << plant->mode()
           << "\",\"initial\":" << plant_init;
        TraceEvent(run_id, "plant_reset", 0, pr.str());
    }

    // Run one tick: ask the behavior for this tick's output, trace it, wrap the
    // inner request in the VISS envelope, and publish. Wire payload for the default
    // behavior is identical to the original probe.
    auto publish_one = [&](const sils::BehaviorContext& ctx) -> sils::BehaviorOutput {
        sils::BehaviorOutput out = behavior->OnTick(ctx);
        std::cout << "{\"run_id\":\"" << run_id << "\",\"instance_id\":\"" << g_instance_id
                  << "\",\"event\":\"behavior_output\",\"behavior\":\"" << behavior->name()
                  << "\",\"tick_seq\":" << ctx.tick_seq << ",\"value\":" << out.value << "}" << std::endl;
        VissRequestTopic sample;
        sample.payload(WrapRequestEnvelope(out.request_json, ctx.tick_seq, response_topic_prefix));
        TraceEvent(run_id, "publish_intent", static_cast<int>(ctx.tick_seq),
                   "\"matched_count\":" + std::to_string(g_matched_count.load()));
        // L1 (§11): emit request_sent so evaluate can correlate the responder's reply.
        if (reqresp) {
            TraceEvent(run_id, "request_sent", static_cast<int>(ctx.tick_seq),
                       "\"request_id\":\"" + RequestId(ctx.tick_seq) + "\",\"path\":\"" +
                           request_path + "\",\"action\":\"" + request_action + "\"");
        }
        writer->write(&sample);
        TraceEvent(run_id, "publish_result", static_cast<int>(ctx.tick_seq));
        return out;
    };

    // Optional input staging (TickInbox, §7.3): subscribe to an input stream and
    // freeze the pending arrivals per tick, so which inputs a tick consumes is
    // deterministic even though arrival timing isn't. Orthogonal to clock mode --
    // freezing happens once per tick loop iteration in either mode. Reuses the
    // VissRequestTopic type on a separate input topic (no new IDL).
    const bool stage_input = GetEnvOr("SILS_STAGE_INPUT", "0") == "1";
    const std::string input_topic = GetEnvOr("SILS_INPUT_TOPIC", "sils/input/stream");
    sils::TickInbox inbox;
    InputStagingListener input_listener(&inbox, run_id, input_topic);
    if (stage_input) {
        Subscriber* input_sub = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr);
        Topic* input_topic_obj =
            participant->create_topic(input_topic, request_type.get_type_name(), TOPIC_QOS_DEFAULT);
        DataReaderQos irq = DATAREADER_QOS_DEFAULT;
        irq.reliability().kind = RELIABLE_RELIABILITY_QOS;
        irq.history().kind = KEEP_LAST_HISTORY_QOS;
        irq.history().depth = 256;
        input_sub->create_datareader(input_topic_obj, irq, &input_listener);
        TraceEvent(run_id, "input_staging_on", 0, "\"input_topic\":\"" + input_topic + "\"");
    }

    // Optional tick-indexed replay (§7.4): pre-recorded input count per tick, so the
    // tick->input attribution is fixed by the file (deterministic, reproducible) --
    // no wall-clock arrival race. Takes precedence over DDS staging when set.
    const std::string replay_file = GetEnvOr("SILS_REPLAY_FILE", "");
    const bool replay_mode = !replay_file.empty();
    sils::ReplaySource replay;
    if (replay_mode) {
        std::string err;
        if (!replay.Load(replay_file, err)) {
            std::cerr << "probe: " << err << std::endl;
            participant->delete_contained_entities();
            DomainParticipantFactory::get_instance()->delete_participant(participant);
            return 2;
        }
        TraceEvent(run_id, "replay_start", 0,
                   "\"file\":\"" + replay_file + "\",\"inputs\":" + std::to_string(replay.total()));
    }

    sils::TickInfo tick;
    uint64_t last_seq = 0;
    uint64_t replay_recv = 0;  // synthetic reception sequence for replayed inputs
    while (clock->WaitForNextTick(tick)) {
        std::size_t staged_count = 0;
        if (replay_mode) {
            // §7.4: deliver this tick's pre-recorded inputs. Emit the same
            // input_received / tick_input_frozen contract as DDS staging, but
            // sourced from the file (source:"replay") so it's fully reproducible.
            std::size_t c = replay.CountForTick(tick.seq);
            std::vector<uint64_t> rs;
            for (std::size_t k = 0; k < c; ++k) {
                ++replay_recv;
                rs.push_back(replay_recv);
                std::cout << "{\"run_id\":\"" << run_id << "\",\"instance_id\":\"" << g_instance_id
                          << "\",\"event\":\"input_received\",\"recv_seq\":" << replay_recv
                          << ",\"source\":\"replay\"}" << std::endl;
            }
            staged_count = c;
            std::cout << "{\"run_id\":\"" << run_id << "\",\"instance_id\":\"" << g_instance_id
                      << "\",\"event\":\"tick_input_frozen\",\"tick_seq\":" << tick.seq
                      << ",\"recv_seqs\":" << RecvSeqsJson(rs) << ",\"count\":" << c
                      << ",\"source\":\"replay\"}" << std::endl;
        } else if (stage_input) {
            // §7.1: confirm this tick's inbound sample set at the tick boundary.
            std::vector<uint64_t> frozen = inbox.FreezeTick();
            staged_count = frozen.size();
            std::cout << "{\"run_id\":\"" << run_id << "\",\"instance_id\":\"" << g_instance_id
                      << "\",\"event\":\"tick_input_frozen\",\"tick_seq\":" << tick.seq
                      << ",\"recv_seqs\":" << RecvSeqsJson(frozen)
                      << ",\"count\":" << frozen.size() << "}" << std::endl;
        }
        // The behavior consumes this tick's staged input count (§7.3) + sim time,
        // and (L3) the plant output fed back from the previous tick (§9 closed loop).
        sils::BehaviorContext ctx{tick.seq, tick.sim_time_ns, staged_count};
        if (plant) ctx.plant_feedback = plant->output();
        sils::BehaviorOutput out = publish_one(ctx);
        if (plant) {
            // Close the loop: the command just published drives the plant one dt; the
            // new output becomes next tick's feedback. Trace the whole tick of the loop
            // (feedback_in -> command -> output) for L3 conformance / model correlation.
            plant->SetInputs(out.value);
            double y = plant->Step(tick.dt_ns);
            std::ostringstream ps;
            ps << std::setprecision(12)
               << "\"tick_seq\":" << tick.seq << ",\"sim_time_ns\":" << tick.sim_time_ns
               << ",\"dt_ns\":" << tick.dt_ns << ",\"feedback_in\":" << ctx.plant_feedback
               << ",\"command\":" << out.value << ",\"output\":" << y;
            TraceEvent(run_id, "plant_step", static_cast<int>(tick.seq), ps.str());
        }
        clock->TickDone(tick);  // lockstep: sends TickComplete; wall: no-op
        last_seq = tick.seq;
    }

    // L1 (§11): responses arrive asynchronously; give in-flight replies time to land
    // and be traced before we tear the response reader down. Skipped in oneway mode.
    if (reqresp && !g_stop.load()) {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(GetEnvIntOr("SILS_RESPONSE_DRAIN_MS", 500)));
    }
    (void)resp_topic_obj; (void)resp_sub;  // owned by participant; freed below

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
