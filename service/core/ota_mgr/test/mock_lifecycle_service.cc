/**
 * @file mock_lifecycle_service.cc
 * @brief Mock Lifecycle Manager Service for testing OtamActorService
 *
 * This mock service subscribes to SetServiceModeRequest and responds with
 * SetServiceModeResponse based on YAML configuration.
 */

#include "otam_common/otam_dds_impl.h"
#include "lifecycle/SetServiceModeRequest.hpp"
#include "lifecycle/SetServiceModeRequestPubSubTypes.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <map>
#include <thread>
#include <chrono>
#include <csignal>
#include <yaml-cpp/yaml.h>

/**
 * @brief Configuration structure for service mode response
 */
struct ServiceModeConfig {
  lifecyclemgr::ReturnCode return_code;
  int response_delay_ms;
  std::string mode;

  ServiceModeConfig()
      : return_code(lifecyclemgr::ReturnCode::SUCCESS),
        response_delay_ms(0),
        mode("automatic") {}
};

/**
 * @brief Mock Lifecycle Manager Service
 */
class MockLifecycleService {
 public:
  MockLifecycleService(const std::string& config_file)
      : config_file_(config_file), running_(false) {
    std::cout << "=== Mock Lifecycle Manager Service ===" << std::endl;
    std::cout << "Config file: " << config_file << std::endl;
  }

  ~MockLifecycleService() {
    Stop();
  }

  /**
   * @brief Initialize the service and load configuration
   */
  bool Initialize() {
    std::cout << "\n[MockLifecycleService] Initializing..." << std::endl;

    // Load configuration from YAML file
    if (!LoadConfiguration()) {
      std::cerr << "Failed to load configuration from: " << config_file_ << std::endl;
      return false;
    }

    PrintConfiguration();

    // Initialize DDS topics
    try {
      // Create subscriber for SetServiceModeRequest
      auto request_type = std::make_shared<lifecyclemgr::SetServiceModeRequestPubSubType>();
      auto request_listener = std::make_shared<SetServiceModeRequestListener>(this);

      request_subscriber_ = std::make_unique<OtamDdsImpl<lifecyclemgr::SetServiceModeRequest>>();
      request_subscriber_->Create(kSetServiceModeRequestTopic, request_type, request_listener);
      // Note: RELIABLE QoS is now set by default in Create()

      std::cout << "Subscribed to: " << kSetServiceModeRequestTopic << std::endl;

      // Create publisher for SetServiceModeResponse
      auto response_type = std::make_shared<lifecyclemgr::SetServiceModeResponsePubSubType>();

      response_publisher_ = std::make_unique<OtamDdsImpl<lifecyclemgr::SetServiceModeResponse>>();
      response_publisher_->Create(kSetServiceModeResponseTopic, response_type, nullptr);
      // Note: RELIABLE QoS is now set by default in Create()

      std::cout << "Publishing to: " << kSetServiceModeResponseTopic << std::endl;

      // Wait for DDS discovery
      std::cout << "Waiting for DDS discovery (3 seconds)..." << std::endl;
      std::this_thread::sleep_for(std::chrono::seconds(3));

      std::cout << "[MockLifecycleService] Initialization complete!" << std::endl;
      return true;

    } catch (const std::exception& e) {
      std::cerr << "DDS initialization failed: " << e.what() << std::endl;
      return false;
    }
  }

  /**
   * @brief Start the service
   */
  void Start() {
    running_ = true;
    std::cout << "\n[MockLifecycleService] Service started. Waiting for requests..." << std::endl;
    std::cout << "Press Ctrl+C to stop.\n" << std::endl;
  }

  /**
   * @brief Stop the service
   */
  void Stop() {
    if (running_) {
      running_ = false;
      std::cout << "\n[MockLifecycleService] Service stopped." << std::endl;

      // Destroy all DDS instances
      if (request_subscriber_) {
        std::cout << "[MockLifecycleService] Destroying request_subscriber_" << std::endl;
        request_subscriber_->Destroy();
      }
      if (response_publisher_) {
        std::cout << "[MockLifecycleService] Destroying response_publisher_" << std::endl;
        response_publisher_->Destroy();
      }
    }
  }

  /**
   * @brief Check if service is running
   */
  bool IsRunning() const {
    return running_;
  }

 private:
  // Topic names matching OtamActorService
  static constexpr const char* kSetServiceModeRequestTopic =
      "mmf/lifecyclemgr/SetServiceModeRequest";
  static constexpr const char* kSetServiceModeResponseTopic =
      "mmf/lifecyclemgr/SetServiceModeResponse";

