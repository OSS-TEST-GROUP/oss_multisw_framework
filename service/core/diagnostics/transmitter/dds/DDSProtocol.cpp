#include "DDSProtocol.hpp"

#include <optional>

#include <fastdds/dds/xtypes/dynamic_types/DynamicTypeBuilderFactory.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>

#include "logger/Log.hpp"

namespace diagnostic_manager {

std::optional<std::string> getEnvPath(const std::string& key) {
	const char* val = std::getenv(key.c_str());
    LOG_DEBUG("getEnvPath: key=" << key << ", value=" << (val ? val : "null"));
	return val ? std::optional<std::string>(val) : std::nullopt;
}

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

    std::optional<std::string> diag_idl_path = getEnvPath("DIAG_IDL_PATH");

    LOG_DEBUG("getEnvPath: key=DIAG_IDL_PATH, value=" << diag_idl_path.value_or("idl"));

    load_dds_dynamic_idl(diag_idl_path.value_or("idl"));

    // Create the subscriber and the publisher
    SubscriberQos sub_qos = SUBSCRIBER_QOS_DEFAULT;
    participant_->get_default_subscriber_qos(sub_qos);
    subscriber_ = participant_->create_subscriber(sub_qos);
    if (subscriber_ == nullptr)
    {
        return false;
    }

    PublisherQos pub_qos = PUBLISHER_QOS_DEFAULT;
    participant_->get_default_publisher_qos(pub_qos);
    publisher_ = participant_->create_publisher(pub_qos);
    if (publisher_ == nullptr) {
        return false;
    }

    return true;
}

void DDSProtocol::Finalize() {
    if (participant_) {
        participant_->delete_contained_entities();

        auto factory = DomainParticipantFactory::get_instance();
        if (factory->delete_participant(participant_) != RETCODE_OK) {
            LOG_ERROR("Failed to delete the participant.");
        }
    }
}

MessageProtocolType DDSProtocol::GetProtocolType() {
    return MessageProtocolType::DDS;
}

