#include "mqtt/async_client.h"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>
#include <condition_variable>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"

// --- MQTT Configuration ---
const std::string BROKER_ADDRESS("tcp://localhost:1883");
const std::string TEST_CLIENT_ID("viss_test_client_cpp");
const std::string REQUEST_TOPIC("VISS/request/test_cpp");
const std::string RESPONSE_TOPIC_PREFIX("VISS/response/");

// --- Test State ---
std::mutex mtx;
std::condition_variable cv;
std::atomic<bool> response_received(false);
rapidjson::Document g_response_json;


class TestCallback : public virtual mqtt::callback {
    void connected(const std::string& cause) override {
        std::cout << "Test client connected to broker." << std::endl;
    }

    void connection_lost(const std::string& cause) override {
        std::cerr << "Test client connection lost: " << cause << std::endl;
    }

    void message_arrived(mqtt::const_message_ptr msg) override {
        std::cout << "Response received on topic: " << msg->get_topic() << std::endl;
        g_response_json.Parse(msg->get_payload().c_str());
        if (g_response_json.HasParseError()) {
            std::cerr << "Could not parse response JSON" << std::endl;
        } else {
            rapidjson::StringBuffer buffer;
            rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
            g_response_json.Accept(writer);
            std::cout << "Response payload:\n" << buffer.GetString() << std::endl;
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

bool run_single_test(mqtt::async_client& client, rapidjson::Document& request_payload) {
    response_received = false;
    g_response_json.SetObject(); // Clear by re-initializing

    std::string request_id = request_payload["requestId"].GetString();
    std::string response_topic = RESPONSE_TOPIC_PREFIX + request_id;

    client.subscribe(response_topic, 1)->wait();
    std::cout << "Subscribed to " << response_topic << std::endl;

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    request_payload.Accept(writer);
    client.publish(REQUEST_TOPIC, buffer.GetString())->wait();
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
    rapidjson::Document get_request;
    get_request.SetObject();
    rapidjson::Document::AllocatorType& allocator1 = get_request.GetAllocator();
    get_request.AddMember("action", "get", allocator1);
    get_request.AddMember("path", "Vehicle.OBD.Speed", allocator1);
    get_request.AddMember("requestId", rapidjson::Value(generate_uuid().c_str(), allocator1), allocator1);
    run_single_test(client, get_request);
    
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // --- Test 2: SET request ---
    std::cout << "\n--- 2. Testing SET request for Vehicle.OBD.Speed ---" << std::endl;
    rapidjson::Document set_request;
    set_request.SetObject();
    rapidjson::Document::AllocatorType& allocator2 = set_request.GetAllocator();
    set_request.AddMember("action", "set", allocator2);
    set_request.AddMember("path", "Vehicle.OBD.Speed", allocator2);
    set_request.AddMember("value", 120, allocator2);
    set_request.AddMember("requestId", rapidjson::Value(generate_uuid().c_str(), allocator2), allocator2);
    run_single_test(client, set_request);

    std::this_thread::sleep_for(std::chrono::seconds(1));

    // --- Test 3: GET after SET ---
    std::cout << "\n--- 3. Testing GET request after SET ---" << std::endl;
    rapidjson::Document get_after_set_request;
    get_after_set_request.SetObject();
    rapidjson::Document::AllocatorType& allocator3 = get_after_set_request.GetAllocator();
    get_after_set_request.AddMember("action", "get", allocator3);
    get_after_set_request.AddMember("path", "Vehicle.OBD.Speed", allocator3);
    get_after_set_request.AddMember("requestId", rapidjson::Value(generate_uuid().c_str(), allocator3), allocator3);
    
    if (run_single_test(client, get_after_set_request)) {
        if (g_response_json.HasMember("data") && g_response_json["data"].IsArray() && g_response_json["data"].Size() > 0 && g_response_json["data"][0].HasMember("value") && g_response_json["data"][0]["value"].IsString()) {
            std::string val = g_response_json["data"][0]["value"].GetString();
            if (val == "120") {
                std::cout << "--- Value verification PASSED ---" << std::endl;
            } else {
                std::cerr << "!!! Value verification FAILED: Expected 120, got " << val << std::endl;
            }
        } else {
            std::cerr << "!!! Value verification FAILED: Could not access value in response." << std::endl;
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
