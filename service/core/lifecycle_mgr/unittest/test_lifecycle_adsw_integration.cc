/**
 * @file test_lifecycle_adsw_integration.cc
 * @brief Integration test for lifecycle_mgr <-> adsw_manager handshake
 *
 * This test validates the service registration handshake between
 * lifecycle_mgr and adsw_manager using real DDS communication.
 *
 * Handshake flow:
 * 1. adsw_manager starts and sends UpdateServiceListRequest every 3s
 * 2. lifecycle_mgr receives request and responds with UpdateServiceListResponse(SUCCESS)
 * 3. adsw_manager receives response and transitions to RUNNING state
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <thread>
#include <chrono>
#include <memory>
#include <atomic>
#include <condition_variable>
#include <mutex>

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>
#include <fastdds/rtps/transport/UDPv4TransportDescriptor.hpp>

#include "endpoint/UpdateServiceListEndPoint.hpp"
#include "endpoint/StartServiceEndPoint.hpp"
#include "transmitter/DDSMessageEndPoint.hpp"
#include "include/MessageType.hpp"

#include "lifecycle/UpdateServiceListRequest.hpp"
#include "lifecycle/UpdateServiceListRequestPubSubTypes.hpp"
#include "lifecycle/StartServiceRequest.hpp"
#include "lifecycle/StartServiceRequestPubSubTypes.hpp"

using namespace lifecycle_manager;
using namespace lifecyclemgr;
using namespace eprosima::fastdds::dds;
using namespace eprosima::fastdds::rtps;

using ::testing::_;
using ::testing::AtLeast;

/**
 * @brief Create a DDS participant with UDP-only transport (no SHM).
 * SHM transport causes inter-participant futex deadlocks in same-process tests
 * when stale shm segments exist from prior runs.
 */
static DomainParticipant* CreateUdpOnlyParticipant(int domain_id, const char* name) {
    DomainParticipantQos qos;
    qos.name(name);
    qos.transport().use_builtin_transports = false;
    auto udp = std::make_shared<UDPv4TransportDescriptor>();
    qos.transport().user_transports.push_back(udp);
    return DomainParticipantFactory::get_instance()->create_participant(domain_id, qos);
}

/**
 * @brief Simulates adsw_manager handshake behavior
 *
 * This class simulates the adsw_manager side of the handshake protocol:
 * - Sends UpdateServiceListRequest periodically (every 3s)
 * - Listens for UpdateServiceListResponse
 * - Transitions to RUNNING state upon receiving SUCCESS response
 */
class AdswManagerSimulator {
public:
    enum class State { INIT, STARTED, RUNNING, STOPPED };

    AdswManagerSimulator() = default;
    ~AdswManagerSimulator() { Stop(); }

    /**
     * @brief Initialize DDS endpoints for handshake
     */
    bool Initialize(DomainParticipant* participant) {
        participant_ = participant;

        if (!participant_) {
            return false;
        }

        publisher_ = participant_->create_publisher(PUBLISHER_QOS_DEFAULT);
        if (!publisher_) {
            return false;
        }

        subscriber_ = participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT);
        if (!subscriber_) {
            return false;
        }

        // Create endpoint for sending requests
        request_endpoint_ = std::make_unique<UpdateServiceListEndPoint>(
            MessageType::UPDATE_SERVICE_LIST_REQUEST,
            "mmf/lifecyclemgr/UpdateServiceListRequest");

        if (!request_endpoint_->Initialize(participant_, publisher_, nullptr)) {
            return false;
        }

        // Create endpoint for receiving responses
        response_endpoint_ = std::make_unique<UpdateServiceListEndPoint>(
            MessageType::UPDATE_SERVICE_LIST_RESPONSE,
            "mmf/lifecyclemgr/UpdateServiceListResponse",
            [this](auto* msg) { OnResponseReceived(msg); });

        if (!response_endpoint_->Initialize(participant_, nullptr, subscriber_)) {
            return false;
        }

