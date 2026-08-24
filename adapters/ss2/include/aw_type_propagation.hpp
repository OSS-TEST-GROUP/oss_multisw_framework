// aw_type_propagation.hpp -- the Fast DDS 2.x spelling of "do not propagate TypeObject".
//
// SAME RULE as service/sils/vecu/src/sils_participant_qos.hpp, which carries the full
// reasoning and the measurement. Read that one first; this file exists only because the API
// differs between the two Fast DDS major lines that this project links at the same time:
//
//     Fast DDS 3.2  (service/sils/vecu)   participant property fastdds.type_propagation
//     Fast DDS 2.6  (this package)        TopicDataType::auto_fill_type_object / _information
//
// The 2.6 line has no such property -- passing it is silently ignored, which is the worst
// possible outcome for a rule whose whole point is that forgetting it FLOODS SOMEBODY
// ELSE'S LOG. Hence a named helper rather than a copied pair of lines.
//
// One-line recap of why: a Fast DDS participant carrying our canonical types, joining a
// domain where Cyclone participants are already running, makes all three Autoware partitions
// log `ddsi_xt_type_add_typeobj with invalid type object` -- +600 lines in a sub-second
// burst, +0 with this applied (measured 2026-08-11, re-measured with a control group
// 2026-08-13, domain 42).
//
// ⚠ This header used to end that sentence with "and segfaulted the decision partition".
// That is RETRACTED: re-measured over 500 s, nothing died -- not the partitions, not a
// freshly-joined Cyclone reader. Nor is the pollution specific to our types; it appears
// with no participant of ours on the domain. The rule is log hygiene, not availability,
// and it costs nothing (matching by type name was measured lossless at 50 Hz).
// docs/sils/evidence/our-typeobject-2026-08-13.md
//
// Apply to EVERY TypeSupport in this package, BEFORE register_type -- the flags are read
// when the type is registered, so setting them afterwards does nothing and looks fine.

#ifndef SILS_SS2_AW_TYPE_PROPAGATION_HPP
#define SILS_SS2_AW_TYPE_PROPAGATION_HPP

#include <fastdds/dds/topic/TypeSupport.hpp>

namespace sils::ss2 {

// BOTH flags are needed. auto_fill_type_object suppresses the TypeObject itself;
// auto_fill_type_information suppresses the TypeInformation that carries the
// TypeIdentifier in SEDP -- and it is the SEDP path the crash arrived on. Turning off
// only the first leaves the identifier on the wire and the reader still tries to resolve
// it via the type-lookup service.
inline void DisableTypePropagation(eprosima::fastdds::dds::TypeSupport& type)
{
    type->auto_fill_type_object(false);
    type->auto_fill_type_information(false);
}

}  // namespace sils::ss2

#endif  // SILS_SS2_AW_TYPE_PROPAGATION_HPP
