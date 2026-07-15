// SiLS VISS responder -- SUT stand-in for L1 protocol fidelity (architecture v3
// Final §11 L1).
//
// L0 proves a sample arrives; L1 proves the request/response CONVERSATION is
// correct. This binary is the responder half (the SUT stand-in, analogous to how
// viss_verify is the independent subscriber): it subscribes to the request topic
// /VIN/Vehicle, and for each VissRequest envelope publishes a correlated response
// on the response topic, echoing the request's requestId. The probe (client)
// receives the response and both sides emit request_sent / response_received, which
// evaluate.py analyze_protocol correlates by request_id.
//
// Error semantics: normally replies status="ok"; if SILS_RESPONDER_ERROR_EVERY=k>0,
// every k-th request gets a well-formed error (status="error", errorCode set) so the
// §11 L1 error path is exercised deterministically.
//
// QoS: reader/writer are RELIABLE + (default) TRANSIENT_LOCAL so a late-joining probe
// still receives responses; tracks SILS_DURABILITY like the probe/verify. Emits
// qos_negotiated so analyze_protocol can check contracted reliability.
//
// Runs as a passive service for SILS_RESPONDER_TIMEOUT_SEC (a run's probe finishes
// well within that), then exits.

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/qos/DataWriterQos.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

#include "VissRequestPubSubTypes.hpp"
#include "json_min.hpp"

namespace {

const std::string kRequestTopic("/VIN/Vehicle");
std::string g_response_topic = "sils/viss/response";

std::string GetEnvOr(const char* name, const std::string& fallback) {
    const char* v = std::getenv(name);
    return v ? std::string(v) : fallback;
}

int GetEnvIntOr(const char* name, int fallback) {
    const char* v = std::getenv(name);
    if (!v) return fallback;
    try { return std::stoi(v); } catch (const std::exception&) { return fallback; }
}

long long WallTimeMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

std::string g_run_id = "local";

void TraceEvent(const std::string& event, const std::string& extra = "") {
    std::cout << "{\"run_id\":\"" << g_run_id << "\",\"role\":\"responder\",\"event\":\""
              << event << "\",\"wall_time_ms\":" << WallTimeMs()
              << (extra.empty() ? "" : ("," + extra)) << "}" << std::endl;
}

using namespace eprosima::fastdds::dds;

// Reads requests, publishes a correlated response for each.
class ResponderListener : public DataReaderListener {
public:
    ResponderListener(DataWriter* writer, int error_every)
        : writer_(writer), error_every_(error_every) {}

    void on_subscription_matched(DataReader* /*r*/, const SubscriptionMatchedStatus& info) override {
        if (info.current_count_change > 0) {
            TraceEvent("endpoint_matched", "\"role\":\"responder-reader\",\"matched_count\":" +
                                               std::to_string(info.current_count));
        }
    }

    void on_data_available(DataReader* reader) override {
        SampleInfo info;
        VissRequestTopic sample;
        while (reader->take_next_sample(&sample, &info) == RETCODE_OK) {
            if (!info.valid_data) continue;
            const std::string& payload = sample.payload();
            const std::string request_id = sils::JsonFindString(payload, "requestId", "");
            if (request_id.empty()) continue;  // not a correlatable request
            ++handled_;
            const bool is_error = error_every_ > 0 && (handled_ % error_every_ == 0);

            std::string resp = "{\"requestId\":\"" + request_id + "\",\"status\":\"" +
                               (is_error ? "error" : "ok") + "\"";
            if (is_error) resp += ",\"errorCode\":\"UNKNOWN_PATH\"";
            resp += "}";

            VissRequestTopic out;
            out.payload(resp);
            writer_->write(&out);
            TraceEvent("responder_reply", "\"request_id\":\"" + request_id + "\",\"status\":\"" +
                                              (is_error ? "error" : "ok") + "\"");
        }
    }