        return true;
    }

    /**
     * @brief Start the handshake process in a separate thread
     */
    void Start() {
        if (state_ != State::INIT) {
            return;
        }

        state_ = State::STARTED;
        running_ = true;

        process_thread_ = std::thread([this]() { ProcessLoop(); });
    }

    /**
     * @brief Stop the handshake process
     */
    void Stop() {
        running_ = false;
        cv_.notify_all();

        if (process_thread_.joinable()) {
            process_thread_.join();
        }

        // Note: Finalize() calls can block in DDS cleanup
        // Let TearDown/destructors handle endpoint cleanup

        state_ = State::STOPPED;
    }

    /**
     * @brief Get current state
     */
    State GetState() const {
        return state_;
    }

    /**
     * @brief Get request count (for retry verification)
     */
    int GetRequestCount() const {
        return request_count_;
    }

    /**
     * @brief Wait for RUNNING state with timeout
     * @return true if RUNNING state reached, false if timeout
     */
    bool WaitForRunning(int timeout_ms) {
        std::unique_lock<std::mutex> lock(mutex_);
        return cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms),
                           [this]() { return state_ == State::RUNNING; });
    }

private:
    /**
     * @brief Main processing loop - sends requests and handles retries
     */
    void ProcessLoop() {
        last_request_time_ = std::chrono::steady_clock::now();

        // Send initial request immediately (no mutex held)
        SendUpdateServiceListRequest();

        while (running_) {
            bool should_send = false;

            {
                std::unique_lock<std::mutex> lock(mutex_);

                // Wait for 3 seconds or until stopped/running
                cv_.wait_for(lock, std::chrono::milliseconds(3000),
                            [this]() { return !running_ || state_ == State::RUNNING; });

                if (!running_ || state_ == State::RUNNING) {
                    break;
                }

                // Check if 3 seconds have passed since last request
                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - last_request_time_).count();

                should_send = (elapsed >= 3000);
            }  // mutex released before write to prevent deadlock with DDS callback

            if (should_send) {
                SendUpdateServiceListRequest();
            }
        }
    }

    /**
     * @brief Send UpdateServiceListRequest to lifecycle_mgr
     */
    void SendUpdateServiceListRequest() {
        UpdateServiceListRequest request;

        // Add a dummy service to the request
        lifecyclemgr::Service service;
        service.name("adsw_manager");
        service.service_type(lifecyclemgr::ServiceType::NORMAL);
        service.recovery_policy(lifecyclemgr::RecoveryPolicy::ALWAYS);
        service.is_enabled(true);
        service.os_type(lifecyclemgr::OsType::ROS2);

        request.service_list().push_back(service);

        if (request_endpoint_) {
            request_endpoint_->Write(&request);
            request_count_++;
            last_request_time_ = std::chrono::steady_clock::now();
        }
    }

    /**
     * @brief Callback when UpdateServiceListResponse is received
     */
    void OnResponseReceived(UpdateServiceListRequest* response_data) {
        // Note: The endpoint template uses UpdateServiceListRequest as storage type
        // but when configured for RESPONSE, it actually receives UpdateServiceListResponse
        // This is a quirk of the current implementation

        // Guard: ignore spurious DDS callbacks before first request is sent
        // DDS can deliver zero-initialized samples when DataWriter/DataReader match
        if (request_count_.load() == 0) {
            return;
        }

        // Cast to response type (safe because we know this endpoint receives responses)
        auto* response = reinterpret_cast<UpdateServiceListResponse*>(response_data);

        if (response && response->ret_code() == ReturnCode::SUCCESS) {
            std::lock_guard<std::mutex> lock(mutex_);
            state_ = State::RUNNING;
            cv_.notify_all();
        }
    }

    DomainParticipant* participant_ = nullptr;
    Publisher* publisher_ = nullptr;
    Subscriber* subscriber_ = nullptr;

    std::unique_ptr<UpdateServiceListEndPoint> request_endpoint_;
    std::unique_ptr<UpdateServiceListEndPoint> response_endpoint_;

    std::atomic<State> state_{State::INIT};
    std::atomic<int> request_count_{0};
    std::atomic<bool> running_{false};

    std::thread process_thread_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::chrono::steady_clock::time_point last_request_time_;
};


