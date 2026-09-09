#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <any>
#include <mutex>
#include <map>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include "VissServerApp.h"
#include "dds_transport.h"
#include "Utils.h"
#include "JsonSchemaValidator.h"

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <queue>
#include <cstdlib>

#include "VissRequestPubSubTypes.hpp"
#include "Logger.h"

using namespace eprosima::fastdds::dds;

// --- DDS Configuration ---
const std::string VISS_DDS_REQUEST_TOPIC("/VIN/Vehicle");

// Forward declaration
class RequestListener;

// DDSTransport class implementation details
struct WriterInfo {
    eprosima::fastdds::dds::DataWriter* writer;
    eprosima::fastdds::dds::Topic* topic;
    std::chrono::steady_clock::time_point last_used;
    bool is_matched = false;
    std::queue<std::string> pending_messages;
};

class DDSPublicationListener : public eprosima::fastdds::dds::DataWriterListener {
public:
    DDSPublicationListener(std::map<std::string, WriterInfo>& writer_map, std::mutex& map_mutex)
        : writer_map_(writer_map), map_mutex_(map_mutex) {}

    void on_publication_matched(
        eprosima::fastdds::dds::DataWriter* writer,
        const eprosima::fastdds::dds::PublicationMatchedStatus& info) override;

private:
    std::map<std::string, WriterInfo>& writer_map_;
    std::mutex& map_mutex_;
};

struct DDSTransportImpl {
    eprosima::fastdds::dds::DomainParticipant* participant;
    eprosima::fastdds::dds::Publisher* publisher;
    eprosima::fastdds::dds::Subscriber* subscriber;
    eprosima::fastdds::dds::Topic* request_topic;
    eprosima::fastdds::dds::DataReader* request_reader;
    eprosima::fastdds::dds::TypeSupport request_type;
    std::unique_ptr<RequestListener> request_listener;
    std::unique_ptr<DDSPublicationListener> publication_listener;
    std::shared_ptr<ObjectPool<RequestData>> pool;
    std::shared_ptr<ThreadSafeQueue<std::shared_ptr<RequestData>>> requestQueue;
    std::shared_ptr<ThreadSafeQueue<std::shared_ptr<PooledResponseMap>>> responseChan;
    std::mutex writer_map_mutex;
    std::map<std::string, WriterInfo> writer_map;
    std::mutex topic_map_mutex_;
    std::map<std::string, std::string> request_topic_map_;
    std::mutex subscription_map_mutex_;
    std::map<std::string, std::string> subscription_topic_map_;

    std::thread response_thread_;
    std::thread cleanup_thread_;
    std::atomic<bool> stop_flag_{false};
    std::condition_variable cv_stop_;
    std::mutex mtx_stop_;

