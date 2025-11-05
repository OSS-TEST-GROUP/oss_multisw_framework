#include <iostream>
#include <string>
#include <sstream>
#include <thread>
#include <chrono>
#include <atomic>
#include <condition_variable>
#include <mutex>

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>

#include "VissRequestPubSubTypes.hpp"

// --- DDS Configuration ---
const std::string REQUEST_TOPIC_NAME("/VIN/Vehicle");
const std::string RESPONSE_TOPIC_PREFIX("VISS/response/");

// --- Test State ---
std::mutex mtx;
std::condition_variable cv;
std::atomic<bool> response_received(false);
std::string g_response_json_str;

class ResponseListener : public eprosima::fastdds::dds::DataReaderListener {
public:
    void on_subscription_matched(
        eprosima::fastdds::dds::DataReader* reader,
        const eprosima::fastdds::dds::SubscriptionMatchedStatus& info) override
    {
        if (info.current_count_change == 1) {
            std::cout << "Subscriber matched on topic: " << reader->get_topicdescription()->get_name() << std::endl;
        } else if (info.current_count_change == -1) {
            std::cout << "Subscriber unmatched on topic: " << reader->get_topicdescription()->get_name() << std::endl;
        }
    }

    void on_data_available(eprosima::fastdds::dds::DataReader* reader) override {
        eprosima::fastdds::dds::SampleInfo info;
        VissRequestTopic sample;
        if (reader->take_next_sample(&sample, &info) == eprosima::fastdds::dds::RETCODE_OK) {
            if (info.valid_data) {
                std::cout << "Response received on topic: " << reader->get_topicdescription()->get_name() << std::endl;
                g_response_json_str = sample.payload();
                std::cout << "Response payload:\n" << g_response_json_str << std::endl;
                response_received = true;
                cv.notify_one();
            }
        }
    }
};

class WriterListener : public eprosima::fastdds::dds::DataWriterListener {
public:
    std::atomic<int> matched_;
    std::condition_variable cv_;
    std::mutex mtx_;

    WriterListener() : matched_(0) {}

    void on_publication_matched(
        eprosima::fastdds::dds::DataWriter* /*writer*/,
        const eprosima::fastdds::dds::PublicationMatchedStatus& info) override
    {
        if (info.current_count_change == 1)
        {
            matched_ = info.current_count;
            std::cout << "Publisher matched." << std::endl;
            cv_.notify_one();
        }
        else if (info.current_count_change == -1)
        {
            matched_ = info.current_count;
            std::cout << "Publisher unmatched." << std::endl;
        }
        else
        {
            std::cout << info.current_count_change
                    << " is not a valid value for PublicationMatchedStatus current count change" << std::endl;
        }
    }
};

void print_interactive_usage() {
    std::cout << "\n--- Interactive DDS Client ---\n"
              << "Commands:\n"
              << "  get <signal_path>                - Send a 'get' request. (e.g., get Vehicle.Speed)\n"
              << "  set <signal_path> <value>        - Send a 'set' request. (e.g., set Vehicle.Chassis.SteeringWheel.Angle 90)\n"
              << "  subscribe <signal_path>          - Send a 'subscribe' request. (e.g., subscribe Vehicle.Speed)\n"
              << "  unsubscribe <subscription_id>    - Send an 'unsubscribe' request.\n"
              << "  json <raw_json_string>           - Send a raw JSON request.\n"
              << "  topic <response_topic_name>      - Set a custom response topic for the next request.\n"
              << "  help                             - Show this help message.\n"
              << "  exit                             - Exit the client.\n"
              << "---------------------------------\n" << std::endl;
}

// Helper to generate a unique request ID
std::string generate_uuid() {
    static std::atomic<long> id_counter(0);
    return "dds-test-" + std::to_string(id_counter++);
}

// Helper to parse subscriptionId from response JSON
std::string parse_subscription_id(const std::string& json_str) {
    const std::string key = "\"subscriptionId\":\"";
    size_t start_pos = json_str.find(key);
    if (start_pos == std::string::npos) {
        return "";
    }
    start_pos += key.length();
    size_t end_pos = json_str.find("\"", start_pos);
    if (end_pos == std::string::npos) {
        return "";
    }
    return json_str.substr(start_pos, end_pos - start_pos);
}

