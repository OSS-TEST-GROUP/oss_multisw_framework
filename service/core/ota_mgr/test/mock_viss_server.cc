/**
 * @file mock_viss_server.cc
 * @brief Mock VISS Server for testing OtamVehicleService
 *
 * This mock server subscribes to VISS requests and responds with vehicle data
 * read from a YAML configuration file.
 */

#include "otam_common/otam_dds_impl.h"
#include "vehicle_info_mgr/VissRequest.hpp"
#include "vehicle_info_mgr/VissRequestPubSubTypes.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <map>
#include <thread>
#include <chrono>
#include <fstream>
#include <sstream>
#include <csignal>
#include <nlohmann/json.hpp>
#include <yaml-cpp/yaml.h>

/**
 * @brief Mock VISS Server that responds to vehicle info requests
 */
class MockVissServer {
 public:
  MockVissServer(const std::string& config_file)
      : config_file_(config_file), running_(false) {
    std::cout << "=== Mock VISS Server ===" << std::endl;
    std::cout << "Config file: " << config_file << std::endl;
  }

  ~MockVissServer() {
    Stop();
  }

  /**
   * @brief Initialize the server and load vehicle data
   */
  bool Initialize() {
    std::cout << "\n[MockVissServer] Initializing..." << std::endl;

    // Load vehicle data from YAML file
    if (!LoadVehicleData()) {
      std::cerr << "Failed to load vehicle data from: " << config_file_ << std::endl;
      return false;
    }

    std::cout << "Loaded " << vehicle_data_.size() << " vehicle data entries" << std::endl;

    // Initialize DDS topics
    try {
      // Create subscriber for VISS requests
      auto request_type = std::make_shared<VissRequestTopicPubSubType>();
      auto request_listener = std::make_shared<VissRequestListener>(this);

      request_subscriber_ = std::make_unique<OtamDdsImpl<VissRequestTopic>>();
      request_subscriber_->Create(kVissRequestTopic, request_type, request_listener);
      // Note: RELIABLE QoS is now set by default in Create()

      std::cout << "Subscribed to: " << kVissRequestTopic << std::endl;

      // Create publisher for VISS responses
      auto response_type = std::make_shared<VissRequestTopicPubSubType>();

      response_publisher_ = std::make_unique<OtamDdsImpl<VissRequestTopic>>();
      response_publisher_->Create(kVissResponseTopic, response_type, nullptr);
      // Note: RELIABLE QoS is now set by default in Create()

      std::cout << "Publishing to: " << kVissResponseTopic << std::endl;

      // Wait for DDS discovery
      std::cout << "Waiting for DDS discovery (3 seconds)..." << std::endl;
      std::this_thread::sleep_for(std::chrono::seconds(3));

      std::cout << "[MockVissServer] Initialization complete!" << std::endl;
      return true;

    } catch (const std::exception& e) {
      std::cerr << "DDS initialization failed: " << e.what() << std::endl;
      return false;
    }
  }

  /**
   * @brief Start the server
   */
  void Start() {
    running_ = true;
    std::cout << "\n[MockVissServer] Server started. Waiting for requests..." << std::endl;
    std::cout << "Press Ctrl+C to stop.\n" << std::endl;
  }

  /**
   * @brief Stop the server
   */
  void Stop() {
    if (running_) {
      running_ = false;
      std::cout << "\n[MockVissServer] Server stopped." << std::endl;

      // Destroy all DDS instances
      if (request_subscriber_) {
        std::cout << "[MockVissServer] Destroying request_subscriber_" << std::endl;
        request_subscriber_->Destroy();
      }
      if (response_publisher_) {
        std::cout << "[MockVissServer] Destroying response_publisher_" << std::endl;
        response_publisher_->Destroy();
      }
    }
  }

  /**
   * @brief Check if server is running
   */
  bool IsRunning() const {
    return running_;
  }

