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

using namespace OtaManager;

class OtamActorServiceTester {
 public:
  OtamActorServiceTester() : next_request_id_(1), running_(true) {
    std::cout << "=== OtamActorService Test Application ===" << std::endl;
  }

  ~OtamActorServiceTester() {
    running_ = false;
    dds_registry_.clear();
  }

  /**
   * @brief Initialize DDS communication
   */
  void Initialize() {
    std::cout << "Initializing DDS topics..." << std::endl;

    try {
      // Initialize Request Publishers
      InitializePublisher(kTopicGetAppUpdateSettings,
                         std::make_shared<GetAppUpdateSettingsPubSubType>());

      InitializePublisher(kTopicSetAppUpdateSettings,
                         std::make_shared<SetAppUpdateSettingsPubSubType>());

      InitializePublisher(kTopicGetAppUpdateState,
                         std::make_shared<GetAppUpdateStatePubSubType>());

      InitializePublisher(kTopicSetAppUpdate,
                         std::make_shared<SetAppUpdatePubSubType>());

      // Initialize Response Subscribers
      auto get_settings_resp_listener = std::make_shared<GetAppUpdateSettingsResponseListener>(this);
      InitializeSubscriber(kTopicGetAppUpdateSettingsResponse,
                          std::make_shared<GetAppUpdateSettingsResponsePubSubType>(),
                          get_settings_resp_listener);

      auto set_settings_resp_listener = std::make_shared<SetAppUpdateSettingsResponseListener>(this);
      InitializeSubscriber(kTopicSetAppUpdateSettingsResponse,
                          std::make_shared<SetAppUpdateSettingsResponsePubSubType>(),
                          set_settings_resp_listener);

      auto get_state_resp_listener = std::make_shared<GetAppUpdateStateResponseListener>(this);
      InitializeSubscriber(kTopicGetAppUpdateStateResponse,
                          std::make_shared<GetAppUpdateStateResponsePubSubType>(),
                          get_state_resp_listener);

      auto set_update_resp_listener = std::make_shared<SetAppUpdateResponseListener>(this);
      InitializeSubscriber(kTopicSetAppUpdateResponse,
                          std::make_shared<SetAppUpdateResponsePubSubType>(),
                          set_update_resp_listener);

      std::cout << "All DDS topics initialized successfully!" << std::endl;

      // Wait for discovery
      std::cout << "Waiting for DDS discovery (3 seconds)..." << std::endl;
      std::this_thread::sleep_for(std::chrono::seconds(3));

    } catch (const std::exception& e) {
      std::cerr << "Initialization failed: " << e.what() << std::endl;
      throw;
    }
  }

