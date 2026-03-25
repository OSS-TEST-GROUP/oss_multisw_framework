#ifndef OTAM_DDS_IMPL_H_
#define OTAM_DDS_IMPL_H_

#include "otam_dds.h"
#include <iostream>
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
 * @author OTA Manager Team
 * @date 2024
 */

/**
 * @class OtamDdsImpl
 * @brief Concrete implementation of DDS communication using FastDDS
 *
 * This class inherits from DdsAbstraction and provides concrete implementations
 * for DDS functionalities tailored for the OTA Manager system. It manages
 * the lifecycle of FastDDS entities including DomainParticipant, Publisher,
 * Subscriber, Topic, DataWriter, and DataReader.
 *
 * @tparam MessageType The DDS message type to be used for communication
 * @see DdsAbstraction
 */
template<typename MessageType>
class OtamDdsImpl : public OtamDds {
public:
    /**
     * @brief Default constructor
     *
     * Initializes all DDS entity pointers to nullptr.
     * Actual DDS entities are created in the Create() method.
     */
    OtamDdsImpl();

    /**
     * @brief Virtual destructor
     *
     * Cleans up all DDS entities in reverse order of creation.
     * Ensures proper resource cleanup to prevent memory leaks.
     */
    ~OtamDdsImpl() override;

    /**
     * @brief Creates necessary FastDDS entities for communication
     *
     * Creates DomainParticipant, Publisher, Subscriber, Topic, DataWriter,
     * and DataReader for the specified topic. Registers the provided type
     * and sets up the listener if provided.
     *
     * @param topic The DDS topic name for communication
     * @param type Shared pointer to TypeSupport for message serialization
     * @param listener Shared pointer to DataReaderListener for message reception
     * @throws std::runtime_error if any DDS entity creation fails
     *
     * @note This method should be called only once per instance
     */
    void Create(const std::string& topic, std::shared_ptr<void> type, std::shared_ptr<void> listener) override;

    /**
     * @brief Sets Quality of Service (QoS) policies for DDS entities
     *
     * Configures QoS settings for both DataWriter and DataReader.
     * Currently supports reliability policy configuration.
     *
     * @param qos QoS configuration value (1 = Reliable, 0 = Best Effort)
     * @throws std::runtime_error if QoS setting fails
     *
     * @note Must be called after Create() method
     */
    void SetQos(uint32_t qos) override;

    /**
     * @brief Writes a DDS message to the network
     *
     * Publishes the provided message using the configured DataWriter.
     * The message is serialized and sent to all subscribers of the topic.
     *
     * @param message Shared pointer to the message to be published
     * @throws std::runtime_error if DataWriter is not created or write operation fails
     *
     * @note The message type must match the registered TypeSupport
     */
    void Write(std::shared_ptr<void> message) override;

    /**
     * @brief Reads a DDS message from the network
     *
     * Attempts to read the next available message from the DataReader.
     * This method is typically used for polling-based message reception.
     * For event-driven reception, use DataReaderListener callbacks instead.
     *
     * @param message Shared pointer where the received message will be stored
     * @throws std::runtime_error if DataReader is not created
     *
     * @note For event-driven systems, prefer using DataReaderListener
     */
    void Read(std::shared_ptr<void> message) override;

    void Destroy() override;

    /**
     * @brief Gets the DomainParticipant instance
     * @return Pointer to the DomainParticipant, nullptr if not created
     */
    eprosima::fastdds::dds::DomainParticipant* GetParticipant() const { return participant_; }

    /**
     * @brief Gets the Publisher instance
     * @return Pointer to the Publisher, nullptr if not created
     */
    eprosima::fastdds::dds::Publisher* GetPublisher() const { return publisher_; }

    /**
     * @brief Gets the Subscriber instance
     * @return Pointer to the Subscriber, nullptr if not created
     */
    eprosima::fastdds::dds::Subscriber* GetSubscriber() const { return subscriber_; }

    /**
     * @brief Gets the Topic instance
     * @return Pointer to the Topic, nullptr if not created
     */
    eprosima::fastdds::dds::Topic* GetTopic() const { return topic_; }

    /**
     * @brief Gets the DataWriter instance
     * @return Pointer to the DataWriter, nullptr if not created
     */
    eprosima::fastdds::dds::DataWriter* GetWriter() const { return writer_; }

    /**
     * @brief Gets the DataReader instance
     * @return Pointer to the DataReader, nullptr if not created
     */
    eprosima::fastdds::dds::DataReader* GetReader() const { return reader_; }

private:
    /** @brief FastDDS domain participant for DDS communication */
    eprosima::fastdds::dds::DomainParticipant* participant_;

