#include "mqtt/async_client.h"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>
#include <condition_variable>
#include "nlohmann/3.11.2/json.hpp"

// --- MQTT Configuration ---
const std::string BROKER_ADDRESS("tcp://localhost:1883");
const std::string TEST_CLIENT_ID("viss_test_client_cpp");
const std::string REQUEST_TOPIC("VISS/request/test_cpp");
const std::string RESPONSE_TOPIC_PREFIX("VISS/response/");

// --- Test State ---
std::mutex mtx;
std::condition_variable cv;
std::atomic<bool> response_received(false);
nlohmann::json g_response_json;


class TestCallback : public virtual mqtt::callback {
    void connected(const std::string& cause) override {
        std::cout << "Test client connected to broker." << std::endl;
    }

    void connection_lost(const std::string& cause) override {
        std::cerr << "Test client connection lost: " << cause << std::endl;
    }

    void message_arrived(mqtt::const_message_ptr msg) override {
        std::cout << "Response received on topic: " << msg->get_topic() << std::endl;
        try {
            g_response_json = nlohmann::json::parse(msg->to_string());
            std::cout << "Response payload:\n" << g_response_json.dump(4) << std::endl;
        } catch (const nlohmann::json::parse_error& e) {
            std::cerr << "Could not parse response JSON: " << e.what() << std::endl;
        }
        response_received = true;
        cv.notify_one();
    }
};

// Helper to generate a unique request ID
std::string generate_uuid() {
    static std::atomic<long> id_counter(0);
    return "cpp-test-" + std::to_string(id_counter++);
}

bool run_single_test(mqtt::async_client& client, const nlohmann::json& request_payload) {
    response_received = false;
    g_response_json.clear();

    std::string request_id = request_payload.at("requestId");
    std::string response_topic = RESPONSE_TOPIC_PREFIX + request_id;

    client.subscribe(response_topic, 1)->wait();
    std::cout << "Subscribed to " << response_topic << std::endl;

    client.publish(REQUEST_TOPIC, request_payload.dump())->wait();
    std::cout << "Published request to " << REQUEST_TOPIC << std::endl;

    std::unique_lock<std::mutex> lock(mtx);
    if (cv.wait_for(lock, std::chrono::seconds(5), []{ return response_received.load(); })) {
        std::cout << "--- Test PASSED ---" << std::endl;
        client.unsubscribe(response_topic)->wait();
        return true;
    } else {
        std::cerr << "!!! Test FAILED: Timeout waiting for response." << std::endl;
        client.unsubscribe(response_topic)->wait();
        return false;
    }
}


int main() {
    TestCallback cb;
    mqtt::async_client client(BROKER_ADDRESS, TEST_CLIENT_ID);
    client.set_callback(cb);

    mqtt::connect_options connOpts;
    connOpts.set_clean_session(true);

    try {
        std::cout << "Connecting to MQTT broker at " << BROKER_ADDRESS << "..." << std::endl;
        client.connect(connOpts)->wait();
    } catch (const mqtt::exception& exc) {
        std::cerr << "Failed to connect to broker: " << exc.what() << std::endl;
        return 1;
    }

    // --- Test 1: GET request ---
    std::cout << "\n--- 1. Testing GET request for Vehicle.OBD.Speed ---" << std::endl;
    nlohmann::json get_request = {
        {"action", "get"},
        {"path", "Vehicle.OBD.Speed"},
        {"requestId", generate_uuid()}
    };
    run_single_test(client, get_request);
    
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // --- Test 2: SET request ---
    std::cout << "\n--- 2. Testing SET request for Vehicle.OBD.Speed ---" << std::endl;
    nlohmann::json set_request = {
        {"action", "set"},
        {"path", "Vehicle.OBD.Speed"},
        {"value", 120},
        {"requestId", generate_uuid()}
    };
    run_single_test(client, set_request);

    std::this_thread::sleep_for(std::chrono::seconds(1));

    // --- Test 3: GET after SET ---
    std::cout << "\n--- 3. Testing GET request after SET ---" << std::endl;
    nlohmann::json get_after_set_request = {
        {"action", "get"},
        {"path", "Vehicle.OBD.Speed"},
        {"requestId", generate_uuid()}
    };
    if (run_single_test(client, get_after_set_request)) {
        try {
            std::string val = g_response_json.at("data")[0].at("value");
            if (val == "120") {
                std::cout << "--- Value verification PASSED ---" << std::endl;
            } else {
                std::cerr << "!!! Value verification FAILED: Expected 120, got " << val << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "!!! Value verification FAILED: Error accessing value in response: " << e.what() << std::endl;
        }
    }

    try {
        client.disconnect()->wait();
        std::cout << "\nDisconnected." << std::endl;
    } catch (const mqtt::exception& exc) {
        std::cerr << "Error during disconnect: " << exc.what() << std::endl;
    }

    return 0;
}
