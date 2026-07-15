// SiLS vECU Phase 1 Vertical PoC -- independent VISS /VIN/Vehicle verifier.
//
// A one-shot subscriber that proves samples published by sils_vecu_viss_probe are
// actually receivable on the DDS bus -- independent evidence, not a self-check by
// the probe itself. This is the "sample 확인" step of architecture v3 Final §17's
// minimal Vertical PoC checklist. It does not implement VecuReport/readiness IDL
// (Phase 2+ scope, §5.4) -- for Phase 1, DDS subscription_matched plus receiving
// >= N samples within a timeout stands in as the readiness/verification surrogate.
//
// Exit code: 0 if min_samples samples with the expected envelope fields were
// received before the timeout, 1 otherwise.

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <string>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

#include "VissRequestPubSubTypes.hpp"

namespace {

// contract id: viss-request (config/sils/contracts/mmf-endpoints.yaml)
const std::string kTopic("/VIN/Vehicle");

// Informational only: which vECU instance_id this verify invocation was scoped
// to (if any). Not used to filter samples -- the real VISS envelope payload
// carries no instance identity to filter on (architecture v3 Final §7.3: identity
// lives in sidecar trace/participant name, never the wire payload) -- this
// verifier still counts any valid-envelope sample on the topic, matching
// Phase 1's single-vECU scope. Kept for symmetry with viss_probe_main.cc so
// trace lines from both sides are greppable by the same field.
std::string g_expected_instance_id = "";

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
        return fallback;
    }
}

long long WallTimeMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

void TraceEvent(const std::string& run_id, const std::string& event,
                const std::string& extra_json_fields = "") {
    std::cout << "{\"run_id\":\"" << run_id << "\",\"expected_instance_id\":\""
              << g_expected_instance_id << "\",\"event\":\"" << event
              << "\",\"wall_time_ms\":" << WallTimeMs() << ",\"topic\":\"" << kTopic << "\""
              << (extra_json_fields.empty() ? "" : ("," + extra_json_fields)) << "}"
              << std::endl;
}

// Lightweight envelope shape check (not a full JSON parse): confirms the payload
// looks like the {"topic":...,"request":...} envelope the probe/SUT use, i.e.
// "필드 일치" at the level Phase 1 needs -- see viss_probe_main.cc.
bool LooksLikeExpectedEnvelope(const std::string& payload) {
    return payload.find("\"topic\":") != std::string::npos &&
           payload.find("\"request\":") != std::string::npos;
}

class VerifyListener : public eprosima::fastdds::dds::DataReaderListener {
public:
    VerifyListener(const std::string& run_id, int min_samples)
        : run_id_(run_id), min_samples_(min_samples) {}

    void on_subscription_matched(
        eprosima::fastdds::dds::DataReader* /*reader*/,
        const eprosima::fastdds::dds::SubscriptionMatchedStatus& info) override {
        if (info.current_count_change > 0) {
            TraceEvent(run_id_, "endpoint_matched",
                       "\"matched_count\":" + std::to_string(info.current_count));
        }
    }

    void on_data_available(eprosima::fastdds::dds::DataReader* reader) override {
        eprosima::fastdds::dds::SampleInfo info;
        VissRequestTopic sample;
        if (reader->take_next_sample(&sample, &info) != eprosima::fastdds::dds::RETCODE_OK) {
            return;
        }
        if (!info.valid_data) return;

        const bool ok = LooksLikeExpectedEnvelope(sample.payload());
        int received_snapshot, valid_snapshot;
        {
            std::lock_guard<std::mutex> lock(mtx_);
            received_count_++;
            if (ok) valid_count_++;
            received_snapshot = received_count_;
            valid_snapshot = valid_count_;
            if (valid_snapshot >= min_samples_) {
                done_ = true;
            }
        }
        TraceEvent(run_id_, "sample_received",
                   "\"valid_envelope\":" + std::string(ok ? "true" : "false") +
                       ",\"received_count\":" + std::to_string(received_snapshot));

        if (valid_snapshot >= min_samples_) {
            cv_.notify_one();
        }
    }

