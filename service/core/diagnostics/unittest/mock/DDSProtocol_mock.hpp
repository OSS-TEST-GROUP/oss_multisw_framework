#ifndef DIAGNOSTICS_UNITTEST_MOCK_DDS_PROTOCOL_MOCK_HPP_
#define DIAGNOSTICS_UNITTEST_MOCK_DDS_PROTOCOL_MOCK_HPP_

#include <gmock/gmock.h>
#include "transmitter/dds/DDSProtocol.hpp"

namespace diagnostic_manager {

class DDSProtocolMock {
    MOCK_METHOD0(Initialize, bool());
    MOCK_METHOD0(Finalize, void());
    MOCK_METHOD0(GetProtocolType, MessageProtocolType());
    MOCK_METHOD1(PublishMessage, bool(std::shared_ptr<Message>));
    MOCK_METHOD1(SubscribeMessage, bool(const std::string& name));
    MOCK_METHOD1(UnsubscribeMessage, bool(const std::string& name));

};

DDSProtocolMock* Mock_DDSProtocol;

DDSProtocol::DDSProtocol() {}
DDSProtocol::~DDSProtocol() {}

bool DDSProtocol::Initialize() {
    return Mock_DDSProtocol->Initialize();
}

void DDSProtocol::Finalize() {
    return Mock_DDSProtocol->Finalize();
}

MessageProtocolType DDSProtocol::GetProtocolType() {
    return Mock_DDSProtocol->GetProtocolType();
}

bool DDSProtocol::PublishMessage(std::shared_ptr<Message> message) {
    return Mock_DDSProtocol->PublishMessage(message);
}

bool DDSProtocol::SubscribeMessage(const std::string& name) {
    return Mock_DDSProtocol->SubscribeMessage(name);
}

bool DDSProtocol::UnsubscribeMessage(const std::string& name) {
    return Mock_DDSProtocol->UnsubscribeMessage(name);
}

bool DDSProtocol::SerializeMessage(DynamicData::_ref_type dds_data, std::shared_ptr<VariantData> variant_data) {
    return true;
}

bool DDSProtocol::DeserializeMessage(DynamicData::_ref_type dds_data, std::shared_ptr<VariantData> variant_data) {
    return true;
}

void DDSProtocol::load_dds_dynamic_idl(const std::string& path) {}

// FastDDS override method
void DDSProtocol::on_data_available(DataReader* reader) {}

void DDSProtocol::on_subscription_matched(DataReader* reader, const SubscriptionMatchedStatus& info) {}

void DDSProtocol::on_data_writer_discovery(DomainParticipant* participant,
                                           eprosima::fastdds::rtps::WriterDiscoveryStatus reason,
                                           const eprosima::fastdds::dds::PublicationBuiltinTopicData& info,
                                           bool& should_be_ignored) {}

bool DDSProtocol::register_type_object(std::shared_ptr<TopicInformation> topic_information) {
    return false;
}

bool DDSProtocol::register_type_object_from_discovery(std::shared_ptr<TopicInformation> topic_information) {
    return true;
}

bool DDSProtocol::register_type_object_from_dynamic_idl(std::shared_ptr<TopicInformation> topic_information) {
    return true;
}

bool DDSProtocol::create_datareader(std::shared_ptr<TopicInformation> topic_information) {
    return true;
}

bool DDSProtocol::delete_datareader(std::shared_ptr<TopicInformation> topic_information) {
    return true;
}

bool DDSProtocol::read_values_from_dynamic_data(DynamicData::_ref_type dds_data, DynamicTypeMember::_ref_type member, TypeKind elem_type,
                                                std::shared_ptr<VariantData> variant_data) {
    return true;
}

} // namespace diagnostic_manager

#endif 