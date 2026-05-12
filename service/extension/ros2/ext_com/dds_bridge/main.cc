// ota_dds_bridge entry-point.
//
// Wires OtaBridge to the full set of OtaManager topics that
// ota_monitor (Dart) currently uses, then idles until SIGINT/SIGTERM.
// All forwarding logic lives in OtaBridge / ForwardListener; this file
// is the topic table + signal handling.

#include "ota_bridge.h"

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

    OtaBridge bridge;
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

    const std::string actor_idl  = "ota_mgr/OtamActorServiceMsg.idl";
    const std::string client_idl = "ota_mgr/OtamClientServiceMsg.idl";
    bool ok = true;

    // ── otaactor_service: 5 request/response pairs (10 topics) ─────────────
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

    // ── otaclient_service: registration + manager broadcasts (4 topics) ────
    const std::string cp = "/mmf/otamanager/otaclient_service/";
    ok &= add("SetOtaClientRegistration",         client_idl, cp + "SetOtaClientRegistration",         /*ros→raw*/true);
    ok &= add("SetOtaClientRegistrationResponse", client_idl, cp + "SetOtaClientRegistrationResponse", /*raw→ros*/false);
    ok &= add("NotifyOtaManagerReady",            client_idl, cp + "NotifyOtaManagerReady",            /*raw→ros*/false);
    ok &= add("NotifyAppUpdateStatus",            client_idl, cp + "NotifyAppUpdateStatus",            /*raw→ros*/false);

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
