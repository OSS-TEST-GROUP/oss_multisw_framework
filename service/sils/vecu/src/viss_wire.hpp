#ifndef SILS_VECU_VISS_WIRE_HPP
#define SILS_VECU_VISS_WIRE_HPP

// Built-in VISS wire codecs -- the concrete IWireCodec impls over the VissRequestTopic
// type (libsils_vecu SDK, docs/sils/sils-extension-model.md P4). Split out of wire.hpp so the
// PORT header names no DDS type: an ECU that ships its own contract type includes only
// wire.hpp / run_loop.hpp and never compiles the VISS-generated sources. The default
// probe (viss_probe_main.cc) and the VISS-envelope example (examples/brake-ecu) include
// THIS header to build their codec and inject it into RunProbe.
//
//   - VissWireCodec : the Phase 1 wire -- a VissRequestTopic whose string payload carries
//                     the VISS request envelope the real server parses (preserves the
//                     exact original payload).
//   - RawValueWireCodec : same VissRequestTopic, but the payload is the bare scalar value
//                     (no envelope) -- for a stand-in whose consumer wants just the number.
//
// Two impls of the SAME type differing only in ENCODING (earned-abstraction, like the
// other ports). A codec over a *different* (custom-IDL) registered type -- the point of
// the port -- lives with its own ECU (examples/brake-ecu-native), not here.

#include <cstdint>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

#include "VissRequestPubSubTypes.hpp"
#include "json_min.hpp"  // sils::JsonFindString (response field extraction)
#include "wire.hpp"      // sils::IWireCodec / WireFault / WireInbound / RequestId

namespace sils {

inline std::string EscapeJsonString(const std::string& in) {
    std::string out;
    out.reserve(in.size());
    for (char c : in) {
        if (c == '"' || c == '\\') out += '\\';
        out += c;
    }
    return out;
}

// Wraps a behavior's inner request JSON in the envelope the real VISS server's
// RequestListener expects: {"requestId":..,"topic":<response topic>,"request":"<escaped inner>"}.
// A bare inner JSON is rejected by the SUT ("not a valid JSON or has missing fields").
inline std::string WrapRequestEnvelope(const std::string& inner_request, uint64_t seq,
                                       const std::string& response_topic_prefix) {
    std::ostringstream response_topic;
    response_topic << response_topic_prefix << "/" << seq;
    std::ostringstream outer;
    outer << R"({"requestId":")" << RequestId(seq) << R"(","topic":")" << response_topic.str()
          << R"(","request":")" << EscapeJsonString(inner_request) << R"("})";
    return outer.str();
}

// VissRequestTopic base shared by the two built-in codecs (same registered type; they
// differ only in how Publish fills the string payload). Owns the type registration and
// the read path -- Take decodes the VISS response/staging fields from the string payload.
class VissTopicCodecBase : public IWireCodec {
public:
    void RegisterType(eprosima::fastdds::dds::DomainParticipant* participant) override {
        type_ = eprosima::fastdds::dds::TypeSupport(new VissRequestTopicPubSubType());
        type_.register_type(participant);
        type_name_ = type_.get_type_name();
    }
    const char* type_name() const override { return type_name_.c_str(); }

    // Read path: take VissRequestTopic samples and pull requestId/status/errorCode out of
    // the JSON string payload (input staging ignores these fields and just counts).
    std::vector<WireInbound> Take(eprosima::fastdds::dds::DataReader* reader) override {
        std::vector<WireInbound> out;
        VissRequestTopic sample;
        eprosima::fastdds::dds::SampleInfo info;
        while (reader->take_next_sample(&sample, &info) == eprosima::fastdds::dds::RETCODE_OK) {
            if (!info.valid_data) continue;
            const std::string& p = sample.payload();
            out.push_back(WireInbound{sils::JsonFindString(p, "requestId", ""),
                                      sils::JsonFindString(p, "status", ""),
                                      sils::JsonFindString(p, "errorCode", "")});
        }
        return out;
    }

protected:
    // Common corrupt payload (deterministic garbage with no recoverable requestId) +
    // the write/duplicate mechanics; subclasses only supply the NORMAL payload string.
    void WriteWithFault(eprosima::fastdds::dds::DataWriter* writer, uint64_t tick_seq,
                        WireFault fault, const std::string& normal_payload) {
        VissRequestTopic sample;
        if (fault == WireFault::Corrupt) {
            sample.payload("<<CORRUPT SILS SCENARIO PAYLOAD tick " + std::to_string(tick_seq) + ">>");
            writer->write(&sample);
            return;
        }
        sample.payload(normal_payload);
        writer->write(&sample);
        if (fault == WireFault::Duplicate) writer->write(&sample);
    }

    eprosima::fastdds::dds::TypeSupport type_;
    std::string type_name_;
};

// Phase 1 wire: VISS request envelope in the string payload (unchanged original payload).
class VissWireCodec : public VissTopicCodecBase {
public:
    explicit VissWireCodec(std::string response_topic_prefix)
        : response_topic_prefix_(std::move(response_topic_prefix)) {}
    void Publish(eprosima::fastdds::dds::DataWriter* writer, const BehaviorOutput& out,
                 uint64_t tick_seq, WireFault fault) override {
        WriteWithFault(writer, tick_seq, fault,
                       WrapRequestEnvelope(out.request_json, tick_seq, response_topic_prefix_));
    }
    const char* name() const override { return "viss-envelope"; }

private:
    std::string response_topic_prefix_;
};

// Alternate encoding on the same type: the bare scalar value, no envelope.
class RawValueWireCodec : public VissTopicCodecBase {
public:
    void Publish(eprosima::fastdds::dds::DataWriter* writer, const BehaviorOutput& out,
                 uint64_t tick_seq, WireFault fault) override {
        std::ostringstream os;
        os << out.value;
        WriteWithFault(writer, tick_seq, fault, os.str());
    }
    const char* name() const override { return "raw-value"; }
};

// SILS_WIRE selects the codec: "raw" -> RawValueWireCodec; anything else -> VISS envelope.
inline std::unique_ptr<IWireCodec> MakeWireCodec(const std::string& kind,
                                                 const std::string& response_topic_prefix) {
    if (kind == "raw") return std::make_unique<RawValueWireCodec>();
    return std::make_unique<VissWireCodec>(response_topic_prefix);
}

// Convenience for the default VISS wire (matches the extension-model example main()).
inline std::unique_ptr<IWireCodec> MakeVissWireCodec(
    std::string response_topic_prefix = "sils/viss_probe/response") {
    return std::make_unique<VissWireCodec>(std::move(response_topic_prefix));
}

}  // namespace sils

#endif  // SILS_VECU_VISS_WIRE_HPP
