#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <mutex>
#include <sstream>
#include <iomanip>
#include <map>
#include <csignal>
#include <condition_variable>
#include <atomic>
#include <zlib.h>
#include <optional>
#include "GrpcVissClient.h"
#include "RestHttpClient.h"
#include "Utils.h"
#include "Logger.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

const std::vector<std::string> G_VEHICLE_PATHS = {
    "Vehicle.ADS.Safety.MRM.Status",
    "Vehicle.ADS.Safety.MRM.Behavior",
    "Vehicle.ADS.Safety.HazardStatus.Level",
    "Vehicle.ADS.Safety.HazardStatus.IsEmergency",
    "Vehicle.ADS.Safety.HazardStatus.NoFault",
    "Vehicle.ADS.Safety.HazardStatus.SafeFault",
    "Vehicle.ADS.Safety.HazardStatus.LatentFault",
    "Vehicle.ADS.Safety.HazardStatus.SinglePointFault",
    "Vehicle.ADS.OperationMode.Availability.IsStop",
    "Vehicle.ADS.OperationMode.Availability.IsAutonomous",
    "Vehicle.ADS.OperationMode.Availability.IsLocal",
    "Vehicle.ADS.OperationMode.Availability.IsRemote",
    "Vehicle.ADS.OperationMode.Availability.IsEmergencyStop",
    "Vehicle.ADS.OperationMode.Availability.IsComfortableStop",
    "Vehicle.ADS.OperationMode.Availability.IsPullOver",
    "Vehicle.ADS.OperationMode.State.Mode",
    "Vehicle.ADS.OperationMode.State.IsAutowareControlEnabled",
    "Vehicle.ADS.OperationMode.State.IsInTransition",
    "Vehicle.ADS.Perception.DetectedObject.ObjectCount",
    "Vehicle.ADS.Perception.DetectedObject.Objects",
    "Vehicle.ADS.Perception.DynamicObject.ObjectCount",
    "Vehicle.ADS.Perception.DynamicObject.Objects",
    "Vehicle.ADS.Perception.TrafficLight.LightCount",
    "Vehicle.ADS.Perception.TrafficLight.Lights",
    "Vehicle.ADS.Planning.Trajectory.PointCount",
    "Vehicle.ADS.Planning.Trajectory.Points",
    "Vehicle.ADS.Planning.Path",
    "Vehicle.ADS.Planning.VelocityFactor.Count",
    "Vehicle.ADS.Planning.VelocityFactor.Factors",
    "Vehicle.ADS.Planning.SteeringFactor.Count",
    "Vehicle.ADS.Planning.SteeringFactor.Factors",
    "Vehicle.ADS.IsEmergency",
    "Vehicle.ADS.Route.RouteState",
    "Vehicle.ADS.Route.Start",
    "Vehicle.ADS.Route.Goal",
    "Vehicle.ADS.Route.Segments",
    "Vehicle.ADS.VehicleKinematics.GeographicPose",
    "Vehicle.ADS.VehicleKinematics.Pose",
    "Vehicle.ADS.VehicleKinematics.Twist",
    "Vehicle.ADS.VehicleKinematics.Accel",
    "Vehicle.ADS.Diagnostics.Count",
    "Vehicle.ADS.Diagnostics.GraphStatus",
    "Vehicle.ADS.Diagnostics.GraphStruct",
    "Vehicle.CurrentLocation.Altitude",
    "Vehicle.CurrentLocation.Latitude",
    "Vehicle.CurrentLocation.Longitude",
    "Vehicle.CurrentLocation.Heading",
    "Vehicle.Acceleration.Lateral",
    "Vehicle.Acceleration.Longitudinal",
    "Vehicle.Acceleration.Vertical",
    "Vehicle.AngularVelocity.Pitch",
    "Vehicle.AngularVelocity.Roll",
    "Vehicle.AngularVelocity.Yaw",
    "Vehicle.Speed",
    "Vehicle.Body.Lights.DirectionIndicator.Left.IsSignaling",
    "Vehicle.Body.Lights.DirectionIndicator.Right.IsSignaling",
    "Vehicle.Body.Lights.Hazard.IsSignaling",
    "Vehicle.Powertrain.Transmission.SelectedGear"
};

struct SignalData {
    std::string timestamp;
    std::map<std::string, std::string> signals;
};

std::atomic<bool> g_running(true);
std::condition_variable g_cv;
std::mutex g_cv_m;

void signal_handler(int signal) {
    LOG_INFO << "Received signal " << signal << ". Stopping VehicleInfoManager...";
    g_running = false;
    g_cv.notify_all();
}