/**
 * @brief Test-only DDS endpoint that receives StartServiceRequest.
 *
 * lifecycle_mgr's StartServiceEndPoint always reads as StartServiceResponse,
 * so a custom subclass is required to correctly deserialize incoming requests.
 */
class StartServiceRequestReceiver : public DDSMessageEndPoint {
public:
    using Callback = std::function<void(StartServiceRequest*)>;

    StartServiceRequestReceiver(std::string topic, Callback cb)
        : DDSMessageEndPoint(std::move(topic)), cb_(std::move(cb)) {
        SetTypeSupport(std::make_shared<TypeSupport>(new StartServiceRequestPubSubType()));
    }

    ~StartServiceRequestReceiver() override = default;

    void OnDataAvailable() override {
        if (Read<StartServiceRequest>(req_) == true && cb_) {
            cb_(&req_);
        }
    }

    void OnDataPublished(bool) override {}

private:
    StartServiceRequest req_;
    Callback cb_;
};

/**
 * @brief Simulates adsw_manager StartService handling:
 * 1. Receives StartServiceRequest from lifecycle_mgr
 * 2. Simulates docker exec (mock: sets container_started_ flag)
 * 3. Sends StartServiceResponse(SUCCESS) back to lifecycle_mgr
 */
class AdswManagerStartServiceSimulator {
public:
    AdswManagerStartServiceSimulator() = default;
    ~AdswManagerStartServiceSimulator() { Stop(); }

    bool Initialize(DomainParticipant* participant) {
        participant_ = participant;
        if (!participant_) return false;

        publisher_ = participant_->create_publisher(PUBLISHER_QOS_DEFAULT);
        if (!publisher_) return false;

        subscriber_ = participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT);
        if (!subscriber_) return false;

        request_receiver_ = std::make_unique<StartServiceRequestReceiver>(
            "mmf/lifecyclemgr/StartServiceRequest",
            [this](StartServiceRequest* req) { OnRequestReceived(req); });

        if (!request_receiver_->Initialize(participant_, nullptr, subscriber_)) {
            return false;
        }

        response_sender_ = std::make_unique<StartServiceEndPoint>(
            MessageType::START_SERVICE_RESPONSE,
            "mmf/lifecyclemgr/StartServiceResponse");

        if (!response_sender_->Initialize(participant_, publisher_, nullptr)) {
            return false;
        }

        return true;
    }

    void Stop() {
        running_ = false;
        cv_.notify_all();
    }

    bool WaitForRequestReceived(int timeout_ms) {
        std::unique_lock<std::mutex> lock(mutex_);
        return cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms),
                            [this]() { return request_received_; });
    }

    bool WaitForResponseSent(int timeout_ms) {
        std::unique_lock<std::mutex> lock(mutex_);
        return cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms),
                            [this]() { return response_sent_; });
    }

    bool IsContainerStarted() const { return container_started_; }
    std::string GetReceivedServiceName() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return received_service_name_;
    }
    bool GetReceivedIsGroup() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return received_is_group_;
    }

