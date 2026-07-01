#ifndef OTAM_DDS_IMPL_H_
#define OTAM_DDS_IMPL_H_

#include "otam_dds.h"
#include <iostream>
#include <mutex>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/qos/DataWriterQos.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/topic/Topic.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

/**
 * @file otam_dds_impl.h
 * @brief Implementation of DDS communication using FastDDS for OTA Manager
 *
 * Refactored to use a shared DomainParticipant/Publisher/Subscriber singleton.
 * Previously each OtamDdsImpl created its own DomainParticipant, causing
 * excessive resource usage when many topics are registered simultaneously.
 *
 * Design:
 *   OtamDdsParticipant (singleton)
 *     └── DomainParticipant (1 instance, domain 0)
 *           ├── Publisher (1 instance, shared)
 *           └── Subscriber (1 instance, shared)
 *
 *   OtamDdsImpl<T> (1 instance per topic)
 *     ├── Topic
 *     ├── DataWriter
 *     └── DataReader
 */

// ============================================================================
// OtamDdsParticipant: Shared DomainParticipant Singleton
// ============================================================================

/**
 * @class OtamDdsParticipant
 * @brief Singleton that owns the single DomainParticipant, Publisher, Subscriber
 *        shared by all OtamDdsImpl instances.
 */
class OtamDdsParticipant {
public:
    /**
     * @brief Get the singleton instance (thread-safe)
     */
    static OtamDdsParticipant& GetInstance() {
        static OtamDdsParticipant instance;
        return instance;
    }

    /**
     * @brief Ensure the participant is created (idempotent)
     * @throws std::runtime_error on failure
     */
    void EnsureCreated() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (participant_) return;

        auto factory = eprosima::fastdds::dds::DomainParticipantFactory::get_instance();
        // Suppress same-participant writer→reader loopback. Production uses cross-process
        // participants so this does not affect real inter-service traffic; it only
        // eliminates spurious self-delivery when Request/Response topics share a
        // participant (triggers false matches in OnSetServiceModeResponseReceived).
        eprosima::fastdds::dds::DomainParticipantQos participant_qos =
            eprosima::fastdds::dds::PARTICIPANT_QOS_DEFAULT;
        participant_qos.properties().properties().emplace_back(
            "fastdds.ignore_local_endpoints", "true");
        // Disable XTypes TypeObject discovery so peers fall back to
        // type-name-only matching. The dynamic-IDL adapter
        // (dds_bridge) parses the same IDL files at runtime but
        // produces a TypeObject with different member IDs / hashes
        // than the IDL-compiled PubSubType OtaManager registers.
        // Without this property OtaManager's strict reader rejects
        // the bridge's writes.
        participant_qos.properties().properties().emplace_back(
            "fastdds.type_propagation", "disabled");
        participant_ = factory->create_participant(0, participant_qos);
        if (!participant_) {
            throw std::runtime_error("OtamDdsParticipant: Failed to create shared DomainParticipant");
        }

        publisher_ = participant_->create_publisher(eprosima::fastdds::dds::PUBLISHER_QOS_DEFAULT, nullptr);
        if (!publisher_) {
            factory->delete_participant(participant_);
            participant_ = nullptr;
            throw std::runtime_error("OtamDdsParticipant: Failed to create shared Publisher");
        }

        subscriber_ = participant_->create_subscriber(eprosima::fastdds::dds::SUBSCRIBER_QOS_DEFAULT, nullptr);
        if (!subscriber_) {
            participant_->delete_contained_entities();
            factory->delete_participant(participant_);
            participant_ = nullptr;
            publisher_   = nullptr;
            throw std::runtime_error("OtamDdsParticipant: Failed to create shared Subscriber");
        }

        std::cout << "OtamDdsParticipant: Shared DomainParticipant created." << std::endl;
    }

    eprosima::fastdds::dds::DomainParticipant* GetParticipant() const { return participant_; }
    eprosima::fastdds::dds::Publisher*         GetPublisher()   const { return publisher_;   }
    eprosima::fastdds::dds::Subscriber*        GetSubscriber()  const { return subscriber_;  }

    /**
     * @brief Destroy the shared participant (call only during shutdown)
     *        All OtamDdsImpl instances must have called Destroy() first.
     */
    void Shutdown() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!participant_) return;
        participant_->delete_contained_entities();
        eprosima::fastdds::dds::DomainParticipantFactory::get_instance()
            ->delete_participant(participant_);
        participant_ = nullptr;
        publisher_   = nullptr;
        subscriber_  = nullptr;
        std::cout << "OtamDdsParticipant: Shared DomainParticipant destroyed." << std::endl;
    }

