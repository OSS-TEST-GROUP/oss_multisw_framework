#ifndef SILS_VECU_WIRE_HPP
#define SILS_VECU_WIRE_HPP

// IWireCodec -- the DDS wire port (libsils_vecu SDK, docs/sils/sils-extension-model.md P4).
//
// The fifth hexagonal port. It owns everything type-specific about what crosses the
// wire in BOTH directions: registering the DDS type, its registered_type name, turning
// a behavior's per-tick BehaviorOutput into published sample(s) (write path, applying
// wire-level faults), and taking + decoding inbound samples off a reader (read path).
// RunProbe (run_loop.hpp) stays wire-agnostic -- it creates the participant/publisher/
// subscriber/topics and drives the trace contract, but delegates encode/decode of the
// concrete DDS type to the codec, so the payload/type is not hard-wired anywhere in the
// tick loop.
//
// This header is the PORT ONLY -- it names no concrete DDS type, so an ECU that ships
// its own contract type (P4b) can include run_loop.hpp / wire.hpp without compiling any
// built-in type's generated sources. The built-in VISS codecs live in viss_wire.hpp; a
// developer's custom codec (e.g. examples/brake-ecu-native's BrakeCmdWireCodec) is a
// peer of those, implementing this same interface.
//
// Fault model: Drop is handled by RunProbe (it simply doesn't call Publish). Corrupt and
// Duplicate are wire-level, so the codec realizes them (a "corrupt" BrakeCmd differs from
// a corrupt JSON string), keeping the negative-path semantics type-appropriate.

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>

#include "behavior.hpp"  // sils::BehaviorOutput

namespace sils {

enum class WireFault { Normal, Corrupt, Duplicate };

// The L1 correlation key convention (part of the trace contract -- RunProbe emits it in
// request_sent/scenario_fault, the VISS envelope embeds it, a typed codec derives it from
// the sample's seq). Shared here so the write path, read path, and trace all agree.
inline std::string RequestId(uint64_t seq) { return "sils-" + std::to_string(seq); }

// One decoded inbound sample the read path reports. A codec fills the fields its type
// actually carries; those it has no notion of stay empty. The VISS response reader uses
// request_id/status/error_code; input staging only needs the arrival count (vector size).
struct WireInbound {
    std::string request_id;
    std::string status;
    std::string error_code;
};

class IWireCodec {
public:
    virtual ~IWireCodec() = default;
    // Register the wire DDS type on the participant (once, before any Publish/Take).
    virtual void RegisterType(eprosima::fastdds::dds::DomainParticipant* participant) = 0;
    // The registered_type name, for create_topic. Valid after RegisterType.
    virtual const char* type_name() const = 0;
    // WRITE path: encode `out` for this tick and write it on `writer` (created for
    // type_name()), realizing the wire-level fault. Normal: one write. Corrupt: one
    // malformed write. Duplicate: two identical writes.
    virtual void Publish(eprosima::fastdds::dds::DataWriter* writer, const BehaviorOutput& out,
                         uint64_t tick_seq, WireFault fault) = 0;
    // READ path (symmetry of Publish): take all currently-available samples off `reader`
    // (created for type_name()) and decode each VALID one into a WireInbound. The codec
    // owns the concrete typed take + field extraction; RunProbe's listeners handle the
    // trace and input-staging bookkeeping, so the read path names no concrete DDS type.
    virtual std::vector<WireInbound> Take(eprosima::fastdds::dds::DataReader* reader) = 0;
    virtual const char* name() const = 0;
};

}  // namespace sils

#endif  // SILS_VECU_WIRE_HPP