 private:
  // Topic names matching OtamVehicleService
  static constexpr const char* kVissRequestTopic = "/VIN/Vehicle";
  static constexpr const char* kVissResponseTopic =
      "/mmf/otamanager/vehicleinfo_service/GetVehicleInfoResponse";

  std::string config_file_;
  std::map<std::string, std::string> vehicle_data_;
  std::unique_ptr<OtamDds> request_subscriber_;
  std::unique_ptr<OtamDds> response_publisher_;
  bool running_;

  /**
   * @brief Load vehicle data from YAML file
   */
  bool LoadVehicleData() {
    try {
      std::cout << "Loading vehicle data from: " << config_file_ << std::endl;

      YAML::Node config = YAML::LoadFile(config_file_);

      if (!config["vehicle_info"]) {
        std::cerr << "YAML file missing 'vehicle_info' section" << std::endl;
        return false;
      }

      YAML::Node vehicle_info = config["vehicle_info"];

      for (auto it = vehicle_info.begin(); it != vehicle_info.end(); ++it) {
        std::string key = it->first.as<std::string>();
        std::string value = it->second.as<std::string>();
        vehicle_data_[key] = value;
        std::cout << "  " << key << ": " << value << std::endl;
      }

      return true;

    } catch (const YAML::Exception& e) {
      std::cerr << "YAML parsing error: " << e.what() << std::endl;
      return false;
    } catch (const std::exception& e) {
      std::cerr << "Error loading vehicle data: " << e.what() << std::endl;
      return false;
    }
  }

  /**
   * @brief Get vehicle data value by path
   */
  std::string GetVehicleData(const std::string& path) const {
    auto it = vehicle_data_.find(path);
    if (it != vehicle_data_.end()) {
      return it->second;
    }
    return "";  // Return empty if not found
  }

  /**
   * @brief Handle incoming VISS request and send response
   */
  void HandleVissRequest(const std::string& payload) {
    std::cout << "\n[MockVissServer] Received VISS Request" << std::endl;
    std::cout << "Payload: " << payload << std::endl;

    try {
      // Parse outer JSON (contains topic and request)
      nlohmann::json outer_json = nlohmann::json::parse(payload);

      if (!outer_json.contains("request")) {
        std::cerr << "Invalid request: missing 'request' field" << std::endl;
        return;
      }

      std::string request_str = outer_json["request"];
      std::string response_topic = outer_json.value("topic", kVissResponseTopic);

      // Parse inner JSON (actual VISS request)
      nlohmann::json request_json = nlohmann::json::parse(request_str);

      std::string action = request_json.value("action", "");
      std::string path = request_json.value("path", "");
      std::string request_id = request_json.value("requestId", "");

      std::cout << "Parsed request:" << std::endl;
      std::cout << "  Action: " << action << std::endl;
      std::cout << "  Path: " << path << std::endl;
      std::cout << "  Request ID: " << request_id << std::endl;
      std::cout << "  Response Topic: " << response_topic << std::endl;

      // Only handle "get" action
      if (action != "get") {
        std::cerr << "Unsupported action: " << action << std::endl;
        SendErrorResponse(request_id, path, "400", "Bad Request",
                         "Unsupported action: " + action);
        return;
      }

      // Get vehicle data
      std::string value = GetVehicleData(path);

      if (value.empty()) {
        std::cout << "Vehicle data not found for path: " << path << std::endl;
        SendErrorResponse(request_id, path, "404", "Not Found",
                         "Vehicle data not found for path: " + path);
      } else {
        std::cout << "Found value: " << value << std::endl;
        SendSuccessResponse(request_id, path, value);
      }

    } catch (const nlohmann::json::parse_error& e) {
      std::cerr << "JSON parse error: " << e.what() << std::endl;
    } catch (const std::exception& e) {
      std::cerr << "Error handling request: " << e.what() << std::endl;
    }
  }