private:
    OtamDdsParticipant() : participant_(nullptr), publisher_(nullptr), subscriber_(nullptr) {}
    ~OtamDdsParticipant() { Shutdown(); }

    // Non-copyable
    OtamDdsParticipant(const OtamDdsParticipant&) = delete;
    OtamDdsParticipant& operator=(const OtamDdsParticipant&) = delete;

    std::mutex mutex_;
    eprosima::fastdds::dds::DomainParticipant* participant_;
    eprosima::fastdds::dds::Publisher*         publisher_;
    eprosima::fastdds::dds::Subscriber*        subscriber_;
};

// ============================================================================
// OtamDdsImpl: Per-topic Topic/DataWriter/DataReader wrapper
// ============================================================================

/**
 * @class OtamDdsImpl
 * @brief Concrete implementation of DDS communication using FastDDS.
 *
 * Uses the shared OtamDdsParticipant singleton instead of creating a new
 * DomainParticipant per topic. Each instance manages only:
 *   - Topic
 *   - DataWriter
 *   - DataReader
 *
 * @tparam MessageType The DDS message type
 */
template<typename MessageType>
class OtamDdsImpl : public OtamDds {
public:
    OtamDdsImpl();
    ~OtamDdsImpl() override;

    void Create(const std::string& topic, std::shared_ptr<void> type, std::shared_ptr<void> listener, bool transient_local = false) override;
    void SetQos(uint32_t qos) override;
    void Write(std::shared_ptr<void> message) override;
    void Read(std::shared_ptr<void> message) override;
    void Destroy() override;

    // Accessors (participant/publisher/subscriber now come from singleton)
    eprosima::fastdds::dds::DomainParticipant* GetParticipant() const {
        return OtamDdsParticipant::GetInstance().GetParticipant();
    }
    eprosima::fastdds::dds::Publisher*  GetPublisher()  const {
        return OtamDdsParticipant::GetInstance().GetPublisher();
    }
    eprosima::fastdds::dds::Subscriber* GetSubscriber() const {
        return OtamDdsParticipant::GetInstance().GetSubscriber();
    }
    eprosima::fastdds::dds::Topic*      GetTopic()      const { return topic_;   }
    eprosima::fastdds::dds::DataWriter* GetWriter()     const { return writer_;  }
    eprosima::fastdds::dds::DataReader* GetReader()     const { return reader_;  }

private:
    /** @brief FastDDS topic for message routing */
    eprosima::fastdds::dds::Topic*      topic_;

    /** @brief FastDDS data writer for message publishing */
    eprosima::fastdds::dds::DataWriter* writer_;

    /** @brief FastDDS data reader for message reception */
    eprosima::fastdds::dds::DataReader* reader_;

    /** @brief Listener for data reception callbacks */
    eprosima::fastdds::dds::DataReaderListener* listener_;

    /** @brief Stored topic name */
    std::string topic_name_;

    /** @brief Keeps the PubSubType object alive */
    std::shared_ptr<void> type_ptr_;

    /** @brief Keeps the listener object alive */
    std::shared_ptr<void> listener_ptr_;

    /** @brief Flag to prevent double Destroy() */
    bool destroyed_;
};

// ============================================================================
// Template Implementation
// ============================================================================

template<typename MessageType>
OtamDdsImpl<MessageType>::OtamDdsImpl()
    : topic_(nullptr),
      writer_(nullptr),
      reader_(nullptr),
      listener_(nullptr),
      destroyed_(false) {}

template<typename MessageType>
OtamDdsImpl<MessageType>::~OtamDdsImpl() {
    Destroy();
}

template<typename MessageType>
void OtamDdsImpl<MessageType>::Destroy() {
    if (destroyed_) {
        std::cout << "OtamDdsImpl: Already destroyed, skipping." << std::endl;
        return;
    }

    std::cout << "OtamDdsImpl: Destroy called. Cleaning up DDS entities." << std::endl;

    auto& shared = OtamDdsParticipant::GetInstance();
    auto* participant = shared.GetParticipant();
    auto* publisher   = shared.GetPublisher();
    auto* subscriber  = shared.GetSubscriber();

    // Delete DataReader
    if (reader_ && subscriber) {
        subscriber->delete_datareader(reader_);
        reader_ = nullptr;
    }

    // Delete DataWriter
    if (writer_ && publisher) {
        publisher->delete_datawriter(writer_);
        writer_ = nullptr;
    }

    // Delete Topic
    if (topic_ && participant) {
        participant->delete_topic(topic_);
        topic_ = nullptr;
    }

    destroyed_ = true;
    std::cout << "OtamDdsImpl: Deleted participant and all contained entities." << std::endl;
    std::cout << "OtamDdsImpl: Destroy completed successfully." << std::endl;
}

