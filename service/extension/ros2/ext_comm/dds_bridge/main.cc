// dds_bridge entry-point.
//
// Wires DdsBridge to the full set of bare-DDS topics that the various
// HMI monitors (Dart) currently use — OtaManager, lifecycle_mgr,
// adsw_manager — then idles until SIGINT/SIGTERM. All forwarding logic
// lives in DdsBridge / ForwardListener; this file is the topic table +
// signal handling.

#include "dds_bridge.h"

#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <string>
#include <thread>

namespace {

std::atomic<bool> g_running{true};
void on_sigint(int) { g_running = false; }

}  // namespace

int main(int argc, char** argv) {
    std::string idl_root = "idl";
    if (argc > 1) idl_root = argv[1];

    DdsBridge bridge;
    if (!bridge.Initialize(idl_root)) return 1;

    // ros_to_raw=true  → subscribe rt/<topic> (ROS-canonical type) +
    //                    publish raw <topic> (IDL type).  Used for
    //                    ros_monitor → OtaManager (request/registration).
    // ros_to_raw=false → subscribe raw <topic> (IDL type) +
    //                    publish rt/<topic> (ROS-canonical type). Used
    //                    for OtaManager → ros_monitor (response/notify).
    auto add = [&](const std::string& struct_name,
                   const std::string& idl_file,
                   const std::string& topic_path,
                   bool ros_to_raw) -> bool {
        return bridge.AddPair({
            struct_name, idl_file,
            topic_path,
            "rt" + topic_path,
            "otamanager::" + struct_name,
            "otamanager::msg::dds_::" + struct_name + "_",
            ros_to_raw,
        });
    };

    const std::string actor_idl     = "ota_mgr/OtamActorServiceMsg.idl";
    const std::string client_idl    = "ota_mgr/OtamClientServiceMsg.idl";
    const std::string heartbeat_idl = "ota_mgr/NotifyOtaMgrHeartbeat.idl";
    bool ok = true;

    // ── otaactor_service: 6 request/response pairs (12 topics) ─────────────
    const std::string ap = "/mmf/otamanager/otaactor_service/";
    ok &= add("GetAppList",                   actor_idl, ap + "GetAppList",                   /*ros→raw*/true);
    ok &= add("GetAppListResponse",           actor_idl, ap + "GetAppListResponse",           /*raw→ros*/false);
    ok &= add("GetAppUpdateSettings",         actor_idl, ap + "GetAppUpdateSettings",         true);
    ok &= add("GetAppUpdateSettingsResponse", actor_idl, ap + "GetAppUpdateSettingsResponse", false);
    ok &= add("SetAppUpdateSettings",         actor_idl, ap + "SetAppUpdateSettings",         true);
    ok &= add("SetAppUpdateSettingsResponse", actor_idl, ap + "SetAppUpdateSettingsResponse", false);
    ok &= add("GetAppUpdateState",            actor_idl, ap + "GetAppUpdateState",            true);
    ok &= add("GetAppUpdateStateResponse",    actor_idl, ap + "GetAppUpdateStateResponse",    false);
    ok &= add("SetAppUpdate",                 actor_idl, ap + "SetAppUpdate",                 true);
    ok &= add("SetAppUpdateResponse",         actor_idl, ap + "SetAppUpdateResponse",         false);
    // Delete-client API surfaces from ota_monitor's tree trash-icon
    // (oss-agent3 client-list-persist round).
    ok &= add("DeleteOtaClientRegistration",         actor_idl, ap + "DeleteOtaClientRegistration",         /*ros→raw*/true);
    ok &= add("DeleteOtaClientRegistrationResponse", actor_idl, ap + "DeleteOtaClientRegistrationResponse", /*raw→ros*/false);

    // ── otaclient_service: registration + manager broadcasts (4 topics) ────
    const std::string cp = "/mmf/otamanager/otaclient_service/";
    ok &= add("SetOtaClientRegistration",         client_idl, cp + "SetOtaClientRegistration",         /*ros→raw*/true);
    ok &= add("SetOtaClientRegistrationResponse", client_idl, cp + "SetOtaClientRegistrationResponse", /*raw→ros*/false);
    ok &= add("NotifyOtaManagerReady",            client_idl, cp + "NotifyOtaManagerReady",            /*raw→ros*/false);
    ok &= add("NotifyAppUpdateStatus",            client_idl, cp + "NotifyAppUpdateStatus",            /*raw→ros*/false);

    // ── diagnostic_service: heartbeat (Manager → monitor only) ─────────────
    // Surfaces the periodic NotifyOtaMgrHeartbeat publish so the monitor's
    // Client/Manager Status card can show seq + ts. Without this entry the
    // bare-DDS publisher's type name doesn't line up with the rmw_fastrtps
    // subscriber rosbridge spawns, and the topic stays invisible to ros2.
    const std::string dp = "/mmf/otamanager/diagnostic_service/";
    ok &= add("NotifyOtaMgrHeartbeat", heartbeat_idl, dp + "NotifyOtaMgrHeartbeat", /*raw→ros*/false);

    // ── lifecyclemgr (F012, lfc_monitor) ───────────────────────────────────
    // lifecycle_mgr publishes/subscribes on raw DDS topics WITHOUT leading
    // slash (e.g. "mmf/lifecyclemgr/NotifyServiceState" — see EndPointFactory
    // in service/core/lifecycle_mgr/endpoint/). OtaManager's bridge uses
    // leading-slash topics so it can do `"rt" + topic_path`; here we
    // explicitly construct `rt/<topic>` ourselves so the wire topic on the
    // ROS-canonical side resolves to `/<topic>` from a roslibdart subscriber.
    //
    // Each lifecycle IDL file lives at <idl_root>/lifecycle/<Struct>.idl and
    // #includes LifecycleCommon.idl (sibling file). The dynamic-IDL parser
    // resolves includes relative to the .idl file's directory.
    auto add_lc = [&](const std::string& struct_name,
                      const std::string& idl_file,
                      const std::string& raw_topic_no_slash,
                      bool ros_to_raw) -> bool {
        return bridge.AddPair({
            struct_name,
            idl_file,
            raw_topic_no_slash,                                  // raw_topic
            "rt/" + raw_topic_no_slash,                          // ros_topic
            "lifecyclemgr::" + struct_name,                      // raw_type
            "lifecyclemgr::msg::dds_::" + struct_name + "_",     // ros_type
            ros_to_raw,
        });
    };

    const std::string lp = "mmf/lifecyclemgr/";
    // Notifications / responses: lifecycle_mgr → monitor (raw → ros)
    //
    // Single-aggregator policy (dual-responder fix): NotifyServiceState and
    // GetServiceStateResponse are PUBLISHED by each adsw_manager (ros2 + agl),
    // not by lifecycle_mgr — so with more than one adsw_manager on the domain
    // there are multiple publishers on these topics and the ForwardListener's
    // trust-on-first-use pin drops all but the first-seen, losing one variant's
    // data on the monitor side. We therefore do NOT bridge these per-responder
    // topics to the monitor. lifecycle_mgr is the SOLE publisher of the
    // aggregated LifecycleDiagReport (carries name/service_state/recovery_policy/
    // container_name for every registered service), which lfc_monitor's
    // _onDiagReport already consumes as its authoritative Services-panel source.
    // adsw_managers still publish Notify/GetServiceStateResponse on the raw DDS
    // topics; lifecycle_mgr subscribes and folds them into DiagReport.
    //   ok &= add_lc("NotifyServiceState",      "lifecycle/NotifyServiceState.idl",     lp + "NotifyServiceState",      false);
    //   ok &= add_lc("GetServiceStateResponse", "lifecycle/GetServiceStateRequest.idl", lp + "GetServiceStateResponse", false);
    ok &= add_lc("UpdateServiceListResponse","lifecycle/UpdateServiceListRequest.idl", lp + "UpdateServiceListResponse",/*raw→ros*/false);
    ok &= add_lc("StartServiceResponse",     "lifecycle/StartServiceRequest.idl",      lp + "StartServiceResponse",     /*raw→ros*/false);
    ok &= add_lc("StopServiceResponse",      "lifecycle/StopServiceRequest.idl",       lp + "StopServiceResponse",      /*raw→ros*/false);
    ok &= add_lc("SetServiceModeResponse",   "lifecycle/SetServiceModeRequest.idl",    lp + "SetServiceModeResponse",   /*raw→ros*/false);
    ok &= add_lc("LifecycleDiagReport",      "lifecycle/LifecycleDiagReport.idl",      lp + "LifecycleDiagReport",      /*raw→ros*/false);
    // Control commands: monitor → lifecycle_mgr (ros → raw)
    ok &= add_lc("GetServiceStateRequest",   "lifecycle/GetServiceStateRequest.idl",   lp + "GetServiceStateRequest",   /*ros→raw*/true);
    ok &= add_lc("StartServiceRequest",      "lifecycle/StartServiceRequest.idl",      lp + "StartServiceRequest",      /*ros→raw*/true);
    ok &= add_lc("StopServiceRequest",       "lifecycle/StopServiceRequest.idl",       lp + "StopServiceRequest",       /*ros→raw*/true);
    ok &= add_lc("SetServiceModeRequest",    "lifecycle/SetServiceModeRequest.idl",    lp + "SetServiceModeRequest",    /*ros→raw*/true);

    // ── F013: container actuator + ctrl/* gate surface (lfc_monitor) ───────
    //
    // Topology recap for the new toolbox:
    //   - adsw_manager  → lifecycle_mgr / monitor : NotifyContainerState
    //     (raw → ros, so a rosbridge subscriber on `/mmf/lifecyclemgr/
    //     NotifyContainerState` sees container start/stop transitions).
    //   - monitor → lifecycle_mgr : ctrl/* requests (ros → raw, monitor
    //     publishes on `rt/mmf/lifecyclemgr/ctrl/<cmd>`; bridge forwards
    //     to the bare-DDS topic lifecycle_mgr subscribes on).
    //   - lifecycle_mgr → monitor : ctrl/* responses (raw → ros). All
    //     five response topics share the IDL struct `CtrlResponse`, so
    //     `add_lc` is called five times with the same struct_name but
    //     different topic suffixes — FastDDS register_type is idempotent
    //     for same-name same-type re-registrations.
    //
    // StartContainerRequest / StopContainerRequest live on the lifecycle_mgr
    // ↔ adsw_manager link and are NOT exposed to the monitor (the monitor
    // talks ctrl/StartContainer instead). Same for the Start/StopContainer
    // Response on that link — only the ctrl/* response shape is bridged.

    // F008: container state → monitor. We bridge lifecycle_mgr's AGGREGATED
    // report (mmf/lifecyclemgr/ContainerStateReport) — NOT the per-adsw
    // mmf/lifecyclemgr/NotifyContainerState — so the monitor sees every
    // container (ros2 + agl) from a SINGLE publisher and the ForwardListener
    // trust-on-first-use never drops a second adsw_manager's containers.
    // The ROS-side topic the monitor subscribes stays unchanged
    // (rt/mmf/lifecyclemgr/NotifyContainerState) so lfc_monitor needs no change.
    ok &= bridge.AddPair({
        "NotifyContainerState",                          // struct_name
        "lifecycle/NotifyContainerState.idl",            // idl_file
        lp + "ContainerStateReport",                     // raw_topic (lifecycle_mgr agg, single pub)
        "rt/" + lp + "NotifyContainerState",             // ros_topic (monitor-facing, unchanged)
        "lifecyclemgr::NotifyContainerState",            // raw_type
        "lifecyclemgr::msg::dds_::NotifyContainerState_", // ros_type
        /*ros_to_raw*/false,                             // raw → ros
    });

    // monitor → lifecycle_mgr: ctrl/* command requests.
    const std::string cp_lc = "mmf/lifecyclemgr/ctrl/";
    ok &= add_lc("CtrlStartServiceRequest",    "lifecycle/LifecycleCtrlRequest.idl",
                 cp_lc + "StartService",       /*ros→raw*/true);
    ok &= add_lc("CtrlStopServiceRequest",     "lifecycle/LifecycleCtrlRequest.idl",
                 cp_lc + "StopService",        /*ros→raw*/true);
    ok &= add_lc("CtrlSetServiceModeRequest",  "lifecycle/LifecycleCtrlRequest.idl",
                 cp_lc + "SetServiceMode",     /*ros→raw*/true);
    ok &= add_lc("CtrlStartContainerRequest",  "lifecycle/LifecycleCtrlRequest.idl",
                 cp_lc + "StartContainer",     /*ros→raw*/true);
    ok &= add_lc("CtrlStopContainerRequest",   "lifecycle/LifecycleCtrlRequest.idl",
                 cp_lc + "StopContainer",      /*ros→raw*/true);

    // lifecycle_mgr → monitor: ctrl/* command responses (all CtrlResponse).
    ok &= add_lc("CtrlResponse",               "lifecycle/LifecycleCtrlRequest.idl",
                 cp_lc + "StartServiceResponse",     /*raw→ros*/false);
    ok &= add_lc("CtrlResponse",               "lifecycle/LifecycleCtrlRequest.idl",
                 cp_lc + "StopServiceResponse",      /*raw→ros*/false);
    ok &= add_lc("CtrlResponse",               "lifecycle/LifecycleCtrlRequest.idl",
                 cp_lc + "SetServiceModeResponse",   /*raw→ros*/false);
    ok &= add_lc("CtrlResponse",               "lifecycle/LifecycleCtrlRequest.idl",
                 cp_lc + "StartContainerResponse",   /*raw→ros*/false);
    ok &= add_lc("CtrlResponse",               "lifecycle/LifecycleCtrlRequest.idl",
                 cp_lc + "StopContainerResponse",    /*raw→ros*/false);

    // F014 follow-up: recovery policy gate
    ok &= add_lc("CtrlSetRecoveryPolicyRequest", "lifecycle/LifecycleCtrlRequest.idl",
                 cp_lc + "SetRecoveryPolicy",        /*ros→raw*/true);
    ok &= add_lc("CtrlResponse",               "lifecycle/LifecycleCtrlRequest.idl",
                 cp_lc + "SetRecoveryPolicyResponse", /*raw→ros*/false);

    if (!ok) {
        std::cerr << "[bridge] one or more pairs failed to register\n";
        return 1;
    }

    std::signal(SIGINT,  on_sigint);
    std::signal(SIGTERM, on_sigint);
    std::cout << "[bridge] running (idl_root=" << idl_root << "). Ctrl-C to exit.\n";
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    std::cout << "[bridge] exiting.\n";
    return 0;
}
