#include <libwebsockets.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <string>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

// Path list from user
static const std::vector<std::string> VISS_PATHS = {
    "Vehicle.Speed", "Vehicle.Powertrain.CombustionEngine.Speed", "Vehicle.Chassis.SteeringWheel.Angle",
    "Vehicle.Chassis.Brake.PedalPosition", "Vehicle.Acceleration.Longitudinal", "Vehicle.Acceleration.Lateral",
    "Vehicle.Acceleration.Vertical", "Vehicle.AngularVelocity.Yaw", "Vehicle.AngularVelocity.Roll", "Vehicle.AngularVelocity.Pitch",
    "Vehicle.CurrentLocation.Latitude", "Vehicle.CurrentLocation.Longitude", "Vehicle.CurrentLocation.Altitude",
    "Vehicle.CurrentLocation.Heading", "Vehicle.Body.Lights.DirectionIndicator.Left.IsSignaling",
    "Vehicle.Body.Lights.DirectionIndicator.Right.IsSignaling", "Vehicle.Body.Lights.Backup.IsOn",
    "Vehicle.Body.Windshield.Front.Wiping.System.IsWiping", "Vehicle.Powertrain.Transmission.TravelledDistance",
    "Vehicle.StartTime", "Vehicle.Chassis.Accelerator.PedalPosition", "Vehicle.Powertrain.FuelSystem.RelativeLevel",
    "Vehicle.Exterior.AirTemperature", "Vehicle.CurrentLocation.SpeedLimit", "Vehicle.TripDuration",
    "Vehicle.Powertrain.Transmission.SelectedGear", "Vehicle.Powertrain.Transmission.CurrentGear",
    "Vehicle.ADAS.AEB.WarningLevel", "Vehicle.Powertrain.DriveTrain.DriveMode", "Vehicle.Powertrain.DriveTrain.WheelDriveMode", 
    "Vehicle.Powertrain.DriveTrain.CouplingOverheat", "Vehicle.Powertrain.DriveTrain.AWD.IsError", "Vehicle.Powertrain.DriveTrain.AWD.Torque",
    "Vehicle.Chassis.SteeringWheel.RotationSpeed", "Vehicle.LowVoltageBattery.CurrentRange",
    "Vehicle.LowVoltageBattery.BscInvFlg", "Vehicle.LowVoltageBattery.CurrentTemperature", "Vehicle.LowVoltageBattery.StateofCharge", "Vehicle.LowVoltageBattery.StateofHealth",
    "Vehicle.Diagnostics.AbsCan", "Vehicle.Diagnostics.AcuCan", "Vehicle.Diagnostics.EmsCan", "Vehicle.Diagnostics.TcuCan",
    "Vehicle.Diagnostics.HvacCan", "Vehicle.Diagnostics.BsdCan", "Vehicle.Diagnostics.EpsCan", "Vehicle.Diagnostics.FcmCan",
    "Vehicle.Diagnostics.FrmCan", "Vehicle.Diagnostics.CgwCan", "Vehicle.Diagnostics.BcmCan", "Vehicle.Diagnostics.PgtmCan",
    "Vehicle.Diagnostics.SkmCan", "Vehicle.Diagnostics.WpcCan", "Vehicle.TraveledTimeSinceStart",
    "Vehicle.Powertrain.FuelSystem.AverageConsumption", "Vehicle.Diagnostics.SbwsCan",
    "Vehicle.Diagnostics.DkuCan", "Vehicle.Diagnostics.SwvmCan", "Vehicle.Diagnostics.SbwaCan",
    "Vehicle.Chassis.Axle.Row1.Wheel.Left.Tire.PressureStatus", "Vehicle.Chassis.Axle.Row1.Wheel.Right.Tire.PressureStatus",
    "Vehicle.Chassis.Axle.Row2.Wheel.Left.Tire.PressureStatus", "Vehicle.Chassis.Axle.Row2.Wheel.Right.Tire.PressureStatus",
    "Vehicle.VehicleIdentification.VIN", "Vehicle.Powertrain.CombustionEngine.IsRunning",
    "Vehicle.Powertrain.CombustionEngine.Torque",
    "Vehicle.Powertrain.CombustionEngine.ECT", "Vehicle.Powertrain.FuelSystem.ConsumptionSinceStart",
    "Vehicle.ADAS.ABS.IsEngaged", "Vehicle.ADAS.TCS.IsEngaged",
    "Vehicle.ADAS.ESC.IsEngaged", "Vehicle.Chassis.Brake.IsDriverEmergencyBrakingDetected",
    "Vehicle.Body.Lights.Brake.IsActive",
    "Vehicle.Powertrain.Transmission.Temperature", "Vehicle.MotionManagement.Steering.SteeringWheel.Torque",
    "Vehicle.MotionManagement.Steering.SteeringWheel.Angle", "Vehicle.Cabin.Infotainment.HMI.SpeedUnit",
    "Vehicle.Powertrain.FuelSystem.IsFuelLevelLow", "Vehicle.Powertrain.FuelSystem.AbsoluteLevel",
    "Vehicle.TraveledDistance", "Vehicle.Cabin.Infotainment.HMI.DistanceUnit",
    "Vehicle.Powertrain.FuelSystem.Range", "Vehicle.LowVoltageBattery.CurrentCurrent",
    "Vehicle.LowVoltageBattery.CurrentVoltage", "Vehicle.Cabin.Seat.Row1.DriverSide.IsBelted",
    "Vehicle.Chassis.Axle.Row1.Wheel.Left.Tire.IsPressureLow",
    "Vehicle.Chassis.Axle.Row1.Wheel.Right.Tire.IsPressureLow",
    "Vehicle.Chassis.Axle.Row2.Wheel.Left.Tire.IsPressureLow",
    "Vehicle.Chassis.Axle.Row2.Wheel.Right.Tire.IsPressureLow",
    "Vehicle.AverageSpeed", "Vehicle.TraveledDistanceSinceStart",
    "Vehicle.IsBrokenDown", "Vehicle.CurrentLocation.Timestamp", "Vehicle.Cabin.Cluster.Speed",
    "Vehicle.Powertrain.FuelSystem.AverageFuelEfficiency"
};