bool DDSProtocol::PublishMessage(std::shared_ptr<Message> message) {
    LOG_INFO("PublishMessage called: topic=" << message->GetName() << ", type=" << message->GetTypeName());
    
    std::shared_ptr<TopicInformation> topic_info;
    {
        std::lock_guard<std::mutex> lock(topic_handling_mutex_);

        auto iter = discovered_topic_.find(message->GetName());
        if (iter == discovered_topic_.end()) {
            LOG_INFO("Topic not found in cache, creating new topic info for: " << message->GetName());
            std::string type_name = message->GetTypeName();
            
            // For ROS messages (rt/ prefixed topics), convert ROS2 DDS type names to simple type names
            if (message->GetName().substr(0, 3) == "rt/") {
                std::string converted_type_name = type_name;
                std::string original_type_name = type_name;
                
                // Handle both directions of conversion:
                // 1. If we get ROS2 DDS type names, convert to simple names for IDL lookup
                if (type_name == "diagnosticmgr::msg::dds_::DiagnosticInfoRequest_") {
                    converted_type_name = "DiagnosticInfoRequest";
                } else if (type_name == "diagnosticmgr::msg::dds_::DiagnosticInfoListRequest_") {
                    converted_type_name = "DiagnosticInfoListRequest";
                } else if (type_name == "diagnosticmgr::msg::dds_::DiagnosticInfoResponse_") {
                    converted_type_name = "DiagnosticInfoResponse";
                } else if (type_name == "diagnosticmgr::msg::dds_::DiagnosticInfoListResponse_") {
                    converted_type_name = "DiagnosticInfoListResponse";
                } else if (type_name == "diagnosticmgr::msg::dds_::DiagnosticNotification_") {
                    converted_type_name = "DiagnosticNotification";
                }
                // 2. If we get simple type names, convert to ROS2 DDS type names for registration
                else if (type_name == "DiagnosticInfoRequest") {
                    original_type_name = "diagnosticmgr::msg::dds_::DiagnosticInfoRequest_";
                    converted_type_name = "DiagnosticInfoRequest";
                } else if (type_name == "DiagnosticInfoListRequest") {
                    original_type_name = "diagnosticmgr::msg::dds_::DiagnosticInfoListRequest_";
                    converted_type_name = "DiagnosticInfoListRequest";
                } else if (type_name == "DiagnosticInfoResponse") {
                    original_type_name = "diagnosticmgr::msg::dds_::DiagnosticInfoResponse_";
                    converted_type_name = "DiagnosticInfoResponse";
                } else if (type_name == "DiagnosticInfoListResponse") {
                    original_type_name = "diagnosticmgr::msg::dds_::DiagnosticInfoListResponse_";
                    converted_type_name = "DiagnosticInfoListResponse";
                } else if (type_name == "DiagnosticNotification") {
                    original_type_name = "diagnosticmgr::msg::dds_::DiagnosticNotification_";
                    converted_type_name = "DiagnosticNotification";
                }
                
                LOG_INFO("Publishing ROS message: type='" << type_name << "' -> converted='" << converted_type_name << "', original='" << original_type_name << "' for topic: " << message->GetName());
                
                topic_info = create_topic_information_from_dynamic_idl(message->GetName(), converted_type_name, original_type_name);
            } else {
                LOG_INFO("Publishing DDS message with type: " << type_name << " for topic: " << message->GetName());
                topic_info = create_topic_information_from_dynamic_idl(message->GetName(), type_name);
            }
            if (topic_info) {
                discovered_topic_[message->GetName()] = topic_info;
                LOG_INFO("Successfully created and cached topic info for: " << message->GetName());
            } else {
                LOG_ERROR("Failed to create topic info for: " << message->GetName());
            }
        } else {
            topic_info = iter->second;
            LOG_VERBOSE("Using cached topic info for: " << message->GetName());
            
            // For ROS messages, show the type mapping info even when using cache
            if (message->GetName().substr(0, 3) == "rt/") {
                LOG_DEBUG("Using cached ROS topic: message_type='" << message->GetTypeName() << "' -> dds_type='" << topic_info->topic_type << "'");
            }
        }
    }

    if (topic_info == nullptr) {
        LOG_ERROR("Failed to publish the message. No topic_information. name=" << message->GetName());
        return false;
    }

    if (topic_info->writer == nullptr) {
        LOG_INFO("DataWriter not found, creating new one for topic: " << message->GetName());
         if (!create_datawriter(topic_info)) {
            LOG_ERROR("Failed to create a datawriter. name=" << message->GetName());
            return false;
         } else {
            LOG_INFO("Successfully created DataWriter for topic: " << message->GetName());
         }
    } else {
        LOG_VERBOSE("Using existing DataWriter for topic: " << message->GetName());
    }

    if (topic_info->remote_type == nullptr) {
        LOG_ERROR("Failed to create dynamic data type for topic: " << message->GetName());
        return false;
    }
    
    LOG_VERBOSE("Creating dynamic data for type: " << topic_info->topic_type);
    DynamicData::_ref_type dds_data = DynamicDataFactory::get_instance()->create_data(topic_info->remote_type);
    
    LOG_VERBOSE("Serializing message data for topic: " << message->GetName());
    if (!SerializeMessage(dds_data, message->GetData())) {
        LOG_ERROR("Failed to serialize the message. name=" << message->GetName());
        return false;
    }

    LOG_INFO("Writing message to DDS for topic: " << message->GetName());
    if (topic_info->writer->write(&dds_data) != RETCODE_OK) {
        LOG_ERROR("Failed to write the message. name=" << message->GetName());
        return false;
    }

    LOG_INFO("Successfully published message to topic: " << message->GetName());
    return true;
}

bool DDSProtocol::SubscribeMessage(const std::string& name) {
    std::lock_guard<std::mutex> lock(topic_handling_mutex_);

    subscribed_topic_.insert(name);
    LOG_INFO("SubscribeMessage a type object. topic=" << name);

    
    if (discovered_topic_.count(name) > 0) {
        auto topic_info = discovered_topic_[name];
        if (!register_type_object(topic_info)) {
            LOG_ERROR("Failed to register a type object. topic=" << topic_info->topic_name << ", " << topic_info->topic_type);
        }
        if (!create_datareader(topic_info)) {
            LOG_ERROR("Failed to create a datareader. topic=" << topic_info->topic_name);
        }
    }
    LOG_INFO("SubscribeMessage completed. topic=" << name  << ", " << discovered_topic_.count(name));

    return true;
}

