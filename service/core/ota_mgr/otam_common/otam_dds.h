#ifndef DDS_ABSTRACTION_H_
#define DDS_ABSTRACTION_H_

#include <string>
#include <memory>
#include <cstdint> // For uint32_t

/**
 * @file otam_dds.h
 * @brief Defines the abstract DdsAbstraction class for DDS communication.
 */

/**
 * @brief Abstract base class for DDS communication functionalities.
 * This class provides a common interface for creating DDS instances,
 * setting QoS, writing, and reading DDS messages.
 */
class OtamDds {
 public:
  /**
   * @brief Virtual destructor for DdsAbstraction.
   * Ensures proper cleanup of derived DDS implementation classes.
   */
  virtual ~OtamDds() = default;

  /**
   * @brief Creates necessary DDS communication instances for a given topic.
   * This includes Participant, Subscriber, and Publisher. [cite: 39]
   * @param topic The DDS topic string to create instances for. [cite: 39]
   * @exception DDS Exception
   * @cycle Once [cite: 39]
   */
  virtual void Create(const std::string& topic, std::shared_ptr<void> type, std::shared_ptr<void> listener) = 0;

  /**
   * @brief Changes the QoS (Quality of Service) settings from default to specified. [cite: 40]
   * @param qos The uint32 value representing the desired QoS settings. [cite: 40]
   * @exception DDS Exception
   * @cycle Once [cite: 40]
   */
  virtual void SetQos(uint32_t qos) = 0;

  /**
   * @brief Sends a DDS message. [cite: 41]
   * The message is passed as a shared pointer to void, requiring dynamic_cast
   * in the derived class to the actual message type.
   * @param message A shared pointer to the DDS message to be sent. [cite: 41]
   * @exception DDS Exception [cite: 41]
   * @cycle On Demand [cite: 41]
   */
  virtual void Write(std::shared_ptr<void> message) = 0;

  /**
   * @brief Receives a DDS message. [cite: 42]
   * The received message data is populated into the provided shared pointer.
   * @param message A shared pointer to void where the received DDS message data will be stored. [cite: 42]
   * @exception DDS Exception [cite: 42]
   * @cycle Event Driven [cite: 42]
   */
  virtual void Read(std::shared_ptr<void> message) = 0; // Or potentially a callback mechanism for event-driven reads
};

#endif  // DDS_ABSTRACTION_H_