    DDSTransportImpl(std::shared_ptr<ObjectPool<RequestData>> p, std::shared_ptr<ThreadSafeQueue<std::shared_ptr<RequestData>>> queue)
        : participant(nullptr)
        , pool(p)
        , publisher(nullptr)
        , subscriber(nullptr)
        , request_topic(nullptr)
        , request_reader(nullptr)
        , request_type(new VissRequestTopicPubSubType())
        , requestQueue(queue)
        , responseChan(std::make_shared<ThreadSafeQueue<std::shared_ptr<PooledResponseMap>>>())
    {
        publication_listener = std::make_unique<DDSPublicationListener>(writer_map, writer_map_mutex);
        response_thread_ = std::thread([this]() {
            while (!stop_flag_.load()) {
                std::shared_ptr<PooledResponseMap> response_doc;
                responseChan->wait_and_pop(response_doc);

                if (response_doc) {
                    // Add debug log to print the response JSON
                    rapidjson::StringBuffer buffer;
                    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
                    response_doc->Accept(writer);
                    // 구독 응답 전문(JSON)은 **데이터 평면**이다 — 샘플마다 남기면
                    // 구독이 붙어 있는 동안 무한히 쌓인다(실측: 376GB/13일).
                    LOG_DEBUG << "DDS Transport Received Response JSON: " << buffer.GetString();
                }

                if (stop_flag_.load() || !response_doc) break;

                LOG_DEBUG << "Received response from ServiceManager.";

                std::string action;
                std::string requestId;
                std::string topic;

                if (response_doc->HasMember("action") && (*response_doc)["action"].IsString()) {
                    action = (*response_doc)["action"].GetString();
                } else {
                    LOG_ERROR << "Error processing response: 'action' is missing or not a string.";
                    continue;
                }

                if (response_doc->HasMember("requestId") && (*response_doc)["requestId"].IsString()) {
                    requestId = (*response_doc)["requestId"].GetString();
                    std::lock_guard<std::mutex> lock(topic_map_mutex_);
                    auto it = request_topic_map_.find(requestId);
                    if (it != request_topic_map_.end()) {
                        topic = it->second;
                        request_topic_map_.erase(it);
                        LOG_INFO << "RequestId " << requestId << " removed from map. "
                                  << request_topic_map_.size() << " requests remaining.";
                    }
                }
                
                if (response_doc->HasMember("subscriptionId") && (*response_doc)["subscriptionId"].IsString()) {
                    std::string subscriptionId = (*response_doc)["subscriptionId"].GetString();
                    if (!response_doc->HasMember("error")) {
                        std::lock_guard<std::mutex> lock(subscription_map_mutex_);
                        if (action == "subscribe") {
                            subscription_topic_map_[subscriptionId] = topic;
                            LOG_INFO << "Subscription successful for ID: " << subscriptionId 
                                      << " on topic: " << topic;
                        } else if (action == "unsubscribe") {
                            if(subscription_topic_map_.erase(subscriptionId)) {
                                LOG_INFO << "Unsubscribed successful for ID: " << subscriptionId;
                            }
                            response_doc->EraseMember("subscriptionId");
                        } else if (action == "subscription") {
                            // The termination signal is for transports that hold a waiting loop.
                            // For DDS, cleanup is handled by the "unsubscribe" action response.
                            // We just log that we received it and do not publish it.
                            if (response_doc->HasMember("status")) {
                                if (std::string((*response_doc)["status"].GetString()) == "terminated") {
                                    LOG_INFO << "DDS transport: Received termination signal for subscription " << subscriptionId << ". No action needed.";
                                    continue;
                                }
                            }

                            auto it = subscription_topic_map_.find(subscriptionId);
                            if (it != subscription_topic_map_.end()) {
                                topic = it->second;
                            } else {
                                LOG_ERROR << "Error: Response topic not found for subscriptionId: " << subscriptionId;
                            }
                        }
                    } else {
                         // Error handling for subscription remains tricky as error is a JSON object.
                         // For now, just log that a subscription/unsubscription failed.
                         LOG_ERROR << "Subscription/Unsubscription failed for ID: " << subscriptionId;
                    }
                }

                if (topic.empty()) {
                    LOG_ERROR << "Error: Response topic could not be determined for requestId: " << requestId;
                    continue;
                }
                
                rapidjson::StringBuffer buffer;
                rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
                response_doc->Accept(writer);

                publish(topic, buffer.GetString());
            }
            LOG_INFO << "DDS response thread shutting down.";
        });

        cleanup_thread_ = std::thread(&DDSTransportImpl::cleanup_writers, this);
    }

    void cleanup_writers() {
        const std::chrono::minutes cleanup_interval(1);
        const std::chrono::minutes writer_timeout(5);

        while (!stop_flag_.load()) {
            std::unique_lock<std::mutex> lock(mtx_stop_);
            cv_stop_.wait_for(lock, cleanup_interval, [this]{ return stop_flag_.load(); });
            
            if(stop_flag_.load()) break;

            std::lock_guard<std::mutex> writer_lock(writer_map_mutex);
            auto now = std::chrono::steady_clock::now();
            for (auto it = writer_map.begin(); it != writer_map.end(); ) {
                if (std::chrono::duration_cast<std::chrono::minutes>(now - it->second.last_used) > writer_timeout) {
                    std::cout << "Cleaning up inactive writer for topic: " << it->first << std::endl;
                    publisher->delete_datawriter(it->second.writer);
                    participant->delete_topic(it->second.topic);
                    it = writer_map.erase(it);
                } else {
                    ++it;
                }
            }
        }
        LOG_INFO << "DDS writer cleanup thread shutting down.";
    }

    void publish(const std::string& topic_name, const std::string& payload) {
        std::lock_guard<std::mutex> lock(writer_map_mutex);
        
        DataWriter* writer = nullptr;
        auto it = writer_map.find(topic_name);
        if (it == writer_map.end()) {
            Topic* topic = participant->create_topic(topic_name, request_type.get_type_name(), TOPIC_QOS_DEFAULT);
            if (topic == nullptr) {
                LOG_ERROR << "Failed to create topic for publishing: " << topic_name;
                return;
            }
            DataWriterQos writer_qos = DATAWRITER_QOS_DEFAULT;
            writer_qos.reliability().kind = RELIABLE_RELIABILITY_QOS;
            writer = publisher->create_datawriter(topic, writer_qos, publication_listener.get());
            if (writer == nullptr) {
                LOG_ERROR << "Failed to create datawriter for topic: " << topic_name;
                participant->delete_topic(topic);
                return;
            }
            writer_map[topic_name] = {writer, topic, std::chrono::steady_clock::now()};
        } else {
            writer = it->second.writer;
            it->second.last_used = std::chrono::steady_clock::now();
        }

        VissRequestTopic data;
        data.payload(payload);
        writer->write(&data);
    }
};

