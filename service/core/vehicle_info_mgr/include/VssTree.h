#ifndef VSS_TREE_H
#define VSS_TREE_H

#include <string>
#include <vector>
#include <cstdint>
#include <memory>
#include <map>
#include "rapidjson/document.h"
#include "Types.h"

namespace viss {

// Forward declaration
struct Node_t;

enum class NodeTypes : uint8_t {
    SENSOR = 1,
    ACTUATOR = 2,
    ATTRIBUTE = 3,
    BRANCH = 4,
    STRUCT = 5,
    PROPERTY = 6,
};

struct Node_t {
    std::string Name;
    NodeTypes NodeType;
    std::string Uuid;
    std::string Description;
    std::string Datatype;
    std::string Min;
    std::string Max;
    std::string Unit;
    uint8_t Allowed;
    std::vector<std::string> AllowedDef;
    std::string DefaultValue;
    uint8_t Validate;
    uint8_t Children;
    Node_t* Parent;
    std::map<std::string, std::shared_ptr<Node_t>> Child;

    Node_t() : Parent(nullptr), Allowed(0), Validate(0), Children(0) {}

    void resolvePathPattern(const std::string& parent_path, const std::string& pattern, std::vector<std::string>& out) {
        // Base case for recursion: if the pattern is empty, we have arrived at a destination node.
        if (pattern.empty()) {
            if (this->NodeType == NodeTypes::BRANCH) {
                // If it's a branch, we find all its children by initiating a wildcard search.
                this->resolvePathPattern(parent_path, "*", out);
            } else {
                // If it's not a branch, it's a valid end-node. Add its full path.
                out.push_back(parent_path);
            }
            return;
        }

        // --- Recursive step ---
        size_t dot_pos = pattern.find('.');
        std::string segment = pattern.substr(0, dot_pos);
        std::string remaining_pattern = (dot_pos == std::string::npos) ? "" : pattern.substr(dot_pos + 1);

        if (segment == "*") {
            for (auto const& [key, val] : Child) {
                std::string current_path = parent_path.empty() ? val->Name : parent_path + "." + val->Name;
                val->resolvePathPattern(current_path, remaining_pattern, out);
            }
        } else {
            auto it = Child.find(segment);
            if (it != Child.end()) {
                std::string current_path = parent_path.empty() ? it->first : parent_path + "." + it->first;
                it->second->resolvePathPattern(current_path, remaining_pattern, out);
            }
        }
    }

public:

    std::string getTypeAsString() const {
        switch (NodeType) {
            case NodeTypes::SENSOR: return "sensor";
            case NodeTypes::ACTUATOR: return "actuator";
            case NodeTypes::ATTRIBUTE: return "attribute";
            case NodeTypes::BRANCH: return "branch";
            case NodeTypes::STRUCT: return "struct";
            case NodeTypes::PROPERTY: return "property";
            default: return "unknown";
        }
    }
};

struct HimTree {
    std::string RootName;
    std::shared_ptr<Node_t> Handle;
    std::string TreeType;
    std::string Domain;
    std::string Version;
    std::string FileName;
    std::string Description;
};

struct SearchData_t {
    std::string NodePath;
    Node_t* NodeHandle;
};

class VSSTree {
public:
    explicit VSSTree(const std::string& him_file_path);
    bool isValid() const;
    viss::Node_t* findNode(const std::string& path);
    viss::Node_t* findRootNode(const std::string& path, std::string& out_found_root_name);
    int searchNodes(const std::string& searchPath, Node_t* rootNode, std::vector<SearchData_t>& searchData, bool anyDepth, bool leafNodesOnly);
    std::vector<std::string> getPaths(const std::string& path, const rapidjson::Document* filter);
    rapidjson::Document getDataTypeTemplate(const std::string& path);

private:
    rapidjson::Value createTemplateRecursive(Node_t* node, rapidjson::Document::AllocatorType& allocator);
    // For binary parsing
    template<typename T>
    bool read_value(std::ifstream& file, T& value);
    std::string read_string(std::ifstream& file);
    bool populate_node_from_stream(Node_t* node, std::ifstream& ifs);
    std::shared_ptr<Node_t> traverse_and_read(std::ifstream& ifs, Node_t* parent);

    // For JSON parsing
    std::shared_ptr<Node_t> parse_json_node(const rapidjson::Value& j, Node_t* parent, const std::string& node_name);
    NodeTypes string_to_nodetype(const std::string& type_str);

    std::shared_ptr<Node_t> VSSReadTree(const std::string& filename);
    std::string him_path_;
    std::map<std::string, viss::HimTree> forest_;
    bool is_valid_;
    std::multimap<std::string, Node_t*> type_expansion_map_;
};

} // namespace viss

#endif // VSS_TREE_H