bool DDSProtocol::UnsubscribeMessage(const std::string& name) {
    std::lock_guard<std::mutex> lock(topic_handling_mutex_);
    LOG_INFO("UnsubscribeMessage a type object. topic=" << name);

    if (subscribed_topic_.count(name) == 0) {
        return false;
    }
    subscribed_topic_.erase(name);

    if (discovered_topic_.count(name) > 0) {
        auto topic_info = discovered_topic_[name];
        if (topic_info->reader) {
            if(!delete_datareader(topic_info)) {
                LOG_ERROR("Failed to delete a datareader. topic=" << name);
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

    for (uint64_t idx = 0; idx < member_list.size(); idx++) {
        auto& type = type_list[idx];
        std::string& member_name = member_list[idx];
        if (type == std::type_index(typeid(char))) { dds_data->set_char8_value(dds_data->get_member_id_by_name(member_name), variant_data->GetValue<char>(member_name)); }
        else if (type == std::type_index(typeid(wchar_t))) { dds_data->set_char16_value(dds_data->get_member_id_by_name(member_name), variant_data->GetValue<wchar_t>(member_name)); }
        else if (type == std::type_index(typeid(int8_t))) { dds_data->set_int8_value(dds_data->get_member_id_by_name(member_name), variant_data->GetValue<int8_t>(member_name)); }
        else if (type == std::type_index(typeid(int16_t))) { dds_data->set_int16_value(dds_data->get_member_id_by_name(member_name), variant_data->GetValue<int16_t>(member_name)); }
        else if (type == std::type_index(typeid(int32_t))) { dds_data->set_int32_value(dds_data->get_member_id_by_name(member_name), variant_data->GetValue<int32_t>(member_name)); }
        else if (type == std::type_index(typeid(int64_t))) { dds_data->set_int64_value(dds_data->get_member_id_by_name(member_name), variant_data->GetValue<int64_t>(member_name)); }
        else if (type == std::type_index(typeid(uint8_t))) { dds_data->set_uint8_value(dds_data->get_member_id_by_name(member_name), variant_data->GetValue<uint8_t>(member_name)); }
        else if (type == std::type_index(typeid(uint16_t))) { dds_data->set_uint16_value(dds_data->get_member_id_by_name(member_name), variant_data->GetValue<uint16_t>(member_name)); }
        else if (type == std::type_index(typeid(uint32_t))) { dds_data->set_uint32_value(dds_data->get_member_id_by_name(member_name), variant_data->GetValue<uint32_t>(member_name)); }
        else if (type == std::type_index(typeid(uint64_t))) { dds_data->set_uint64_value(dds_data->get_member_id_by_name(member_name), variant_data->GetValue<uint64_t>(member_name)); }
        else if (type == std::type_index(typeid(float))) { dds_data->set_float32_value(dds_data->get_member_id_by_name(member_name), variant_data->GetValue<float>(member_name)); }
        else if (type == std::type_index(typeid(double))) { dds_data->set_float64_value(dds_data->get_member_id_by_name(member_name), variant_data->GetValue<double>(member_name)); }
        else if (type == std::type_index(typeid(std::string))) { dds_data->set_string_value(dds_data->get_member_id_by_name(member_name), variant_data->GetValue<std::string>(member_name)); }
        else if (type == std::type_index(typeid(std::wstring))) { dds_data->set_wstring_value(dds_data->get_member_id_by_name(member_name), variant_data->GetValue<std::wstring>(member_name)); }
        // else if (type == std::type_index(typeid(VariantData))) {}
        else if (type == std::type_index(typeid(std::vector<char>))) { dds_data->set_char8_values(dds_data->get_member_id_by_name(member_name), variant_data->GetValue<std::vector<char>>(member_name)); }
        else if (type == std::type_index(typeid(std::vector<wchar_t>))) { dds_data->set_char16_values(dds_data->get_member_id_by_name(member_name), variant_data->GetValue<std::vector<wchar_t>>(member_name)); }
        else if (type == std::type_index(typeid(std::vector<int8_t>))) { dds_data->set_int8_values(dds_data->get_member_id_by_name(member_name), variant_data->GetValue<std::vector<int8_t>>(member_name)); }
        else if (type == std::type_index(typeid(std::vector<int16_t>))) { dds_data->set_int16_values(dds_data->get_member_id_by_name(member_name), variant_data->GetValue<std::vector<int16_t>>(member_name)); }
        else if (type == std::type_index(typeid(std::vector<int32_t>))) { dds_data->set_int32_values(dds_data->get_member_id_by_name(member_name), variant_data->GetValue<std::vector<int32_t>>(member_name)); }
        else if (type == std::type_index(typeid(std::vector<int64_t>))) { dds_data->set_int64_values(dds_data->get_member_id_by_name(member_name), variant_data->GetValue<std::vector<int64_t>>(member_name)); }
        else if (type == std::type_index(typeid(std::vector<uint8_t>))) { dds_data->set_uint8_values(dds_data->get_member_id_by_name(member_name), variant_data->GetValue<std::vector<uint8_t>>(member_name)); }
        else if (type == std::type_index(typeid(std::vector<uint16_t>))) { dds_data->set_uint16_values(dds_data->get_member_id_by_name(member_name), variant_data->GetValue<std::vector<uint16_t>>(member_name)); }
        else if (type == std::type_index(typeid(std::vector<uint32_t>))) { dds_data->set_uint32_values(dds_data->get_member_id_by_name(member_name), variant_data->GetValue<std::vector<uint32_t>>(member_name)); }
        else if (type == std::type_index(typeid(std::vector<uint64_t>))) { dds_data->set_uint64_values(dds_data->get_member_id_by_name(member_name), variant_data->GetValue<std::vector<uint64_t>>(member_name)); }
        else if (type == std::type_index(typeid(std::vector<float>))) { dds_data->set_float32_values(dds_data->get_member_id_by_name(member_name), variant_data->GetValue<std::vector<float>>(member_name)); }
        else if (type == std::type_index(typeid(std::vector<double>))) { dds_data->set_float64_values(dds_data->get_member_id_by_name(member_name), variant_data->GetValue<std::vector<double>>(member_name)); }
        else if (type == std::type_index(typeid(std::vector<bool>))) { dds_data->set_boolean_values(dds_data->get_member_id_by_name(member_name), variant_data->GetValue<std::vector<bool>>(member_name)); }
        else if (type == std::type_index(typeid(std::vector<std::string>))) { dds_data->set_string_values(dds_data->get_member_id_by_name(member_name), variant_data->GetValue<std::vector<std::string>>(member_name)); }
        else if (type == std::type_index(typeid(std::vector<std::wstring>))) { dds_data->set_wstring_values(dds_data->get_member_id_by_name(member_name), variant_data->GetValue<std::vector<std::wstring>>(member_name)); }
        else {
            LOG_ERROR("Not supported type. name=" << member_list[idx]);
        }
    }

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
            LOG_ERROR("Failed to get member descriptor.");
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
                    LOG_ERROR("No type descriptor for sequence. name=" << name.c_str());
                }
                auto seq_type = type_desc->element_type()->get_kind();

                if (seq_type == TK_STRUCTURE){
                    std::shared_ptr<VariantData> child = std::make_shared<VariantData>();
                    child->SetName(name.c_str());

                    LOG_DEBUG("Recursive sequence of struct. " << name);
                    DynamicData::_ref_type loan_data = dds_data->loan_value(member->get_id());
                    if (DeserializeMessage(loan_data, child)) {
                        variant_data->SetValue(name.c_str(), std::move(*child));
                    }
                }else if (!read_values_from_dynamic_data(dds_data, member, seq_type, variant_data)) {
                    LOG_ERROR("Not supported sequence type.");
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
                LOG_ERROR("Type of kind[" << (int32_t)type << "] is not supported yet.");
            }
        }
    }
    return true;
}