private:
    void OnRequestReceived(StartServiceRequest* req) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!req->service_list().empty()) {
                received_service_name_ = req->service_list()[0];
            }
            received_is_group_ = req->is_group();
            request_received_ = true;
        }
        cv_.notify_all();

        container_started_ = true;  // simulate docker exec

        StartServiceResponse response;
        response.ret_code(ReturnCode::SUCCESS);
        if (response_sender_) {
            response_sender_->Write(&response);
        }

        {
            std::lock_guard<std::mutex> lock(mutex_);
            response_sent_ = true;
        }
        cv_.notify_all();
    }

    DomainParticipant* participant_ = nullptr;
    Publisher* publisher_ = nullptr;
    Subscriber* subscriber_ = nullptr;

    std::unique_ptr<StartServiceRequestReceiver> request_receiver_;
    std::unique_ptr<StartServiceEndPoint> response_sender_;

    std::atomic<bool> running_{true};
    std::atomic<bool> container_started_{false};

    mutable std::mutex mutex_;
    std::condition_variable cv_;
    bool request_received_ = false;
    bool response_sent_ = false;
    std::string received_service_name_;
    bool received_is_group_ = false;
};

/**
 * @brief Test fixture for lifecycle_mgr <-> adsw_manager integration tests
 */
class LifecycleAdswIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use UDP-only transport: SHM causes futex deadlocks between same-process
        // participants when stale shared memory segments exist from prior test runs.
        lcm_participant_ = CreateUdpOnlyParticipant(0, "LifecycleManager_Participant");
        ASSERT_NE(lcm_participant_, nullptr);

        adsw_participant_ = CreateUdpOnlyParticipant(0, "AdswManager_Participant");
        ASSERT_NE(adsw_participant_, nullptr);

        adsw_simulator_ = std::make_unique<AdswManagerSimulator>();
        ASSERT_TRUE(adsw_simulator_->Initialize(adsw_participant_));

        start_service_sim_ = std::make_unique<AdswManagerStartServiceSimulator>();
        ASSERT_TRUE(start_service_sim_->Initialize(adsw_participant_));

        // Wait for DDS discovery
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    void TearDown() override {
        if (start_service_sim_) {
            start_service_sim_->Stop();
            start_service_sim_.reset();
        }

        if (adsw_simulator_) {
            adsw_simulator_->Stop();
            adsw_simulator_.reset();  // endpoint destructors delete DDS entities
        }

        if (lcm_participant_) {
            lcm_participant_->delete_contained_entities();
            DomainParticipantFactory::get_instance()->delete_participant(lcm_participant_);
            lcm_participant_ = nullptr;
        }

        if (adsw_participant_) {
            adsw_participant_->delete_contained_entities();
            DomainParticipantFactory::get_instance()->delete_participant(adsw_participant_);
            adsw_participant_ = nullptr;
        }
    }

    DomainParticipant* lcm_participant_ = nullptr;
    DomainParticipant* adsw_participant_ = nullptr;
    std::unique_ptr<AdswManagerSimulator> adsw_simulator_;
    std::unique_ptr<AdswManagerStartServiceSimulator> start_service_sim_;
};


/**
 * @brief TC001: Handshake success within 3 seconds
 *
 * Verifies that when adsw_manager starts, it sends UpdateServiceListRequest
 * and lifecycle_mgr responds within 3 seconds, causing adsw to transition to RUNNING.
 */
TEST_F(LifecycleAdswIntegrationTest, Handshake_SuccessWithin3Seconds) {
    // Create response endpoint outside of callback (needs to persist)
    // Create response endpoint outside of callback (needs to persist)
    auto lcm_response_endpoint = std::make_unique<UpdateServiceListEndPoint>(
        MessageType::UPDATE_SERVICE_LIST_RESPONSE,
        "mmf/lifecyclemgr/UpdateServiceListResponse");

    Publisher* pub = lcm_participant_->create_publisher(PUBLISHER_QOS_DEFAULT);
    ASSERT_TRUE(lcm_response_endpoint->Initialize(lcm_participant_, pub, nullptr));

    // Create lifecycle_mgr request listener endpoint
    auto lcm_request_endpoint = std::make_unique<UpdateServiceListEndPoint>(
        MessageType::UPDATE_SERVICE_LIST_REQUEST,
        "mmf/lifecyclemgr/UpdateServiceListRequest",
        [&lcm_response_endpoint](UpdateServiceListRequest* req) {
            // When request is received, send response
            UpdateServiceListResponse response;
            response.ret_code(ReturnCode::SUCCESS);
            lcm_response_endpoint->Write(&response);
        });

    Subscriber* sub = lcm_participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT);
    ASSERT_TRUE(lcm_request_endpoint->Initialize(lcm_participant_, nullptr, sub));

    // Wait for DDS discovery (lcm endpoints and adsw endpoints must match)
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Start adsw_manager simulator
    adsw_simulator_->Start();

    // Wait up to 5 seconds for RUNNING state
    bool reached_running = adsw_simulator_->WaitForRunning(5000);

    // Verify results
    EXPECT_TRUE(reached_running)
        << "RUNNING state not reached within 5 seconds";
    EXPECT_EQ(adsw_simulator_->GetState(), AdswManagerSimulator::State::RUNNING);
    EXPECT_GE(adsw_simulator_->GetRequestCount(), 1)
        << "At least one request should be sent";
}


