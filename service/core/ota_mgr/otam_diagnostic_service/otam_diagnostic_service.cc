#include "otam_diagnostic_service.h"

#include <iostream>
#include <utility>

constexpr const char* OtamDiagnosticService::kOtaMgrInfoTopic;

OtamDiagnosticService::OtamDiagnosticService() {}

void OtamDiagnosticService::Initialize() {
  if (state_ != ServiceState::UNINITIALIZED) {
    std::cout << "OtamDiagnosticService already initialized"
              << std::endl;
    return;
  }
  name_ = "OtamDiagnosticService";

  std::cout << "Initializing OtamDiagnosticService..." << std::endl;

  // Create DDS implementation for NotifyOtaMgrInfo
  auto dds_impl = std::make_shared<OtamDdsImpl<OtaManager::NotifyOtaMgrInfo>>();

  // Create DDS entities
  notify_ota_mgr_info_ = std::make_shared<OtaManager::NotifyOtaMgrInfoPubSubType>();
  dds_impl->Create(kOtaMgrInfoTopic,
                   std::static_pointer_cast<void>(notify_ota_mgr_info_),
                   nullptr);

  RegisterDds(kOtaMgrInfoTopic, dds_impl);

  // -------------------------------------------------------------------------
  // Late-subscriber support
  //
  // Problem: PublishOtaMgrInfo() is called during service initialization
  // (before any external subscriber connects). With VOLATILE durability
  // (FastDDS default), those messages are silently dropped.
  //
  // Solution: Attach a DataWriterListener that fires on_publication_matched
  // whenever a subscriber connects. Until the first subscriber appears, every
  // PublishOtaMgrInfo() call stores the message in pending_msgs_. As soon as
  // a subscriber connects, FlushPendingMessages() delivers them in order.
  // -------------------------------------------------------------------------
  writer_ = dds_impl->GetWriter();
  if (writer_) {
    writer_matched_listener_ = std::make_shared<WriterMatchedListener>(this);
    writer_->set_listener(
        writer_matched_listener_.get(),
        eprosima::fastdds::dds::StatusMask::publication_matched());
    std::cout << "OtamDiagnosticService: WriterMatchedListener attached"
              << std::endl;
  } else {
    std::cerr << "OtamDiagnosticService: Warning - DataWriter unavailable, "
              << "late-subscriber buffering disabled" << std::endl;
  }

  state_ = ServiceState::INITIALIZED;
  std::cout << "OtamDiagnosticService initialized successfully" << std::endl;
}

void OtamDiagnosticService::Start() {
  state_ = ServiceState::STARTED;
  std::cout << "OtamDiagnosticService: Starting OtamDiagnosticService."
            << std::endl;
}

void OtamDiagnosticService::Stop() {
  std::cout << "OtamDiagnosticService: Stopping OtamDiagnosticService."
            << std::endl;

  // Detach the listener first (blocks until any in-flight callback returns)
  // then null out writer_ under the pending_mutex_ so FlushPendingMessages
  // on the DDS thread sees a consistent state.
  {
    eprosima::fastdds::dds::DataWriter* w = nullptr;
    {
      std::lock_guard<std::mutex> lock(pending_mutex_);
      w = writer_;
      writer_ = nullptr;
    }
    if (w) {
      w->set_listener(nullptr, eprosima::fastdds::dds::StatusMask::none());
    }
  }

  // Destroy all DDS instances
  for (auto& pair : dds_registry_) {
    if (pair.second) {
      std::cout << "OtamDiagnosticService: Destroying DDS for topic: "
                << pair.first << std::endl;
      pair.second->Destroy();
    }
  }
  state_ = ServiceState::UNINITIALIZED;
}