    /** @brief FastDDS publisher for outgoing messages */
    eprosima::fastdds::dds::Publisher* publisher_;

    /** @brief FastDDS subscriber for incoming messages */
    eprosima::fastdds::dds::Subscriber* subscriber_;

    /** @brief FastDDS topic for message routing */
    eprosima::fastdds::dds::Topic* topic_;

    /** @brief FastDDS data writer for message publishing */
    eprosima::fastdds::dds::DataWriter* writer_;

    /** @brief FastDDS data reader for message reception */
    eprosima::fastdds::dds::DataReader* reader_;

    /** @brief Listener for data reception callbacks */
    eprosima::fastdds::dds::DataReaderListener* listener_;

    /** @brief Stored topic name for internal reference */
    std::string topic_name_;

    /** @brief Stored type shared_ptr to keep PubSubType alive - DECLARED FIRST to be destroyed LAST */
    std::shared_ptr<void> type_ptr_;

    /** @brief Stored listener shared_ptr to keep listener alive */
    std::shared_ptr<void> listener_ptr_;

    /** @brief Flag to track if Destroy() has been called */
    bool destroyed_;
};

// ============================================================================
// Template Implementation
// ============================================================================

template<typename MessageType>
OtamDdsImpl<MessageType>::OtamDdsImpl()
    : participant_(nullptr),
      publisher_(nullptr),
      subscriber_(nullptr),
      topic_(nullptr),
      writer_(nullptr),
      reader_(nullptr),
      listener_(nullptr),
      destroyed_(false) {
    // Constructor: All members initialized to nullptr
    // DDS entities are created in the Create method
}

template<typename MessageType>
OtamDdsImpl<MessageType>::~OtamDdsImpl() {
    Destroy();
}

template<typename MessageType>
void OtamDdsImpl<MessageType>::Destroy() {
    // Check if already destroyed to make this function idempotent
    if (destroyed_) {
        std::cout << "OtamDdsImpl: Already destroyed, skipping." << std::endl;
        return;
    }

    std::cout << "OtamDdsImpl: Destroy called. Cleaning up DDS entities." << std::endl;

    // Delete contained entities first (DataWriter, DataReader, Publisher,
    // Subscriber, Topic) before deleting the participant.
    // delete_participant() returns RETCODE_PRECONDITION_NOT_MET if any
    // contained entity still exists, leaving the participant alive and
    // SHM transport files in an inconsistent state (zombie ports on restart).
    if (participant_) {
        eprosima::fastdds::dds::ReturnCode_t ret =
            participant_->delete_contained_entities();
        if (ret != eprosima::fastdds::dds::RETCODE_OK) {
            std::cerr << "OtamDdsImpl: Warning - delete_contained_entities failed: "
                      << ret << std::endl;
        }
        // Null out all raw pointers - they are now invalid
        writer_    = nullptr;
        reader_    = nullptr;
        publisher_ = nullptr;
        subscriber_ = nullptr;
        topic_     = nullptr;

        auto factory = eprosima::fastdds::dds::DomainParticipantFactory::get_instance();
        if (factory) {
            ret = factory->delete_participant(participant_);
            if (ret != eprosima::fastdds::dds::RETCODE_OK) {
                std::cerr << "OtamDdsImpl: Warning - delete_participant failed: "
                          << ret << std::endl;
            }
        }
        participant_ = nullptr;
        std::cout << "OtamDdsImpl: Deleted participant and all contained entities." << std::endl;
    }

    // Mark as destroyed to prevent double deletion
    destroyed_ = true;
    std::cout << "OtamDdsImpl: Destroy completed successfully." << std::endl;
}