  std::string config_file_;
  ServiceModeConfig config_;
  std::map<std::string, ServiceModeConfig> scenarios_;
  std::string active_scenario_;
  std::unique_ptr<OtamDds> request_subscriber_;
  std::unique_ptr<OtamDds> response_publisher_;
  bool running_;

  /**
   * @brief Load configuration from YAML file
   */
  bool LoadConfiguration() {
    try {
      std::cout << "Loading configuration from: " << config_file_ << std::endl;

      YAML::Node config = YAML::LoadFile(config_file_);

      // Load main service mode response config
      if (config["service_mode_response"]) {
        YAML::Node resp_config = config["service_mode_response"];

        std::string return_code_str = resp_config["return_code"].as<std::string>("SUCCESS");
        config_.return_code = ParseReturnCode(return_code_str);
        config_.response_delay_ms = resp_config["response_delay_ms"].as<int>(0);
        config_.mode = resp_config["mode"].as<std::string>("automatic");
      }

      // Load scenarios
      if (config["scenarios"]) {
        YAML::Node scenarios = config["scenarios"];
        for (auto it = scenarios.begin(); it != scenarios.end(); ++it) {
          std::string scenario_name = it->first.as<std::string>();
          YAML::Node scenario_config = it->second;

          ServiceModeConfig scenario;
          std::string return_code_str = scenario_config["return_code"].as<std::string>("SUCCESS");
          scenario.return_code = ParseReturnCode(return_code_str);
          scenario.response_delay_ms = scenario_config["response_delay_ms"].as<int>(0);
          scenario.mode = "manual";

          scenarios_[scenario_name] = scenario;
          std::cout << "  Loaded scenario: " << scenario_name << std::endl;
        }
      }

      // Load active scenario
      if (config["active_scenario"]) {
        active_scenario_ = config["active_scenario"].as<std::string>("success");
        std::cout << "Active scenario: " << active_scenario_ << std::endl;

        // If active scenario exists, use its config
        auto it = scenarios_.find(active_scenario_);
        if (it != scenarios_.end()) {
          config_ = it->second;
          std::cout << "Using scenario configuration: " << active_scenario_ << std::endl;
        }
      }

      return true;

    } catch (const YAML::Exception& e) {
      std::cerr << "YAML parsing error: " << e.what() << std::endl;
      return false;
    } catch (const std::exception& e) {
      std::cerr << "Error loading configuration: " << e.what() << std::endl;
      return false;
    }
  }

  /**
   * @brief Parse return code string to enum
   */
  lifecyclemgr::ReturnCode ParseReturnCode(const std::string& code_str) {
    if (code_str == "SUCCESS") {
      return lifecyclemgr::ReturnCode::SUCCESS;
    } else if (code_str == "FAILED") {
      return lifecyclemgr::ReturnCode::FAILED;
    } else {
      std::cerr << "Unknown return code: " << code_str << ", using SUCCESS" << std::endl;
      return lifecyclemgr::ReturnCode::SUCCESS;
    }
  }

  /**
   * @brief Convert return code enum to string
   */
  std::string ReturnCodeToString(lifecyclemgr::ReturnCode code) {
    switch (code) {
      case lifecyclemgr::ReturnCode::SUCCESS:
        return "SUCCESS";
      case lifecyclemgr::ReturnCode::FAILED:
        return "FAILED";
      default:
        return "UNKNOWN";
    }
  }

  /**
   * @brief Convert service mode enum to string
   */
  std::string ServiceModeToString(lifecyclemgr::ServiceMode mode) {
    switch (mode) {
      case lifecyclemgr::ServiceMode::NORMAL:
        return "NORMAL";
      case lifecyclemgr::ServiceMode::OTA_MODE:
        return "OTA_MODE";
      default:
        return "UNKNOWN";
    }
  }

  /**
   * @brief Convert OS type enum to string
   */
  std::string OsTypeToString(lifecyclemgr::OsType os_type) {
    switch (os_type) {
      case lifecyclemgr::OsType::ROS2:
        return "ROS2";
      case lifecyclemgr::OsType::AGL:
        return "AGL";
      case lifecyclemgr::OsType::AUTOSAR:
        return "AUTOSAR";
      default:
        return "UNKNOWN";
    }
  }