  /**
   * @brief Run the interactive test menu
   */
  void Run() {
    while (running_) {
      DisplayMenu();

      int choice;
      std::cout << "\nEnter your choice (0-4): ";
      std::cin >> choice;
      std::cin.ignore();  // Clear newline

      switch (choice) {
        case 0:
          running_ = false;
          std::cout << "Exiting test application..." << std::endl;
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
          std::cout << "Invalid choice. Please try again." << std::endl;
          break;
      }

      if (running_) {
        std::cout << "\nPress Enter to continue...";
        std::cin.get();
      }
    }
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

  std::map<std::string, std::unique_ptr<OtamDdsImpl>> dds_registry_;
  int32_t next_request_id_;
  std::atomic<bool> running_;

  /**
   * @brief Initialize a DDS publisher
   */
  void InitializePublisher(const std::string& topic_name,
                          std::shared_ptr<void> type_support) {
    auto dds_impl = std::make_unique<OtamDdsImpl>();
    dds_impl->Create(topic_name, type_support, nullptr);
    dds_impl->SetQos(1);  // RELIABLE QoS
    dds_registry_[topic_name] = std::move(dds_impl);
  }

  /**
   * @brief Initialize a DDS subscriber
   */
  void InitializeSubscriber(const std::string& topic_name,
                           std::shared_ptr<void> type_support,
                           std::shared_ptr<void> listener) {
    auto dds_impl = std::make_unique<OtamDdsImpl>();
    dds_impl->Create(topic_name, type_support, listener);
    dds_impl->SetQos(1);  // RELIABLE QoS
    dds_registry_[topic_name] = std::move(dds_impl);
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
    std::cout << "\n--- Test GetAppUpdateSettings ---" << std::endl;

    std::string app_name;
    std::cout << "Enter application name: ";
    std::getline(std::cin, app_name);

    auto it = dds_registry_.find(kTopicGetAppUpdateSettings);
    if (it == dds_registry_.end()) {
      std::cerr << "Error: DDS publisher not found for topic" << std::endl;
      return;
    }

    auto request = std::make_shared<GetAppUpdateSettings>();
    request->id(next_request_id_++);
    request->app_name(app_name);

    std::cout << "Sending GetAppUpdateSettings request..." << std::endl;
    std::cout << "  Request ID: " << request->id() << std::endl;
    std::cout << "  App Name: " << request->app_name() << std::endl;

    it->second->Write(request);
    std::cout << "Request sent! Waiting for response..." << std::endl;
  }

  /**
   * @brief Test SetAppUpdateSettings
   */
  void TestSetAppUpdateSettings() {
    std::cout << "\n--- Test SetAppUpdateSettings ---" << std::endl;

    std::string app_name;
    std::cout << "Enter application name: ";
    std::getline(std::cin, app_name);

    std::string settings;
    std::cout << "Enter settings (JSON format): ";
    std::getline(std::cin, settings);

    auto it = dds_registry_.find(kTopicSetAppUpdateSettings);
    if (it == dds_registry_.end()) {
      std::cerr << "Error: DDS publisher not found for topic" << std::endl;
      return;
    }

    auto request = std::make_shared<SetAppUpdateSettings>();
    request->id(next_request_id_++);
    request->app_name(app_name);
    request->app_settings(settings);

    std::cout << "Sending SetAppUpdateSettings request..." << std::endl;
    std::cout << "  Request ID: " << request->id() << std::endl;
    std::cout << "  App Name: " << request->app_name() << std::endl;
    std::cout << "  Settings: " << request->app_settings() << std::endl;

    it->second->Write(request);
    std::cout << "Request sent! Waiting for response..." << std::endl;
  }

  /**
   * @brief Test GetAppUpdateState
   */
  void TestGetAppUpdateState() {
    std::cout << "\n--- Test GetAppUpdateState ---" << std::endl;

    std::string app_name;
    std::cout << "Enter application name: ";
    std::getline(std::cin, app_name);

    auto it = dds_registry_.find(kTopicGetAppUpdateState);
    if (it == dds_registry_.end()) {
      std::cerr << "Error: DDS publisher not found for topic" << std::endl;
      return;
    }

    auto request = std::make_shared<GetAppUpdateState>();
    request->id(next_request_id_++);
    request->app_name(app_name);

    std::cout << "Sending GetAppUpdateState request..." << std::endl;
    std::cout << "  Request ID: " << request->id() << std::endl;
    std::cout << "  App Name: " << request->app_name() << std::endl;

    it->second->Write(request);
    std::cout << "Request sent! Waiting for response..." << std::endl;
  }

  /**
   * @brief Test SetAppUpdate
   */
  void TestSetAppUpdate() {
    std::cout << "\n--- Test SetAppUpdate ---" << std::endl;

    std::string app_name;
    std::cout << "Enter application name: ";
    std::getline(std::cin, app_name);

    auto it = dds_registry_.find(kTopicSetAppUpdate);
    if (it == dds_registry_.end()) {
      std::cerr << "Error: DDS publisher not found for topic" << std::endl;
      return;
    }

    auto request = std::make_shared<SetAppUpdate>();
    request->id(next_request_id_++);
    request->app_name(app_name);

    std::cout << "Sending SetAppUpdate request..." << std::endl;
    std::cout << "  Request ID: " << request->id() << std::endl;
    std::cout << "  App Name: " << request->app_name() << std::endl;

    it->second->Write(request);
    std::cout << "Request sent! Waiting for response..." << std::endl;
    std::cout << "\nNote: This will trigger lifecycle mode change and vehicle info checks." << std::endl;
  }

  // ========== Response Listeners ==========

  /**
   * @brief Listener for GetAppUpdateSettingsResponse
   */
  class GetAppUpdateSettingsResponseListener : public eprosima::fastdds::dds::DataReaderListener {
   public:
    GetAppUpdateSettingsResponseListener(OtamActorServiceTester* tester) : tester_(tester) {}

    void on_data_available(eprosima::fastdds::dds::DataReader* reader) override {
      GetAppUpdateSettingsResponse message;
      eprosima::fastdds::dds::SampleInfo info;

      if (reader->take_next_sample(&message, &info) == eprosima::fastdds::dds::RETCODE_OK) {
        if (info.valid_data) {
          std::cout << "\n[RESPONSE] GetAppUpdateSettingsResponse received:" << std::endl;
          std::cout << "  Request ID: " << message.id() << std::endl;
          std::cout << "  App Name: " << message.app_name() << std::endl;
          std::cout << "  Settings: " << message.app_settings() << std::endl;
          std::cout << "  Return Code: " << static_cast<int>(message.ret_code().code()) << std::endl;
        }
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
          std::cout << "  Return Code: " << static_cast<int>(message.ret_code().code()) << std::endl;
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
          std::cout << "  Return Code: " << static_cast<int>(message.ret_code().code()) << std::endl;
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
          std::cout << "  Return Code: " << static_cast<int>(message.ret_code().code()) << std::endl;
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