/**
 * @brief TC002: Retry after 3 seconds without response
 *
 * Verifies that adsw_manager retries sending UpdateServiceListRequest
 * every 3 seconds when no response is received.
 */
TEST_F(LifecycleAdswIntegrationTest, Handshake_RetryAfter3Seconds) {
    // Start adsw_manager without lifecycle_mgr responding
    adsw_simulator_->Start();

    // Wait 7 seconds (should trigger at least 2 retries: 0s, 3s, 6s)
    std::this_thread::sleep_for(std::chrono::milliseconds(7000));

    // Verify multiple requests were sent
    EXPECT_GE(adsw_simulator_->GetRequestCount(), 2)
        << "adsw_manager should retry at least twice in 7 seconds";

    // Verify still in STARTED state (not RUNNING)
    EXPECT_EQ(adsw_simulator_->GetState(), AdswManagerSimulator::State::STARTED);
}


/**
 * @brief TC003: Response contains SUCCESS code
 *
 * Verifies that lifecycle_mgr responds with ReturnCode::SUCCESS.
 */
TEST_F(LifecycleAdswIntegrationTest, Response_ContainsSuccessCode) {
    // Create response endpoint outside of callback
    auto lcm_response_endpoint = std::make_unique<UpdateServiceListEndPoint>(
        MessageType::UPDATE_SERVICE_LIST_RESPONSE,
        "mmf/lifecyclemgr/UpdateServiceListResponse");

    Publisher* pub = lcm_participant_->create_publisher(PUBLISHER_QOS_DEFAULT);
    ASSERT_TRUE(lcm_response_endpoint->Initialize(lcm_participant_, pub, nullptr));

    // Create lifecycle_mgr request listener endpoint
    auto lcm_request_endpoint = std::make_unique<UpdateServiceListEndPoint>(
        MessageType::UPDATE_SERVICE_LIST_REQUEST,
        "mmf/lifecyclemgr/UpdateServiceListRequest",
        [&lcm_response_endpoint](UpdateServiceListRequest* req) {
            UpdateServiceListResponse response;
            response.ret_code(ReturnCode::SUCCESS);
            lcm_response_endpoint->Write(&response);
        });

    Subscriber* sub = lcm_participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT);
    ASSERT_TRUE(lcm_request_endpoint->Initialize(lcm_participant_, nullptr, sub));
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    adsw_simulator_->Start();
    ASSERT_TRUE(adsw_simulator_->WaitForRunning(5000));

    // Verify adsw_manager reached RUNNING state (which means SUCCESS response was received)
    EXPECT_EQ(adsw_simulator_->GetState(), AdswManagerSimulator::State::RUNNING);
}


/**
 * @brief TC004: Timeout after 10 seconds without lifecycle_mgr
 *
 * Verifies that adsw_manager continues retrying but does not transition
 * to RUNNING when lifecycle_mgr is not responding.
 */
