#include "GrpcProvider.h"
#include "Utils.h"
#include "Logger.h"
#include "ConfigManager.h"
#include <iostream>
#include <chrono>

// Helper to generate a unique enough request ID
static std::string generate_request_id() {
    static std::atomic<long> counter(0);
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    return "grpc-provider-" + std::to_string(ms) + "-" + std::to_string(counter++);
}

// --- GrpcProvider::Subscription Implementation ---

GrpcProvider::Subscription::Subscription(
    std::string path,
    std::string id,
    std::unique_ptr<grpc::ClientContext> context,
    std::unique_ptr<grpc::ClientReaderInterface<grpcProtobufMessages::SubscribeStreamMessage>> stream,
    std::function<void(const VehicleData&)> data_handler,
    std::function<void(const std::string&)> cleanup_handler)
    : m_path(std::move(path)),
      m_id(std::move(id)),
      m_is_active(false),
      m_context(std::move(context)),
      m_stream(std::move(stream)),
      m_data_handler(std::move(data_handler)),
      m_cleanup_handler(std::move(cleanup_handler)) {}

GrpcProvider::Subscription::~Subscription() {
    stop();
}

void GrpcProvider::Subscription::start() {
    m_is_active = true;
    m_thread = std::thread(&Subscription::run, this);
}

void GrpcProvider::Subscription::stop() {
    bool expected = true;
    if (m_is_active.compare_exchange_strong(expected, false)) {
        LOG_INFO << "Stopping subscription for path: " << m_path << " (ID: " << m_id << ")";
        if (m_context) {
            m_context->TryCancel();
        }
        if (m_thread.joinable()) {
            m_thread.join();
        }
        LOG_INFO << "Successfully stopped subscription for path: " << m_path << " (ID: " << m_id << ")";
    }
}
/*
bool GrpcProvider::Subscription::is_active() const {
    return m_is_active.load();
}

std::string GrpcProvider::Subscription::get_id() const {
    return m_id;
}
*/
std::string GrpcProvider::Subscription::get_path() const {
    return m_path;
}

void GrpcProvider::Subscription::run() {
    grpcProtobufMessages::SubscribeStreamMessage event;
    LOG_INFO << "m_is_active.load(): " << m_is_active.load();
    while (m_is_active.load() && m_stream->Read(&event)) {
        if (event.mtype() == grpcProtobufMessages::SubscribeResponseType::EVENT && event.has_event()) {
            const auto& sub_event = event.event();
            if (sub_event.has_successresponse()) {
                const auto& data_pack = sub_event.successresponse().datapack();
                for (const auto& data : data_pack.data()) {
                    if (data.dp_size() > 0) {
                        VehicleData vd;
                        vd.path = data.path();
                        const auto& first_dp = data.dp(0);
                        vd.value = first_dp.value();
                        try {
                            vd.timestamp = std::stoll(first_dp.ts());
                        } catch (const std::invalid_argument&) {
                            vd.timestamp = 0;
                        }

                        if (m_data_handler) {
                            /*
                            try {
                                LOG_INFO << "gRPC Sub Received: " << vd.path << " = " << std::any_cast<std::string>(vd.value);
                            } catch (const std::bad_any_cast& e) {
                                LOG_ERROR << "Failed to cast gRPC subscription value to string for logging.";
                            }
                            */
                            m_data_handler(vd);
                        }
                    }
                }
            }
        }
    }

    if (m_is_active.load()) {
        LOG_WARN << "gRPC stream for " << m_path << " (ID: " << m_id << ") terminated unexpectedly. Queuing for cleanup.";
        if (m_cleanup_handler) {
            m_cleanup_handler(m_id);
        }
    } else {
        LOG_INFO << "gRPC stream for " << m_path << " (ID: " << m_id << ") terminated as requested.";
    }
}

// --- GrpcProvider Implementation ---

GrpcProvider::GrpcProvider()
    : m_name("GrpcProvider") {
    auto& config = ConfigManager::getInstance();
    m_server_address = config.provider.grpcAddress;
    m_channel = grpc::CreateChannel(m_server_address, grpc::InsecureChannelCredentials());
    m_stub = grpcProtobufMessages::VISS::NewStub(m_channel);
}