int main(int argc, char* argv[]) {
    // --- DDS Setup ---
    eprosima::fastdds::dds::DomainParticipantQos pqos;
    pqos.name("dds_client_test");
    eprosima::fastdds::dds::DomainParticipant* participant =
        eprosima::fastdds::dds::DomainParticipantFactory::get_instance()->create_participant(7, pqos, nullptr, eprosima::fastdds::dds::StatusMask::none());
    if (participant == nullptr) {
        std::cerr << "Failed to create DDS participant" << std::endl;
        return 1;
    }

    eprosima::fastdds::dds::Publisher* publisher = participant->create_publisher(eprosima::fastdds::dds::PUBLISHER_QOS_DEFAULT, nullptr);
    eprosima::fastdds::dds::Subscriber* subscriber = participant->create_subscriber(eprosima::fastdds::dds::SUBSCRIBER_QOS_DEFAULT, nullptr);

    eprosima::fastdds::dds::TypeSupport request_type(new VissRequestTopicPubSubType());
    request_type.register_type(participant);

    WriterListener writer_listener;
    eprosima::fastdds::dds::Topic* request_topic_obj = participant->create_topic(REQUEST_TOPIC_NAME, request_type.get_type_name(), eprosima::fastdds::dds::TOPIC_QOS_DEFAULT);
    eprosima::fastdds::dds::DataWriterQos writer_qos = eprosima::fastdds::dds::DATAWRITER_QOS_DEFAULT;
    writer_qos.reliability().kind = eprosima::fastdds::dds::RELIABLE_RELIABILITY_QOS;
    eprosima::fastdds::dds::DataWriter* writer = publisher->create_datawriter(request_topic_obj, writer_qos, &writer_listener);

    ResponseListener listener;
    
    // Wait for publisher to match a subscriber
    {
        std::unique_lock<std::mutex> lock(writer_listener.mtx_);
        if (!writer_listener.cv_.wait_for(lock, std::chrono::seconds(5), [&writer_listener]{ return writer_listener.matched_ > 0; })) {
            std::cerr << "!!! ERROR: Publisher could not match any subscriber on topic " << REQUEST_TOPIC_NAME << ". Exiting." << std::endl;
            // Cleanup and exit if no match
            publisher->delete_datawriter(writer);
            participant->delete_publisher(publisher);
            participant->delete_subscriber(subscriber);
            participant->delete_topic(request_topic_obj);
            eprosima::fastdds::dds::DomainParticipantFactory::get_instance()->delete_participant(participant);
            return 1;
        }
    }

    print_interactive_usage();
    std::string response_topic_name_override;
    std::map<std::string, std::pair<eprosima::fastdds::dds::Topic*, eprosima::fastdds::dds::DataReader*>> active_subscriptions;

    while (true) {
        std::cout << "> ";
        std::string line;
        if (!std::getline(std::cin, line)) {
            break; // End of input
        }

        std::stringstream ss(line);
        std::string command;
        ss >> command;

        if (command == "exit") {
            break;
        } else if (command == "help") {
            print_interactive_usage();
            continue;
        } else if (command == "topic") {
            ss >> response_topic_name_override;
            std::cout << "Response topic for next request set to: " << response_topic_name_override << std::endl;
            continue;
        }

        std::string request_id = generate_uuid();
        std::string response_topic_name = response_topic_name_override.empty() 
                                           ? (RESPONSE_TOPIC_PREFIX + request_id) 
                                           : response_topic_name_override;

        eprosima::fastdds::dds::Topic* response_topic = participant->create_topic(response_topic_name, request_type.get_type_name(), eprosima::fastdds::dds::TOPIC_QOS_DEFAULT);
        eprosima::fastdds::dds::DataReaderQos reader_qos = eprosima::fastdds::dds::DATAREADER_QOS_DEFAULT;
        reader_qos.reliability().kind = eprosima::fastdds::dds::RELIABLE_RELIABILITY_QOS;
        eprosima::fastdds::dds::DataReader* reader = subscriber->create_datareader(response_topic, reader_qos, &listener);

        std::string request_inner_str;

        if (command == "get") {
            std::string path;
            ss >> path;
            std::stringstream r_ss;
            r_ss << R"({"action":"get","path":")" << path << R"(","requestId":")" << request_id << R"("})";
            request_inner_str = r_ss.str();
        } else if (command == "set") {
            std::string path, value;
            ss >> path >> value;
            std::stringstream r_ss;
            // Naive check if value is numeric or boolean, otherwise treat as string
            bool is_numeric = true;
            try {
                std::stod(value);
            } catch (const std::exception&) {
                is_numeric = false;
            }
            if (value == "true" || value == "false" || is_numeric) {
                r_ss << R"({"action":"set","path":")" << path << R"(","value":)" << value << R"(,"requestId":")" << request_id << R"("})";
            } else {
                r_ss << R"({"action":"set","path":")" << path << R"(","value":")" << value << R"(","requestId":")" << request_id << R"("})";
            }
            request_inner_str = r_ss.str();
        } else if (command == "subscribe") {
            std::string path;
            ss >> path;
            std::stringstream r_ss;
            r_ss << R"({"action":"subscribe","path":")" << path << R"(","requestId":")" << request_id << R"("})";
            request_inner_str = r_ss.str();
        } else if (command == "unsubscribe") {
            std::string sub_id;
            ss >> sub_id;

            auto it = active_subscriptions.find(sub_id);
            if (it == active_subscriptions.end()) {
                std::cerr << "Subscription ID not found: " << sub_id << std::endl;
                continue;
            }

            // Send unsubscribe request
            std::stringstream r_ss;
            r_ss << R"({"action":"unsubscribe","subscriptionId":")" << sub_id << R"(","requestId":")" << request_id << R"("})";
            request_inner_str = r_ss.str();

            // Use the topic associated with the subscription for the response
            response_topic_name = it->second.first->get_name();
            
            // Create a temporary reader for the unsubscribe response
            eprosima::fastdds::dds::DataReader* temp_reader = subscriber->create_datareader(it->second.first, reader_qos, &listener);

            std::stringstream outer_ss;
            outer_ss << R"({"topic":")" << response_topic_name << R"(","request":")" << request_inner_str << R"("})";

            response_received = false;
            VissRequestTopic sample;
            sample.payload(outer_ss.str());
            writer->write(&sample);
            std::cout << "Published unsubscribe request. Waiting for confirmation..." << std::endl;

            {
                std::unique_lock<std::mutex> lock(mtx);
                if (cv.wait_for(lock, std::chrono::seconds(10), []{ return response_received.load(); })) {
                    std::cout << "--- Unsubscribe confirmed for " << sub_id << " ---\n" << std::endl;
                    // Clean up the persistent subscription resources
                    subscriber->delete_datareader(it->second.second);
                    participant->delete_topic(it->second.first);
                    active_subscriptions.erase(it);
                } else {
                    std::cerr << "!!! FAILED: Timeout waiting for unsubscribe confirmation.\n" << std::endl;
                }
            }
            subscriber->delete_datareader(temp_reader); // clean up temp reader
            continue; // Move to next command

        } else if (command == "json") {
            std::string raw_json;
            std::getline(ss, raw_json);
            // remove leading space
            if (!raw_json.empty() && raw_json[0] == ' ') {
                raw_json = raw_json.substr(1);
            }
            request_inner_str = raw_json;
        } else {
            std::cerr << "Unknown command: " << command << std::endl;
            subscriber->delete_datareader(reader);
            participant->delete_topic(response_topic);
            continue;
        }

        // Escape the inner JSON string for the outer JSON payload
        std::string escaped_inner_str;
        for (char c : request_inner_str) {
            if (c == '"' || c == '\\') {
                escaped_inner_str += '\\';
            }
            escaped_inner_str += c;
        }

        std::stringstream outer_ss;
        outer_ss << R"({"topic":")" << response_topic_name << R"(","request":")" << escaped_inner_str << R"("})";


        response_received = false; // Reset for this request
        VissRequestTopic sample;
        sample.payload(outer_ss.str());

        std::cout << "Sending payload: " << outer_ss.str() << std::endl; // Log the payload
        writer->write(&sample);

        std::cout << "Published request to " << REQUEST_TOPIC_NAME << std::endl;
        std::cout << "Waiting for response on " << response_topic_name << "..." << std::endl;

        {
            std::unique_lock<std::mutex> lock(mtx);
            if (cv.wait_for(lock, std::chrono::seconds(10), []{ return response_received.load(); })) {
                if (command == "subscribe") {
                    std::string sub_id = parse_subscription_id(g_response_json_str);
                    if (!sub_id.empty()) {
                        active_subscriptions[sub_id] = {response_topic, reader};
                        std::cout << "--- Subscribed successfully. Subscription ID: " << sub_id << " ---" << std::endl;
                        std::cout << "--- Listening for events on topic: " << response_topic->get_name() << " ---\n" << std::endl;
                        // For subscribe, we DON'T clean up, so we skip to the next loop.
                        continue;
                    } else {
                        std::cerr << "!!! FAILED to get subscriptionId from response.\n" << std::endl;
                    }
                } else {
                     std::cout << "--- Response received successfully ---\n" << std::endl;
                }
            } else {
                std::cerr << "!!! FAILED: Timeout waiting for response.\n" << std::endl;
            }
        }
        
        // Clean up per-request resources for non-subscription requests
        subscriber->delete_datareader(reader);
        participant->delete_topic(response_topic);
        response_topic_name_override.clear(); // Reset custom topic
    }

    // --- Final Cleanup ---
    std::cout << "Exiting client. Cleaning up " << active_subscriptions.size() << " active subscriptions..." << std::endl;
    for (auto const& [key, val] : active_subscriptions) {
        subscriber->delete_datareader(val.second);
        participant->delete_topic(val.first);
    }
    publisher->delete_datawriter(writer);
    participant->delete_publisher(publisher);
    participant->delete_subscriber(subscriber);
    participant->delete_topic(request_topic_obj);
    eprosima::fastdds::dds::DomainParticipantFactory::get_instance()->delete_participant(participant);

    return 0;
}
