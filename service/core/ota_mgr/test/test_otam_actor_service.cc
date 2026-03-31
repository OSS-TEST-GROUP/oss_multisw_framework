/**
 * @file test_otam_actor_service.cc
 * @brief Test application for OtamActorService
 *
 * This test application provides an interactive menu to test all
 * OtamActorService functionalities via DDS communication.
 */

#include "otam_common/otam_dds_impl.h"
#include "otam_common/otam_dds_idl.h"

#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>
#include <limits>
#include <cstdio>
#include <cstring>

using namespace OtaManager;

class OtamActorServiceTester {
 public:
  OtamActorServiceTester() : next_request_id_(1), running_(true) {
    std::cout << "=== OtamActorService Test Application ===" << std::endl;
  }

  ~OtamActorServiceTester() {
    running_ = false;
    // StopPublisher();
  }

  /**
   * @brief Initialize DDS communication
   */
  void Initialize() {
    std::cout << "[DEBUG] Initialize() started" << std::endl;
    std::cout << "Initializing DDS topics..." << std::endl;

    try {
      // Initialize Request Publishers
      std::cout << "[DEBUG] Initializing GetAppUpdateSettings publisher..." << std::endl;
      InitializePublisher<GetAppUpdateSettings>(kTopicGetAppUpdateSettings,
                         std::make_shared<GetAppUpdateSettingsPubSubType>());

      std::cout << "[DEBUG] Initializing SetAppUpdateSettings publisher..." << std::endl;
      InitializePublisher<SetAppUpdateSettings>(kTopicSetAppUpdateSettings,
                         std::make_shared<SetAppUpdateSettingsPubSubType>());

      std::cout << "[DEBUG] Initializing GetAppUpdateState publisher..." << std::endl;
      InitializePublisher<GetAppUpdateState>(kTopicGetAppUpdateState,
                         std::make_shared<GetAppUpdateStatePubSubType>());

      std::cout << "[DEBUG] Initializing SetAppUpdate publisher..." << std::endl;
      InitializePublisher<SetAppUpdate>(kTopicSetAppUpdate,
                         std::make_shared<SetAppUpdatePubSubType>());

      // Initialize Response Subscribers
      std::cout << "[DEBUG] Creating GetAppUpdateSettingsResponse listener..." << std::endl;
      auto get_settings_resp_listener = std::make_shared<GetAppUpdateSettingsResponseListener>(this);
      std::cout << "[DEBUG] Initializing GetAppUpdateSettingsResponse subscriber..." << std::endl;
      InitializeSubscriber<GetAppUpdateSettingsResponse>(kTopicGetAppUpdateSettingsResponse,
                          std::make_shared<GetAppUpdateSettingsResponsePubSubType>(),
                          get_settings_resp_listener);

      std::cout << "[DEBUG] Creating SetAppUpdateSettingsResponse listener..." << std::endl;
      auto set_settings_resp_listener = std::make_shared<SetAppUpdateSettingsResponseListener>(this);
      std::cout << "[DEBUG] Initializing SetAppUpdateSettingsResponse subscriber..." << std::endl;
      InitializeSubscriber<SetAppUpdateSettingsResponse>(kTopicSetAppUpdateSettingsResponse,
                          std::make_shared<SetAppUpdateSettingsResponsePubSubType>(),
                          set_settings_resp_listener);

      std::cout << "[DEBUG] Creating GetAppUpdateStateResponse listener..." << std::endl;
      auto get_state_resp_listener = std::make_shared<GetAppUpdateStateResponseListener>(this);
      std::cout << "[DEBUG] Initializing GetAppUpdateStateResponse subscriber..." << std::endl;
      InitializeSubscriber<GetAppUpdateStateResponse>(kTopicGetAppUpdateStateResponse,
                          std::make_shared<GetAppUpdateStateResponsePubSubType>(),
                          get_state_resp_listener);

      std::cout << "[DEBUG] Creating SetAppUpdateResponse listener..." << std::endl;
      auto set_update_resp_listener = std::make_shared<SetAppUpdateResponseListener>(this);
      std::cout << "[DEBUG] Initializing SetAppUpdateResponse subscriber..." << std::endl;
      InitializeSubscriber<SetAppUpdateResponse>(kTopicSetAppUpdateResponse,
                          std::make_shared<SetAppUpdateResponsePubSubType>(),
                          set_update_resp_listener);

      std::cout << "All DDS topics initialized successfully!" << std::endl;

      // Wait for discovery
      std::cout << "Waiting for DDS discovery (3 seconds)..." << std::endl;
      std::this_thread::sleep_for(std::chrono::seconds(3));

      std::cout << "[DEBUG] Initialize() completed successfully" << std::endl;

    } catch (const std::exception& e) {
      std::cerr << "[ERROR] Initialization failed: " << e.what() << std::endl;
      throw;
    }
  }