TEST_F(LifecycleAdswIntegrationTest, Handshake_TimeoutAfter10Seconds) {
    // Start adsw_manager without any lifecycle_mgr endpoint
    adsw_simulator_->Start();

    // Wait 10 seconds
    bool reached_running = adsw_simulator_->WaitForRunning(10000);

    // Verify did NOT reach RUNNING
    EXPECT_FALSE(reached_running)
        << "adsw_manager should NOT reach RUNNING without lifecycle_mgr response";

    // Verify multiple retries occurred (at 0s, 3s, 6s, 9s = 4 requests)
    EXPECT_GE(adsw_simulator_->GetRequestCount(), 3)
        << "adsw_manager should retry at least 3 times in 10 seconds";

    // Still in STARTED state
    EXPECT_EQ(adsw_simulator_->GetState(), AdswManagerSimulator::State::STARTED);
}


/**
 * @brief TC005: State transition sequence INIT -> STARTED -> RUNNING
 *
 * Verifies the complete state transition sequence during handshake.
 */
TEST_F(LifecycleAdswIntegrationTest, StateTransition_InitStartedRunning) {
    // Verify initial state
    EXPECT_EQ(adsw_simulator_->GetState(), AdswManagerSimulator::State::INIT);

    // Create response endpoint outside of callback
    auto lcm_response_endpoint = std::make_unique<UpdateServiceListEndPoint>(
        MessageType::UPDATE_SERVICE_LIST_RESPONSE,
        "mmf/lifecyclemgr/UpdateServiceListResponse");

    Publisher* pub = lcm_participant_->create_publisher(PUBLISHER_QOS_DEFAULT);
    ASSERT_TRUE(lcm_response_endpoint->Initialize(lcm_participant_, pub, nullptr));

    // Create lifecycle_mgr responder
    auto lcm_request_endpoint = std::make_unique<UpdateServiceListEndPoint>(
        MessageType::UPDATE_SERVICE_LIST_REQUEST,
        "mmf/lifecyclemgr/UpdateServiceListRequest",
        [&lcm_response_endpoint](UpdateServiceListRequest* req) {
            UpdateServiceListResponse response;
            response.ret_code(ReturnCode::SUCCESS);
            lcm_response_endpoint->Write(&response);
        });

    Subscriber* sub = lcm_participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT);
    ASSERT_TRUE(lcm_request_endpoint->Initialize(lcm_participant_, nullptr, sub));
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Start -> STARTED state
    adsw_simulator_->Start();
    EXPECT_EQ(adsw_simulator_->GetState(), AdswManagerSimulator::State::STARTED);

    // Wait for RUNNING
    ASSERT_TRUE(adsw_simulator_->WaitForRunning(5000));
    EXPECT_EQ(adsw_simulator_->GetState(), AdswManagerSimulator::State::RUNNING);
}


/**
 * @brief TC006: F003 — Full StartService handshake success
 *
 * lifecycle_mgr publishes StartServiceRequest("autoware").
 * Simulator receives, sets container_started flag, replies SUCCESS.
 * lifecycle_mgr endpoint receives StartServiceResponse(SUCCESS).
 */
