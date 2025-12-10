#include "VssTree.h"
#include "Utils.h"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <stdexcept>
#include <sstream>
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/error/en.h"
#include "Logger.h"


namespace viss {

template<typename T>
bool VSSTree::read_value(std::ifstream& file, T& value) {
    return file.read(reinterpret_cast<char*>(&value), sizeof(T)).good();
}

std::string VSSTree::read_string(std::ifstream& file) {
    uint8_t len;
    if (!this->read_value(file, len)) return "";
    if (len == 0) return "";
    std::string str(len, '\0');
    if (!file.read(&str[0], len)) return "";
    return str;
}

bool VSSTree::populate_node_from_stream(Node_t* node, std::ifstream& ifs) {
    // This function now mimics the C version's `populateNode` function more closely.
    // It assumes the stream is positioned correctly at the start of a node's data.
    node->Name = this->read_string(ifs);

    std::string type_str = this->read_string(ifs);
    if (type_str == "sensor") node->NodeType = NodeTypes::SENSOR;
    else if (type_str == "actuator") node->NodeType = NodeTypes::ACTUATOR;
    else if (type_str == "attribute") node->NodeType = NodeTypes::ATTRIBUTE;
    else if (type_str == "branch") node->NodeType = NodeTypes::BRANCH;
    else if (type_str == "struct") node->NodeType = NodeTypes::STRUCT;
    else if (type_str == "property") node->NodeType = NodeTypes::PROPERTY;
    else node->NodeType = NodeTypes::BRANCH; // Default case

    node->Uuid = this->read_string(ifs);
    
    // The C-code uses uint16_t for description length, so we handle it separately.
    uint16_t descrLen;
    if (!this->read_value(ifs, descrLen)) return false;
    if (descrLen > 0) {
        node->Description.resize(descrLen);
        if (!ifs.read(&node->Description[0], descrLen)) return false;
    }

    // Re-implementing with explicit length checks like in the C source
    node->Datatype = this->read_string(ifs);
    node->Min = this->read_string(ifs);
    node->Max = this->read_string(ifs);
    node->Unit = this->read_string(ifs);

    uint16_t allowedLen;
    if (!this->read_value(ifs, allowedLen)) return false;
    if (allowedLen > 0) {
        std::string allowed_buf(allowedLen, '\0');
        if (!ifs.read(&allowed_buf[0], allowedLen)) return false;
        // The C parser's logic for splitting this buffer is complex.
        // For now, we store the raw buffer, which might need further parsing if used.
        node->Allowed = 1; // Placeholder
        node->AllowedDef.push_back(allowed_buf);
    } else {
        node->Allowed = 0;
    }

    node->DefaultValue = this->read_string(ifs);
    
    uint8_t validateLen;
    if (!this->read_value(ifs, validateLen)) return false;
    if (validateLen > 0) {
        std::string validate_str(validateLen, '\0');
        if(!ifs.read(&validate_str[0], validateLen)) return false;
        node->Validate = 0;
        if (validate_str.find("write-only") != std::string::npos) node->Validate = 1;
        else if (validate_str.find("read-write") != std::string::npos) node->Validate = 2;
        if (validate_str.find("consent") != std::string::npos) node->Validate += 10;
    } else {
        node->Validate = 0;
    }

    if (!this->read_value(ifs, node->Children)) return false;

    if (node->Datatype.rfind("Types.", 0) == 0) {
        std::string type_key = node->Datatype;
        size_t pos = type_key.find('[');
        if (pos != std::string::npos) {
            type_key = type_key.substr(0, pos);
        }
        type_expansion_map_.insert({type_key, node});
    }
    
    return true;
}

std::shared_ptr<Node_t> VSSTree::traverse_and_read(std::ifstream& ifs, Node_t* parent) {
    auto this_node = std::make_shared<Node_t>();
    if (!this->populate_node_from_stream(this_node.get(), ifs)) {
        // Check for EOF, which is not an error but the end of recursion
        if (ifs.eof()) return nullptr; 
        // Otherwise, it's a parse error
        LOG_ERROR << "Failed to populate node.";
        return nullptr;
    }
    this_node->Parent = parent;

    for (uint8_t i = 0; i < this_node->Children; ++i) {
        auto child_node = this->traverse_and_read(ifs, this_node.get());
        if (child_node) {
            this_node->Child[child_node->Name] = child_node;
        } else {
            // This could indicate a corrupted file or an issue with the parsing logic
            LOG_ERROR << "Failed to read a child node for " << this_node->Name;
            // Depending on strictness, we could return nullptr here to abort all parsing
        }
    }
    return this_node;
}

// Forward declaration for the new recursive JSON parsing function
std::shared_ptr<Node_t> VSSTree::parse_json_node(const rapidjson::Value& j, Node_t* parent, const std::string& node_name) {
    auto node = std::make_shared<Node_t>();
    node->Parent = parent;
    node->Name = node_name;

    if (j.HasMember("type") && j["type"].IsString()) node->NodeType = string_to_nodetype(j["type"].GetString());
    if (j.HasMember("uuid") && j["uuid"].IsString()) node->Uuid = j["uuid"].GetString();
    if (j.HasMember("description") && j["description"].IsString()) node->Description = j["description"].GetString();
    if (j.HasMember("datatype") && j["datatype"].IsString()) node->Datatype = j["datatype"].GetString();
    if (j.HasMember("unit") && j["unit"].IsString()) node->Unit = j["unit"].GetString();
    
    // For attributes that can be non-string types, we need to handle them carefully.
    // RapidJSON doesn't have a simple .dump() like nlohmann, so we might skip non-string for simplicity for now.
    if (j.HasMember("min") && j["min"].IsString()) node->Min = j["min"].GetString();
    if (j.HasMember("max") && j["max"].IsString()) node->Max = j["max"].GetString();
    if (j.HasMember("default") && j["default"].IsString()) node->DefaultValue = j["default"].GetString();


    if (j.HasMember("allowed") && j["allowed"].IsArray()) {
        node->Allowed = 1;
        for (const auto& item : j["allowed"].GetArray()) {
            if (item.IsString()) {
                node->AllowedDef.push_back(item.GetString());
            }
        }
    } else {
        node->Allowed = 0;
    }

    node->Validate = 0;

    if (node->Datatype.rfind("Types.", 0) == 0) {
        std::string type_key = node->Datatype;
        size_t pos = type_key.find('[');
        if (pos != std::string::npos) {
            type_key = type_key.substr(0, pos);
        }
        type_expansion_map_.insert({type_key, node.get()});
    }

    if (j.HasMember("children") && j["children"].IsObject()) {
        const auto& children = j["children"];
        for (auto it = children.MemberBegin(); it != children.MemberEnd(); ++it) {
            auto child_node = parse_json_node(it->value, node.get(), it->name.GetString());
            if (child_node) {
                node->Child[it->name.GetString()] = child_node;
            }
        }
        node->Children = node->Child.size();
    } else {
        node->Children = 0;
    }

    return node;
}

NodeTypes VSSTree::string_to_nodetype(const std::string& type_str) {
    if (type_str == "sensor") return NodeTypes::SENSOR;
    if (type_str == "actuator") return NodeTypes::ACTUATOR;
    if (type_str == "attribute") return NodeTypes::ATTRIBUTE;
    if (type_str == "branch") return NodeTypes::BRANCH;
    if (type_str == "struct") return NodeTypes::STRUCT;
    if (type_str == "property") return NodeTypes::PROPERTY;
    return NodeTypes::BRANCH; // Default
}


// Implementation of VSSReadTree, now handles both binary and JSON
std::shared_ptr<Node_t> VSSTree::VSSReadTree(const std::string& filename) {
    std::filesystem::path file_path(filename);
    if (file_path.extension() == ".json") {
        std::ifstream file(filename);
        if (!file.is_open()) {
            LOG_ERROR << "Error opening JSON file: " << filename;
            return nullptr;
        }
        rapidjson::IStreamWrapper isw(file);
        rapidjson::Document d;
        d.ParseStream(isw);

        if (d.HasParseError()) {
            LOG_ERROR << "Error parsing JSON file: " << filename << " - " 
                      << rapidjson::GetParseError_En(d.GetParseError()) 
                      << " at offset " << d.GetErrorOffset();
            return nullptr;
        }

        if (!d.IsObject() || d.MemberCount() != 1) {
            LOG_ERROR << "Error: JSON root is not a single object in " << filename;
            return nullptr;
        }

        auto root_it = d.MemberBegin();
        return parse_json_node(root_it->value, nullptr, root_it->name.GetString());
    } else {
        // Existing binary format reading logic
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            return nullptr;
        }
        if (file.peek() == EOF) {
            return std::make_shared<Node_t>();
        }
        return this->traverse_and_read(file, nullptr);
    }
}