void DDSProtocol::load_dds_dynamic_idl(const std::string& path) {
    if (!dds_dynamic_idl_parser_->Initialize(path)) {
        return;
    }
    auto type_support_list = dds_dynamic_idl_parser_->ParseAll();

    for (auto& iter : type_support_list) {
        std::string type_name = iter->type_support->get_type_name();
        parsed_dynamic_type_supports_[type_name] = iter;
        LOG_VERBOSE("Loaded dynamic type: " << type_name);
    }

    LOG_INFO("loaded " << type_support_list.size() << " dynamic types.");
}

// FastDDS override method
void DDSProtocol::on_data_available(DataReader* reader) {
    std::shared_ptr<TopicInformation> topic_info;

    {
        std::lock_guard<std::mutex> lock(topic_handling_mutex_);

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
            LOG_DEBUG("Received a message. " << reader->get_topicdescription()->get_name());

            variant_data->SetName(topic_info->topic_name);
            if (DeserializeMessage(new_data, variant_data)) {
                std::shared_ptr<Message> message = std::make_shared<Message>(
                    topic_info->topic_name, MessageProtocolType::DDS, topic_info->topic_type, variant_data
                );
                ForwardMessage(message);
            } else{
                LOG_ERROR("Failed to parse the message.");
            }
        }
    }
}