class VehicleInfoManager {
public:
    VehicleInfoManager(const std::string& grpcAddr, const std::string& restUrl)
        : m_grpcAddr(grpcAddr), m_restUrl(restUrl), m_stopFlag(false) {}

    void start() {
        m_client.setEventCallback([this](const std::string& event) {
            handleEvent(event);
        });

        m_client.setSubscriptionCallback([this](const std::string& path, const std::string& subId) {
            std::lock_guard<std::mutex> lock(m_subMutex);
            m_activeSubscriptions[path] = subId;
            LOG_INFO << "[VehicleInfoManager] Subscription confirmed. Path: " << path << ", SubID: " << subId;
        });

        m_client.start(m_grpcAddr);

        // Subscribe to standard VISS signals
        for (const auto& path : G_VEHICLE_PATHS) {
            std::string req = "{\"action\": \"subscribe\", \"path\": \"" + path + "\", \"filter\": {\"variant\": \"timebased\", \"parameter\": \"1000\"}}";
            m_client.sendRequest(req);
        }

        m_collectionThread = std::thread(&VehicleInfoManager::collectionLoop, this);
        LOG_INFO << "VehicleInfoManager started.";
    }

    void stop() {
        m_stopFlag = true;
        if (m_collectionThread.joinable()) {
            m_collectionThread.join();
        }

        // Unsubscribe all active subscriptions
        {
            std::lock_guard<std::mutex> lock(m_subMutex);
            for (const auto& pair : m_activeSubscriptions) {
                std::string req = "{\"action\": \"unsubscribe\", \"subscriptionId\": \"" + pair.second + "\"}";
                m_client.sendRequest(req);
                LOG_INFO << "[VehicleInfoManager] Unsubscribed from " << pair.first << " (SubID: " << pair.second << ")";
            }
            m_activeSubscriptions.clear();
        }

        m_client.stop();
        LOG_INFO << "VehicleInfoManager stopped.";
    }

private:
    void handleEvent(const std::string& eventJson) {
        LOG_INFO << "[VehicleInfoManager] Received subscription event. : " << eventJson;
        rapidjson::Document doc;
        doc.Parse(eventJson.c_str());
        if (doc.HasParseError() || !doc.IsObject() || !doc.HasMember("data")) {
            LOG_ERROR << "[VehicleInfoManager] Failed to parse event JSON or missing data key.";
            return;
        }

        const rapidjson::Value& data = doc["data"];
        if (data.IsArray()) {
            for (auto& d : data.GetArray()) {
                processDataNode(d);
            }
        } else if (data.IsObject()) {
            processDataNode(data);
        }
    }

    void processDataNode(const rapidjson::Value& d) {
        if (!d.IsObject() || !d.HasMember("path") || !d.HasMember("dp")) return;
        
        std::string path = d["path"].GetString();
        const rapidjson::Value& dp = d["dp"];

        if (dp.IsArray()) {
            for (auto& p : dp.GetArray()) {
                processDataPoint(path, p);
            }
        } else if (dp.IsObject()) {
            processDataPoint(path, dp);
        }
    }

    void processDataPoint(const std::string& path, const rapidjson::Value& p) {
        if (!p.IsObject() || !p.HasMember("value") || !p.HasMember("ts")) return;

        std::string value;
        if (p["value"].IsString()) {
            value = p["value"].GetString();
        } else {
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            p["value"].Accept(writer);
            value = buffer.GetString();
        }
        std::string ts = p["ts"].GetString();

        std::lock_guard<std::mutex> lock(m_dataMutex);
        m_currentData.timestamp = ts;
        m_currentData.signals[path] = value;
        LOG_INFO << "[VehicleInfoManager] Received " << path << ": " << value.substr(0,10) << " at " << ts;

        m_buffer.push_back(m_currentData);
    }

    void collectionLoop() {
        while (!m_stopFlag) {
            std::this_thread::sleep_for(std::chrono::seconds(10));
            sendBufferedData();
        }
    }

    std::string escapeCsv(const std::string& field) {
        if (field.find(',') == std::string::npos &&
            field.find('"') == std::string::npos &&
            field.find('\n') == std::string::npos &&
            field.find('\r') == std::string::npos) {
            return field;
        }

        std::string escaped = "\"";
        for (char c : field) {
            if (c == '"') {
                escaped += "\"\"";
            } else {
                escaped += c;
            }
        }
        escaped += "\"";
        return escaped;
    }