// Dummy implementation for VSSsearchNodes as it's not the focus
int VSSTree::searchNodes(const std::string& searchPath, Node_t* rootNode, std::vector<SearchData_t>& searchData, bool anyDepth, bool leafNodesOnly) {
    // This function needs a proper implementation based on path matching logic.
    // For now, we'll leave it empty to allow compilation.
    return 0;
}


VSSTree::VSSTree(const std::string& him_file_path) : him_path_(him_file_path), is_valid_(false) {
    std::filesystem::path him_fs_path(him_file_path);
    std::ifstream file(him_fs_path);
    if (!file.is_open()) {
        LOG_ERROR << "Error reading " << him_path_ 
                  << " (resolved to: " << std::filesystem::absolute(him_fs_path) << ")";
        return;
    }

    std::filesystem::path himDirectory = std::filesystem::path(him_path_).parent_path();
    std::string line;
    HimTree* current_tree = nullptr;
    std::string current_root_name;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }

        size_t rootIndex = line.find("HIM.");
        if (rootIndex != std::string::npos && line.back() == ':') {
            current_root_name = line.substr(rootIndex + 4, line.length() - (rootIndex + 4) - 1);
            forest_[current_root_name] = {}; // Create a new entry in the map
            current_tree = &forest_[current_root_name];
            current_tree->RootName = current_root_name;
        } else if (current_tree != nullptr) {
            size_t colon_pos = line.find(':');
            if (colon_pos != std::string::npos) {
                std::string key = trim(line.substr(0, colon_pos));
                std::string value = trim(line.substr(colon_pos + 1));
                
                if (key == "local") {
                    std::filesystem::path fullPath = himDirectory / value;
                    current_tree->FileName = fullPath.string();
                    current_tree->Handle = this->VSSReadTree(fullPath.string());
                    if (current_tree->Handle == nullptr) {
                        LOG_ERROR << "Error parsing " << fullPath.string();
                        return; // Abort initialization
                    }
                } else if (key == "type") {
                    current_tree->TreeType = value;
                } else if (key == "domain") {
                    current_tree->Domain = value;
                } else if (key == "version") {
                    current_tree->Version = value;
                } else if (key == "description") {
                    current_tree->Description = value;
                }
            }
        }
    }

    // --- 2nd pass: Expand types ---
    for (const auto& [type_path, node_to_expand] : type_expansion_map_) {
        Node_t* type_node = findNode(type_path);
        if (type_node) {
            for (const auto& [key, val] : type_node->Child) {
                if (node_to_expand->Child.find(key) == node_to_expand->Child.end()) {
                    // Share the node from the 'Types' tree.
                    // Note: The Parent pointer of the shared node will still point to its original parent in the 'Types' tree.
                    node_to_expand->Child[key] = val;
                }
            }
            node_to_expand->Children = node_to_expand->Child.size();
        }
    }
    
    is_valid_ = true;
}