// Constructor for testing with a mock stub and channel
GrpcProvider::GrpcProvider(std::unique_ptr<grpcProtobufMessages::VISS::StubInterface> mock_stub, std::shared_ptr<grpc::ChannelInterface> mock_channel)
    : m_name("GrpcProvider"),
      m_server_address("mock-server"),
      m_channel(std::move(mock_channel)),
      m_stub(std::move(mock_stub)) {
    // This constructor is for testing purposes.
}

GrpcProvider::~GrpcProvider() {
    if (m_monitoring_active.load()) {
        stop();
    }
    LOG_INFO << "GrpcProvider destructor completed.";
}

void GrpcProvider::start() {
    if (m_monitoring_active.exchange(true)) {
        LOG_WARN << "GrpcProvider already started.";
        return;
    }
    LOG_INFO << "GrpcProvider started for address: " << m_server_address;
    m_is_stopping = false;
    m_state_monitor_thread = std::thread(&GrpcProvider::state_monitor_loop, this);
    m_cleanup_thread = std::thread(&GrpcProvider::cleanup_thread_main, this);
}

void GrpcProvider::stop() {
    if (m_is_stopping.exchange(true)) {
        return; // Stop is already in progress.
    }

    LOG_INFO << "GrpcProvider::stop() called.";

    // 1. Signal loops to terminate.
    m_monitoring_active = false;

    // 2. Unblock the cleanup_thread if it's waiting.
    m_cleanup_queue.push(""); // Poison pill

    // 4. Wait for all provider-managed threads to finish.
    if (m_state_monitor_thread.joinable()) {
        m_state_monitor_thread.join();
    }
    if (m_cleanup_thread.joinable()) {
        m_cleanup_thread.join();
    }

    // 5. Clean up all subscription objects (which includes joining their threads).
    cleanup_all_subscriptions();

    LOG_INFO << "GrpcProvider::stop() completed.";
}

std::string GrpcProvider::getName() const {
    return m_name;
}

void GrpcProvider::setDataChannel(const std::shared_ptr<ThreadSafeQueue<VehicleData>>& channel) {
    m_data_channel = channel;
}

void GrpcProvider::setValue(const std::string& path, const std::any& value) {
    grpcProtobufMessages::SetRequestMessage request;
    request.set_path(path);
    
    try {
        request.set_value(std::any_cast<std::string>(value));
    } catch (const std::bad_any_cast& e) {
        LOG_ERROR << "Failed to cast value for path " << path << ": " << e.what();
        return;
    }

    grpcProtobufMessages::SetResponseMessage response;
    grpc::ClientContext context;

    grpc::Status status = m_stub->SetRequest(&context, request, &response);

    if (!status.ok()) {
        LOG_ERROR << "gRPC SetRequest failed: " << status.error_message();
    }
}

void GrpcProvider::setVssTree(std::shared_ptr<viss::VSSTree> vss_tree) {
    // GrpcProvider is a client and doesn't host the VSS tree.
}

void GrpcProvider::subscribe(const std::string& path) {
    std::lock_guard<std::mutex> lock(m_subscription_mutex);

    if (m_path_to_subscription_id.count(path)) {
        LOG_WARN << "Path " << path << " is already subscribed.";
        return;
    }

    grpcProtobufMessages::SubscribeRequestMessage request;
    request.set_requestid(generate_request_id());
    request.set_path(path);

    auto context = std::make_unique<grpc::ClientContext>();
    auto stream = m_stub->SubscribeRequest(context.get(), request);
    
    grpcProtobufMessages::SubscribeStreamMessage response;
    if (stream && stream->Read(&response)) {
        if (response.mtype() == grpcProtobufMessages::SubscribeResponseType::RESPONSE && response.has_response()) {
            const auto& sub_response = response.response();
            if (response.status() == grpcProtobufMessages::ResponseStatus::SUCCESS) {
                std::string subscription_id = sub_response.subscriptionid();

                auto data_handler = [this](const VehicleData& vd) {
                    if (m_data_channel) {
                        m_data_channel->push(vd);
                    }
                };
                auto cleanup_handler = [this](const std::string& id) {
                    m_cleanup_queue.push(id);
                };

                auto subscription = std::make_shared<Subscription>(
                    path, subscription_id, std::move(context), std::move(stream), data_handler, cleanup_handler);
                
                m_subscriptions[subscription_id] = subscription;
                m_path_to_subscription_id[path] = subscription_id;
                
                subscription->start();
                LOG_INFO << "Subscribed to " << path << " with subscription ID: " << subscription_id;
            } else {
                LOG_ERROR << "gRPC subscription failed for path " << path << ": " << sub_response.errorresponse().reason();
            }
        }
    } else {
        LOG_ERROR << "Failed to get subscription ID for path: " << path;
    }
}