  /**
   * @brief Send success response
   */
  void SendSuccessResponse(const std::string& request_id,
                          const std::string& path,
                          const std::string& value) {
    // Get current timestamp
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();

    // Create VISS response JSON
    nlohmann::json response = {
        {"action", "get"},
        {"requestId", request_id},
        {"path", path},
        {"value", value},
        {"timestamp", std::to_string(timestamp)}
    };

    // Create VissRequestTopic message
    auto response_msg = std::make_shared<VissRequestTopic>();
    response_msg->payload(response.dump());

    std::cout << "\n[MockVissServer] Sending Response:" << std::endl;
    std::cout << response.dump(2) << std::endl;

    // Publish response
    response_publisher_->Write(std::static_pointer_cast<void>(response_msg));
    std::cout << "Response sent successfully!\n" << std::endl;
  }

  /**
   * @brief Send error response
   */
  void SendErrorResponse(const std::string& request_id,
                        const std::string& path,
                        const std::string& error_number,
                        const std::string& error_reason,
                        const std::string& error_message) {
    // Get current timestamp
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();

    // Create VISS error response JSON
    nlohmann::json response = {
        {"action", "get"},
        {"requestId", request_id},
        {"path", path},
        {"error", {
            {"number", error_number},
            {"reason", error_reason},
            {"message", error_message}
        }},
        {"timestamp", std::to_string(timestamp)}
    };

    // Create VissRequestTopic message
    auto response_msg = std::make_shared<VissRequestTopic>();
    response_msg->payload(response.dump());

    std::cout << "\n[MockVissServer] Sending Error Response:" << std::endl;
    std::cout << response.dump(2) << std::endl;

    // Publish response
    response_publisher_->Write(std::static_pointer_cast<void>(response_msg));
    std::cout << "Error response sent!\n" << std::endl;
  }

  /**
   * @brief DDS Listener for VISS Requests
   */
  class VissRequestListener : public eprosima::fastdds::dds::DataReaderListener {
   public:
    VissRequestListener(MockVissServer* server) : server_(server) {}

    void on_data_available(eprosima::fastdds::dds::DataReader* reader) override {
      VissRequestTopic message;
      eprosima::fastdds::dds::SampleInfo info;

      try {
        if (reader->take_next_sample(&message, &info) == eprosima::fastdds::dds::RETCODE_OK) {
          if (info.valid_data) {
            // Handle the request
            server_->HandleVissRequest(message.payload());
          }
        }
      } catch (const std::exception& e) {
        std::cerr << "Exception in listener: " << e.what() << std::endl;
      }
    }

   private:
    MockVissServer* server_;
  };
};

/**
 * @brief Signal handler for graceful shutdown
 */
MockVissServer* g_server = nullptr;

void SignalHandler(int signal) {
  std::cout << "\nReceived signal " << signal << std::endl;
  if (g_server) {
    g_server->Stop();
  }
}

/**
 * @brief Main function
 */
int main(int argc, char* argv[]) {
  std::string config_file = "mock_data/vehicle_info.yaml";

  // Parse command line arguments
  if (argc > 1) {
    config_file = argv[1];
  }

  std::cout << "========================================" << std::endl;
  std::cout << "  Mock VISS Server" << std::endl;
  std::cout << "========================================" << std::endl;
  std::cout << "Usage: " << argv[0] << " [config_file]" << std::endl;
  std::cout << "Default config: mock_data/vehicle_info.yaml" << std::endl;
  std::cout << "========================================\n" << std::endl;

  try {
    MockVissServer server(config_file);
    g_server = &server;

    // Set up signal handler for Ctrl+C
    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);

    // Initialize and start server
    if (!server.Initialize()) {
      std::cerr << "Server initialization failed!" << std::endl;
      return 1;
    }

    server.Start();

    // Keep server running until stopped
    while (server.IsRunning()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "Mock VISS Server terminated." << std::endl;
    return 0;

  } catch (const std::exception& e) {
    std::cerr << "Fatal error: " << e.what() << std::endl;
    return 1;
  }
}
