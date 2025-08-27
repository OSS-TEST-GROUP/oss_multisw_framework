#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include "nlohmann/3.11.2/json.hpp"

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
nlohmann::json g_response_json;

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
                try {
                    g_response_json = nlohmann::json::parse(sample.payload());
                    std::cout << "Response payload:\n" << g_response_json.dump(4) << std::endl;
                } catch (const nlohmann::json::parse_error& e) {
                    std::cerr << "Could not parse response JSON: " << e.what() << std::endl;
                }
                response_received = true;
                //cv.notify_one();
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

void print_usage(const char* prog_name) {
    std::cout << "Usage: " << prog_name << " [options]\n\n"
              << "Options:\n"
              << "  --help                         Show this help message.\n"
              << "  --topic <topic_name>           Set the response topic name. (Default: test/sub/topic)\n"
              << "  --action <action_type>         Set the action type (e.g., get, set). (Default: get)\n"
              << "  --signal <signal_path>         Set the VSS signal path. (Default: Vehicle.CurrentLocation.Altitude)\n"
              << "  --json <json_string>           Send the provided JSON string as the request payload, ignoring --action and --signal.\n\n"
              << "Available signals from NetworkProvider:\n"
              << "  - Vehicle.FuelLevel\n"
              << "  - Vehicle.CurrentLocation.Altitude\n"
              << "  - Vehicle.CurrentLocation.Heading\n"
              << "  - Vehicle.CurrentLocation.Latitude\n"
              << "  - Vehicle.CurrentLocation.Longitude\n"
              << "  - Vehicle.Body.BodyType\n"
              << "  - Vehicle.Body.Mirrors.DriverSide.IsFolded\n"
              << "  - Vehicle.Body.Mirrors.DriverSide.IsHeatingOn\n"
              << "  - Vehicle.Body.Mirrors.DriverSide.IsLocked\n"
              << std::endl;
}

// Helper to generate a unique request ID
std::string generate_uuid() {
    static std::atomic<long> id_counter(0);
    return "dds-test-" + std::to_string(id_counter++);
}

int main(int argc, char* argv[]) {
    std::string response_topic_arg = "test/sub/topic";
    std::string action = "get";
    std::string signal = "Vehicle.CurrentLocation.Altitude";
    std::string json_request_str;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help") {
            print_usage(argv[0]);
            return 0;
        } else if (arg == "--topic" && i + 1 < argc) {
            response_topic_arg = argv[++i];
        } else if (arg == "--action" && i + 1 < argc) {
            action = argv[++i];
        } else if (arg == "--signal" && i + 1 < argc) {
            signal = argv[++i];
        } else if (arg == "--json" && i + 1 < argc) {
            json_request_str = argv[++i];
        }
    }

    // --- DDS Setup ---
    eprosima::fastdds::dds::DomainParticipant* participant =
        eprosima::fastdds::dds::DomainParticipantFactory::get_instance()->create_participant_with_default_profile(nullptr, eprosima::fastdds::dds::StatusMask::none());
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
    
    // --- Send Request ---
    std::string request_id = generate_uuid();
    std::string response_topic_name = response_topic_arg.empty() ? (RESPONSE_TOPIC_PREFIX + request_id) : response_topic_arg;
    
    eprosima::fastdds::dds::Topic* response_topic = participant->create_topic(response_topic_name, request_type.get_type_name(), eprosima::fastdds::dds::TOPIC_QOS_DEFAULT);
    eprosima::fastdds::dds::DataReaderQos reader_qos = eprosima::fastdds::dds::DATAREADER_QOS_DEFAULT;
    reader_qos.reliability().kind = eprosima::fastdds::dds::RELIABLE_RELIABILITY_QOS;
    eprosima::fastdds::dds::DataReader* reader = subscriber->create_datareader(response_topic, reader_qos, &listener);

    std::string request_inner_str;
    if (!json_request_str.empty()) {
        std::cout << "\n--- Sending raw JSON request ---" << std::endl;
        request_inner_str = json_request_str;
    } else {
        std::cout << "\n--- Sending " << action << " request for " << signal << " ---" << std::endl;
        nlohmann::json request_inner = {
            {"action", action},
            {"path", signal},
            {"requestId", request_id}
        };
        request_inner_str = request_inner.dump();
    }

    nlohmann::json request_outer = {
        {"topic", response_topic_name},
        {"request", request_inner_str}
    };

    {
        std::unique_lock<std::mutex> lock(writer_listener.mtx_);
        if (!writer_listener.cv_.wait_for(lock, std::chrono::seconds(2), [&writer_listener]{ return writer_listener.matched_ > 0; })) {
            std::cerr << "!!! ERROR: Publisher could not match any subscriber on topic " << REQUEST_TOPIC_NAME << std::endl;
        }
    }
    
    if (writer_listener.matched_ > 0) {
        VissRequestTopic sample;
        sample.payload(request_outer.dump());
        writer->write(&sample);
        std::cout << "Published request to " << REQUEST_TOPIC_NAME << std::endl;
        std::cout << "Waiting for response on " << response_topic_name << std::endl;

        {
            std::unique_lock<std::mutex> lock(mtx);
            if (cv.wait_for(lock, std::chrono::seconds(10), []{ return response_received.load(); })) {
                std::cout << "--- Test PASSED ---" << std::endl;
            } else {
                std::cerr << "!!! Test FAILED: Timeout waiting for response." << std::endl;
            }
        }
    }

    subscriber->delete_datareader(reader);
    participant->delete_topic(response_topic);

    // --- Cleanup ---
    publisher->delete_datawriter(writer);
    participant->delete_publisher(publisher);
    participant->delete_subscriber(subscriber);
    participant->delete_topic(request_topic_obj);
    eprosima::fastdds::dds::DomainParticipantFactory::get_instance()->delete_participant(participant);

    return 0;
}