bool VSSTree::isValid() const {
    return is_valid_;
}

viss::Node_t* VSSTree::findNode(const std::string& path) {
    std::string root_name;
    Node_t* root = findRootNode(path, root_name);
    if (!root) {
        return nullptr;
    }

    if (path == root_name) {
        return root;
    }

    std::string remaining_path = path.substr(root_name.length() + 1);
    std::stringstream ss(remaining_path);
    std::string segment;
    Node_t* current_node = root;

    while (std::getline(ss, segment, '.')) {
        auto it = current_node->Child.find(segment);
        if (it != current_node->Child.end()) {
            current_node = it->second.get();
        } else {
            return nullptr; // Not found
        }
    }
    return current_node;
}

viss::Node_t* VSSTree::findRootNode(const std::string& path, std::string& out_found_root_name) {
    out_found_root_name.clear();
    size_t first_dot = path.find('.');
    std::string root_key = (first_dot == std::string::npos) ? path : path.substr(0, first_dot);

    auto it = forest_.find(root_key);
    if (it != forest_.end()) {
        out_found_root_name = it->first;
        return it->second.Handle.get();
    }

    LOG_ERROR << "Failed to find root node for key: " << root_key;
    return nullptr;
}

std::vector<std::string> VSSTree::getPaths(const std::string& path, const rapidjson::Document* filter) {
    std::vector<std::string> resulting_paths;
    
    Node_t* base_node = findNode(path);
    if (!base_node) {
        return resulting_paths; // Base path does not exist, return empty
    }

    if (filter && filter->IsObject() && filter->HasMember("filter")) {
        const auto& filter_val = (*filter)["filter"];
        const rapidjson::Value* expressions = &filter_val;
        rapidjson::Document doc;
        if (!filter_val.IsArray()) {
            doc.SetArray();
            doc.PushBack(rapidjson::Value(filter_val, doc.GetAllocator()), doc.GetAllocator());
            expressions = &doc;
        }

        for (const auto& exp : expressions->GetArray()) {
            if (exp.IsObject() && exp.HasMember("variant") && exp["variant"].IsString()) {
                if (strcmp(exp["variant"].GetString(), "paths") == 0) {
                    if (exp.HasMember("parameter")) {
                        const auto& param = exp["parameter"];
                        if (param.IsArray()) {
                            for (const auto& p : param.GetArray()) {
                                if (p.IsString()) {
                                    base_node->resolvePathPattern(path, p.GetString(), resulting_paths);
                                }
                            }
                        } else if (param.IsString()) {
                            base_node->resolvePathPattern(path, param.GetString(), resulting_paths);
                        }
                    }
                    // When 'paths' filter is present, it dictates the result.
                    return resulting_paths;
                }
            }
        }
    }

    // No 'paths' filter, so just return the valid base path
    resulting_paths.push_back(path);
    return resulting_paths;
}