template<typename MessageType>
void OtamDdsImpl<MessageType>::Create(const std::string& topic,
                                      std::shared_ptr<void> type,
                                      std::shared_ptr<void> listener,
                                      bool transient_local) {
    std::cout << "OtamDdsImpl: Creating DDS entities for topic: " << topic << std::endl;
    topic_name_ = topic;

    std::cout << "TypeSupport!!!!!" << type.get() << std::endl;
    type_ptr_     = type;
    listener_ptr_ = listener;

    if (listener) {
        listener_ = std::static_pointer_cast<eprosima::fastdds::dds::DataReaderListener>(listener).get();
    } else {
        listener_ = nullptr;
    }
    std::cout << "DataReaderListener!!!!! listener_=" << listener_ << std::endl;

    // Ensure the shared participant is ready
    OtamDdsParticipant::GetInstance().EnsureCreated();

    auto* participant = OtamDdsParticipant::GetInstance().GetParticipant();
    auto* publisher   = OtamDdsParticipant::GetInstance().GetPublisher();
    auto* subscriber  = OtamDdsParticipant::GetInstance().GetSubscriber();

    // Build TypeSupport with no-op deleter (type_ptr_ owns the object)
    auto* raw_type = static_cast<eprosima::fastdds::dds::TopicDataType*>(type_ptr_.get());
    eprosima::fastdds::dds::TypeSupport type_support;
    static_cast<std::shared_ptr<eprosima::fastdds::dds::TopicDataType>&>(type_support)
        .reset(raw_type, [](eprosima::fastdds::dds::TopicDataType*) {});

    std::cout << "Create DomainParticipant!!!!!" << std::endl;  // keep legacy log (used by log-parsing consumers)

    try {
        // Register TypeSupport with shared participant
        // If the type name is already registered, FastDDS reuses it safely.
        type_support.register_type(participant);

        std::cout << "Register TypeSupport!!!!! " << type_support.get() << std::endl;
        std::cout << "type_support.get_type_name() = " << type_support.get_type_name() << std::endl;

        // Create Topic
        topic_ = participant->create_topic(topic_name_, type_support.get_type_name(),
            eprosima::fastdds::dds::TOPIC_QOS_DEFAULT);
        if (!topic_) {
            throw std::runtime_error("DDS Exception: Failed to create Topic: " + topic_name_);
        }
        std::cout << "Create Topic!!!!!" << std::endl;

        std::cout << "Create Publisher!!!!!" << std::endl;  // keep legacy log

        // Create DataWriter on shared Publisher
        eprosima::fastdds::dds::DataWriterQos writer_qos = eprosima::fastdds::dds::DATAWRITER_QOS_DEFAULT;
        // F011: disable POSIX-shm data_sharing — when other FastDDS participants
        // live in sibling containers (host network), their shm segments are
        // unreachable and the reader spams "Failed to open segment" errors.
        writer_qos.data_sharing().off();
        writer_qos.reliability().kind = eprosima::fastdds::dds::RELIABLE_RELIABILITY_QOS;
        if (transient_local) {
            writer_qos.durability().kind = eprosima::fastdds::dds::TRANSIENT_LOCAL_DURABILITY_QOS;
        }
        writer_ = publisher->create_datawriter(topic_, writer_qos);
        if (!writer_) {
            throw std::runtime_error("DDS Exception: Failed to create DataWriter for: " + topic_name_);
        }
        std::cout << "Create Writer!!!!! (RELIABLE QoS"
                  << (transient_local ? ", TRANSIENT_LOCAL" : "") << ")" << std::endl;

        std::cout << "Create Subscriber!!!!!" << std::endl;  // keep legacy log

        // Create DataReader on shared Subscriber
        // Durability: VOLATILE - keeps FastDDS default (also matches TRANSIENT_LOCAL publishers)
        // DDS QoS compatibility rule: VOLATILE subscriber can receive from TRANSIENT_LOCAL publisher
        eprosima::fastdds::dds::DataReaderQos reader_qos = eprosima::fastdds::dds::DATAREADER_QOS_DEFAULT;
        // F011: see writer counterpart — disable shm data_sharing to silence
        // "Failed to open segment" noise from sibling-container writers.
        reader_qos.data_sharing().off();
        reader_qos.reliability().kind = eprosima::fastdds::dds::RELIABLE_RELIABILITY_QOS;
        reader_ = subscriber->create_datareader(topic_, reader_qos, listener_);
        if (!reader_) {
            throw std::runtime_error("DDS Exception: Failed to create DataReader for: " + topic_name_);
        }
        std::cout << "Create Reader!!!!! (RELIABLE QoS)" << std::endl;

        std::cout << "OtamDdsImpl: All DDS entities created successfully." << std::endl;
    }
    catch (const std::exception& e) {
        // Clean up any entities that were created before the exception
        if (reader_ && subscriber)  { subscriber->delete_datareader(reader_); reader_ = nullptr; }
        if (writer_ && publisher)   { publisher->delete_datawriter(writer_);  writer_ = nullptr; }
        if (topic_ && participant)  { participant->delete_topic(topic_);       topic_  = nullptr; }
        std::cerr << "OtamDdsImpl: Exception during DDS entity creation: " << e.what() << std::endl;
        throw;
    }
}