template<typename MessageType>
void OtamDdsImpl<MessageType>::Create(const std::string& topic, std::shared_ptr<void> type, std::shared_ptr<void> listener) {
    std::cout << "OtamDdsImpl: Creating DDS entities for topic: " << topic << std::endl;
    topic_name_ = topic;

    std::cout << "TypeSupport!!!!!" << type.get() << std::endl;

    // IMPORTANT: Store the shared_ptr to keep PubSubType alive.
    // type_ptr_ is the single owner of the TopicDataType object.
    type_ptr_ = type;

    // Construct TypeSupport with a no-op deleter so that when the
    // DomainParticipant releases its copy (via delete_participant /
    // delete_contained_entities), it does NOT free the raw pointer.
    // type_ptr_ is the sole owner and will free the object when it
    // goes out of scope (preventing the double-free / SIGABRT that
    // would otherwise occur with two independent ownership chains).
    //
    // TypeSupport only exposes TypeSupport(TopicDataType*) which takes
    // sole ownership.  We work around this by default-constructing and
    // then resetting the underlying shared_ptr via the base-class reset()
    // with a no-op deleter.
    auto* raw_type = static_cast<eprosima::fastdds::dds::TopicDataType*>(type_ptr_.get());
    eprosima::fastdds::dds::TypeSupport type_support;
    static_cast<std::shared_ptr<eprosima::fastdds::dds::TopicDataType>&>(type_support)
        .reset(raw_type, [](eprosima::fastdds::dds::TopicDataType*) {});

    // Store listener shared_ptr to keep it alive
    listener_ptr_ = listener;
    if (listener) {
        listener_ = std::static_pointer_cast<eprosima::fastdds::dds::DataReaderListener>(listener).get();
    } else {
        listener_ = nullptr;
    }
    std::cout << "DataReaderListener!!!!! listener_=" << listener_ << std::endl;

    try {
        // Step 1: Create DomainParticipant
        // The DomainParticipant represents the local DDS domain entity
        participant_ = eprosima::fastdds::dds::DomainParticipantFactory::get_instance()
            ->create_participant(0, eprosima::fastdds::dds::PARTICIPANT_QOS_DEFAULT);
        if (!participant_) {
            throw std::runtime_error("DDS Exception: Failed to create DomainParticipant.");
        }
        std::cout << "Create DomainParticipant!!!!!" << std::endl;

        // Step 2: Register TypeSupport
        // Register the message type with the participant for serialization

        type_support.register_type(participant_);

        std::cout << "Register TypeSupport!!!!! " << type_support.get() << std::endl;

        // Step 3: Create Topic
        // The topic defines the data type and name for communication
        std::cout << "type_support.get_type_name() = " << type_support.get_type_name() << std::endl;

        topic_ = participant_->create_topic(topic_name_, type_support.get_type_name(),
            eprosima::fastdds::dds::TOPIC_QOS_DEFAULT);
        if (!topic_) {
            throw std::runtime_error("DDS Exception: Failed to create Topic.");
        }
        std::cout << "Create Topic!!!!!" << std::endl;

        // Step 4: Create Publisher
        // Publisher manages DataWriters for outgoing messages
        publisher_ = participant_->create_publisher(eprosima::fastdds::dds::PUBLISHER_QOS_DEFAULT, nullptr);
        if (!publisher_) {
            throw std::runtime_error("DDS Exception: Failed to create Publisher.");
        }
        std::cout << "Create Publisher!!!!!" << std::endl;

        // Step 5: Create DataWriter
        // DataWriter is responsible for publishing messages to the topic
        eprosima::fastdds::dds::DataWriterQos writer_qos = eprosima::fastdds::dds::DATAWRITER_QOS_DEFAULT;
        writer_qos.reliability().kind = eprosima::fastdds::dds::RELIABLE_RELIABILITY_QOS;
        writer_ = publisher_->create_datawriter(topic_, writer_qos);
        if (!writer_) {
            throw std::runtime_error("DDS Exception: Failed to create DataWriter.");
        }
        std::cout << "Create Writer!!!!! (RELIABLE QoS)" << std::endl;

        // Step 6: Create Subscriber
        // Subscriber manages DataReaders for incoming messages
        subscriber_ = participant_->create_subscriber(eprosima::fastdds::dds::SUBSCRIBER_QOS_DEFAULT, nullptr);
        if (!subscriber_) {
            throw std::runtime_error("DDS Exception: Failed to create Subscriber.");
        }
        std::cout << "Create Subscriber!!!!!" << std::endl;

        // Step 7: Create DataReader
        // DataReader is responsible for receiving messages from the topic
        eprosima::fastdds::dds::DataReaderQos reader_qos = eprosima::fastdds::dds::DATAREADER_QOS_DEFAULT;
        reader_qos.reliability().kind = eprosima::fastdds::dds::RELIABLE_RELIABILITY_QOS;
        reader_ = subscriber_->create_datareader(topic_, reader_qos, listener_);
        if (!reader_) {
            throw std::runtime_error("DDS Exception: Failed to create DataReader.");
        }
        std::cout << "Create Reader!!!!! (RELIABLE QoS)" << std::endl;

        std::cout << "OtamDdsImpl: All DDS entities created successfully." << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "OtamDdsImpl: Exception during DDS entity creation: " << e.what() << std::endl;
        throw;
    }
}

