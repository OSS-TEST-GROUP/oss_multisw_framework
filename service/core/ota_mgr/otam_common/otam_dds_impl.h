  #ifndef OTAM_DDS_IMPL_H_
  #define OTAM_DDS_IMPL_H_

  #include "dds_abstraction.h"
  #include <fastdds/dds/domain/DomainParticipant.hpp>
  #include <fastdds/dds/publisher/Publisher.hpp>
  #include <fastdds/dds/subscriber/Subscriber.hpp>
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
   * @see DdsAbstraction
   */
  class OtamDdsImpl : public DdsAbstraction {
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
  };

  #endif // OTAM_DDS_IMPL_H_