  /**
   * @brief Run the interactive test menu
   */
  void Run() {
    printf("[DEBUG] Run() started, running_=%d\n", running_.load());
    fflush(stdout);

    char buffer[256];

    while (running_) {
      printf("[DEBUG] Before DisplayMenu()\n");
      fflush(stdout);

      DisplayMenu();

      printf("[DEBUG] After DisplayMenu()\n");
      fflush(stdout);

      printf("\nEnter your choice (0-4): ");
      fflush(stdout);

      printf("[DEBUG] Before fgets()\n");
      fflush(stdout);

      // C-style input handling
      int choice = -1;
      if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
          fprintf(stderr, "[ERROR] Failed to read input!\n");
          clearerr(stdin);
          continue;
      }

      printf("[DEBUG] After fgets(), buffer=[%s]\n", buffer);
      fflush(stdout);

      // Strip trailing newline from buffer
      buffer[strcspn(buffer, "\n")] = 0;

      // Convert string to int
      if (sscanf(buffer, "%d", &choice) != 1) {
          printf("Invalid input. Please enter a number (0-4).\n");
          continue;
      }

      printf("[DEBUG] Converted to int: %d\n", choice);
      fflush(stdout);

      switch (choice) {
        case 0:
          running_ = false;
          printf("Exiting test application...\n");
          break;
        case 1:
          TestGetAppUpdateSettings();
          break;
        case 2:
          TestSetAppUpdateSettings();
          break;
        case 3:
          TestGetAppUpdateState();
          break;
        case 4:
          TestSetAppUpdate();
          break;
        default:
          printf("Invalid choice. Please try again.\n");
          break;
      }

      if (running_) {
        printf("\nPress Enter to continue...");
        fflush(stdout);
        fgets(buffer, sizeof(buffer), stdin);
      }
    }

    printf("[DEBUG] Run() finished\n");
    fflush(stdout);

