// SiLS test input source -- exercises TickInbox staging (architecture v3 Final §7.3).
//
// Stand-in for an asynchronous SUT output: publishes VissRequestTopic samples on
// an input topic (default sils/input/stream) at a fixed rate for a fixed count, on
// the run's DDS domain. A staging-enabled probe (SILS_STAGE_INPUT=1) subscribes,
// stages each arrival into its TickInbox, and freezes the set per tick. This is a
// test fixture, not a production component -- it just produces async input so the
// staging attribution (§7.3) can be observed end-to-end.
//
// Reuses VissRequestTopic (string payload) as a generic input sample -- no new IDL.

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
#include <fastdds/dds/topic/TypeSupport.hpp>

#include "VissRequestPubSubTypes.hpp"
#include "sils_participant_qos.hpp"

using namespace eprosima::fastdds::dds;

namespace {
std::string GetEnvOr(const char* n, const std::string& fb) {
    const char* v = std::getenv(n);
    return v ? std::string(v) : fb;
}
int ArgInt(int argc, char** argv, const std::string& flag, int fb) {
    for (int i = 1; i < argc - 1; ++i)
        if (flag == argv[i]) { try { return std::stoi(argv[i + 1]); } catch (...) { return fb; } }
    return fb;
}
std::string ArgStr(int argc, char** argv, const std::string& flag, const std::string& fb) {
    for (int i = 1; i < argc - 1; ++i)
        if (flag == argv[i]) return argv[i + 1];
    return fb;
}
}  // namespace

int main(int argc, char** argv) {
    const std::string run_id = GetEnvOr("SILS_RUN_ID", "local");
    const std::string topic_name = ArgStr(argc, argv, "--topic",
                                          GetEnvOr("SILS_INPUT_TOPIC", "sils/input/stream"));
    const int count = ArgInt(argc, argv, "--count", 20);
    const int rate_ms = ArgInt(argc, argv, "--rate-ms", 100);
    const int startup_ms = ArgInt(argc, argv, "--startup-ms", 1000);

    int domain_id = 0;
    const std::string denv = GetEnvOr("VISS_DDS_DOMAIN_ID", "");
    if (!denv.empty()) { try { domain_id = std::stoi(denv); } catch (...) {} }

    DomainParticipantQos pqos;
    sils::ApplySilsParticipantQos(pqos);   // see sils_participant_qos.hpp
    pqos.name("sils_input_source:" + run_id);
    DomainParticipant* participant =
        DomainParticipantFactory::get_instance()->create_participant(
            domain_id, pqos, nullptr, StatusMask::none());
    if (!participant) {
        std::cerr << "input_source: failed to create participant on domain " << domain_id << std::endl;
        return 2;
    }

    TypeSupport type(new VissRequestTopicPubSubType());
    type.register_type(participant);
    Publisher* pub = participant->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr);
    Topic* topic = participant->create_topic(topic_name, type.get_type_name(), TOPIC_QOS_DEFAULT);

    DataWriterQos wqos = DATAWRITER_QOS_DEFAULT;
    wqos.reliability().kind = RELIABLE_RELIABILITY_QOS;
    wqos.history().kind = KEEP_LAST_HISTORY_QOS;
    wqos.history().depth = 256;
    DataWriter* writer = pub->create_datawriter(topic, wqos, nullptr);

    // Let the probe's input reader discover us so no early sample is dropped.
    std::this_thread::sleep_for(std::chrono::milliseconds(startup_ms));

    std::cout << "{\"run_id\":\"" << run_id << "\",\"event\":\"input_source_start\",\"topic\":\""
              << topic_name << "\",\"count\":" << count << ",\"rate_ms\":" << rate_ms << "}" << std::endl;

    for (int i = 1; i <= count; ++i) {
        VissRequestTopic sample;
        sample.payload("{\"input_seq\":" + std::to_string(i) + "}");
        writer->write(&sample);
        std::cout << "{\"run_id\":\"" << run_id << "\",\"event\":\"input_source_sent\",\"seq\":"
                  << i << ",\"topic\":\"" << topic_name << "\"}" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(rate_ms));
    }

    std::cout << "{\"run_id\":\"" << run_id << "\",\"event\":\"input_source_done\",\"sent\":"
              << count << "}" << std::endl;

    participant->delete_contained_entities();
    DomainParticipantFactory::get_instance()->delete_participant(participant);
    return 0;
}