rapidjson::Document VSSTree::getDataTypeTemplate(const std::string& path) {
    Node_t* node = findNode(path);
    if (!node) {
        throw std::runtime_error("Node not found: " + path);
    }
    rapidjson::Document doc;
    doc.CopyFrom(createTemplateRecursive(node, doc.GetAllocator()), doc.GetAllocator());
    return doc;
}

rapidjson::Value VSSTree::createTemplateRecursive(Node_t* node, rapidjson::Document::AllocatorType& allocator) {
    if (!node) {
        return rapidjson::Value(rapidjson::kNullType);
    }

    std::string datatype = node->Datatype;
    bool is_array = false;
    size_t bracket_pos = datatype.find('[');
    if (bracket_pos != std::string::npos) {
        is_array = true;
        datatype = datatype.substr(0, bracket_pos);
    }

    rapidjson::Value result;
    if (datatype.rfind("Types.", 0) == 0) {
        if (node->Child.empty()) {
            LOG_ERROR << "Error: Node " << node->Name << " with datatype " << node->Datatype << " has no children.";
            result.SetNull();
        } else {
            result.SetObject();
            for (const auto& [key, child_node] : node->Child) {
                rapidjson::Value key_val(key.c_str(), allocator);
                result.AddMember(key_val, createTemplateRecursive(child_node.get(), allocator), allocator);
            }
        }
    } else {
        if (!node->Child.empty()) {
             result.SetObject();
             for (const auto& [key, child_node] : node->Child) {
                rapidjson::Value key_val(key.c_str(), allocator);
                result.AddMember(key_val, createTemplateRecursive(child_node.get(), allocator), allocator);
            }
        } else {
            result.SetNull();
        }
    }

    if (is_array) {
        rapidjson::Value array_val(rapidjson::kArrayType);
        array_val.PushBack(result, allocator);
        return array_val;
    }

    return result;
}

