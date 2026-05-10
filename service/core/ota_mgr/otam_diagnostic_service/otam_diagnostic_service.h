#ifndef OTAM_DIAGNOSTIC_SERVICE_H_
#define OTAM_DIAGNOSTIC_SERVICE_H_

#include "otam_common/otam_service.h"
#include "otam_common/otam_dds_impl.h"
#include "otam_common/otam_dds_idl.h"

#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/core/status/PublicationMatchedStatus.hpp>

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include <atomic>

// OTA Manager Diagnostic Service
// Manages DDS communication for OTA application state updates
class OtamDiagnosticService : public OtamService {
 public:
  OtamDiagnosticService();
  ~OtamDiagnosticService() = default;

  // Initialize the diagnostic service and DDS entities
  void Initialize() override;
  void Start() override;
  void Stop() override;

  // Publish OTA manager information with error code and message
  // @param err_code: Error code representing OTA state
  // @param err_msg: Error message describing the error
  //
  // If no subscriber is currently connected, the message is buffered internally
  // and delivered automatically when the first subscriber connects.
  virtual void PublishOtaMgrInfo(int32_t err_code, const std::string& err_msg);

  // -----------------------------------------------------------------------
  // F007 — periodic heartbeat publish.
  //
  // Start() spawns a background thread that publishes a
  // otamanager::NotifyOtaMgrHeartbeat every `heartbeat_interval_` (default
  // 3 s, overridable via env HEARTBEAT_INTERVAL_SEC at construction time).
  // Stop() signals the thread and joins it; safe to Start/Stop repeatedly.
  //
  // The heartbeat shares the same DDS participant as NotifyOtaMgrInfo but
  // a separate DataWriter and topic, so its publish cadence is decoupled
  // from the late-subscriber buffering used by PublishOtaMgrInfo.
  //
  // Topic: /mmf/otamanager/diagnostic_service/NotifyOtaMgrHeartbeat
  // -----------------------------------------------------------------------
  virtual void PublishHeartbeatOnce();   // exposed for unit tests
  std::chrono::seconds heartbeat_interval() const { return heartbeat_interval_; }
  int32_t heartbeat_seq() const { return heartbeat_seq_.load(); }

 private:
  // -----------------------------------------------------------------------
  // DataWriter listener: tracks subscriber connection events
  // When a subscriber connects, buffered messages are flushed in order.
  // -----------------------------------------------------------------------
  class WriterMatchedListener : public eprosima::fastdds::dds::DataWriterListener {
   public:
    explicit WriterMatchedListener(OtamDiagnosticService* service)
        : service_(service) {}

    void on_publication_matched(
        eprosima::fastdds::dds::DataWriter* writer,
        const eprosima::fastdds::dds::PublicationMatchedStatus& info) override;

   private:
    OtamDiagnosticService* service_;
  };

  // -----------------------------------------------------------------------
  // Flushes all buffered messages to the DataWriter.
  // Safe to call from both the DDS callback thread and the caller thread.
  // -----------------------------------------------------------------------
  void FlushPendingMessages();

  // Registry of DDS abstractions keyed by topic name
  std::map<std::string, std::shared_ptr<OtamDds>>
      dds_registry_;

  // Topic name constant
  static constexpr const char* kOtaMgrInfoTopic =
      "/mmf/otamanager/diagnostic_service/NotifyOtaMgrInfo";

  std::shared_ptr<otamanager::NotifyOtaMgrInfoPubSubType> notify_ota_mgr_info_;

  // F007 — heartbeat fields.
  static constexpr const char* kOtaMgrHeartbeatTopic =
      "/mmf/otamanager/diagnostic_service/NotifyOtaMgrHeartbeat";
  std::shared_ptr<otamanager::NotifyOtaMgrHeartbeatPubSubType>
      notify_ota_mgr_heartbeat_;
  std::chrono::seconds heartbeat_interval_{3};
  std::thread heartbeat_thread_;
  std::atomic<bool> heartbeat_running_{false};
  std::atomic<int32_t> heartbeat_seq_{0};
  std::condition_variable heartbeat_cv_;
  std::mutex heartbeat_cv_mutex_;

  // Internal: drives the heartbeat thread. Sleeps on heartbeat_cv_ so
  // Stop() can wake the thread immediately instead of waiting out the
  // current interval.
  void HeartbeatLoop();
  void StartHeartbeat();
  void StopHeartbeat();

  // -----------------------------------------------------------------------
  // Late-subscriber support: raw DataWriter pointer (lifetime tied to dds_impl)
  // -----------------------------------------------------------------------
  eprosima::fastdds::dds::DataWriter* writer_{nullptr};
  std::shared_ptr<WriterMatchedListener> writer_matched_listener_;

  // Number of currently matched subscribers (updated atomically by DDS thread)
  std::atomic<int32_t> matched_count_{0};

  // Messages buffered while no subscriber is connected
  std::vector<std::pair<int32_t, std::string>> pending_msgs_;
  std::mutex pending_mutex_;

  // Register a DDS abstraction
  void RegisterDds(const std::string& topic_name,
                   std::shared_ptr<OtamDds> dds);

  // Get DDS abstraction by topic name
  std::shared_ptr<OtamDds> GetDds(
      const std::string& topic_name);
};

#endif  // OTAM_DIAGNOSTIC_SERVICE_H_
