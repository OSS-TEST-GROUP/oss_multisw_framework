// SiLS plant-coupling (Option 2) VehicleControl verifier -- independent, standalone.
//
// The bus-coupled counterpart of sils_vecu_viss_verify: a one-shot subscriber that proves
// the commands published by a plant-coupling vECU (steer-ecu / vehicle-interface, via
// RunVehicleProbe) are actually receivable AND well-formed on the DDS bus -- independent
// evidence, not a self-check. It subscribes to the vECU's OUT channel (default sils/ego/cmd,
// the canonical VehicleControl the plant consumes) and counts valid commands.
//
// Why it exists (S4 follow-up): evaluate.py needs a TERMINAL to conclude a run (§5.2). A
// coordinator-driven deployment emits run_pass; a raw bus run (no coordinator) had none, so
// it evaluated INCOMPLETE. This verifier emits the SAME verifier-producer events the VISS
// verifier does (verify_start / endpoint_matched / sample_received / verify_pass|fail), which
// evaluate already treats as a standalone terminal (VERIFIER_TERMINAL) -- so a bus run
// becomes judgeable with no coordinator and NO schema change. It does not judge determinism
// or loop-closure (that is evaluate's analyze_vehicle); it is the "samples receivable +
// well-formed" (L0-over-the-bus) evidence, mirroring the VISS verifier's role.
//
// A command is "valid" if steering_angle and acceleration are finite and gear is a defined
// value (0=P 1=R 2=N 3=D, VehicleControl.idl / vehicle_sample.hpp).
//
// Exit code: 0 if min_samples valid commands were received before the timeout, 1 otherwise.

#include <atomic>
#include <chrono>
#include <cmath>
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

#include "VehicleControlPubSubTypes.hpp"

namespace {

std::string g_topic = "sils/ego/cmd";        // vECU OUT channel (canonical/vehicle-control)
std::string g_expected_instance_id = "";     // informational; symmetry with the probe trace

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
              << "\",\"wall_time_ms\":" << WallTimeMs() << ",\"topic\":\"" << g_topic << "\""
              << (extra_json_fields.empty() ? "" : ("," + extra_json_fields)) << "}"
              << std::endl;
}

// Well-formed canonical command: finite actuation + a defined gear (0..3). The bus
// counterpart of viss_verify's envelope check.
bool LooksLikeValidControl(const VehicleControl& m) {
    return std::isfinite(m.steering_angle()) && std::isfinite(m.acceleration()) &&
           m.gear() <= 3;  // gear is octet (>=0); 0=P 1=R 2=N 3=D
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
        VehicleControl sample;
        if (reader->take_next_sample(&sample, &info) != eprosima::fastdds::dds::RETCODE_OK) {
            return;
        }
        if (!info.valid_data) return;

        const bool ok = LooksLikeValidControl(sample);
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
    g_topic = GetEnvOr("SILS_VEHICLE_VERIFY_TOPIC", "sils/ego/cmd");
    const int timeout_sec = GetEnvIntOr("SILS_VEHICLE_VERIFY_TIMEOUT_SEC", 10);
    const int min_samples = GetEnvIntOr("SILS_VEHICLE_VERIFY_MIN_SAMPLES", 1);

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
    pqos.name("sils_vecu_vehicle_verify:" + run_id);
    DomainParticipant* participant =
        DomainParticipantFactory::get_instance()->create_participant(
            domain_id, pqos, nullptr, StatusMask::none());
    if (participant == nullptr) {
        std::cerr << "Failed to create DDS participant on domain " << domain_id << std::endl;
        return 1;
    }

    TypeSupport control_type(new VehicleControlPubSubType());
    control_type.register_type(participant);

    Subscriber* subscriber = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr);
    Topic* topic = participant->create_topic(g_topic, control_type.get_type_name(), TOPIC_QOS_DEFAULT);

    // Match the probe's VehicleControl writer QoS (RELIABLE + KEEP_LAST + TRANSIENT_LOCAL
    // by default) so RxO matches and a late-joining verifier still sees the run's commands.
    DataReaderQos reader_qos = DATAREADER_QOS_DEFAULT;
    reader_qos.reliability().kind = RELIABLE_RELIABILITY_QOS;
    if (GetEnvOr("SILS_DURABILITY", "transient-local") != "volatile") {
        reader_qos.durability().kind = TRANSIENT_LOCAL_DURABILITY_QOS;
        reader_qos.history().kind = KEEP_LAST_HISTORY_QOS;
        reader_qos.history().depth = 256;
    }

    VerifyListener listener(run_id, min_samples);
    DataReader* reader = subscriber->create_datareader(topic, reader_qos, &listener);
    if (reader == nullptr) {
        std::cerr << "Failed to create DataReader for topic " << g_topic << std::endl;
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