rapidjson::Document VSSTree::getMetadata(const std::string& path, int depth) {
    LOG_INFO << "Getting metadata for path: " << path << " with depth: " << depth;
    rapidjson::Document doc;
    doc.SetObject();
    Node_t* node = findNode(path);
    if (!node) {
        LOG_WARN << "Node not found for path: " << path;
        // The ServiceManager will check if the doc is empty.
        return doc;
    }
    
    rapidjson::Value node_val = getMetadataRecursive(node, depth, 1, doc.GetAllocator());
    rapidjson::Value key(node->Name.c_str(), doc.GetAllocator());
    doc.AddMember(key, node_val, doc.GetAllocator());

    #ifdef DEBUG_LOGGING
        rapidjson::StringBuffer buffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);
        LOG_DEBUG << "Generated metadata: " << buffer.GetString();
    #endif

    return doc;
}

rapidjson::Value VSSTree::getMetadataRecursive(Node_t* node, int max_depth, int current_depth, rapidjson::Document::AllocatorType& allocator) {
    LOG_DEBUG << "Recursively getting metadata for node: " << node->Name << " at depth: " << current_depth;
    rapidjson::Value result(rapidjson::kObjectType);
    
    // Add all relevant metadata fields from the Node_t struct
    result.AddMember("type", rapidjson::Value(node->getTypeAsString().c_str(), allocator), allocator);
    if (!node->Description.empty()) {
        result.AddMember("description", rapidjson::Value(node->Description.c_str(), allocator), allocator);
    }
    if (!node->Datatype.empty()) {
        result.AddMember("datatype", rapidjson::Value(node->Datatype.c_str(), allocator), allocator);
    }
    if (!node->Unit.empty()) {
        result.AddMember("unit", rapidjson::Value(node->Unit.c_str(), allocator), allocator);
    }
    if (!node->Min.empty()) {
        result.AddMember("min", rapidjson::Value(node->Min.c_str(), allocator), allocator);
    }
    if (!node->Max.empty()) {
        result.AddMember("max", rapidjson::Value(node->Max.c_str(), allocator), allocator);
    }
    if (node->Allowed > 0 && !node->AllowedDef.empty()) {
         rapidjson::Value allowed_array(rapidjson::kArrayType);
         for(const auto& val : node->AllowedDef) {
            allowed_array.PushBack(rapidjson::Value(val.c_str(), allocator), allocator);
         }
        result.AddMember("allowed", allowed_array, allocator);
    }
    if (!node->DefaultValue.empty()) {
        result.AddMember("default", rapidjson::Value(node->DefaultValue.c_str(), allocator), allocator);
    }

    // Recurse through children if depth allows (depth 0 means infinite)
    if (!node->Child.empty() && (max_depth == 0 || current_depth < max_depth)) {
        rapidjson::Value children(rapidjson::kObjectType);
        for (const auto& [key, child_node] : node->Child) {
            rapidjson::Value key_val(key.c_str(), allocator);
            children.AddMember(key_val, getMetadataRecursive(child_node.get(), max_depth, current_depth + 1, allocator), allocator);
        }
        result.AddMember("children", children, allocator);
    }

    return result;
}


} // namespace viss
