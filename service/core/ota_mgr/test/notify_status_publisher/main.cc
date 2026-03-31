/**
 * @file main.cc
 * @brief DDS Publisher for TC-3 testing
 *
 * Simulates ota_manager publishing NotifyAppUpdateStatus to ota_client.
 * Reads "app_name status" pairs from stdin and publishes them immediately via DDS.
 * Terminates on empty line input.
 *
 * Topic: /mmf/otamanager/otaclient_service/NotifyAppUpdateStatus
 * Type:  OtaManager::NotifyAppUpdateStatus
 *
 * Usage:
 *   $ ./notify_status_publisher
 *   adsw_monitor downloading
 *   adsw_monitor installing
 *   adsw_monitor complete
 *   (empty line → quit)
 */

#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <chrono>

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/topic/Topic.hpp>
#include <fastdds/dds/core/policy/QosPolicies.hpp>

#include "OtamClientServiceMsg.hpp"
#include "OtamClientServiceMsgPubSubTypes.hpp"

using namespace eprosima::fastdds::dds;

static constexpr const char* TOPIC_NAME =
    "/mmf/otamanager/otaclient_service/NotifyAppUpdateStatus";

static constexpr int32_t DEFAULT_CLIENT_ID = 1;

int main()
{
    /* ── 1. DomainParticipant ── */
    DomainParticipant* participant =
        DomainParticipantFactory::get_instance()->create_participant(
            0, PARTICIPANT_QOS_DEFAULT);
    if (!participant) {
        std::cerr << "[ERROR] Failed to create DomainParticipant\n";
        return 1;
    }

    /* ── 2. Register TypeSupport ── */
    TypeSupport type_support(
        new OtaManager::NotifyAppUpdateStatusPubSubType());
    if (RETCODE_OK != type_support.register_type(participant)) {
        std::cerr << "[ERROR] Failed to register type\n";
        DomainParticipantFactory::get_instance()->delete_participant(participant);
        return 1;
    }

    /* ── 3. Create Topic ── */
    Topic* topic = participant->create_topic(
        TOPIC_NAME,
        type_support.get_type_name(),
        TOPIC_QOS_DEFAULT);
    if (!topic) {
        std::cerr << "[ERROR] Failed to create topic: " << TOPIC_NAME << "\n";
        DomainParticipantFactory::get_instance()->delete_participant(participant);
        return 1;
    }

    /* ── 4. Publisher / DataWriter ── */
    Publisher* publisher = participant->create_publisher(PUBLISHER_QOS_DEFAULT);
    if (!publisher) {
        std::cerr << "[ERROR] Failed to create publisher\n";
        DomainParticipantFactory::get_instance()->delete_participant(participant);
        return 1;
    }

    DataWriterQos writer_qos = DATAWRITER_QOS_DEFAULT;
    /* TRANSIENT_LOCAL: messages are retained even if subscriber connects later */
    writer_qos.durability().kind = TRANSIENT_LOCAL_DURABILITY_QOS;
    writer_qos.reliability().kind = RELIABLE_RELIABILITY_QOS;

    DataWriter* writer = publisher->create_datawriter(topic, writer_qos);
    if (!writer) {
        std::cerr << "[ERROR] Failed to create DataWriter\n";
        DomainParticipantFactory::get_instance()->delete_participant(participant);
        return 1;
    }

    std::cout << "[INFO] NotifyAppUpdateStatus publisher ready.\n";
    std::cout << "[INFO] Topic: " << TOPIC_NAME << "\n";
    std::cout << "[INFO] Input format: \"app_name status\"  (empty line to quit)\n";
    std::cout << std::flush;

    /* ── 5. Input Loop ── */
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty()) {
            break;
        }

        std::istringstream iss(line);
        std::string app_name, status;
        if (!(iss >> app_name >> status)) {
            std::cerr << "[WARN] Invalid input (expected: app_name status): "
                      << line << "\n";
            continue;
        }

        OtaManager::NotifyAppUpdateStatus msg;
        msg.client_id(DEFAULT_CLIENT_ID);
        msg.app_name(app_name);
        msg.status(status);

        if (RETCODE_OK == writer->write(&msg)) {
            std::cout << "[PUB] client_id=" << msg.client_id()
                      << " app_name=" << msg.app_name()
                      << " status=" << msg.status() << "\n"
                      << std::flush;
        } else {
            std::cerr << "[ERROR] write() failed\n";
        }
    }

    /* ── 6. Cleanup ── */
    publisher->delete_datawriter(writer);
    participant->delete_publisher(publisher);
    participant->delete_topic(topic);
    DomainParticipantFactory::get_instance()->delete_participant(participant);

    std::cout << "[INFO] Publisher stopped.\n";
    return 0;
}