// Internal callback handler class
class RequestListener : public DataReaderListener {
    DDSTransportImpl* p_impl;

public:
    RequestListener(DDSTransportImpl* impl) : p_impl(impl) {}
    void on_data_available(DataReader* reader) override;
};

// DDSTransport public methods
DDSTransport::DDSTransport(std::shared_ptr<ObjectPool<RequestData>> pool, std::shared_ptr<ThreadSafeQueue<std::shared_ptr<RequestData>>> queue) 
    : p_impl(new DDSTransportImpl(pool, queue)) {
    p_impl->request_listener = std::make_unique<RequestListener>(p_impl);
}

DDSTransport::~DDSTransport() {
    p_impl->stop_flag_.store(true);
    p_impl->cv_stop_.notify_one();
    // Push a null pointer to unblock the thread
    p_impl->responseChan->push(nullptr);
    if(p_impl->response_thread_.joinable()) {
        p_impl->response_thread_.join();
    }
    if(p_impl->cleanup_thread_.joinable()) {
        p_impl->cleanup_thread_.join();
    }

    if (p_impl->participant != nullptr) {
        if (p_impl->request_reader != nullptr) {
            p_impl->request_reader->set_listener(nullptr);
        }
        std::lock_guard<std::mutex> writer_lock(p_impl->writer_map_mutex);
        for (auto const& [key, val] : p_impl->writer_map) {
            if (val.writer != nullptr) {
                val.writer->set_listener(nullptr);
            }
        }
        p_impl->participant->delete_contained_entities();
        DomainParticipantFactory::get_instance()->delete_participant(p_impl->participant);
        p_impl->participant = nullptr;
    }
    delete p_impl;
}

void DDSTransport::start() {
    if (p_impl->participant != nullptr) {
        LOG_INFO << "DDSTransport already started.";
        return;
    }
    // ★ get_default_participant_qos(), not a default-constructed QoS.
    //
    // A bare `DomainParticipantQos pqos;` never sees the XML profile at all -- Fast DDS
    // applies it only when create_participant() is handed PARTICIPANT_QOS_DEFAULT BY ADDRESS
    // (3.2.2, DomainParticipantFactory.cpp:160). This participant was therefore running
    // without the transport / data-sharing settings the shipped profile intends.
    DomainParticipantQos pqos = DomainParticipantFactory::get_instance()->get_default_participant_qos();
    pqos.name("viss_dds_transport");
    // ★ DO NOT PROPAGATE TypeObject. Cyclone participants on the same domain cannot ingest
    // it and log `invalid type object` -- 1260 lines across the three Autoware partitions,
    // measured 2026-08-12 on domain 42. They do not crash. The failure lands on the reader,
    // so this protects the neighbours' logs, not this process.
    // docs/sils/evidence/mmf-typeobject-2026-08-12.md
    pqos.properties().properties().emplace_back("fastdds.type_propagation", "disabled");

    // VISS sits on the pure-DDS plane. Precedence: VISS_DDS_DOMAIN_ID (per-service
    // override -- SILS contracts pin it to the vECU's domain, §3.4) > DDS_DOMAIN_ID
    // (canonical DDS plane, written by hmi_launcher Settings) > 0. ROS_DOMAIN_ID is
    // the separate ROS2 plane and is deliberately NOT consulted here.
    int domain_id = 0; // Default domain ID
    const char* domain_env_name = "VISS_DDS_DOMAIN_ID";
    const char* domain_id_env = std::getenv(domain_env_name);
    if (!domain_id_env) {
        domain_env_name = "DDS_DOMAIN_ID";
        domain_id_env = std::getenv(domain_env_name);
    }
    if (domain_id_env) {
        try {
            domain_id = std::stoi(domain_id_env);
            LOG_INFO << "Using DDS Domain ID from environment variable " << domain_env_name << ": " << domain_id;
        } catch (const std::invalid_argument& e) {
            LOG_ERROR << "Invalid argument for " << domain_env_name << ": " << e.what() << ". Using default domain ID 0.";
        } catch (const std::out_of_range& e) {
            LOG_ERROR << domain_env_name << " is out of range: " << e.what() << ". Using default domain ID 0.";
        }
    } else {
        LOG_INFO << "VISS_DDS_DOMAIN_ID / DDS_DOMAIN_ID not set. Using default domain ID 0.";
    }

    p_impl->participant = DomainParticipantFactory::get_instance()->create_participant(domain_id, pqos, nullptr, StatusMask::none());
    if (p_impl->participant == nullptr) {
        LOG_ERROR << "Failed to create DDS participant";
        return;
    }

    p_impl->request_type.register_type(p_impl->participant);
    p_impl->publisher = p_impl->participant->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr);
    p_impl->subscriber = p_impl->participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr);

    p_impl->request_topic = p_impl->participant->create_topic(VISS_DDS_REQUEST_TOPIC, p_impl->request_type.get_type_name(), TOPIC_QOS_DEFAULT);

    DataReaderQos reader_qos = DATAREADER_QOS_DEFAULT;
    reader_qos.reliability().kind = RELIABLE_RELIABILITY_QOS;
    p_impl->request_reader = p_impl->subscriber->create_datareader(p_impl->request_topic, reader_qos, p_impl->request_listener.get());

    LOG_INFO << "DDS Transport started and subscribed to " << VISS_DDS_REQUEST_TOPIC;
}