struct per_session_data_wss {
    struct lws *wsi;
    size_t path_index;
    bool all_subscribed;
};

static int callback_wss(struct lws *wsi, enum lws_callback_reasons reason,
                        void *user, void *in, size_t len) {
    struct per_session_data_wss *pss = (struct per_session_data_wss *)user;

    switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            pss->path_index = 0;
            pss->all_subscribed = false;
            std::cout << "--- WSS Connection Established (VISSv3) ---" << std::endl;
            lws_callback_on_writable(wsi);
            break;

        case LWS_CALLBACK_CLIENT_WRITEABLE: {
            if (pss->path_index >= VISS_PATHS.size()) {
                if (!pss->all_subscribed) {
                    std::cout << "--- All subscription requests sent. Waiting for notifications... ---" << std::endl;
                    pss->all_subscribed = true;
                }
                break;
            }

            // Prepare a VISS SUBSCRIBE request with 'changed' filter
            rapidjson::Document doc;
            doc.SetObject();
            auto& allocator = doc.GetAllocator();
            
            doc.AddMember("action", "subscribe", allocator);
            doc.AddMember("path", rapidjson::Value(VISS_PATHS[pss->path_index].c_str(), allocator), allocator);
            
            rapidjson::Value filter(rapidjson::kObjectType);
            filter.AddMember("variant", "timebased", allocator);
            
            rapidjson::Value parameter(rapidjson::kObjectType);
            parameter.AddMember("period", "300", allocator);
            filter.AddMember("parameter", parameter, allocator);
            
            doc.AddMember("filter", filter, allocator);
            
            doc.AddMember("requestId", rapidjson::Value(("req-" + std::to_string(pss->path_index)).c_str(), allocator), allocator);

            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            doc.Accept(writer);
            std::string payload = buffer.GetString();

            std::cout << "[" << pss->path_index + 1 << "/" << VISS_PATHS.size() << "] Subscribing to: " << VISS_PATHS[pss->path_index] << std::endl;

            unsigned char buf[LWS_PRE + 1024];
            unsigned char *p = &buf[LWS_PRE];
            memcpy(p, payload.c_str(), payload.length());

            lws_write(wsi, p, payload.length(), LWS_WRITE_TEXT);
            
            pss->path_index++;
            // Request another writeable callback to send the next subscription immediately
            if (pss->path_index < VISS_PATHS.size()) {
                lws_callback_on_writable(wsi);
            }
            break;
        }

        case LWS_CALLBACK_CLIENT_RECEIVE: {
            std::string msg((const char *)in, len);
            std::cout << "\n[Notification/Response] " << msg << std::endl;
            // DO NOT cancel service, keep listening
            break;
        }

        case LWS_CALLBACK_CLIENT_CLOSED:
            std::cout << "--- WSS Connection Closed ---" << std::endl;
            break;

        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            std::cerr << "--- WSS Connection Error: " << (in ? (char *)in : "unknown") << " ---" << std::endl;
            break;

        default:
            break;
    }

    return 0;
}

static const struct lws_protocols protocols[] = {
    { "VISSv3", callback_wss, sizeof(struct per_session_data_wss), 0 },
    { NULL, NULL, 0, 0 }
};

int main(int argc, char **argv) {
    struct lws_context_creation_info info;
    struct lws_client_connect_info i;
    struct lws_context *context;
    const char *url = "localhost";
    int port = 6443;

    memset(&info, 0, sizeof(info));
    info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
    info.protocols = protocols;
    info.port = CONTEXT_PORT_NO_LISTEN;

    context = lws_create_context(&info);
    if (!context) {
        std::cerr << "Failed to create LWS context" << std::endl;
        return 1;
    }

    memset(&i, 0, sizeof(i));
    i.context = context;
    i.address = url;
    i.port = port;
    i.path = "/";
    i.host = i.address;
    i.origin = i.address;
    i.protocol = protocols[0].name;
    
    // Skip SSL verification for self-signed certificates
    i.ssl_connection = LCCSCF_USE_SSL | LCCSCF_ALLOW_SELFSIGNED | LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK;

    std::cout << "Connecting to WSS " << url << ":" << port << "..." << std::endl;
    if (!lws_client_connect_via_info(&i)) {
        std::cerr << "Failed to initiate WSS connection" << std::endl;
        lws_context_destroy(context);
        return 1;
    }

    // Main loop: keep running forever (or until interrupted)
    while (lws_service(context, 100) >= 0);

    lws_context_destroy(context);
    std::cout << "Test completed." << std::endl;

    return 0;
}
