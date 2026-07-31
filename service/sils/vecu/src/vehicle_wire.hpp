#ifndef SILS_VECU_VEHICLE_WIRE_HPP
#define SILS_VECU_VEHICLE_WIRE_HPP

// Vehicle plant-coupling codec -- (de)serializes the canonical STATE/ACTUATION structs
// for MORAI/AWSIM (Option 2, bus-coupled external world). docs/sils/sils-morai-plant-adapter-
// design.md; types in idl/dds/sils/{EgoState,VehicleControl}.idl.
//
// This is NOT an IWireCodec. IWireCodec (wire.hpp) is the VISS single-type request model:
// its Publish takes a scalar BehaviorOutput and its Take yields a WireInbound
// {request_id,status,error_code}. Neither can carry a structured EgoState / VehicleControl.
// So the vehicle path uses a purpose-built codec that translates between the generated DDS
// types and PLAIN SDK-level PODs (EgoStateSample / VehicleControlSample) -- the same reason
// run_loop names no concrete DDS type (T1-A): behavior/run_loop consume the PODs, and only
// THIS header (and the consumer that links sils_vehicle_idl) touches the generated types.
//
// A sim adapter is the anti-corruption layer that fills EgoState in canonical SI (deg->rad,
// km/h->m/s, ...) BEFORE it reaches the bus; this codec assumes canonical values and does
// no unit conversion (that belongs to the adapter, not the consumer). See the IDL headers.
//
// Header-only, same shape as viss_wire.hpp; needs the generated EgoState/VehicleControl
// PubSubTypes (compiled into sils_vehicle_idl), so only a MORAI-mode consumer includes it.

#include <cstdint>
#include <string>
#include <vector>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

#include "ControlTargetPubSubTypes.hpp"
#include "EgoStatePubSubTypes.hpp"
#include "VehicleControlPubSubTypes.hpp"
#include "vehicle_sample.hpp"  // EgoStateSample / VehicleControlSample / ControlTargetSample (DDS-free)

namespace sils {

// ── Codec ────────────────────────────────────────────────────────────────────
// Registers both canonical types on the participant, decodes inbound EgoState samples
// (read path, the `in` channel) and encodes VehicleControl samples (write path, the `out`
// channel). run_id stamps every published command so a verifier can correlate to the run.
class VehicleWireCodec {
public:
    explicit VehicleWireCodec(std::string run_id) : run_id_(std::move(run_id)) {}

    // Register EgoState + VehicleControl once, before any reader/writer is created.
    void RegisterTypes(eprosima::fastdds::dds::DomainParticipant* participant) {
        state_type_ = eprosima::fastdds::dds::TypeSupport(new EgoStatePubSubType());
        state_type_.register_type(participant);
        state_type_name_ = state_type_.get_type_name();

        control_type_ = eprosima::fastdds::dds::TypeSupport(new VehicleControlPubSubType());
        control_type_.register_type(participant);
        control_type_name_ = control_type_.get_type_name();

        target_type_ = eprosima::fastdds::dds::TypeSupport(new ControlTargetPubSubType());
        target_type_.register_type(participant);
        target_type_name_ = target_type_.get_type_name();
    }

    // Registered type names, for create_topic on the resolved channel topics.
    const char* state_type_name() const { return state_type_name_.c_str(); }
    const char* control_type_name() const { return control_type_name_.c_str(); }
    const char* control_target_type_name() const { return target_type_name_.c_str(); }

    // READ path (`in` channel): take all available EgoState samples and decode each valid
    // one into an EgoStateSample POD. Same take-loop mechanics as viss_wire's Take.
    std::vector<EgoStateSample> TakeState(eprosima::fastdds::dds::DataReader* reader) {
        std::vector<EgoStateSample> out;
        EgoState s;
        eprosima::fastdds::dds::SampleInfo info;
        while (reader->take_next_sample(&s, &info) == eprosima::fastdds::dds::RETCODE_OK) {
            if (!info.valid_data) continue;
            EgoStateSample e;
            e.seq = s.seq();
            e.stamp_ns = s.stamp_ns();
            e.px = s.position().x();  e.py = s.position().y();  e.pz = s.position().z();
            e.roll = s.orientation_rpy().x();  e.pitch = s.orientation_rpy().y();
            e.yaw = s.orientation_rpy().z();
            e.vx = s.velocity().x();  e.vy = s.velocity().y();  e.vz = s.velocity().z();
            e.ax = s.acceleration().x();  e.ay = s.acceleration().y();  e.az = s.acceleration().z();
            e.yaw_rate = s.yaw_rate();
            e.steering_angle = s.steering_angle();
            e.gear = s.gear();
            out.push_back(e);
        }
        return out;
    }

    // READ path (`in` channel, Profile B): take ControlTarget samples (Autoware target via
    // the bridge) and decode each valid one into a ControlTargetSample POD.
    std::vector<ControlTargetSample> TakeControlTarget(eprosima::fastdds::dds::DataReader* reader) {
        std::vector<ControlTargetSample> out;
        ControlTarget s;
        eprosima::fastdds::dds::SampleInfo info;
        while (reader->take_next_sample(&s, &info) == eprosima::fastdds::dds::RETCODE_OK) {
            if (!info.valid_data) continue;
            ControlTargetSample t;
            t.seq = s.seq();
            t.stamp_ns = s.stamp_ns();
            t.steering_angle = s.steering_angle();
            t.acceleration = s.acceleration();
            t.gear = s.gear();
            out.push_back(t);
        }
        return out;
    }

    // WRITE path (`out` channel): encode a VehicleControlSample and write it. seq/stamp are
    // the caller's (tick) values; run_id is stamped from construction.
    void PublishControl(eprosima::fastdds::dds::DataWriter* writer,
                        const VehicleControlSample& v) {
        VehicleControl m;
        m.run_id(run_id_);
        m.seq(v.seq);
        m.stamp_ns(v.stamp_ns);
        m.steering_angle(v.steering_angle);
        m.acceleration(v.acceleration);
        m.gear(v.gear);
        writer->write(&m);
    }

private:
    std::string run_id_;
    eprosima::fastdds::dds::TypeSupport state_type_;
    eprosima::fastdds::dds::TypeSupport control_type_;
    eprosima::fastdds::dds::TypeSupport target_type_;
    std::string state_type_name_;
    std::string control_type_name_;
    std::string target_type_name_;
};

}  // namespace sils

#endif  // SILS_VECU_VEHICLE_WIRE_HPP