template<typename MessageType>
void OtamDdsImpl<MessageType>::SetQos(uint32_t qos) {
    std::cout << "OtamDdsImpl: Setting QoS to " << qos << std::endl;

    auto reliability_kind = (qos == 1)
        ? eprosima::fastdds::dds::RELIABLE_RELIABILITY_QOS
        : eprosima::fastdds::dds::BEST_EFFORT_RELIABILITY_QOS;

    auto* publisher  = OtamDdsParticipant::GetInstance().GetPublisher();
    auto* subscriber = OtamDdsParticipant::GetInstance().GetSubscriber();

    try {
        if (writer_ && publisher && topic_) {
            publisher->delete_datawriter(writer_);
            writer_ = nullptr;

            eprosima::fastdds::dds::DataWriterQos writer_qos = eprosima::fastdds::dds::DATAWRITER_QOS_DEFAULT;
            writer_qos.reliability().kind = reliability_kind;
            writer_ = publisher->create_datawriter(topic_, writer_qos);
            if (!writer_) throw std::runtime_error("DDS Exception: Failed to recreate DataWriter.");
            std::cout << "OtamDdsImpl: DataWriter recreated with new QoS. writer_=" << writer_ << std::endl;
        }

        if (reader_ && subscriber && topic_) {
            subscriber->delete_datareader(reader_);
            reader_ = nullptr;

            eprosima::fastdds::dds::DataReaderQos reader_qos = eprosima::fastdds::dds::DATAREADER_QOS_DEFAULT;
            reader_qos.reliability().kind = reliability_kind;
            reader_ = subscriber->create_datareader(topic_, reader_qos, listener_);
            if (!reader_) throw std::runtime_error("DDS Exception: Failed to recreate DataReader.");
            std::cout << "OtamDdsImpl: DataReader recreated with new QoS. reader_=" << reader_ << std::endl;
        }

        std::cout << "OtamDdsImpl: SetQos completed successfully." << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "OtamDdsImpl: Exception during QoS configuration: " << e.what() << std::endl;
        throw;
    }
}

template<typename MessageType>
void OtamDdsImpl<MessageType>::Write(std::shared_ptr<void> message) {
    std::cout << "OtamDdsImpl: Writing DDS message." << std::endl;
    std::cout << "OtamDdsImpl: writer_=" << writer_ << ", message.get()=" << message.get() << std::endl;

    if (!writer_) throw std::runtime_error("DDS Exception: DataWriter not created.");
    if (!message) throw std::runtime_error("DDS Exception: Message is null.");

    try {
        std::cout << "OtamDdsImpl: Calling writer_->write()..." << std::endl;
        auto typed_message = std::static_pointer_cast<MessageType>(message);
        eprosima::fastdds::dds::ReturnCode_t ret = writer_->write(typed_message.get());
        std::cout << "OtamDdsImpl: writer_->write() returned: " << ret << std::endl;

        if (ret != eprosima::fastdds::dds::RETCODE_OK) {
            throw std::runtime_error("DDS Exception: Failed to write DDS message.");
        }
        std::cout << "OtamDdsImpl: Successfully wrote message" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "OtamDdsImpl: Exception during message writing: " << e.what() << std::endl;
        throw;
    }
}

template<typename MessageType>
void OtamDdsImpl<MessageType>::Read(std::shared_ptr<void> message) {
    std::cout << "OtamDdsImpl: Reading DDS message (polling mode)." << std::endl;

    if (!reader_) throw std::runtime_error("DDS Exception: DataReader not created.");

    try {
        eprosima::fastdds::dds::SampleInfo info;
        auto typed_message = std::static_pointer_cast<MessageType>(message);

        if (reader_->take_next_sample(typed_message.get(), &info) == eprosima::fastdds::dds::RETCODE_OK) {
            if (info.valid_data) {
                std::cout << "OtamDdsImpl: Read message successfully" << std::endl;
            } else {
                std::cout << "OtamDdsImpl: No valid data available to read" << std::endl;
            }
        } else {
            std::cout << "OtamDdsImpl: Failed to take next sample (no new data or error)" << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "OtamDdsImpl: Exception during message reading: " << e.what() << std::endl;
        throw;
    }
}

#endif // OTAM_DDS_IMPL_H_