  /**
   * @brief Print current configuration
   */
  void PrintConfiguration() {
    std::cout << "\n=== Current Configuration ===" << std::endl;
    std::cout << "  Return Code: " << ReturnCodeToString(config_.return_code) << std::endl;
    std::cout << "  Response Delay: " << config_.response_delay_ms << " ms" << std::endl;
    std::cout << "  Mode: " << config_.mode << std::endl;
    std::cout << "============================\n" << std::endl;
  }

  /**
   * @brief Handle incoming SetServiceModeRequest
   */
  void HandleSetServiceModeRequest(const lifecyclemgr::SetServiceModeRequest& request) {
    std::cout << "\n[MockLifecycleService] SetServiceModeRequest received!" << std::endl;
    std::cout << "  OS Type: " << OsTypeToString(request.os_type()) << std::endl;
    std::cout << "  Service Mode: " << ServiceModeToString(request.service_mode()) << std::endl;
    std::cout << "  Is Group: " << (request.is_group() ? "true" : "false") << std::endl;

    const auto& service_list = request.service_list();
    if (!service_list.empty()) {
      std::cout << "  Service List:" << std::endl;
      for (const auto& service : service_list) {
        std::cout << "    - " << service << std::endl;
      }
    } else {
      std::cout << "  Service List: (empty - apply to all services)" << std::endl;
    }

    // Apply response delay if configured
    if (config_.response_delay_ms > 0) {
      std::cout << "Waiting " << config_.response_delay_ms << " ms before response..." << std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(config_.response_delay_ms));
    }

    // Send response
    SendSetServiceModeResponse();
  }

  /**
   * @brief Send SetServiceModeResponse
   */
  void SendSetServiceModeResponse() {
    auto response = std::make_shared<lifecyclemgr::SetServiceModeResponse>();
    response->ret_code(config_.return_code);

    std::cout << "\n[MockLifecycleService] Sending SetServiceModeResponse:" << std::endl;
    std::cout << "  Return Code: " << ReturnCodeToString(config_.return_code)
              << " (" << static_cast<int>(config_.return_code) << ")" << std::endl;

    // Publish response
    response_publisher_->Write(std::static_pointer_cast<void>(response));
    std::cout << "Response sent successfully!\n" << std::endl;
  }

  /**
   * @brief DDS Listener for SetServiceModeRequest
   */
  class SetServiceModeRequestListener : public eprosima::fastdds::dds::DataReaderListener {
   public:
    SetServiceModeRequestListener(MockLifecycleService* service) : service_(service) {}

    void on_data_available(eprosima::fastdds::dds::DataReader* reader) override {
      lifecyclemgr::SetServiceModeRequest message;
      eprosima::fastdds::dds::SampleInfo info;

      try {
        if (reader->take_next_sample(&message, &info) == eprosima::fastdds::dds::RETCODE_OK) {
          if (info.valid_data) {
            // Handle the request
            service_->HandleSetServiceModeRequest(message);
          }
        }
      } catch (const std::exception& e) {
        std::cerr << "Exception in listener: " << e.what() << std::endl;
      }
    }

   private:
    MockLifecycleService* service_;
  };
};

/**
 * @brief Signal handler for graceful shutdown
 */
MockLifecycleService* g_service = nullptr;

void SignalHandler(int signal) {
  std::cout << "\nReceived signal " << signal << std::endl;
  if (g_service) {
    g_service->Stop();
  }
}

/**
 * @brief Main function
 */
int main(int argc, char* argv[]) {
  std::string config_file = "mock_data/lifecycle_config.yaml";

  // Parse command line arguments
  if (argc > 1) {
    config_file = argv[1];
  }

  std::cout << "========================================" << std::endl;
  std::cout << "  Mock Lifecycle Manager Service" << std::endl;
  std::cout << "========================================" << std::endl;
  std::cout << "Usage: " << argv[0] << " [config_file]" << std::endl;
  std::cout << "Default config: mock_data/lifecycle_config.yaml" << std::endl;
  std::cout << "========================================\n" << std::endl;

  try {
    MockLifecycleService service(config_file);
    g_service = &service;

    // Set up signal handler for Ctrl+C
    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);

    // Initialize and start service
    if (!service.Initialize()) {
      std::cerr << "Service initialization failed!" << std::endl;
      return 1;
    }

    service.Start();

    // Keep service running until stopped
    while (service.IsRunning()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "Mock Lifecycle Service terminated." << std::endl;
    return 0;

  } catch (const std::exception& e) {
    std::cerr << "Fatal error: " << e.what() << std::endl;
    return 1;
  }
}