TEST_F(LifecycleAdswIntegrationTest, StartService_HandshakeSuccess) {
    // lifecycle_mgr response receiver
    bool response_received = false;
    ReturnCode received_code = ReturnCode::FAILED;
    std::mutex resp_mutex;
    std::condition_variable resp_cv;

    auto lcm_response_ep = std::make_unique<StartServiceEndPoint>(
        MessageType::START_SERVICE_RESPONSE,
        "mmf/lifecyclemgr/StartServiceResponse",
        [&](StartServiceResponse* resp) {
            std::lock_guard<std::mutex> lk(resp_mutex);
            received_code = resp->ret_code();
            response_received = true;
            resp_cv.notify_all();
        });

    Subscriber* sub = lcm_participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT);
    ASSERT_TRUE(lcm_response_ep->Initialize(lcm_participant_, nullptr, sub));
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // lifecycle_mgr sends StartServiceRequest
    auto lcm_request_ep = std::make_unique<StartServiceEndPoint>(
        MessageType::START_SERVICE_REQUEST,
        "mmf/lifecyclemgr/StartServiceRequest");
    Publisher* pub = lcm_participant_->create_publisher(PUBLISHER_QOS_DEFAULT);
    ASSERT_TRUE(lcm_request_ep->Initialize(lcm_participant_, pub, nullptr));
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    StartServiceRequest req;
    req.service_list().push_back("autoware");
    req.is_group(false);
    req.os_type(OsType::ROS2);
    EXPECT_TRUE(lcm_request_ep->Write(&req));

    // Wait for simulator to receive request and send response
    ASSERT_TRUE(start_service_sim_->WaitForRequestReceived(3000))
        << "Simulator did not receive StartServiceRequest within 3 seconds";

    // Wait for lifecycle_mgr to receive response
    std::unique_lock<std::mutex> lk(resp_mutex);
    bool got_response = resp_cv.wait_for(lk, std::chrono::milliseconds(3000),
                                          [&]() { return response_received; });

    EXPECT_TRUE(got_response) << "lifecycle_mgr did not receive StartServiceResponse";
    EXPECT_EQ(received_code, ReturnCode::SUCCESS);
    EXPECT_TRUE(start_service_sim_->IsContainerStarted())
        << "Simulator should have executed container start (mock docker exec)";
}


/**
 * @brief TC007: F003 — Service name forwarded correctly
 *
 * Verifies that the service_name sent in StartServiceRequest
 * is received intact by the adsw_manager simulator.
 */
TEST_F(LifecycleAdswIntegrationTest, StartService_ServiceNameCorrect) {
    auto lcm_request_ep = std::make_unique<StartServiceEndPoint>(
        MessageType::START_SERVICE_REQUEST,
        "mmf/lifecyclemgr/StartServiceRequest");
    Publisher* pub = lcm_participant_->create_publisher(PUBLISHER_QOS_DEFAULT);
    ASSERT_TRUE(lcm_request_ep->Initialize(lcm_participant_, pub, nullptr));
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    StartServiceRequest req;
    req.service_list().push_back("autoware");
    req.is_group(false);
    req.os_type(OsType::ROS2);
    EXPECT_TRUE(lcm_request_ep->Write(&req));

    ASSERT_TRUE(start_service_sim_->WaitForRequestReceived(3000))
        << "Simulator did not receive request";

    EXPECT_EQ(start_service_sim_->GetReceivedServiceName(), "autoware");
}


/**
 * @brief TC008: F003 — is_group flag forwarded correctly
 *
 * Verifies that is_group=false is received intact by the simulator.
 */
TEST_F(LifecycleAdswIntegrationTest, StartService_IsGroupFlag) {
    auto lcm_request_ep = std::make_unique<StartServiceEndPoint>(
        MessageType::START_SERVICE_REQUEST,
        "mmf/lifecyclemgr/StartServiceRequest");
    Publisher* pub = lcm_participant_->create_publisher(PUBLISHER_QOS_DEFAULT);
    ASSERT_TRUE(lcm_request_ep->Initialize(lcm_participant_, pub, nullptr));
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    StartServiceRequest req;
    req.service_list().push_back("autoware");
    req.is_group(false);
    req.os_type(OsType::ROS2);
    EXPECT_TRUE(lcm_request_ep->Write(&req));

    ASSERT_TRUE(start_service_sim_->WaitForRequestReceived(3000));
    EXPECT_FALSE(start_service_sim_->GetReceivedIsGroup());
}


/**
 * @brief TC009: F003 — StartServiceResponse contains SUCCESS code
 *
 * Explicitly verifies the response code is ReturnCode::SUCCESS.
 */
