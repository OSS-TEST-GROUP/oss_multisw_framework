#ifndef SILS_VECU_WIRE_HPP
#define SILS_VECU_WIRE_HPP

// IWireCodec -- the DDS wire port (libsils_vecu SDK, docs/sils-extension-model.md P4).
//
// The fifth hexagonal port. It owns everything type-specific about what goes ON the
// wire: registering the DDS type, its registered_type name, and turning a behavior's
// per-tick BehaviorOutput into published sample(s) (applying wire-level faults). RunProbe
// (run_loop.hpp) stays wire-agnostic -- it creates the participant/publisher/topic/writer
// and drives the trace contract, but delegates "encode this output and write it" to the
// codec, so the DDS payload/type is no longer hard-wired to the VISS envelope.
//
//   - VissWireCodec : the Phase 1 wire -- a VissRequestTopic whose string payload carries
//                     the VISS request envelope the real server parses (preserves the
//                     exact original payload).
//   - RawValueWireCodec : same VissRequestTopic, but the payload is the bare scalar value
//                     (no envelope) -- for a stand-in whose consumer wants just the number.
//
// Two impls of the SAME type differing only in ENCODING (earned-abstraction, like the
// other ports). P4b adds a codec over a *different* (custom-IDL) registered type, which
// is the point of the port: a developer publishes their own contract type by implementing
// IWireCodec, with no change to RunProbe.
//
// Fault model: Drop is handled by RunProbe (it simply doesn't call Publish). Corrupt and
// Duplicate are wire-level, so the codec realizes them (a "corrupt" BrakeCmd differs from
// a corrupt JSON string), keeping the negative-path semantics type-appropriate.

#include <memory>
#include <sstream>
#include <string>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

#include "VissRequestPubSubTypes.hpp"
#include "behavior.hpp"  // sils::BehaviorOutput

namespace sils {

enum class WireFault { Normal, Corrupt, Duplicate };

// The L1 correlation key convention (part of the trace contract -- RunProbe emits it in
// request_sent/scenario_fault, the VISS envelope embeds it). Shared here so both agree.
inline std::string RequestId(uint64_t seq) { return "sils-" + std::to_string(seq); }

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

class IWireCodec {
public:
    virtual ~IWireCodec() = default;
    // Register the wire DDS type on the participant (once, before any Publish).
    virtual void RegisterType(eprosima::fastdds::dds::DomainParticipant* participant) = 0;
    // The registered_type name, for create_topic. Valid after RegisterType.
    virtual const char* type_name() const = 0;
    // Encode `out` for this tick and write it on `writer` (created for type_name()),
    // realizing the wire-level fault. Normal: one write. Corrupt: one malformed write.
    // Duplicate: two identical writes.
    virtual void Publish(eprosima::fastdds::dds::DataWriter* writer, const BehaviorOutput& out,
                         uint64_t tick_seq, WireFault fault) = 0;
    virtual const char* name() const = 0;
};

// VissRequestTopic base shared by the two built-in codecs (same registered type; they
// differ only in how Publish fills the string payload).
class VissTopicCodecBase : public IWireCodec {
public:
    void RegisterType(eprosima::fastdds::dds::DomainParticipant* participant) override {
        type_ = eprosima::fastdds::dds::TypeSupport(new VissRequestTopicPubSubType());
        type_.register_type(participant);
        type_name_ = type_.get_type_name();
    }
    const char* type_name() const override { return type_name_.c_str(); }

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

#endif  // SILS_VECU_WIRE_HPP