void DDSProtocol::on_subscription_matched(DataReader* reader, const SubscriptionMatchedStatus& info) {
    if (info.current_count_change == 1)
    {
        LOG_VERBOSE("Subscriber matched.");
    }
    else if (info.current_count_change == -1)
    {
        LOG_VERBOSE("Subscriber unmatched.");
    }
    else
    {
        LOG_ERROR(info.current_count_change
                << " is not a valid value for SubscriptionMatchedStatus current count change");
    }
}
void DDSProtocol::on_data_writer_discovery(DomainParticipant* participant,
                                           eprosima::fastdds::rtps::WriterDiscoveryStatus reason,
                                           const eprosima::fastdds::dds::PublicationBuiltinTopicData& info,
                                           bool& should_be_ignored) {
    std::shared_ptr<TopicInformation> topic_info;
    bool need_to_subscribe = false;
    {
        std::lock_guard<std::mutex> lock(topic_handling_mutex_);
        if (discovered_topic_.count(info.topic_name.to_string()) > 0) {
            return;
        }
        LOG_VERBOSE("on_data_writer_discovery: " << info.topic_name.to_string() << ", " << info.type_name.to_string());

        // Notify run thread that type has been discovered
        topic_info = std::make_shared<TopicInformation>();
        topic_info->topic_name = info.topic_name.to_string();
        topic_info->topic_type = info.type_name.to_string();

        // Get remote type information and use it to retrieve the type object
        topic_info->remote_type_information = info.type_information.type_information;

        discovered_topic_[topic_info->topic_name] = topic_info;

        // If it's already subscribed topic
        if (subscribed_topic_.count(topic_info->topic_name) > 0) {
            need_to_subscribe = true;
        }
    }

    if (need_to_subscribe) {
        if (!register_type_object(topic_info)) {
            LOG_ERROR("Failed to register a type object. topic=" << topic_info->topic_name);
            return;
        }
        if (!create_datareader(topic_info)) {
            LOG_ERROR("Failed to create a datareader. topic=" << topic_info->topic_name);
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
    
    auto ret_complete = DomainParticipantFactory::get_instance()->type_object_registry().get_type_object(
                type_id_complete, topic_information->remote_type_object);

    auto ret_minimal = DomainParticipantFactory::get_instance()->type_object_registry().get_type_object(
                type_id_minimal, topic_information->remote_type_object);

    if (RETCODE_OK != ret_complete)
    {
        LOG_DEBUG("register_type_object_from_discovery register_type_object complete " << topic_information->topic_name << ", type_id=" << ret_complete);
        //return false;
    }
    if (RETCODE_OK != ret_minimal)
    {
        LOG_DEBUG("register_type_object_from_discovery register_type_object minimal " << topic_information->topic_name << ", type_id=" << ret_minimal);
        //return false;
    }

    if (RETCODE_OK != DomainParticipantFactory::get_instance()->type_object_registry().get_type_object(
            type_id_complete, topic_information->remote_type_object))
    {
        LOG_DEBUG("register_type_object_from_discovery get_type_object complete " << topic_information->topic_name << ", type_id=" << topic_information->remote_type);
        return false;
    }
    if (RETCODE_OK != DomainParticipantFactory::get_instance()->type_object_registry().get_type_object(
            type_id_minimal, topic_information->remote_type_object))
    {
        LOG_DEBUG("register_type_object_from_discovery get_type_object minimal " << topic_information->topic_name << ", type_id=" << topic_information->remote_type);
        return false;
    }

    // Register remotely discovered type
    DynamicTypeBuilder::_ref_type type_builder = DynamicTypeBuilderFactory::get_instance()->create_type_w_type_object(
        topic_information->remote_type_object);
    if (!type_builder)
    {
        LOG_DEBUG("register_type_object_from_discovery create_type_w_type_object null for " << topic_information->topic_name);
        return false;
    }

    // Build the type
    topic_information->remote_type = type_builder->build();
    if (!topic_information->remote_type)
    {
        LOG_DEBUG("register_type_object_from_discovery fail topic_information->remote_type null for " << topic_information->topic_name);
        return false;
    }

    topic_information->type_support = std::make_shared<TypeSupport>(new DynamicPubSubType(topic_information->remote_type, topic_information->remote_type_information));
    ReturnCode_t result = topic_information->type_support->register_type(participant_, topic_information->topic_type);
    if (result != RETCODE_OK && result != RETCODE_PRECONDITION_NOT_MET)
    {
        LOG_ERROR("Type registration failed for discovered type " << topic_information->topic_type << " (topic: " << topic_information->topic_name << "), return code: " << (int)result);
        return false;
    }
    
    if (result == RETCODE_PRECONDITION_NOT_MET) {
        LOG_DEBUG("Discovered type already registered: " << topic_information->topic_type << " for topic: " << topic_information->topic_name);
    } else {
        LOG_DEBUG("Successfully registered discovered type: " << topic_information->topic_type << " for topic: " << topic_information->topic_name);
    }
 
    return true;
}

bool DDSProtocol::register_type_object_from_dynamic_idl(std::shared_ptr<TopicInformation> topic_information) {
    LOG_VERBOSE("Looking for type: " << topic_information->topic_type << " for topic: " << topic_information->topic_name);
    
    // Try to find the type directly first
    auto iter = parsed_dynamic_type_supports_.find(topic_information->topic_type);
    
    // If not found, try to map ROS2 DDS type names to simple names
    if (iter == parsed_dynamic_type_supports_.end()) {
        std::string mapped_type_name = topic_information->topic_type;
        
        // Map ROS2 DDS type names to simple type names
        if (topic_information->topic_type == "diagnosticmgr::msg::dds_::DiagnosticInfoRequest_") {
            mapped_type_name = "DiagnosticInfoRequest";
        } else if (topic_information->topic_type == "diagnosticmgr::msg::dds_::DiagnosticInfoListRequest_") {
            mapped_type_name = "DiagnosticInfoListRequest";
        } else if (topic_information->topic_type == "diagnosticmgr::msg::dds_::DiagnosticInfoResponse_") {
            mapped_type_name = "DiagnosticInfoResponse";
        } else if (topic_information->topic_type == "diagnosticmgr::msg::dds_::DiagnosticInfoListResponse_") {
            mapped_type_name = "DiagnosticInfoListResponse";
        } else if (topic_information->topic_type == "diagnosticmgr::msg::dds_::DiagnosticNotification_") {
            mapped_type_name = "DiagnosticNotification";
        }
        
        LOG_VERBOSE("Mapped ROS2 type '" << topic_information->topic_type << "' to '" << mapped_type_name << "'");
        iter = parsed_dynamic_type_supports_.find(mapped_type_name);
    }
    
    if (iter == parsed_dynamic_type_supports_.end()) {
        LOG_VERBOSE("Type not found in dynamic IDL: " << topic_information->topic_type << " for topic: " << topic_information->topic_name);
        LOG_VERBOSE("Available types:");
        for (const auto& pair : parsed_dynamic_type_supports_) {
            LOG_VERBOSE("  - " << pair.first);
        }
        return false;
    }

    topic_information->remote_type = iter->second->dyn_type;
    topic_information->type_support = iter->second->type_support;

    // Check if type is already registered to avoid duplicate registration
    // Use the original type name (ROS2 format) for registration
    LOG_INFO("Attempting to register type: " << topic_information->topic_type << " for topic: " << topic_information->topic_name);
    ReturnCode_t result = topic_information->type_support->register_type(participant_, topic_information->topic_type);
    if (result != RETCODE_OK && result != RETCODE_PRECONDITION_NOT_MET) {
        LOG_ERROR("Type registration failed for " << topic_information->topic_type << " (topic: " << topic_information->topic_name << "), return code: " << (int)result);
        return false;
    }
    
    if (result == RETCODE_PRECONDITION_NOT_MET) {
        LOG_DEBUG("Type already registered: " << topic_information->topic_type << " for topic: " << topic_information->topic_name);
    } else {
        LOG_DEBUG("Successfully registered type: " << topic_information->topic_type << " for topic: " << topic_information->topic_name);
    }
    
    return true;
}

bool DDSProtocol::create_datareader(std::shared_ptr<TopicInformation> topic_information) {
    // Create the topic
    if (topic_information->topic == nullptr) {
        TopicQos topic_qos = TOPIC_QOS_DEFAULT;
        participant_->get_default_topic_qos(topic_qos);
        
        LOG_INFO("Creating topic: name=" << topic_information->topic_name << ", type=" << topic_information->topic_type);
        topic_information->topic = participant_->create_topic(topic_information->topic_name, topic_information->topic_type, topic_qos);
        if (topic_information->topic == nullptr)
        {
            LOG_ERROR("Failed to create topic: name=" << topic_information->topic_name << ", type=" << topic_information->topic_type);
            return false;
        }
    }

    DataReaderQos reader_qos = DATAREADER_QOS_DEFAULT;
    subscriber_->get_default_datareader_qos(reader_qos);
    topic_information->reader = subscriber_->create_datareader(topic_information->topic, reader_qos);
    if (topic_information->reader == nullptr) {
        return false;
    }

    LOG_INFO("Created datareader for topic: " << topic_information->topic_name << ", " << topic_information->topic_type);

    return true;
}

bool DDSProtocol::delete_datareader(std::shared_ptr<TopicInformation> topic_information) {
    if (subscriber_->delete_datareader(topic_information->reader) != RETCODE_OK) {
        return false;
    }
    topic_information->reader = nullptr;

    if (topic_information->reader == nullptr) {
        if (participant_->delete_topic(topic_information->topic) != RETCODE_OK) {
            return false;
        }
        topic_information->topic = nullptr;
    }
    return true;
}

bool DDSProtocol::create_datawriter(std::shared_ptr<TopicInformation> topic_information) {
    LOG_INFO("Creating DataWriter for topic: " << topic_information->topic_name << ", type: " << topic_information->topic_type);
    
    if (topic_information->topic == nullptr) {
        LOG_INFO("Topic not found, creating new topic: " << topic_information->topic_name);
        TopicQos topic_qos = TOPIC_QOS_DEFAULT;
        participant_->get_default_topic_qos(topic_qos);
        topic_information->topic = participant_->create_topic(
            topic_information->topic_name,
            topic_information->topic_type,
            topic_qos
        );
        if (topic_information->topic == nullptr) {
            LOG_ERROR("Failed to create topic: " << topic_information->topic_name << ", type: " << topic_information->topic_type);
            return false;
        } else {
            LOG_INFO("Successfully created topic: " << topic_information->topic_name);
        }
    } else {
        LOG_VERBOSE("Using existing topic for DataWriter: " << topic_information->topic_name);
    }

    LOG_VERBOSE("Creating DataWriter with default QoS for topic: " << topic_information->topic_name);
    DataWriterQos writer_qos = DATAWRITER_QOS_DEFAULT;
    publisher_->get_default_datawriter_qos(writer_qos);
    topic_information->writer = publisher_->create_datawriter(
        topic_information->topic,
        writer_qos
    );

    if (topic_information->writer == nullptr) {
        LOG_ERROR("Failed to create DataWriter for topic: " << topic_information->topic_name);
        return false;
    }

    LOG_INFO("Successfully created DataWriter for topic: " << topic_information->topic_name);
    return true;
}

bool DDSProtocol::delete_datawriter(std::shared_ptr<TopicInformation> topic_information) {
    // Delete the DataWriter
    if (publisher_->delete_datawriter(topic_information->writer) != RETCODE_OK) {
        LOG_ERROR("Failed to delete datawriter for topic: " << topic_information->topic_name);
        return false;
    }
    topic_information->writer = nullptr;

    // Delete the Topic (only if it's not in use by other entities)
    if (topic_information->reader == nullptr) {
        if (participant_->delete_topic(topic_information->topic) != RETCODE_OK) {
            LOG_ERROR("Failed to delete topic for datawriter: " << topic_information->topic_name);
            return false;
        }
        topic_information->topic = nullptr;
    }
    return true;
}

std::shared_ptr<TopicInformation> DDSProtocol::create_topic_information_from_dynamic_idl(const std::string& name, const std::string& type) {
    std::shared_ptr<TopicInformation> topic_information = std::make_shared<TopicInformation>();
    topic_information->topic_name = name;
    topic_information->topic_type = type;

    if (!register_type_object_from_dynamic_idl(topic_information)) {
        LOG_ERROR("Failed to create topic_information from dynamic idl. type=" << type);
        return nullptr;
    }
    return topic_information;
}

// Overloaded version for ROS messages that need type name conversion
std::shared_ptr<TopicInformation> DDSProtocol::create_topic_information_from_dynamic_idl(const std::string& name, const std::string& converted_type, const std::string& original_type) {
    LOG_INFO("Creating topic info for ROS message: topic=" << name << ", converted_type=" << converted_type << ", original_type=" << original_type);
    
    std::shared_ptr<TopicInformation> topic_information = std::make_shared<TopicInformation>();
    topic_information->topic_name = name;
    topic_information->topic_type = original_type;  // Use original ROS2 DDS type for registration

    // Look up the converted type in dynamic IDL
    LOG_VERBOSE("Looking up converted type in dynamic IDL: " << converted_type);
    auto iter = parsed_dynamic_type_supports_.find(converted_type);
    if (iter == parsed_dynamic_type_supports_.end()) {
        LOG_ERROR("Failed to find converted type in dynamic IDL: " << converted_type);
        LOG_VERBOSE("Available types:");
        for (const auto& pair : parsed_dynamic_type_supports_) {
            LOG_VERBOSE("  - " << pair.first);
        }
        return nullptr;
    }

    LOG_VERBOSE("Found converted type in dynamic IDL: " << converted_type);
    topic_information->remote_type = iter->second->dyn_type;
    topic_information->type_support = iter->second->type_support;
    
    // Register type with original ROS2 DDS name
    LOG_INFO("Registering type with original name: " << original_type << " for topic: " << name);
    ReturnCode_t result = topic_information->type_support->register_type(participant_, original_type);
    if (result != RETCODE_OK && result != RETCODE_PRECONDITION_NOT_MET) {
        LOG_ERROR("Type registration failed for " << original_type << " (topic: " << name << "), return code: " << (int)result);
        return nullptr;
    }
    
    if (result == RETCODE_PRECONDITION_NOT_MET) {
        LOG_DEBUG("Type already registered: " << original_type << " for topic: " << name);
    } else {
        LOG_DEBUG("Successfully registered type: " << original_type << " for topic: " << name);
    }
    
    LOG_INFO("Successfully created topic information for ROS message: " << name);
    return topic_information;
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
