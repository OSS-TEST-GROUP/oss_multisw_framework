  #include "otam_dds_impl.h"
  #include <iostream>
  #include <fastdds/dds/domain/DomainParticipantFactory.hpp>
  #include <fastdds/dds/publisher/qos/DataWriterQos.hpp>
  #include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
  
  /**
   * @file otam_dds_impl.cc
   * @brief Implementation of OtamDdsImpl class
   */

  OtamDdsImpl::OtamDdsImpl()
      : participant_(nullptr),
        publisher_(nullptr),
        subscriber_(nullptr),
        topic_(nullptr),
        writer_(nullptr),
        reader_(nullptr),
        listener_(nullptr) {
      // Constructor: All members initialized to nullptr
      // DDS entities are created in the Create method
  }

  OtamDdsImpl::~OtamDdsImpl() {
      std::cout << "OtamDdsImpl: Destructor called. Cleaning up DDS entities." << std::endl;

      // Clean up DDS entities in reverse order of creation to avoid dependencies
      if (reader_ && subscriber_) {
          subscriber_->delete_datareader(reader_);
          reader_ = nullptr;
      }
      std::cout << "Deleted reader_!!!!!" << std::endl;

      if (writer_ && publisher_) {
          publisher_->delete_datawriter(writer_);
          writer_ = nullptr;
      }
      std::cout << "Deleted writer_!!!!!" << std::endl;

      if (topic_ && participant_) {
          participant_->delete_topic(topic_);
          topic_ = nullptr;
      }
      std::cout << "Deleted topic_!!!!!" << std::endl;

      if (publisher_ && participant_) {
          participant_->delete_publisher(publisher_);
          publisher_ = nullptr;
      }
      std::cout << "Deleted publisher_!!!!!" << std::endl;

      if (subscriber_ && participant_) {
          participant_->delete_subscriber(subscriber_);
          subscriber_ = nullptr;
      }
      std::cout << "Deleted subscriber_!!!!!" << std::endl;

      if (participant_) {
          auto factory = eprosima::fastdds::dds::DomainParticipantFactory::get_instance();
          if (factory) {
              eprosima::fastdds::dds::ReturnCode_t ret = factory->delete_participant(participant_);
              if (ret != eprosima::fastdds::dds::RETCODE_OK) {
                  std::cerr << "Warning: Failed to delete DomainParticipant" << std::endl;
              }
          }
          participant_ = nullptr;
      }
      std::cout << "Deleted participant_!!!!!" << std::endl;
  }

  void OtamDdsImpl::Create(const std::string& topic, std::shared_ptr<void> type, std::shared_ptr<void> listener) {
      std::cout << "OtamDdsImpl: Creating DDS entities for topic: " << topic << std::endl;
      topic_name_ = topic;
      
      std::cout << "TypeSupport!!!!!" << type.get() << std::endl;
      eprosima::fastdds::dds::TypeSupport type_support(static_cast<eprosima::fastdds::dds::TopicDataType*>(type.get()));

      listener_ = std::static_pointer_cast<eprosima::fastdds::dds::DataReaderListener>(listener).get();
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
          writer_ = publisher_->create_datawriter(topic_, writer_qos);
          if (!writer_) {
              throw std::runtime_error("DDS Exception: Failed to create DataWriter.");
          }
          std::cout << "Create Writer!!!!!" << std::endl;

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
          reader_ = subscriber_->create_datareader(topic_, reader_qos, listener_);
          if (!reader_) {
              throw std::runtime_error("DDS Exception: Failed to create DataReader.");
          }
          std::cout << "Create Reader!!!!!" << std::endl;

          std::cout << "OtamDdsImpl: All DDS entities created successfully." << std::endl;
      }
      catch (const std::exception& e) {
          std::cerr << "OtamDdsImpl: Exception during DDS entity creation: " << e.what() << std::endl;
          throw;
      }
  }

  void OtamDdsImpl::SetQos(uint32_t qos) {
      std::cout << "OtamDdsImpl: Setting QoS to " << qos << std::endl;

      try {
          // Configure DataWriter QoS
          if (writer_) {
              eprosima::fastdds::dds::DataWriterQos writer_qos;
              writer_->get_qos(writer_qos);

              // Set reliability policy based on qos parameter
              // 1 = Reliable (guaranteed delivery), 0 = Best Effort (faster but may lose messages)
              if (qos == 1) {
                  writer_qos.reliability().kind = eprosima::fastdds::dds::RELIABLE_RELIABILITY_QOS;
              } else {
                  writer_qos.reliability().kind = eprosima::fastdds::dds::BEST_EFFORT_RELIABILITY_QOS;
              }

              if (writer_->set_qos(writer_qos) != eprosima::fastdds::dds::RETCODE_OK) {
                  throw std::runtime_error("DDS Exception: Failed to set DataWriter QoS.");
              }

              std::cout << "OtamDdsImpl: DataWriter QoS updated. Reliability: "
                        << (writer_qos.reliability().kind == eprosima::fastdds::dds::RELIABLE_RELIABILITY_QOS ? "RELIABLE" : "BEST_EFFORT")
                        << std::endl;
          }

          // Configure DataReader QoS to match DataWriter
          if (reader_) {
              eprosima::fastdds::dds::DataReaderQos reader_qos;
              reader_->get_qos(reader_qos);

              if (qos == 1) {
                  reader_qos.reliability().kind = eprosima::fastdds::dds::RELIABLE_RELIABILITY_QOS;
              } else {
                  reader_qos.reliability().kind = eprosima::fastdds::dds::BEST_EFFORT_RELIABILITY_QOS;
              }

              if (reader_->set_qos(reader_qos) != eprosima::fastdds::dds::RETCODE_OK) {
                  throw std::runtime_error("DDS Exception: Failed to set DataReader QoS.");
              }

              std::cout << "OtamDdsImpl: DataReader QoS updated. Reliability: "
                        << (reader_qos.reliability().kind == eprosima::fastdds::dds::RELIABLE_RELIABILITY_QOS ? "RELIABLE" : "BEST_EFFORT")
                        << std::endl;
          }
      }
      catch (const std::exception& e) {
          std::cerr << "OtamDdsImpl: Exception during QoS configuration: " << e.what() << std::endl;
          throw;
      }
  }

  void OtamDdsImpl::Write(std::shared_ptr<void> message) {
      std::cout << "OtamDdsImpl: Writing DDS message." << std::endl;

      if (!writer_) {
          throw std::runtime_error("DDS Exception: DataWriter not created. Cannot write message.");
      }

      try {
          // Publish the message using FastDDS DataWriter
          // The message is serialized automatically based on the registered TypeSupport
          if (writer_->write(message.get()) != eprosima::fastdds::dds::RETCODE_OK) {
              throw std::runtime_error("DDS Exception: Failed to write DDS message.");
          }
          std::cout << "OtamDdsImpl: Successfully wrote message" << std::endl;
      }
      catch (const std::exception& e) {
          std::cerr << "OtamDdsImpl: Exception during message writing: " << e.what() << std::endl;
          throw;
      }
  }

  void OtamDdsImpl::Read(std::shared_ptr<void> message) {
      std::cout << "OtamDdsImpl: Reading DDS message (polling mode)." << std::endl;

      if (!reader_) {
          throw std::runtime_error("DDS Exception: DataReader not created. Cannot read message.");
      }

      try {
          eprosima::fastdds::dds::SampleInfo info;

          // Attempt to read the next available sample
          // Note: For event-driven systems, prefer using DataReaderListener callbacks
          if (reader_->take_next_sample(message.get(), &info) == eprosima::fastdds::dds::RETCODE_OK) {
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