TEST_F(LifecycleAdswIntegrationTest, StartService_ResponseCodeSuccess) {
    bool response_received = false;
    ReturnCode received_code = ReturnCode::FAILED;
    std::mutex resp_mutex;
    std::condition_variable resp_cv;

    auto lcm_response_ep = std::make_unique<StartServiceEndPoint>(
        MessageType::START_SERVICE_RESPONSE,
        "mmf/lifecyclemgr/StartServiceResponse",
        [&](StartServiceResponse* resp) {
            std::lock_guard<std::mutex> lk(resp_mutex);
            received_code = resp->ret_code();
            response_received = true;
            resp_cv.notify_all();
        });

    Subscriber* sub = lcm_participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT);
    ASSERT_TRUE(lcm_response_ep->Initialize(lcm_participant_, nullptr, sub));

    auto lcm_request_ep = std::make_unique<StartServiceEndPoint>(
        MessageType::START_SERVICE_REQUEST,
        "mmf/lifecyclemgr/StartServiceRequest");
    Publisher* pub = lcm_participant_->create_publisher(PUBLISHER_QOS_DEFAULT);
    ASSERT_TRUE(lcm_request_ep->Initialize(lcm_participant_, pub, nullptr));
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    StartServiceRequest req;
    req.service_list().push_back("autoware");
    req.is_group(false);
    req.os_type(OsType::ROS2);
    EXPECT_TRUE(lcm_request_ep->Write(&req));

    std::unique_lock<std::mutex> lk(resp_mutex);
    bool got = resp_cv.wait_for(lk, std::chrono::milliseconds(5000),
                                 [&]() { return response_received; });

    ASSERT_TRUE(got) << "No StartServiceResponse received within 5 seconds";
    EXPECT_EQ(received_code, ReturnCode::SUCCESS);
}


/**
 * @brief TC010: F003 — Timeout: no response without simulator
 *
 * When no simulator is responding, lifecycle_mgr's response endpoint
 * receives nothing within 5 seconds.
 */
TEST_F(LifecycleAdswIntegrationTest, StartService_TimeoutWithoutSimulator) {
    // Use a fresh participant so start_service_sim_ is not matched
    DomainParticipant* isolated = CreateUdpOnlyParticipant(1, "IsolatedParticipant");
    ASSERT_NE(isolated, nullptr);

    bool response_received = false;
    std::mutex resp_mutex;
    std::condition_variable resp_cv;

    auto lcm_response_ep = std::make_unique<StartServiceEndPoint>(
        MessageType::START_SERVICE_RESPONSE,
        "mmf/lifecyclemgr/StartServiceResponse",
        [&](StartServiceResponse* /*resp*/) {
            std::lock_guard<std::mutex> lk(resp_mutex);
            response_received = true;
            resp_cv.notify_all();
        });

    Subscriber* sub = isolated->create_subscriber(SUBSCRIBER_QOS_DEFAULT);
    ASSERT_TRUE(lcm_response_ep->Initialize(isolated, nullptr, sub));

    auto lcm_request_ep = std::make_unique<StartServiceEndPoint>(
        MessageType::START_SERVICE_REQUEST,
        "mmf/lifecyclemgr/StartServiceRequest");
    Publisher* pub = isolated->create_publisher(PUBLISHER_QOS_DEFAULT);
    ASSERT_TRUE(lcm_request_ep->Initialize(isolated, pub, nullptr));
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    StartServiceRequest req;
    req.service_list().push_back("autoware");
    req.is_group(false);
    req.os_type(OsType::ROS2);
    EXPECT_TRUE(lcm_request_ep->Write(&req));

    std::unique_lock<std::mutex> lk(resp_mutex);
    bool got = resp_cv.wait_for(lk, std::chrono::milliseconds(5000),
                                 [&]() { return response_received; });

    EXPECT_FALSE(got) << "Should NOT receive response when no simulator is active";

    lcm_response_ep.reset();
    lcm_request_ep.reset();
    isolated->delete_contained_entities();
    DomainParticipantFactory::get_instance()->delete_participant(isolated);
}


int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