    bool WaitForDone(int timeout_sec) {
        std::unique_lock<std::mutex> lock(mtx_);
        return cv_.wait_for(lock, std::chrono::seconds(timeout_sec), [this] { return done_; });
    }

    int received_count() const { return received_count_; }
    int valid_count() const { return valid_count_; }

private:
    std::string run_id_;
    int min_samples_;
    std::mutex mtx_;
    std::condition_variable cv_;
    bool done_ = false;
    int received_count_ = 0;
    int valid_count_ = 0;
};

}  // namespace

int main() {
    const std::string run_id = GetEnvOr("SILS_RUN_ID", "local");
    g_expected_instance_id = GetEnvOr("SILS_VECU_INSTANCE_ID", "");
    const int timeout_sec = GetEnvIntOr("SILS_VISS_VERIFY_TIMEOUT_SEC", 10);
    const int min_samples = GetEnvIntOr("SILS_VISS_VERIFY_MIN_SAMPLES", 1);

    int domain_id = 0;
    const std::string domain_env = GetEnvOr("VISS_DDS_DOMAIN_ID", "");
    if (!domain_env.empty()) {
        try {
            domain_id = std::stoi(domain_env);
        } catch (const std::exception& e) {
            std::cerr << "Invalid VISS_DDS_DOMAIN_ID=\"" << domain_env << "\": " << e.what()
                      << ". Using default domain 0." << std::endl;
        }
    }

    using namespace eprosima::fastdds::dds;

    TraceEvent(run_id, "verify_start", "\"domain_id\":" + std::to_string(domain_id));

    DomainParticipantQos pqos;
    // Suffixed with run_id (not instance_id -- one verifier checks a whole run,
    // not one vECU instance) so concurrent runs on the same domain don't collide
    // on participant name (§9.5).
    pqos.name("sils_vecu_viss_verify:" + run_id);
    DomainParticipant* participant =
        DomainParticipantFactory::get_instance()->create_participant(
            domain_id, pqos, nullptr, StatusMask::none());
    if (participant == nullptr) {
        std::cerr << "Failed to create DDS participant on domain " << domain_id << std::endl;
        return 1;
    }

    TypeSupport request_type(new VissRequestTopicPubSubType());
    request_type.register_type(participant);

    Subscriber* subscriber = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr);
    Topic* topic = participant->create_topic(kTopic, request_type.get_type_name(), TOPIC_QOS_DEFAULT);

    DataReaderQos reader_qos = DATAREADER_QOS_DEFAULT;
    reader_qos.reliability().kind = RELIABLE_RELIABILITY_QOS;  // must match probe's writer QoS
    // Request TRANSIENT_LOCAL so a verifier that joins after the probe already burst
    // its samples still receives the run's history (see probe writer QoS). Durability
    // RxO is strict: reader-requested TRANSIENT_LOCAL matches only a TRANSIENT_LOCAL
    // writer, so this MUST track SILS_DURABILITY with the probe. volatile -> volatile.
    if (GetEnvOr("SILS_DURABILITY", "transient-local") != "volatile") {
        reader_qos.durability().kind = TRANSIENT_LOCAL_DURABILITY_QOS;
        reader_qos.history().kind = KEEP_LAST_HISTORY_QOS;
        reader_qos.history().depth = 256;
    }

    VerifyListener listener(run_id, min_samples);
    DataReader* reader = subscriber->create_datareader(topic, reader_qos, &listener);
    if (reader == nullptr) {
        std::cerr << "Failed to create DataReader for topic " << kTopic << std::endl;
        return 1;
    }

    const bool ok = listener.WaitForDone(timeout_sec);

    TraceEvent(run_id, ok ? "verify_pass" : "verify_fail",
               "\"received_count\":" + std::to_string(listener.received_count()) +
                   ",\"valid_count\":" + std::to_string(listener.valid_count()) +
                   ",\"min_samples\":" + std::to_string(min_samples));

    subscriber->delete_datareader(reader);
    participant->delete_subscriber(subscriber);
    participant->delete_topic(topic);
    DomainParticipantFactory::get_instance()->delete_participant(participant);

    return ok ? 0 : 1;
}