    void sendBufferedData() {
        std::vector<SignalData> dataToSend;
        {
            std::lock_guard<std::mutex> lock(m_dataMutex);
            if (m_buffer.empty()) return;
            dataToSend = std::move(m_buffer);
            m_buffer.clear();
        }

        LOG_INFO << "Preparing to send " << dataToSend.size() << " data points.";

        // 1. Generate CSV
        std::stringstream ss;
        ss << "Timestamp";
        for (const auto& path : G_VEHICLE_PATHS) {
            ss << "," << path;
        }
        ss << "\n";

        for (const auto& d : dataToSend) {
            ss << escapeCsv(d.timestamp);
            for (const auto& path : G_VEHICLE_PATHS) {
                auto it = d.signals.find(path);
                if (it != d.signals.end()) {
                    ss << "," << escapeCsv(it->second);
                } else {
                    ss << ",";
                }
            }
            ss << "\n";
        }
        std::string csv = ss.str();

        // 2. Gzip Compress
        std::vector<char> gzipped = gzipCompress(csv);

        // 3. Send via REST
        if (m_restClient.postBinary(m_restUrl, gzipped.data(), gzipped.size(), "obigo2025")) {
            LOG_INFO << "Data sent successfully.";
        } else {
            LOG_ERROR << "Failed to send data.";
        }
    }

    std::vector<char> gzipCompress(const std::string& data) {
        z_stream zs;
        memset(&zs, 0, sizeof(zs));

        if (deflateInit2(&zs, Z_BEST_COMPRESSION, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
            return {};
        }

        zs.next_in = (Bytef*)data.data();
        zs.avail_in = data.size();

        int ret;
        char outbuffer[32768];
        std::vector<char> outstring;

        do {
            zs.next_out = reinterpret_cast<Bytef*>(outbuffer);
            zs.avail_out = sizeof(outbuffer);

            ret = deflate(&zs, Z_FINISH);

            if (outstring.size() < zs.total_out) {
                outstring.insert(outstring.end(), outbuffer, outbuffer + (zs.total_out - outstring.size()));
            }
        } while (ret == Z_OK);

        deflateEnd(&zs);

        if (ret != Z_STREAM_END) {
            return {};
        }

        return outstring;
    }

    std::string m_grpcAddr;
    std::string m_restUrl;
    GrpcVissClient m_client;
    RestHttpClient m_restClient;
    
    std::vector<SignalData> m_buffer;
    SignalData m_currentData;
    std::mutex m_dataMutex;
    
    std::thread m_collectionThread;
    bool m_stopFlag;

    std::map<std::string, std::string> m_activeSubscriptions; // path -> subId
    std::mutex m_subMutex;
};

std::optional<std::string> getEnv(const std::string& key) {
    const char* val = std::getenv(key.c_str());
    return val ? std::optional<std::string>(val) : std::nullopt;
}


int main(int argc, char** argv) {
    std::string grpcAddr = "localhost:5443";
    //std::string restUrl  = "http://localhost:8080/upload";

    std::string restUrl = getEnv("REST_URL").value_or("http://localhost:8080/upload");

    // Parse command-line options
    // Usage: vehicle_info_manager [options]
    //   -g, --grpc-addr <addr>  gRPC server address (default: localhost:5443)
    //   -r, --rest-url  <url>   REST upload URL    (default: http://localhost:8080/upload)
    //   -h, --help              Show this help message
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if ((arg == "-g" || arg == "--grpc-addr") && i + 1 < argc) {
            grpcAddr = argv[++i];
        } else if ((arg == "-r" || arg == "--rest-url") && i + 1 < argc) {
            restUrl = argv[++i];
        } else if (arg == "-h" || arg == "--help") {
            std::cout << "Usage: vehicle_info_manager [options]\n"
                      << "  -g, --grpc-addr <addr>  gRPC server address (default: " << grpcAddr << ")\n"
                      << "  -r, --rest-url  <url>   REST upload URL    (default: " << restUrl  << ")\n"
                      << "  -h, --help              Show this help message\n";
            return 0;
        } else {
            std::cerr << "Unknown option: " << arg << "\n"
                      << "Use --help to see available options.\n";
            return 1;
        }
    }

    LOG_INFO << "grpcAddr : " << grpcAddr;
    LOG_INFO << "restUrl  : " << restUrl;

    VehicleInfoManager manager(grpcAddr, restUrl);
    manager.start();

    // Register signal handlers
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    LOG_INFO << "VehicleInfoManager is running. Press Ctrl+C to exit...";

    // Wait for SIGINT or SIGTERM
    std::unique_lock<std::mutex> lk(g_cv_m);
    g_cv.wait(lk, []{ return !g_running; });

    manager.stop();
    LOG_INFO << "VehicleInfoManager has shut down gracefully.";
    return 0;
}