    int handled() const { return handled_; }

private:
    DataWriter* writer_;
    int error_every_;
    int handled_ = 0;
};

int ResolveDomain() {
    const std::string env = GetEnvOr("VISS_DDS_DOMAIN_ID", "");
    if (env.empty()) return 0;
    try { return std::stoi(env); } catch (const std::exception&) { return 0; }
}

// Apply the contracted QoS (RELIABLE + TRANSIENT_LOCAL unless SILS_DURABILITY=volatile)
// and emit qos_negotiated for the given role.
bool TransientLocal() { return GetEnvOr("SILS_DURABILITY", "transient-local") != "volatile"; }

void TraceQos(const std::string& role) {
    TraceEvent("qos_negotiated", "\"role\":\"" + role + "\",\"reliability\":\"RELIABLE\"" +
                                     ",\"durability\":\"" +
                                     (TransientLocal() ? "TRANSIENT_LOCAL" : "VOLATILE") + "\"");
}

}  // namespace

int main() {
    g_run_id = GetEnvOr("SILS_RUN_ID", "local");
    g_response_topic = GetEnvOr("SILS_RESPONSE_TOPIC", "sils/viss/response");
    const int error_every = GetEnvIntOr("SILS_RESPONDER_ERROR_EVERY", 0);
    const int timeout_sec = GetEnvIntOr("SILS_RESPONDER_TIMEOUT_SEC", 15);
    const int domain_id = ResolveDomain();

    TraceEvent("responder_start", "\"domain_id\":" + std::to_string(domain_id) +
                                      ",\"response_topic\":\"" + g_response_topic + "\"");

    DomainParticipantQos pqos;
    pqos.name("sils_vecu_viss_responder:" + g_run_id);
    DomainParticipant* participant = DomainParticipantFactory::get_instance()->create_participant(
        domain_id, pqos, nullptr, StatusMask::none());
    if (participant == nullptr) {
        std::cerr << "responder: failed to create participant on domain " << domain_id << std::endl;
        return 1;
    }

    TypeSupport type(new VissRequestTopicPubSubType());
    type.register_type(participant);

    // Response writer (publishes on the response topic).
    Publisher* publisher = participant->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr);
    Topic* resp_topic = participant->create_topic(g_response_topic, type.get_type_name(), TOPIC_QOS_DEFAULT);
    DataWriterQos wqos = DATAWRITER_QOS_DEFAULT;
    wqos.reliability().kind = RELIABLE_RELIABILITY_QOS;
    if (TransientLocal()) {
        wqos.durability().kind = TRANSIENT_LOCAL_DURABILITY_QOS;
        wqos.history().kind = KEEP_LAST_HISTORY_QOS;
        wqos.history().depth = 256;
    }
    DataWriter* writer = publisher->create_datawriter(resp_topic, wqos, nullptr);
    if (writer == nullptr) {
        std::cerr << "responder: failed to create response DataWriter" << std::endl;
        return 1;
    }
    TraceQos("responder-writer");

    // Request reader (subscribes to the request topic).
    Subscriber* subscriber = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr);
    Topic* req_topic = participant->create_topic(kRequestTopic, type.get_type_name(), TOPIC_QOS_DEFAULT);
    DataReaderQos rqos = DATAREADER_QOS_DEFAULT;
    rqos.reliability().kind = RELIABLE_RELIABILITY_QOS;
    if (TransientLocal()) {
        rqos.durability().kind = TRANSIENT_LOCAL_DURABILITY_QOS;
        rqos.history().kind = KEEP_LAST_HISTORY_QOS;
        rqos.history().depth = 256;
    }
    TraceQos("responder-reader");
    ResponderListener listener(writer, error_every);
    DataReader* reader = subscriber->create_datareader(req_topic, rqos, &listener);
    if (reader == nullptr) {
        std::cerr << "responder: failed to create request DataReader" << std::endl;
        return 1;
    }

    // Passive service: serve for the timeout window, then exit.
    std::this_thread::sleep_for(std::chrono::seconds(timeout_sec));

    TraceEvent("responder_stop", "\"handled\":" + std::to_string(listener.handled()));

    participant->delete_contained_entities();
    DomainParticipantFactory::get_instance()->delete_participant(participant);
    return 0;
}
