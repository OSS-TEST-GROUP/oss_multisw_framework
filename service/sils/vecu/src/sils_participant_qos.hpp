// sils_participant_qos.hpp -- the one participant-QoS rule every SiLS DDS process must obey.
//
// ★ WHY THIS EXISTS AS A SHARED HELPER AND NOT A COPIED LINE
//
// There are nine DomainParticipant creation sites under service/sils/vecu/ and six more in
// adapters/ss2/. The rule below has to hold at ALL of them, and the cost of missing one is
// not a degraded participant -- it is ANOTHER PROCESS CRASHING. A rule with that blast
// radius must have exactly one place to read and one place to change.
//
// ★ THE RULE: DO NOT PROPAGATE TypeObject
//
// MEASURED 2026-08-11 and RE-MEASURED WITH A CONTROL GROUP 2026-08-13, ROS domain 42,
// against Autoware partitions running Cyclone 0.10.5. Bring the partitions up FIRST, then
// join a Fast DDS participant carrying our canonical types, and all three partitions log
//
//     dq.builtin: type [COMPLETE 15a6f61913...]: ddsi_xt_type_add_typeobj
//                 with invalid type object
//
// +600 lines in a sub-second burst, carrying three TypeIdentifiers that were not on the
// domain before. Flip this one switch and the same rig logs +0. That is the whole basis
// for the rule, and it is a solid one.
//
// The failure lands on the READER, so a process that follows this rule protects its
// NEIGHBOURS, NOT ITSELF. Order decides whether it shows at all: with our writer already
// present when the Cyclone node starts, nothing is logged. That is why it looked
// intermittent for a week, and why an honest test starts the readers first.
//
// ★ WHAT THIS RULE DOES *NOT* CLAIM  (both halves were once written here and were wrong)
//
// (1) It does not prevent a crash. The 2026-08-11 text said this "SEGFAULTED THE DECISION
//     PARTITION". Re-measured over 500 s with the switch as the only variable: the three
//     partitions kept the SAME container IDs, and a freshly-joined rclcpp-Cyclone reader
//     (aw_vehicle_status_bridge -- the very process that segfaulted on 2026-08-10) also
//     survived, receiving 8941 samples. The segfaults in the 08-10/08-11 logs were real;
//     the attribution to TypeObject is not supported. Note the 08-11 rig had eight stray
//     run_partitions.sh runners on it, each holding an EXIT trap over the same containers.
//
// (2) It is not specific to our generated types. This file used to argue "MMF's raw Fast
//     DDS participants coexist without trouble, so it is specifically ours". Both halves
//     are false: unpatched MMF logs 1260 of these against the same partitions (08-12), and
//     224 appear with NO participant of ours on the domain at all (08-13), reported by
//     Autoware's own nodes. This error is endemic to this Cyclone/Autoware build; our types
//     are one contributor among several. Ours are the ones we can switch off.
//
// So the grade is LOG HYGIENE, not availability. Keep the rule anyway -- see the cost below,
// and note that this noise cost two days of misdiagnosis, twice.
// Full re-measurement: docs/sils/evidence/our-typeobject-2026-08-13.md
//
// ★ WHAT IT COSTS -- measured 2026-08-13: NOTHING
//
// Matching falls back to topic and type NAME; each side then deserialises with its own
// local type. Until 08-13 that was an assertion in a comment. Measured against a real
// subscriber, patched and unpatched, the reader saw the same 50 Hz with zero loss
// (4955 samples / 100 s patched, 8941 / 180 s unpatched -- both exactly the writer's rate).
//
// What we DO lose is remote type discovery and the runtime type-compatibility check. That
// check was never load-bearing here -- every participant on these contracts is ours and
// generated from idl/dds/sils/. But it is not free either: the vECU links Fast DDS 3.2
// while adapters/ss2 links 2.6, so the two sides ARE built separately, and an IDL drift
// between them would now deserialise silently instead of failing to match. Keep idl/dds/sils
// the single source and rebuild both trees together.
//
// ★ THE OTHER TREE
//
// adapters/ss2 links Fast DDS 2.6, which has no such property. There the equivalent is
// TopicDataType::auto_fill_type_object(false) + auto_fill_type_information(false), applied
// per TypeSupport before register_type -- see adapters/ss2/include/aw_type_propagation.hpp.
// Same rule, different spelling, because the API changed between the two major lines.
//
// Background on the failure mode from the other direction (matched but silently receiving
// nothing): docs/interop/popcorn-para-dds-interop.md §4.7 on feature/para-dds-interop,
// which is where the fastdds.type_propagation remedy was first verified.

#ifndef SILS_VECU_SILS_PARTICIPANT_QOS_HPP
#define SILS_VECU_SILS_PARTICIPANT_QOS_HPP

#include <cstdlib>
#include <cstring>

#include <fastdds/dds/domain/qos/DomainParticipantQos.hpp>

namespace sils {

// Apply to EVERY DomainParticipantQos in this tree, immediately before create_participant.
//
// Valid values of the property are enabled (default) | disabled | minimal_bandwidth |
// registration_only. "disabled" is the only one that puts nothing on the wire;
// minimal_bandwidth still propagates a TypeIdentifier, which is the part Cyclone chokes on.
//
// ★ SILS_TYPE_PROPAGATION -- the control-group escape hatch, and the only reason this rule
// is readable from outside a rebuild. The claim above ("segfaulted the decision partition")
// was written from a rig that could not be re-run without one, because the remedy shipped
// in the same commit as the observation: with it compiled in, every later run measures zero
// and proves nothing. An unfalsifiable safety claim is worth less than the measurement it
// came from. Set it to `enabled` to put the TypeObject back on the wire and reproduce.
//
// This is a TEST switch. It is deliberately loud in the "unsafe" direction only -- unset,
// empty, or any unrecognised value all mean disabled, so no deployment can lose the rule by
// typo. Never set it outside an isolated domain: the failure lands on OTHER processes.
inline void ApplySilsParticipantQos(eprosima::fastdds::dds::DomainParticipantQos& qos)
{
    const char* override_value = std::getenv("SILS_TYPE_PROPAGATION");
    const bool reenable = (override_value != nullptr) && (std::strcmp(override_value, "enabled") == 0);

    qos.properties().properties().emplace_back("fastdds.type_propagation",
                                               reenable ? "enabled" : "disabled");
}

}  // namespace sils

#endif  // SILS_VECU_SILS_PARTICIPANT_QOS_HPP