template<typename MessageType>
void OtamDdsImpl<MessageType>::SetQos(uint32_t qos) {
    std::cout << "OtamDdsImpl: Setting QoS to " << qos << std::endl;

    try {
        // Prepare QoS configuration
        eprosima::fastdds::dds::ReliabilityQosPolicyKind reliability_kind;
        if (qos == 1) {
            reliability_kind = eprosima::fastdds::dds::RELIABLE_RELIABILITY_QOS;
        } else {
            reliability_kind = eprosima::fastdds::dds::BEST_EFFORT_RELIABILITY_QOS;
        }

        // Recreate DataWriter with new QoS
        if (writer_ && publisher_ && topic_) {
            std::cout << "OtamDdsImpl: Recreating DataWriter with new QoS" << std::endl;
            std::cout << "OtamDdsImpl: writer_=" << writer_ << ", publisher_=" << publisher_ << ", topic_=" << topic_ << std::endl;

            // Delete existing DataWriter
            if (publisher_->delete_datawriter(writer_) != eprosima::fastdds::dds::RETCODE_OK) {
                throw std::runtime_error("DDS Exception: Failed to delete DataWriter.");
            }
            writer_ = nullptr;
            std::cout << "OtamDdsImpl: DataWriter deleted successfully" << std::endl;

            // Create new DataWriter with updated QoS
            eprosima::fastdds::dds::DataWriterQos writer_qos = eprosima::fastdds::dds::DATAWRITER_QOS_DEFAULT;
            writer_qos.reliability().kind = reliability_kind;

            writer_ = publisher_->create_datawriter(topic_, writer_qos);
            if (!writer_) {
                throw std::runtime_error("DDS Exception: Failed to recreate DataWriter.");
            }

            std::cout << "OtamDdsImpl: DataWriter recreated successfully. writer_=" << writer_
                      << ", Reliability: "
                      << (reliability_kind == eprosima::fastdds::dds::RELIABLE_RELIABILITY_QOS ? "RELIABLE" : "BEST_EFFORT")
                      << std::endl;
        } else {
            std::cout << "OtamDdsImpl: Skipping DataWriter recreation (writer_=" << writer_
                      << ", publisher_=" << publisher_ << ", topic_=" << topic_ << ")" << std::endl;
        }

        // Recreate DataReader with new QoS
        if (reader_ && subscriber_ && topic_) {
            std::cout << "OtamDdsImpl: Recreating DataReader with new QoS" << std::endl;

            // Delete existing DataReader
            if (subscriber_->delete_datareader(reader_) != eprosima::fastdds::dds::RETCODE_OK) {
                throw std::runtime_error("DDS Exception: Failed to delete DataReader.");
            }
            reader_ = nullptr;
            std::cout << "OtamDdsImpl: DataReader deleted successfully" << std::endl;

            // Create new DataReader with updated QoS
            eprosima::fastdds::dds::DataReaderQos reader_qos = eprosima::fastdds::dds::DATAREADER_QOS_DEFAULT;
            reader_qos.reliability().kind = reliability_kind;

            reader_ = subscriber_->create_datareader(topic_, reader_qos, listener_);
            if (!reader_) {
                throw std::runtime_error("DDS Exception: Failed to recreate DataReader.");
            }

            std::cout << "OtamDdsImpl: DataReader recreated successfully. reader_=" << reader_
                      << ", Reliability: "
                      << (reliability_kind == eprosima::fastdds::dds::RELIABLE_RELIABILITY_QOS ? "RELIABLE" : "BEST_EFFORT")
                      << std::endl;
        } else {
            std::cout << "OtamDdsImpl: Skipping DataReader recreation (reader_=" << reader_
                      << ", subscriber_=" << subscriber_ << ", topic_=" << topic_ << ")" << std::endl;
        }

        std::cout << "OtamDdsImpl: SetQos completed successfully" << std::endl;
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

    if (!writer_) {
        throw std::runtime_error("DDS Exception: DataWriter not created. Cannot write message.");
    }

    if (!message) {
        throw std::runtime_error("DDS Exception: Message is null. Cannot write message.");
    }

    try {
        std::cout << "OtamDdsImpl: Calling writer_->write()..." << std::endl;
        // Cast to MessageType for type safety
        auto typed_message = std::static_pointer_cast<MessageType>(message);

        // Publish the message using FastDDS DataWriter
        // The message is serialized automatically based on the registered TypeSupport
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

    if (!reader_) {
        throw std::runtime_error("DDS Exception: DataReader not created. Cannot read message.");
    }

    try {
        eprosima::fastdds::dds::SampleInfo info;

        // Cast to MessageType for type safety
        auto typed_message = std::static_pointer_cast<MessageType>(message);

        // Attempt to read the next available sample
        // Note: For event-driven systems, prefer using DataReaderListener callbacks
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