void OtamDiagnosticService::PublishOtaMgrInfo(int32_t err_code,
                                               const std::string& err_msg) {
  if (state_ != ServiceState::STARTED) {
    std::cout << "OtamDiagnosticService: Not started, buffering message: "
              << err_msg << std::endl;
    std::lock_guard<std::mutex> lock(pending_mutex_);
    pending_msgs_.push_back({err_code, err_msg});
    return;
  }

  // If no subscriber is currently connected, buffer the message.
  // It will be delivered by FlushPendingMessages() when a subscriber joins.
  if (matched_count_.load() == 0) {
    std::lock_guard<std::mutex> lock(pending_mutex_);
    pending_msgs_.push_back({err_code, err_msg});
    std::cout << "OtamDiagnosticService: [BUFFERED] No subscriber yet - "
              << "pending=" << pending_msgs_.size()
              << " err_code=" << err_code
              << " msg=" << err_msg << std::endl;
    return;
  }

  // At least one subscriber is connected.
  // Flush any buffered messages first to preserve chronological order.
  FlushPendingMessages();

  // Publish the current message
  auto dds = GetDds(kOtaMgrInfoTopic);
  if (!dds) {
    std::cout << "OtamDiagnosticService: DDS not found for topic: "
              << kOtaMgrInfoTopic << std::endl;
    return;
  }

  auto msg = std::make_shared<OtaManager::NotifyOtaMgrInfo>();
  msg->err_code(err_code);
  msg->err_msg(err_msg);

  std::cout << "OtamDiagnosticService: Publishing OtaMgrInfo"
            << " err_code=" << err_code
            << " err_msg=" << err_msg << std::endl;

  dds->Write(std::static_pointer_cast<void>(msg));
}

// -----------------------------------------------------------------------------
// WriterMatchedListener::on_publication_matched
//
// Called by the FastDDS internal thread whenever a subscriber
// connects or disconnects from this DataWriter's topic.
// -----------------------------------------------------------------------------
void OtamDiagnosticService::WriterMatchedListener::on_publication_matched(
    eprosima::fastdds::dds::DataWriter* /*writer*/,
    const eprosima::fastdds::dds::PublicationMatchedStatus& info) {
  service_->matched_count_.store(info.current_count);

  if (info.current_count_change > 0) {
    std::cout << "OtamDiagnosticService: Subscriber connected"
              << " (total=" << info.current_count << ")"
              << " - flushing buffered messages..." << std::endl;
    service_->FlushPendingMessages();
  } else if (info.current_count_change < 0) {
    std::cout << "OtamDiagnosticService: Subscriber disconnected"
              << " (remaining=" << info.current_count << ")" << std::endl;
  }
}

// -----------------------------------------------------------------------------
// FlushPendingMessages
//
// Drains pending_msgs_ and writes each entry directly through the DataWriter.
// Uses a swap to minimise lock hold time; DDS writes happen outside the lock.
// Thread-safe: may be called concurrently from the DDS callback thread and
// the application thread.
// -----------------------------------------------------------------------------
void OtamDiagnosticService::FlushPendingMessages() {
  // Capture writer_ and swap pending messages under the same lock.
  // This prevents a race where Stop() sets writer_=nullptr and destroys
  // the DataWriter while we are still iterating the flush loop.
  std::vector<std::pair<int32_t, std::string>> to_flush;
  eprosima::fastdds::dds::DataWriter* w = nullptr;
  {
    std::lock_guard<std::mutex> lock(pending_mutex_);
    w = writer_;
    if (!w || pending_msgs_.empty()) return;
    to_flush.swap(pending_msgs_);
  }

  std::cout << "OtamDiagnosticService: Flushing " << to_flush.size()
            << " buffered message(s)..." << std::endl;

  for (auto& [code, msg] : to_flush) {
    auto pending_msg = std::make_shared<OtaManager::NotifyOtaMgrInfo>();
    pending_msg->err_code(code);
    pending_msg->err_msg(msg);
    w->write(pending_msg.get());
    std::cout << "OtamDiagnosticService: [FLUSHED] err_code=" << code
              << " msg=" << msg << std::endl;
  }
}

void OtamDiagnosticService::RegisterDds(const std::string& topic_name,
                                         std::shared_ptr<OtamDds> dds) {
  dds_registry_[topic_name] = dds;
  std::cout << "Registered DDS for topic: " << topic_name << std::endl;
}

std::shared_ptr<OtamDds> OtamDiagnosticService::GetDds(
    const std::string& topic_name) {
  auto it = dds_registry_.find(topic_name);
  if (it != dds_registry_.end()) {
    return it->second;
  }
  return nullptr;
}
