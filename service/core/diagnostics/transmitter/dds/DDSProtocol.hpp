#ifndef DIAGNOSTICS_TRASMITTER_DDS_PROTOCOL_HPP_
#define DIAGNOSTICS_TRASMITTER_DDS_PROTOCOL_HPP_

#include <iostream>
#include <unordered_map>
#include <mutex>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantListener.hpp>
#include <fastdds/dds/xtypes/dynamic_types/DynamicDataFactory.hpp>
#include <fastdds/dds/xtypes/dynamic_types/DynamicType.hpp>

#include <fastdds/dds/log/Log.hpp>

#include "transmitter/MessageProtocol.hpp"
#include "transmitter/dds/DDSDynamicIDLParser.hpp"
#include "diagnostic/VariantData.hpp"

using namespace eprosima::fastdds::dds;

namespace diagnostic_manager {

class TopicInformation {
  public:
    TopicInformation() : topic(nullptr), reader(nullptr), writer(nullptr) {}

    std::string topic_name;
    std::string topic_type;

    xtypes::TypeInformation remote_type_information;
    xtypes::TypeObject remote_type_object;
    DynamicType::_ref_type remote_type;
    // xtypes::TypeIdentifier type_id_complete;
    // xtypes::TypeIdentifier type_id_minimal;

    Topic* topic;
    DataReader* reader;
    DataWriter* writer;

    std::shared_ptr<TypeSupport> type_support;
};

class DDSProtocol : public MessageProtocol, DomainParticipantListener {
  public:
    DDSProtocol();
    virtual ~DDSProtocol();

    bool Initialize();
    void Finalize();

    MessageProtocolType GetProtocolType();

    bool PublishMessage(std::shared_ptr<Message> message);

    bool SubscribeMessage(const std::string& name);
    bool UnsubscribeMessage(const std::string& name);

  private:
    bool SerializeMessage(DynamicData::_ref_type dds_data, std::shared_ptr<VariantData> variant_data);
    bool DeserializeMessage(DynamicData::_ref_type dds_data, std::shared_ptr<VariantData> variant_data);

    void load_dds_dynamic_idl(const std::string& path);

    // DomainParticipantListener method
    void on_data_available(DataReader* reader) override;
    void on_subscription_matched(DataReader* reader, const SubscriptionMatchedStatus& info) override;
    void on_data_writer_discovery(DomainParticipant* participant,
                                  eprosima::fastdds::rtps::WriterDiscoveryStatus reason,
                                  const eprosima::fastdds::dds::PublicationBuiltinTopicData& info,
                                  bool& should_be_ignored) override;

    bool register_type_object(std::shared_ptr<TopicInformation> topic_information);
    bool register_type_object_from_discovery(std::shared_ptr<TopicInformation> topic_information);
    bool register_type_object_from_dynamic_idl(std::shared_ptr<TopicInformation> topic_information);
    bool create_datareader(std::shared_ptr<TopicInformation> topic_information);
    bool delete_datareader(std::shared_ptr<TopicInformation> topic_information);
    bool create_datawriter(std::shared_ptr<TopicInformation> topic_information);
    bool delete_datawriter(std::shared_ptr<TopicInformation> topic_information);

    std::shared_ptr<TopicInformation> create_topic_information_from_dynamic_idl(const std::string& name, const std::string& type);

    bool read_values_from_dynamic_data(DynamicData::_ref_type dds_data, DynamicTypeMember::_ref_type member, TypeKind elem_type,
                                       std::shared_ptr<VariantData> variant_data);

    DomainParticipant* participant_;
    Subscriber* subscriber_;
    Publisher* publisher_;

    std::mutex topic_handling_mutex_;
    std::unordered_map<std::string, std::shared_ptr<TopicInformation>> discovered_topic_;

    std::set<std::string> subscribed_topic_;

    std::unique_ptr<DDSDynamicIDLParser> dds_dynamic_idl_parser_;
    std::unordered_map<std::string, std::shared_ptr<DynamicIDLTypeSupport>> parsed_dynamic_type_supports_;
};

} // namespace diagnostic_manager

#endif
