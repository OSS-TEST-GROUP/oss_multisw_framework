#include "DDSProtocol.hpp"

#include <fastdds/dds/xtypes/dynamic_types/DynamicTypeBuilderFactory.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>

namespace diagnostic_manager {

DDSProtocol::DDSProtocol() : participant_(nullptr), subscriber_(nullptr) {
    dds_dynamic_idl_parser_ = std::make_unique<DDSDynamicIDLParser>();

    // Enable all logging categories
    eprosima::fastdds::dds::Log::SetVerbosity(eprosima::fastdds::dds::Log::Info);
    eprosima::fastdds::dds::Log::SetCategoryFilter(std::regex(""));
}
DDSProtocol::~DDSProtocol() {}

bool DDSProtocol::Initialize() {
    auto factory = DomainParticipantFactory::get_instance();

    StatusMask participant_mask = StatusMask::data_available();
    participant_mask << eprosima::fastdds::dds::StatusMask::subscription_matched();

    participant_ = factory->create_participant_with_default_profile(this, participant_mask);

    load_dds_dynamic_idl("idl");

    // Create the subscriber
    SubscriberQos sub_qos = SUBSCRIBER_QOS_DEFAULT;
    participant_->get_default_subscriber_qos(sub_qos);
    subscriber_ = participant_->create_subscriber(sub_qos);
    if (nullptr == subscriber_)
    {
        return false;
    }

    return true;
}

void DDSProtocol::Finalize() {
    if (participant_) {
        participant_->delete_contained_entities();

        auto factory = DomainParticipantFactory::get_instance();
        if (factory->delete_participant(participant_) != RETCODE_OK) {
            std::cerr << "Failed to delete the participant." << std::endl;
        }
    }
}

MessageProtocolType DDSProtocol::GetProtocolType() {
    return MessageProtocolType::DDS;
}

bool DDSProtocol::PublishMessage(std::shared_ptr<Message> message) {
    return true;
}

bool DDSProtocol::SubscribeMessage(const std::string& name) {
    std::lock_guard<std::mutex> lock(subsription_handling_mutex_);

    subscribed_topic_.insert(name);

    if (discovered_topic_.count(name) > 0) {
        auto topic_info = discovered_topic_[name];
        if (!register_type_object(topic_info)) {
            std::cerr << "Failed to register a type object. topic=" << topic_info->topic_name << std::endl;
        }
        if (!create_datareader(topic_info)) {
            std::cerr << "Failed to create a datareader. topic=" << topic_info->topic_name << std::endl;
        }
    }
    return true;
}

bool DDSProtocol::UnsubscribeMessage(const std::string& name) {
    std::lock_guard<std::mutex> lock(subsription_handling_mutex_);

    if (subscribed_topic_.count(name) == 0) {
        return false;
    }
    subscribed_topic_.erase(name);

    if (discovered_topic_.count(name) > 0) {
        auto topic_info = discovered_topic_[name];
        if (topic_info->reader) {
            if(!delete_datareader(topic_info)) {
                std::cerr << "Failed to delete a datareader. topic=" << name << std::endl;
            }
        }
    }

    return true;
}

bool DDSProtocol::SerializeMessage(DynamicData::_ref_type dds_data, std::shared_ptr<VariantData> variant_data) {
    TypeDescriptor::_ref_type type_descriptor {traits<TypeDescriptor>::make_shared()};
    type_descriptor->kind(TK_STRUCTURE);
    type_descriptor->name(variant_data->GetName());

    std::vector<std::string> member_list = variant_data->GetKeyList();
    std::vector<std::type_index> type_list;
    for (auto& iter : member_list) {
        type_list.push_back(variant_data->GetValueType(iter));
    }

    DynamicTypeBuilder::_ref_type struct_builder {DynamicTypeBuilderFactory::get_instance()->create_type(type_descriptor)};

    MemberDescriptor::_ref_type member_descriptor;
    for (uint64_t idx = 0; idx < member_list.size(); idx++) {
        member_descriptor = traits<MemberDescriptor>::make_shared();
        member_descriptor->name(member_list[idx]);

        auto& type = type_list[idx];
        if (type == std::type_index(typeid(char))) { member_descriptor->type(DynamicTypeBuilderFactory::get_instance()->get_primitive_type(TK_CHAR8)); }
        else if (type == std::type_index(typeid(wchar_t))) { member_descriptor->type(DynamicTypeBuilderFactory::get_instance()->get_primitive_type(TK_CHAR16)); }
        else if (type == std::type_index(typeid(int8_t))) { member_descriptor->type(DynamicTypeBuilderFactory::get_instance()->get_primitive_type(TK_INT8)); }
        else if (type == std::type_index(typeid(int16_t))) { member_descriptor->type(DynamicTypeBuilderFactory::get_instance()->get_primitive_type(TK_INT16)); }
        else if (type == std::type_index(typeid(int32_t))) { member_descriptor->type(DynamicTypeBuilderFactory::get_instance()->get_primitive_type(TK_INT32)); }
        else if (type == std::type_index(typeid(int64_t))) { member_descriptor->type(DynamicTypeBuilderFactory::get_instance()->get_primitive_type(TK_INT64)); }
        else if (type == std::type_index(typeid(uint8_t))) { member_descriptor->type(DynamicTypeBuilderFactory::get_instance()->get_primitive_type(TK_UINT8)); }
        else if (type == std::type_index(typeid(uint16_t))) { member_descriptor->type(DynamicTypeBuilderFactory::get_instance()->get_primitive_type(TK_UINT16)); }
        else if (type == std::type_index(typeid(uint32_t))) { member_descriptor->type(DynamicTypeBuilderFactory::get_instance()->get_primitive_type(TK_UINT32)); }
        else if (type == std::type_index(typeid(uint64_t))) { member_descriptor->type(DynamicTypeBuilderFactory::get_instance()->get_primitive_type(TK_UINT64)); }
        else if (type == std::type_index(typeid(float))) { member_descriptor->type(DynamicTypeBuilderFactory::get_instance()->get_primitive_type(TK_FLOAT32)); }
        else if (type == std::type_index(typeid(double))) { member_descriptor->type(DynamicTypeBuilderFactory::get_instance()->get_primitive_type(TK_FLOAT64)); }
        else if (type == std::type_index(typeid(std::string))) { member_descriptor->type(DynamicTypeBuilderFactory::get_instance()->
                                                                    create_string_type(static_cast<uint32_t>(LENGTH_UNLIMITED))->build()); }
        else if (type == std::type_index(typeid(std::wstring))) { member_descriptor->type(DynamicTypeBuilderFactory::get_instance()->
                                                                    create_wstring_type(static_cast<uint32_t>(LENGTH_UNLIMITED))->build()); }
        // else if (type == std::type_index(typeid(VariantData))) {}
        else if (type == std::type_index(typeid(std::vector<char>))) { member_descriptor->type(DynamicTypeBuilderFactory::get_instance()->create_sequence_type(
                DynamicTypeBuilderFactory::get_instance()->get_primitive_type(TK_CHAR8), variant_data->GetValue<std::vector<char>>(member_list[idx]).size())->build()); }
        else if (type == std::type_index(typeid(std::vector<wchar_t>))) { member_descriptor->type(DynamicTypeBuilderFactory::get_instance()->create_sequence_type(
                DynamicTypeBuilderFactory::get_instance()->get_primitive_type(TK_CHAR16), variant_data->GetValue<std::vector<wchar_t>>(member_list[idx]).size())->build()); }
        else if (type == std::type_index(typeid(std::vector<int8_t>))) { member_descriptor->type(DynamicTypeBuilderFactory::get_instance()->create_sequence_type(
                DynamicTypeBuilderFactory::get_instance()->get_primitive_type(TK_INT8), variant_data->GetValue<std::vector<int8_t>>(member_list[idx]).size())->build()); }
        else if (type == std::type_index(typeid(std::vector<int16_t>))) { member_descriptor->type(DynamicTypeBuilderFactory::get_instance()->create_sequence_type(
                DynamicTypeBuilderFactory::get_instance()->get_primitive_type(TK_INT16), variant_data->GetValue<std::vector<int16_t>>(member_list[idx]).size())->build()); }
        else if (type == std::type_index(typeid(std::vector<int32_t>))) { member_descriptor->type(DynamicTypeBuilderFactory::get_instance()->create_sequence_type(
                DynamicTypeBuilderFactory::get_instance()->get_primitive_type(TK_INT32), variant_data->GetValue<std::vector<int32_t>>(member_list[idx]).size())->build()); }
        else if (type == std::type_index(typeid(std::vector<int64_t>))) { member_descriptor->type(DynamicTypeBuilderFactory::get_instance()->create_sequence_type(
                DynamicTypeBuilderFactory::get_instance()->get_primitive_type(TK_INT64), variant_data->GetValue<std::vector<int64_t>>(member_list[idx]).size())->build()); }
        else if (type == std::type_index(typeid(std::vector<uint8_t>))) { member_descriptor->type(DynamicTypeBuilderFactory::get_instance()->create_sequence_type(
                DynamicTypeBuilderFactory::get_instance()->get_primitive_type(TK_UINT8), variant_data->GetValue<std::vector<uint8_t>>(member_list[idx]).size())->build()); }
        else if (type == std::type_index(typeid(std::vector<uint16_t>))) { member_descriptor->type(DynamicTypeBuilderFactory::get_instance()->create_sequence_type(
                DynamicTypeBuilderFactory::get_instance()->get_primitive_type(TK_UINT16), variant_data->GetValue<std::vector<uint16_t>>(member_list[idx]).size())->build()); }
        else if (type == std::type_index(typeid(std::vector<uint32_t>))) { member_descriptor->type(DynamicTypeBuilderFactory::get_instance()->create_sequence_type(
                DynamicTypeBuilderFactory::get_instance()->get_primitive_type(TK_UINT32), variant_data->GetValue<std::vector<uint32_t>>(member_list[idx]).size())->build()); }
        else if (type == std::type_index(typeid(std::vector<uint64_t>))) { member_descriptor->type(DynamicTypeBuilderFactory::get_instance()->create_sequence_type(
                DynamicTypeBuilderFactory::get_instance()->get_primitive_type(TK_UINT64), variant_data->GetValue<std::vector<uint64_t>>(member_list[idx]).size())->build()); }
        else if (type == std::type_index(typeid(std::vector<float>))) { member_descriptor->type(DynamicTypeBuilderFactory::get_instance()->create_sequence_type(
                DynamicTypeBuilderFactory::get_instance()->get_primitive_type(TK_FLOAT32), variant_data->GetValue<std::vector<float>>(member_list[idx]).size())->build()); }
        else if (type == std::type_index(typeid(std::vector<double>))) { member_descriptor->type(DynamicTypeBuilderFactory::get_instance()->create_sequence_type(
                DynamicTypeBuilderFactory::get_instance()->get_primitive_type(TK_FLOAT64), variant_data->GetValue<std::vector<double>>(member_list[idx]).size())->build()); }
        else if (type == std::type_index(typeid(std::vector<bool>))) { member_descriptor->type(DynamicTypeBuilderFactory::get_instance()->create_sequence_type(
                DynamicTypeBuilderFactory::get_instance()->get_primitive_type(TK_BOOLEAN), variant_data->GetValue<std::vector<bool>>(member_list[idx]).size())->build()); }

        else if (type == std::type_index(typeid(std::vector<std::string>))) { member_descriptor->type(DynamicTypeBuilderFactory::get_instance()->create_sequence_type(
                DynamicTypeBuilderFactory::get_instance()->create_string_type(static_cast<uint32_t>(LENGTH_UNLIMITED))->build(), variant_data->GetValue<std::vector<std::string>>(member_list[idx]).size())->build()); }
        else if (type == std::type_index(typeid(std::vector<std::wstring>))) { member_descriptor->type(DynamicTypeBuilderFactory::get_instance()->create_sequence_type(
                DynamicTypeBuilderFactory::get_instance()->create_wstring_type(static_cast<uint32_t>(LENGTH_UNLIMITED))->build(), variant_data->GetValue<std::vector<std::wstring>>(member_list[idx]).size())->build()); }
        // else if (type == std::type_index(typeid(std::vector<VariantData>))) { member_descriptor->member_descriptor->type(DynamicTypeBuilderFactory::get_instance()->get_primitive_type()); }
        else {
            std::cerr << "Not supported type. name=" << member_list[idx] << std::endl;
        }
        struct_builder->add_member(member_descriptor);
    }

    DynamicType::_ref_type struct_type {struct_builder->build()};
    DynamicData::_ref_type data {DynamicDataFactory::get_instance()->create_data(struct_type)};

    for (uint64_t idx = 0; idx < member_list.size(); idx++) {
                auto& type = type_list[idx];
        std::string& member_name = member_list[idx];
        if (type == std::type_index(typeid(char))) { data->set_char8_value(data->get_member_id_by_name(member_name), variant_data->GetValue<char>(member_name)); }
        else if (type == std::type_index(typeid(wchar_t))) { data->set_char16_value(data->get_member_id_by_name(member_name), variant_data->GetValue<wchar_t>(member_name)); }
        else if (type == std::type_index(typeid(int8_t))) { data->set_int8_value(data->get_member_id_by_name(member_name), variant_data->GetValue<int8_t>(member_name)); }
        else if (type == std::type_index(typeid(int16_t))) { data->set_int16_value(data->get_member_id_by_name(member_name), variant_data->GetValue<int16_t>(member_name)); }
        else if (type == std::type_index(typeid(int32_t))) { data->set_int32_value(data->get_member_id_by_name(member_name), variant_data->GetValue<int32_t>(member_name)); }
        else if (type == std::type_index(typeid(int64_t))) { data->set_int64_value(data->get_member_id_by_name(member_name), variant_data->GetValue<int64_t>(member_name)); }
        else if (type == std::type_index(typeid(uint8_t))) { data->set_uint8_value(data->get_member_id_by_name(member_name), variant_data->GetValue<uint8_t>(member_name)); }
        else if (type == std::type_index(typeid(uint16_t))) { data->set_uint16_value(data->get_member_id_by_name(member_name), variant_data->GetValue<uint16_t>(member_name)); }
        else if (type == std::type_index(typeid(uint32_t))) { data->set_uint32_value(data->get_member_id_by_name(member_name), variant_data->GetValue<uint32_t>(member_name)); }
        else if (type == std::type_index(typeid(uint64_t))) { data->set_uint64_value(data->get_member_id_by_name(member_name), variant_data->GetValue<uint64_t>(member_name)); }
        else if (type == std::type_index(typeid(float))) { data->set_float32_value(data->get_member_id_by_name(member_name), variant_data->GetValue<float>(member_name)); }
        else if (type == std::type_index(typeid(double))) { data->set_float64_value(data->get_member_id_by_name(member_name), variant_data->GetValue<double>(member_name)); }
        else if (type == std::type_index(typeid(std::string))) { data->set_string_value(data->get_member_id_by_name(member_name), variant_data->GetValue<std::string>(member_name)); }
        else if (type == std::type_index(typeid(std::wstring))) { data->set_wstring_value(data->get_member_id_by_name(member_name), variant_data->GetValue<std::wstring>(member_name)); }
        // else if (type == std::type_index(typeid(VariantData))) {}
        else if (type == std::type_index(typeid(std::vector<char>))) { data->set_char8_values(data->get_member_id_by_name(member_name), variant_data->GetValue<std::vector<char>>(member_name)); }
        else if (type == std::type_index(typeid(std::vector<wchar_t>))) { data->set_char16_values(data->get_member_id_by_name(member_name), variant_data->GetValue<std::vector<wchar_t>>(member_name)); }
        else if (type == std::type_index(typeid(std::vector<int8_t>))) { data->set_int8_values(data->get_member_id_by_name(member_name), variant_data->GetValue<std::vector<int8_t>>(member_name)); }
        else if (type == std::type_index(typeid(std::vector<int16_t>))) { data->set_int16_values(data->get_member_id_by_name(member_name), variant_data->GetValue<std::vector<int16_t>>(member_name)); }
        else if (type == std::type_index(typeid(std::vector<int32_t>))) { data->set_int32_values(data->get_member_id_by_name(member_name), variant_data->GetValue<std::vector<int32_t>>(member_name)); }
        else if (type == std::type_index(typeid(std::vector<int64_t>))) { data->set_int64_values(data->get_member_id_by_name(member_name), variant_data->GetValue<std::vector<int64_t>>(member_name)); }
        else if (type == std::type_index(typeid(std::vector<uint8_t>))) { data->set_uint8_values(data->get_member_id_by_name(member_name), variant_data->GetValue<std::vector<uint8_t>>(member_name)); }
        else if (type == std::type_index(typeid(std::vector<uint16_t>))) { data->set_uint16_values(data->get_member_id_by_name(member_name), variant_data->GetValue<std::vector<uint16_t>>(member_name)); }
        else if (type == std::type_index(typeid(std::vector<uint32_t>))) { data->set_uint32_values(data->get_member_id_by_name(member_name), variant_data->GetValue<std::vector<uint32_t>>(member_name)); }
        else if (type == std::type_index(typeid(std::vector<uint64_t>))) { data->set_uint64_values(data->get_member_id_by_name(member_name), variant_data->GetValue<std::vector<uint64_t>>(member_name)); }
        else if (type == std::type_index(typeid(std::vector<float>))) { data->set_float32_values(data->get_member_id_by_name(member_name), variant_data->GetValue<std::vector<float>>(member_name)); }
        else if (type == std::type_index(typeid(std::vector<double>))) { data->set_float64_values(data->get_member_id_by_name(member_name), variant_data->GetValue<std::vector<double>>(member_name)); }
        else if (type == std::type_index(typeid(std::vector<bool>))) { data->set_boolean_values(data->get_member_id_by_name(member_name), variant_data->GetValue<std::vector<bool>>(member_name)); }
        else if (type == std::type_index(typeid(std::vector<std::string>))) { data->set_string_values(data->get_member_id_by_name(member_name), variant_data->GetValue<std::vector<std::string>>(member_name)); }
        else if (type == std::type_index(typeid(std::vector<std::wstring>))) { data->set_wstring_values(data->get_member_id_by_name(member_name), variant_data->GetValue<std::vector<std::wstring>>(member_name)); }
        else {
            std::cerr << "Not supported type. name=" << member_list[idx] << std::endl;
        }
    }

    dds_data = data;
    return true;
}

bool DDSProtocol::DeserializeMessage(DynamicData::_ref_type dds_data, std::shared_ptr<VariantData> variant_data) {
    DynamicTypeMembersByName members;
    dds_data->type()->get_all_members_by_name(members);

    for (auto& iter : members) {
        auto member = iter.second;
        auto name = member->get_name();

        MemberDescriptor::_ref_type member_desc = {traits<MemberDescriptor>::make_shared()};
        if (member->get_descriptor(member_desc) != RETCODE_OK) {
            std::cerr << "Failed to get member descriptor." << std::endl;
            continue;
        }

        auto type = member_desc->type()->get_kind();
        switch (type) {
            case TK_BOOLEAN: {
                bool value; dds_data->get_boolean_value(value, member->get_id());
                variant_data->SetValue(name.c_str(), value);
                break;
            }
            case TK_CHAR8: {
                char value; dds_data->get_char8_value(value, member->get_id());
                variant_data->SetValue(name.c_str(), value);
                break;
            }
            case TK_CHAR16: {
                wchar_t value; dds_data->get_char16_value(value, member->get_id());
                variant_data->SetValue(name.c_str(), value);
                break;
            }
            case TK_BYTE:
            case TK_UINT8: {
                uint8_t value; dds_data->get_uint8_value(value, member->get_id());
                variant_data->SetValue(name.c_str(), value);
                break;
            }
            case TK_INT8: {
                int8_t value; dds_data->get_int8_value(value, member->get_id());
                variant_data->SetValue(name.c_str(), value);
                break;
            }
            case TK_INT16: {
                int16_t value; dds_data->get_int16_value(value, member->get_id());
                variant_data->SetValue(name.c_str(), value);
                break;
            }
            case TK_UINT16: {
                uint16_t value; dds_data->get_uint16_value(value, member->get_id());
                variant_data->SetValue(name.c_str(), value);
                break;
            }
            case TK_INT32: {
                int32_t value; dds_data->get_int32_value(value, member->get_id());
                variant_data->SetValue(name.c_str(), value);
                break;
            }
            case TK_UINT32: {
                uint32_t value; dds_data->get_uint32_value(value, member->get_id());
                variant_data->SetValue(name.c_str(), value);
                break;
            }
            case TK_INT64: {
                int64_t value; dds_data->get_int64_value(value, member->get_id());
                variant_data->SetValue(name.c_str(), value);
                break;
            }
            case TK_UINT64: {
                uint64_t value; dds_data->get_uint64_value(value, member->get_id());
                variant_data->SetValue(name.c_str(), value);
                break;
            }
            case TK_FLOAT32: {
                float value; dds_data->get_float32_value(value, member->get_id());
                variant_data->SetValue(name.c_str(), value);
                break;
            }
            case TK_FLOAT64: {
                double value; dds_data->get_float64_value(value, member->get_id());
                variant_data->SetValue(name.c_str(), value);
                break;
            }
            case TK_STRING8: {
                std::string value; dds_data->get_string_value(value, member->get_id());
                variant_data->SetValue(name.c_str(), value);
                break;
            }
            case TK_STRING16: {
                std::wstring value; dds_data->get_wstring_value(value, member->get_id());
                variant_data->SetValue(name.c_str(), value);
                break;
            }
            case TK_SEQUENCE: {
                TypeDescriptor::_ref_type type_desc = {traits<TypeDescriptor>::make_shared()};
                member_desc->type()->get_descriptor(type_desc);
                if (type_desc == nullptr) {
                    std::cerr << "No type descriptor for sequence. name=" << name.c_str() << std::endl;
                }
                auto seq_type = type_desc->element_type()->get_kind();

                if (!read_values_from_dynamic_data(dds_data, member, seq_type, variant_data)) {
                    std::cerr << "Not supported sequence type." << std::endl;
                }
                break;
            }
            case TK_STRUCTURE: {
                std::shared_ptr<VariantData> child = std::make_shared<VariantData>();
                child->SetName(name.c_str());

                DynamicData::_ref_type loan_data = dds_data->loan_value(member->get_id());
                if (DeserializeMessage(loan_data, child)) {
                    variant_data->SetValue(name.c_str(), std::move(*child));
                }
                dds_data->return_loaned_value(loan_data);
                break;
            }
            case TK_ENUM:
            default: {
                std::cerr << "Type of kind[" << (int32_t)type << "] is not supported yet." << std::endl;
            }
        }
    }

    return true;
}

void DDSProtocol::load_dds_dynamic_idl(const std::string& path) {
    if (!dds_dynamic_idl_parser_->Initialize("idl")) {
        return;
    }
    auto type_support_list = dds_dynamic_idl_parser_->ParseAll();

    for (auto& iter : type_support_list) {
        parsed_dynamic_type_supports_[iter->type_support->get_type_name()] = iter;
    }

    std::cout << "Parsed " << parsed_dynamic_type_supports_.size() << " idl files dynamically." << std::endl;
}

// FastDDS override method
void DDSProtocol::on_data_available(DataReader* reader) {
    std::shared_ptr<TopicInformation> topic_info;

    {
        std::lock_guard<std::mutex> lock(subsription_handling_mutex_);

        auto topic_info_iter = discovered_topic_.find(reader->get_topicdescription()->get_name());
        if (topic_info_iter == discovered_topic_.end()) {
            return;
        } else {
            topic_info = topic_info_iter->second;
        }
    }

    DynamicData::_ref_type new_data = DynamicDataFactory::get_instance()->create_data(topic_info->remote_type);
    SampleInfo info;

    if (reader->take_next_sample(&new_data, &info) == eprosima::fastdds::dds::RETCODE_OK)
    {
        if (info.valid_data)
        {
            std::shared_ptr<VariantData> variant_data = std::make_shared<VariantData>();

            // /* Publication GUID */
            // InstanceHandle_t publication_handle = info.publication_handle;
            // std::cout << "- Received a message. " << reader->get_topicdescription()->get_name() << " publication_handle=" << publication_handle << std::endl;

            variant_data->SetName(topic_info->topic_type);
            if (DeserializeMessage(new_data, variant_data)) {
                std::shared_ptr<Message> message = std::make_shared<Message>(
                    topic_info->topic_name, MessageProtocolType::DDS, topic_info->topic_type, variant_data
                );
                ForwardMessage(message);
            } else{
                std::cerr << "Failed to parse the message." << std::endl;
            }
        }
    }
}

void DDSProtocol::on_subscription_matched(DataReader* reader, const SubscriptionMatchedStatus& info) {
    if (info.current_count_change == 1)
    {
        // std::cout << "Subscriber matched." << std::endl;
    }
    else if (info.current_count_change == -1)
    {
        // std::cout << "Subscriber unmatched." << std::endl;
    }
    else
    {
        std::cout << info.current_count_change
                << " is not a valid value for SubscriptionMatchedStatus current count change" << std::endl;
    }
}
void DDSProtocol::on_data_writer_discovery(DomainParticipant* participant,
                                           eprosima::fastdds::rtps::WriterDiscoveryStatus reason,
                                           const eprosima::fastdds::dds::PublicationBuiltinTopicData& info,
                                           bool& should_be_ignored) {
    std::lock_guard<std::mutex> lock(subsription_handling_mutex_);

    if (discovered_topic_.count(info.topic_name.to_string()) > 0) {
        return;
    }

    // Notify run thread that type has been discovered
    std::shared_ptr<TopicInformation> topic_info = std::make_shared<TopicInformation>();
    topic_info->topic_name = info.topic_name.to_string();
    topic_info->topic_type = info.type_name.to_string();

    // Get remote type information and use it to retrieve the type object
    topic_info->remote_type_information = info.type_information.type_information;

    discovered_topic_[topic_info->topic_name] = topic_info;

    // If it's already subscribed topic
    if (subscribed_topic_.count(topic_info->topic_name) > 0) {
        if (!register_type_object(topic_info)) {
            std::cerr << "Failed to register a type object. topic=" << topic_info->topic_name << std::endl;
            return;
        }
        if (!create_datareader(topic_info)) {
            std::cerr << "Failed to register a datareader. topic=" << topic_info->topic_name << std::endl;
        }
    }

}

bool DDSProtocol::register_type_object(std::shared_ptr<TopicInformation> topic_information) {
    if (register_type_object_from_dynamic_idl(topic_information)) {
        return true;
    }
    if (register_type_object_from_discovery(topic_information)) {
        return true;
    }
    return false;
}

bool DDSProtocol::register_type_object_from_discovery(std::shared_ptr<TopicInformation> topic_information) {

    auto type_id_complete = topic_information->remote_type_information.complete().typeid_with_size().type_id();
    auto type_id_minimal = topic_information->remote_type_information.minimal().typeid_with_size().type_id();

    if ((RETCODE_OK != DomainParticipantFactory::get_instance()->type_object_registry().get_type_object(
                type_id_complete, topic_information->remote_type_object)) &&
            (RETCODE_OK != DomainParticipantFactory::get_instance()->type_object_registry().get_type_object(
                type_id_minimal, topic_information->remote_type_object)))
    {
        return false;
    }

    // Register remotely discovered type
    DynamicTypeBuilder::_ref_type type_builder = DynamicTypeBuilderFactory::get_instance()->create_type_w_type_object(
        topic_information->remote_type_object);
    if (!type_builder)
    {
        return false;
    }

    // Build the type
    topic_information->remote_type = type_builder->build();
    if (!topic_information->remote_type)
    {
        return false;
    }

    topic_information->type_support = std::make_shared<TypeSupport>(new DynamicPubSubType(topic_information->remote_type, topic_information->remote_type_information));
    if (RETCODE_OK != topic_information->type_support->register_type(participant_, topic_information->topic_type))
    {
        return false;
    }

    return true;
}

bool DDSProtocol::register_type_object_from_dynamic_idl(std::shared_ptr<TopicInformation> topic_information) {
    auto iter = parsed_dynamic_type_supports_.find(topic_information->topic_type);
    if (iter == parsed_dynamic_type_supports_.end()) {
        return false;
    }

    topic_information->remote_type = iter->second->dyn_type;
    topic_information->type_support = iter->second->type_support;

    topic_information->type_support->register_type(participant_, topic_information->topic_type);
    return true;
}

bool DDSProtocol::create_datareader(std::shared_ptr<TopicInformation> topic_information) {
    // Create the topic
    TopicQos topic_qos = TOPIC_QOS_DEFAULT;
    participant_->get_default_topic_qos(topic_qos);
    topic_information->topic = participant_->create_topic(topic_information->topic_name, topic_information->topic_type, topic_qos);
    if (topic_information->topic == nullptr)
    {
        return false;
    }

    DataReaderQos reader_qos = DATAREADER_QOS_DEFAULT;
    subscriber_->get_default_datareader_qos(reader_qos);
    topic_information->reader = subscriber_->create_datareader(topic_information->topic, reader_qos);

    return true;
}

bool DDSProtocol::delete_datareader(std::shared_ptr<TopicInformation> topic_information) {
    if (subscriber_->delete_datareader(topic_information->reader) != RETCODE_OK) {
        return false;
    }
    topic_information->reader = nullptr;

    if (participant_->delete_topic(topic_information->topic) != RETCODE_OK) {
        return false;
    }
    topic_information->topic = nullptr;
    return true;
}

bool DDSProtocol::read_values_from_dynamic_data(DynamicData::_ref_type dds_data, DynamicTypeMember::_ref_type member, TypeKind elem_type,
                                                std::shared_ptr<VariantData> variant_data) {
    switch (elem_type) {
        case TK_BOOLEAN: {
            std::vector<bool> value; dds_data->get_boolean_values(value, member->get_id());
            variant_data->SetValue(member->get_name().c_str(), std::move(value));
            break;
        }
        case TK_CHAR8: {
            std::vector<char> value; dds_data->get_char8_values(value, member->get_id());
            variant_data->SetValue(member->get_name().c_str(), std::move(value));
            break;
        }
        case TK_CHAR16: {
            std::vector<wchar_t> value; dds_data->get_char16_values(value, member->get_id());
            variant_data->SetValue(member->get_name().c_str(), std::move(value));
            break;
        }
        case TK_BYTE:
        case TK_UINT8: {
            std::vector<uint8_t> value; dds_data->get_uint8_values(value, member->get_id());
            variant_data->SetValue(member->get_name().c_str(), std::move(value));
            break;
        }
        case TK_INT8: {
            std::vector<int8_t> value; dds_data->get_int8_values(value, member->get_id());
            variant_data->SetValue(member->get_name().c_str(), std::move(value));
            break;
        }
        case TK_INT16: {
            std::vector<int16_t> value; dds_data->get_int16_values(value, member->get_id());
            variant_data->SetValue(member->get_name().c_str(), std::move(value));
            break;
        }
        case TK_UINT16: {
            std::vector<uint16_t> value; dds_data->get_uint16_values(value, member->get_id());
            variant_data->SetValue(member->get_name().c_str(), std::move(value));
            break;
        }
        case TK_INT32: {
            std::vector<int32_t> value; dds_data->get_int32_values(value, member->get_id());
            variant_data->SetValue(member->get_name().c_str(), std::move(value));
            break;
        }
        case TK_UINT32: {
            std::vector<uint32_t> value; dds_data->get_uint32_values(value, member->get_id());
            variant_data->SetValue(member->get_name().c_str(), std::move(value));
            break;
        }
        case TK_INT64: {
            std::vector<int64_t> value; dds_data->get_int64_values(value, member->get_id());
            variant_data->SetValue(member->get_name().c_str(), std::move(value));
            break;
        }
        case TK_UINT64: {
            std::vector<uint64_t> value; dds_data->get_uint64_values(value, member->get_id());
            variant_data->SetValue(member->get_name().c_str(), std::move(value));
            break;
        }
        case TK_FLOAT32: {
            std::vector<float> value; dds_data->get_float32_values(value, member->get_id());
            variant_data->SetValue(member->get_name().c_str(), std::move(value));
            break;
        }
        case TK_FLOAT64: {
            std::vector<double> value; dds_data->get_float64_values(value, member->get_id());
            variant_data->SetValue(member->get_name().c_str(), std::move(value));
            break;
        }
        case TK_STRING8: {
            std::vector<std::string> value; dds_data->get_string_values(value, member->get_id());
            variant_data->SetValue(member->get_name().c_str(), std::move(value));
            break;
        }
        case TK_STRING16: {
            std::vector<std::wstring> value; dds_data->get_wstring_values(value, member->get_id());
            variant_data->SetValue(member->get_name().c_str(), std::move(value));
            break;
        }
        case TK_STRUCTURE:
        default: {
            return false;
        }
    }
    return true;
}

} // namespace diagnostic_manager
