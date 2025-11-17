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
};

struct DDSTransportImpl {
    eprosima::fastdds::dds::DomainParticipant* participant;
    eprosima::fastdds::dds::Publisher* publisher;
    eprosima::fastdds::dds::Subscriber* subscriber;
    eprosima::fastdds::dds::Topic* request_topic;
    eprosima::fastdds::dds::DataReader* request_reader;
    eprosima::fastdds::dds::TypeSupport request_type;
    std::unique_ptr<RequestListener> request_listener;
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
        response_thread_ = std::thread([this]() {
            while (!stop_flag_.load()) {
                std::shared_ptr<PooledResponseMap> response_doc;
                responseChan->wait_and_pop(response_doc);

                if (response_doc) {
                    // Add debug log to print the response JSON
                    rapidjson::StringBuffer buffer;
                    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
                    response_doc->Accept(writer);
                    LOG_INFO << "DDS Transport Received Response JSON: " << buffer.GetString();
                }

                if (stop_flag_.load() || !response_doc) break;

                LOG_INFO << "Received response from ServiceManager.";

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
            writer = publisher->create_datawriter(topic, writer_qos, nullptr);
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
        if (p_impl->publisher != nullptr) {
            for(auto const& [key, val] : p_impl->writer_map) {
                p_impl->publisher->delete_datawriter(val.writer);
                p_impl->participant->delete_topic(val.topic);
            }
            p_impl->participant->delete_publisher(p_impl->publisher);
        }
        if (p_impl->subscriber != nullptr) {
            if (p_impl->request_reader != nullptr) {
                p_impl->subscriber->delete_datareader(p_impl->request_reader);
            }
            p_impl->participant->delete_subscriber(p_impl->subscriber);
        }
        if (p_impl->request_topic != nullptr) {
            p_impl->participant->delete_topic(p_impl->request_topic);
        }
        DomainParticipantFactory::get_instance()->delete_participant(p_impl->participant);
    }
    delete p_impl;
}

void DDSTransport::start() {
    DomainParticipantQos pqos;
    pqos.name("viss_dds_transport");
    //pqos.name("");
    p_impl->participant = DomainParticipantFactory::get_instance()->create_participant(7, pqos, nullptr, StatusMask::none());
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

            // Ensure the filter is always an array for ServiceManager
            if (request_doc_ptr->HasMember("filter") && (*request_doc_ptr)["filter"].IsObject()) {
                rapidjson::Value filter_array(rapidjson::kArrayType);
                filter_array.PushBack((*request_doc_ptr)["filter"], request_doc_ptr->GetAllocator());
                (*request_doc_ptr).RemoveMember("filter");
                (*request_doc_ptr).AddMember("filter", filter_array, request_doc_ptr->GetAllocator());
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