    // Stop DDS resources before exiting
    StopPublisher();
  }

 private:
  // Topic name constants
  static constexpr const char* kTopicGetAppUpdateSettings =
      "/mmf/otamanager/otaactor_service/GetAppUpdateSettings";
  static constexpr const char* kTopicGetAppUpdateSettingsResponse =
      "/mmf/otamanager/otaactor_service/GetAppUpdateSettingsResponse";
  static constexpr const char* kTopicSetAppUpdateSettings =
      "/mmf/otamanager/otaactor_service/SetAppUpdateSettings";
  static constexpr const char* kTopicSetAppUpdateSettingsResponse =
      "/mmf/otamanager/otaactor_service/SetAppUpdateSettingsResponse";
  static constexpr const char* kTopicGetAppUpdateState =
      "/mmf/otamanager/otaactor_service/GetAppUpdateState";
  static constexpr const char* kTopicGetAppUpdateStateResponse =
      "/mmf/otamanager/otaactor_service/GetAppUpdateStateResponse";
  static constexpr const char* kTopicSetAppUpdate =
      "/mmf/otamanager/otaactor_service/SetAppUpdate";
  static constexpr const char* kTopicSetAppUpdateResponse =
      "/mmf/otamanager/otaactor_service/SetAppUpdateResponse";

  std::map<std::string, std::unique_ptr<OtamDds>> dds_registry_;
  int32_t next_request_id_;
  std::atomic<bool> running_;

  /**
   * @brief Initialize a DDS publisher
   */
  template<typename MessageType>
  void InitializePublisher(const std::string& topic_name,
                          std::shared_ptr<void> type_support) {
    auto dds_impl = std::make_unique<OtamDdsImpl<MessageType>>();
    dds_impl->Create(topic_name, type_support, nullptr);
    // Note: RELIABLE QoS is now set by default in Create()
    dds_registry_[topic_name] = std::move(dds_impl);
  }

  /**
   * @brief Initialize a DDS subscriber
   */
  template<typename MessageType>
  void InitializeSubscriber(const std::string& topic_name,
                           std::shared_ptr<void> type_support,
                           std::shared_ptr<void> listener) {
    auto dds_impl = std::make_unique<OtamDdsImpl<MessageType>>();
    dds_impl->Create(topic_name, type_support, listener);
    // Note: RELIABLE QoS is now set by default in Create()
    dds_registry_[topic_name] = std::move(dds_impl);
  }

  /**
   * @brief Stop all DDS publishers and subscribers
   */
  void StopPublisher() {
    std::cout << "Stopping DDS publishers and subscribers..." << std::endl;

    // Destroy all DDS instances
    for (auto& pair : dds_registry_) {
      if (pair.second) {
        std::cout << "Destroying DDS for topic: " << pair.first << std::endl;
        pair.second->Destroy();
      }
    }

    dds_registry_.clear();
    std::cout << "All DDS instances stopped." << std::endl;
  }

  /**
   * @brief Display the test menu
   */
  void DisplayMenu() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "  OtamActorService Test Menu" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "1. Test GetAppUpdateSettings" << std::endl;
    std::cout << "2. Test SetAppUpdateSettings" << std::endl;
    std::cout << "3. Test GetAppUpdateState" << std::endl;
    std::cout << "4. Test SetAppUpdate" << std::endl;
    std::cout << "0. Exit" << std::endl;
    std::cout << "========================================" << std::endl;
  }

  /**
   * @brief Test GetAppUpdateSettings
   */
  void TestGetAppUpdateSettings() {
    printf("\n--- Test GetAppUpdateSettings ---\n");

    char buffer[256];
    printf("Enter application name: ");
    fflush(stdout);

    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
      fprintf(stderr, "Error: Failed to read input\n");
      return;
    }
    buffer[strcspn(buffer, "\n")] = 0;  // strip trailing newline

    std::string app_name(buffer);

    auto it = dds_registry_.find(kTopicGetAppUpdateSettings);
    if (it == dds_registry_.end()) {
      fprintf(stderr, "Error: DDS publisher not found for topic\n");
      return;
    }

    auto request = std::make_shared<GetAppUpdateSettings>();
    request->id(next_request_id_++);
    request->app_name(app_name);

    printf("Sending GetAppUpdateSettings request...\n");
    printf("  Request ID: %d\n", request->id());
    printf("  App Name: %s\n", request->app_name().c_str());

    it->second->Write(std::static_pointer_cast<void>(request));
    printf("Request sent! Waiting for response...\n");
  }

  /**
   * @brief Test SetAppUpdateSettings
   */
  void TestSetAppUpdateSettings() {
    printf("\n--- Test SetAppUpdateSettings ---\n");

    char buffer[256];
    printf("Enter application name: ");
    fflush(stdout);

    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
      fprintf(stderr, "Error: Failed to read input\n");
      return;
    }
    buffer[strcspn(buffer, "\n")] = 0;  // strip trailing newline
    std::string app_name(buffer);

    printf("Enter settings (JSON format): ");
    fflush(stdout);

    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
      fprintf(stderr, "Error: Failed to read input\n");
      return;
    }
    buffer[strcspn(buffer, "\n")] = 0;  // strip trailing newline
    std::string settings(buffer);

    auto it = dds_registry_.find(kTopicSetAppUpdateSettings);
    if (it == dds_registry_.end()) {
      fprintf(stderr, "Error: DDS publisher not found for topic\n");
      return;
    }

    auto request = std::make_shared<SetAppUpdateSettings>();
    request->id(next_request_id_++);
    request->app_name(app_name);
    request->app_settings(settings);

    printf("Sending SetAppUpdateSettings request...\n");
    printf("  Request ID: %d\n", request->id());
    printf("  App Name: %s\n", request->app_name().c_str());
    printf("  Settings: %s\n", request->app_settings().c_str());

    it->second->Write(std::static_pointer_cast<void>(request));
    printf("Request sent! Waiting for response...\n");
  }

  /**
   * @brief Test GetAppUpdateState
   */
  void TestGetAppUpdateState() {
    printf("\n--- Test GetAppUpdateState ---\n");

    char buffer[256];
    printf("Enter application name: ");
    fflush(stdout);

    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
      fprintf(stderr, "Error: Failed to read input\n");
      return;
    }
    buffer[strcspn(buffer, "\n")] = 0;  // strip trailing newline

    std::string app_name(buffer);

    auto it = dds_registry_.find(kTopicGetAppUpdateState);
    if (it == dds_registry_.end()) {
      fprintf(stderr, "Error: DDS publisher not found for topic\n");
      return;
    }

    auto request = std::make_shared<GetAppUpdateState>();
    request->id(next_request_id_++);
    request->app_name(app_name);

    printf("Sending GetAppUpdateState request...\n");
    printf("  Request ID: %d\n", request->id());
    printf("  App Name: %s\n", request->app_name().c_str());

    it->second->Write(std::static_pointer_cast<void>(request));
    printf("Request sent! Waiting for response...\n");
  }

  /**
   * @brief Test SetAppUpdate
   */
  void TestSetAppUpdate() {
    printf("\n--- Test SetAppUpdate ---\n");

    char buffer[256];
    printf("Enter application name: ");
    fflush(stdout);

    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
      fprintf(stderr, "Error: Failed to read input\n");
      return;
    }
    buffer[strcspn(buffer, "\n")] = 0;  // strip trailing newline

    std::string app_name(buffer);

    auto it = dds_registry_.find(kTopicSetAppUpdate);
    if (it == dds_registry_.end()) {
      fprintf(stderr, "Error: DDS publisher not found for topic\n");
      return;
    }

    auto request = std::make_shared<SetAppUpdate>();
    request->id(next_request_id_++);
    request->app_name(app_name);

    printf("Sending SetAppUpdate request...\n");
    printf("  Request ID: %d\n", request->id());
    printf("  App Name: %s\n", request->app_name().c_str());

    it->second->Write(std::static_pointer_cast<void>(request));
    printf("Request sent! Waiting for response...\n");
    printf("\nNote: This will trigger lifecycle mode change and vehicle info checks.\n");
  }

  // ========== Response Listeners ==========

  /**
   * @brief Listener for GetAppUpdateSettingsResponse
   */
  class GetAppUpdateSettingsResponseListener : public eprosima::fastdds::dds::DataReaderListener {
   public:
    GetAppUpdateSettingsResponseListener(OtamActorServiceTester* tester) : tester_(tester) {
      std::cout << "[DEBUG] GetAppUpdateSettingsResponseListener created, tester_=" << tester_ << std::endl;
    }

    void on_data_available(eprosima::fastdds::dds::DataReader* reader) override {
      try {
        std::cout << "[DEBUG] GetAppUpdateSettingsResponseListener::on_data_available called" << std::endl;

        GetAppUpdateSettingsResponse message;
        eprosima::fastdds::dds::SampleInfo info;

        if (reader->take_next_sample(&message, &info) == eprosima::fastdds::dds::RETCODE_OK) {
          if (info.valid_data) {
            std::cout << "\n[RESPONSE] GetAppUpdateSettingsResponse received:" << std::endl;
            std::cout << "  Request ID: " << message.id() << std::endl;
            std::cout << "  App Name: " << message.app_name() << std::endl;
            std::cout << "  Settings: " << message.app_settings() << std::endl;
            std::cout << "  Return Code: " << static_cast<int>(message.ret_code()) << std::endl;
          }
        }
      } catch (const std::exception& e) {
        std::cerr << "[ERROR] Exception in GetAppUpdateSettingsResponseListener: " << e.what() << std::endl;
      }
    }

   private:
    OtamActorServiceTester* tester_;
  };

  /**
   * @brief Listener for SetAppUpdateSettingsResponse
   */
  class SetAppUpdateSettingsResponseListener : public eprosima::fastdds::dds::DataReaderListener {
   public:
    SetAppUpdateSettingsResponseListener(OtamActorServiceTester* tester) : tester_(tester) {}

    void on_data_available(eprosima::fastdds::dds::DataReader* reader) override {
      SetAppUpdateSettingsResponse message;
      eprosima::fastdds::dds::SampleInfo info;

      if (reader->take_next_sample(&message, &info) == eprosima::fastdds::dds::RETCODE_OK) {
        if (info.valid_data) {
          std::cout << "\n[RESPONSE] SetAppUpdateSettingsResponse received:" << std::endl;
          std::cout << "  Request ID: " << message.id() << std::endl;
          std::cout << "  Return Code: " << static_cast<int>(message.ret_code()) << std::endl;
        }
      }
    }

   private:
    OtamActorServiceTester* tester_;
  };

  /**
   * @brief Listener for GetAppUpdateStateResponse
   */
  class GetAppUpdateStateResponseListener : public eprosima::fastdds::dds::DataReaderListener {
   public:
    GetAppUpdateStateResponseListener(OtamActorServiceTester* tester) : tester_(tester) {}

    void on_data_available(eprosima::fastdds::dds::DataReader* reader) override {
      GetAppUpdateStateResponse message;
      eprosima::fastdds::dds::SampleInfo info;

      if (reader->take_next_sample(&message, &info) == eprosima::fastdds::dds::RETCODE_OK) {
        if (info.valid_data) {
          std::cout << "\n[RESPONSE] GetAppUpdateStateResponse received:" << std::endl;
          std::cout << "  Request ID: " << message.id() << std::endl;
          std::cout << "  App Name: " << message.app_name() << std::endl;
          std::cout << "  State Info: " << message.app_state_info() << std::endl;
          std::cout << "  Return Code: " << static_cast<int>(message.ret_code()) << std::endl;
        }
      }
    }

   private:
    OtamActorServiceTester* tester_;
  };

  /**
   * @brief Listener for SetAppUpdateResponse
   */
  class SetAppUpdateResponseListener : public eprosima::fastdds::dds::DataReaderListener {
   public:
    SetAppUpdateResponseListener(OtamActorServiceTester* tester) : tester_(tester) {}

    void on_data_available(eprosima::fastdds::dds::DataReader* reader) override {
      SetAppUpdateResponse message;
      eprosima::fastdds::dds::SampleInfo info;

      if (reader->take_next_sample(&message, &info) == eprosima::fastdds::dds::RETCODE_OK) {
        if (info.valid_data) {
          std::cout << "\n[RESPONSE] SetAppUpdateResponse received:" << std::endl;
          std::cout << "  Request ID: " << message.id() << std::endl;
          std::cout << "  App Name: " << message.app_name() << std::endl;
          std::cout << "  Return Code: " << static_cast<int>(message.ret_code()) << std::endl;
        }
      }
    }

   private:
    OtamActorServiceTester* tester_;
  };
};

/**
 * @brief Main function
 */
int main(int argc, char* argv[]) {
  try {
    OtamActorServiceTester tester;
    tester.Initialize();
    tester.Run();

    std::cout << "Test application finished." << std::endl;
    return 0;

  } catch (const std::exception& e) {
    std::cerr << "Fatal error: " << e.what() << std::endl;
    return 1;
  }
}