void DDSTransport::stop() {
    LOG_INFO << "DDS Transport stopped.";
}

void DDSPublicationListener::on_publication_matched(
    eprosima::fastdds::dds::DataWriter* writer,
    const eprosima::fastdds::dds::PublicationMatchedStatus& info)
{
    LOG_INFO << "Publication matched for topic: " << writer->get_topic()->get_name()
            << " | Total count: " << info.total_count
            << " | Total count change: " << info.total_count_change
            << " | Current count: " << info.current_count
            << " | Current count change: " << info.current_count_change;

}


//“/mmf/otamanager/vehicleinfo_service/GetVehicleInfo” 
//“/mmf/otamanager/vehicleinfo_service/GetVehicleInfoResponse”




// Callback implementations
void RequestListener::on_data_available(eprosima::fastdds::dds::DataReader* reader) {
    eprosima::fastdds::dds::SampleInfo info;
    VissRequestTopic received_sample;

    if (reader->take_next_sample(&received_sample, &info) == eprosima::fastdds::dds::RETCODE_OK) {
        if (info.valid_data) {
            rapidjson::Document payload_doc;
            payload_doc.Parse(received_sample.payload().c_str());

            if (payload_doc.HasParseError() || !payload_doc.IsObject() || !payload_doc.HasMember("topic") || !payload_doc["topic"].IsString() || !payload_doc.HasMember("request") || !payload_doc["request"].IsString()) {
                LOG_ERROR << "DDS message payload is not a valid JSON or has missing fields.";
                return;
            }

            std::string response_topic = payload_doc["topic"].GetString();
            
            std::string request_str = payload_doc["request"].GetString();
            auto request_doc_ptr = JsonSchemaValidator::getInstance().validate(request_str);
            if (!request_doc_ptr) {
                // validate() already prints detailed errors
                return;
            }

            // Ensure 'filter' is an array, and its 'parameter' is also an array for ServiceManager.
            if (request_doc_ptr->HasMember("filter")) {
                auto& allocator = request_doc_ptr->GetAllocator();
                rapidjson::Value& filters = (*request_doc_ptr)["filter"];
                rapidjson::Value new_filter_array(rapidjson::kArrayType);

                // Lambda to process a single filter object
                auto process_filter = [&](rapidjson::Value& filter_obj) {
                    if (filter_obj.IsObject() && filter_obj.HasMember("parameter") && !filter_obj["parameter"].IsArray()) {
                        rapidjson::Value param_copy(filter_obj["parameter"], allocator); // Deep copy
                        filter_obj.RemoveMember("parameter");
                        rapidjson::Value new_param_array(rapidjson::kArrayType);
                        new_param_array.PushBack(param_copy, allocator);
                        filter_obj.AddMember("parameter", new_param_array, allocator);
                    }
                    new_filter_array.PushBack(filter_obj, allocator);
                };

                if (filters.IsArray()) {
                    for (auto& filter : filters.GetArray()) {
                        process_filter(filter);
                    }
                } else if (filters.IsObject()) {
                    process_filter(filters);
                }

                // Replace original 'filter' with the new, processed array
                request_doc_ptr->RemoveMember("filter");
                request_doc_ptr->AddMember("filter", new_filter_array, allocator);
            }
            
            auto requestData = p_impl->pool->acquire();

            if (!request_doc_ptr->HasMember("requestId") || !(*request_doc_ptr)["requestId"].IsString()) {
                LOG_ERROR << "Bad Request: 'requestId' is missing or not a string in DDS message.";
                return;
            }
            requestData->requestId = (*request_doc_ptr)["requestId"].GetString();

            requestData->request_doc->Swap(*request_doc_ptr);

            requestData->responseChan = p_impl->responseChan;
            {
                std::lock_guard<std::mutex> lock(p_impl->topic_map_mutex_);
                p_impl->request_topic_map_[requestData->requestId] = response_topic;
            }

            p_impl->requestQueue->push(requestData);
        }
    }
}