void GrpcProvider::unsubscribe(const std::string& path) {
    std::string subscription_id;
    {
        std::lock_guard<std::mutex> lock(m_subscription_mutex);
        if (m_path_to_subscription_id.find(path) == m_path_to_subscription_id.end()) {
            LOG_WARN << "Unsubscribe called for a path that is not subscribed: " << path;
            return;
        }
        subscription_id = m_path_to_subscription_id.at(path);
    }

    // Send an explicit unsubscribe request to the server
    grpcProtobufMessages::UnsubscribeRequestMessage unsub_request;
    unsub_request.set_subscriptionid(subscription_id);
    grpcProtobufMessages::UnsubscribeResponseMessage unsub_response;
    grpc::ClientContext unsub_context;

    m_stub->UnsubscribeRequest(&unsub_context, unsub_request, &unsub_response);

    // Clean up local resources
    std::lock_guard<std::mutex> lock(m_subscription_mutex);
    if (m_subscriptions.count(subscription_id)) {
        m_subscriptions.at(subscription_id)->stop();
        m_subscriptions.erase(subscription_id);
    }
    m_path_to_subscription_id.erase(path);

    LOG_INFO << "Unsubscribed and cleaned up resources for path: " << path;
}

void GrpcProvider::cleanup_all_subscriptions() {
    std::lock_guard<std::mutex> lock(m_subscription_mutex);
    for (auto& pair : m_subscriptions) {
        if (pair.second) {
            pair.second->stop();
        }
    }
    m_subscriptions.clear();
    m_path_to_subscription_id.clear();
    LOG_INFO << "All gRPC subscriptions have been cleaned up.";
}

void GrpcProvider::cleanup_thread_main() {
    while (true) {
        std::string sub_id_to_clean;
        m_cleanup_queue.wait_and_pop(sub_id_to_clean);

        if (sub_id_to_clean.empty()) { // Poison pill
            break;
        }

        std::string path_to_clean;
        std::shared_ptr<Subscription> sub_to_stop;
        {
            std::lock_guard<std::mutex> lock(m_subscription_mutex);
            if (m_subscriptions.count(sub_id_to_clean)) {
                sub_to_stop = m_subscriptions.at(sub_id_to_clean);
                path_to_clean = sub_to_stop->get_path();
            }
        }
        
        if (sub_to_stop) {
            LOG_INFO << "Cleanup thread processing subscription ID: " << sub_id_to_clean << " (path: " << path_to_clean << ")";
            sub_to_stop->stop();
            std::lock_guard<std::mutex> lock(m_subscription_mutex);
            m_subscriptions.erase(sub_id_to_clean);
            m_path_to_subscription_id.erase(path_to_clean);
        }
    }
    LOG_INFO << "gRPC cleanup thread finished.";
}

void GrpcProvider::state_monitor_loop() {
    grpc_connectivity_state last_known_state = GRPC_CHANNEL_IDLE;

    while (m_monitoring_active.load()) {
        grpc_connectivity_state current_state = m_channel->GetState(true);

        if (current_state != last_known_state) {
            LOG_INFO << "gRPC Channel State changed to: " << current_state;
            if (last_known_state == GRPC_CHANNEL_READY && current_state != GRPC_CHANNEL_READY) {
                LOG_WARN << "gRPC connection lost. Cleaning up all subscriptions.";
                cleanup_all_subscriptions();
            }
            if (current_state == GRPC_CHANNEL_READY) {
                LOG_INFO << "gRPC channel is ready. Subscribing to Vehicle.*";
                subscribe("Vehicle.*");
            }
            last_known_state = current_state;
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    LOG_INFO << "gRPC state monitoring loop finished.";
}
