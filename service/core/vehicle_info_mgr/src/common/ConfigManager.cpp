#include "ConfigManager.h"
#include "Utils.h"
#include "Logger.h"

ConfigManager& ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

void ConfigManager::initializeFromTree(std::shared_ptr<viss::VSSTree> tree) {
    if (!tree) {
        LOG_WARN << "[ConfigManager] Cannot initialize from null VSS tree.";
        return;
    }

    // 1. Load Supported Protocols
    auto protocol_node = tree->findNode("Server.Support.Protocol");
    if (protocol_node && !protocol_node->DefaultValue.empty()) {
        supported_protocols_ = parse_json_array_string(protocol_node->DefaultValue);
        LOG_INFO << "[ConfigManager] Loaded supported protocols: " << protocol_node->DefaultValue;
    }

    // 2. Load HTTP Config
    auto http_port_node = tree->findNode("Server.Config.Protocol.Http.Primary.PortNum");
    if (http_port_node && !http_port_node->DefaultValue.empty()) {
        http.port = std::stoi(http_port_node->DefaultValue);
        LOG_INFO << "[ConfigManager] HTTP Port: " << http.port;
    }
    auto http_cert_node = tree->findNode("Server.Config.Protocol.Http.Primary.CertPath");
    if (http_cert_node && !http_cert_node->DefaultValue.empty()) {
        http.certPath = http_cert_node->DefaultValue;
    }
    auto http_key_node = tree->findNode("Server.Config.Protocol.Http.Primary.KeyPath");
    if (http_key_node && !http_key_node->DefaultValue.empty()) {
        http.keyPath = http_key_node->DefaultValue;
    }

    // 3. Load MQTT Config
    auto mqtt_port_node = tree->findNode("Server.Config.Protocol.Mqtt.PortNum");
    if (mqtt_port_node && !mqtt_port_node->DefaultValue.empty()) {
        mqtt.port = std::stoi(mqtt_port_node->DefaultValue);
        LOG_INFO << "[ConfigManager] MQTT Port: " << mqtt.port;
    }
    auto mqtt_topic_node = tree->findNode("Server.Config.Protocol.Mqtt.Primary.Topic");
    if (mqtt_topic_node && !mqtt_topic_node->DefaultValue.empty()) {
        mqtt.primaryTopic = mqtt_topic_node->DefaultValue;
        LOG_INFO << "[ConfigManager] MQTT Topic: " << mqtt.primaryTopic;
    }

    // 4. Load WebSocket Config
    auto ws_port_node = tree->findNode("Server.Config.Protocol.Websocket.Primary.PortNum");
    if (ws_port_node && !ws_port_node->DefaultValue.empty()) {
        websocket.port = std::stoi(ws_port_node->DefaultValue);
        LOG_INFO << "[ConfigManager] WebSocket Port: " << websocket.port;
    }
    auto ws_cert_node = tree->findNode("Server.Config.Protocol.Websocket.Primary.CertPath");
    if (ws_cert_node && !ws_cert_node->DefaultValue.empty()) {
        websocket.certPath = ws_cert_node->DefaultValue;
    }
    auto ws_key_node = tree->findNode("Server.Config.Protocol.Websocket.Primary.KeyPath");
    if (ws_key_node && !ws_key_node->DefaultValue.empty()) {
        websocket.keyPath = ws_key_node->DefaultValue;
    }

    // 5. Load gRPC Config
    auto grpc_port_node = tree->findNode("Server.Config.Protocol.Grpc.Protobuf.PortNum");
    if (grpc_port_node && !grpc_port_node->DefaultValue.empty()) {
        grpc.port = std::stoi(grpc_port_node->DefaultValue);
        LOG_INFO << "[ConfigManager] gRPC Port: " << grpc.port;
    }

    // 6. Load DDS Config
    auto dds_domain_node = tree->findNode("Server.Config.Protocol.Dds.DomainId");
    if (dds_domain_node && !dds_domain_node->DefaultValue.empty()) {
        dds.domainId = std::stoul(dds_domain_node->DefaultValue);
        LOG_INFO << "[ConfigManager] DDS Domain ID: " << dds.domainId;
    }
    auto dds_prefix_node = tree->findNode("Server.Config.Protocol.Dds.TopicPrefix");
    if (dds_prefix_node && !dds_prefix_node->DefaultValue.empty()) {
        dds.topicPrefix = dds_prefix_node->DefaultValue;
        LOG_INFO << "[ConfigManager] DDS Topic Prefix: " << dds.topicPrefix;
    }

    // 7. Load UDS Config
    auto uds_path_node = tree->findNode("Server.Config.Protocol.Uds.Path");
    if (uds_path_node && !uds_path_node->DefaultValue.empty()) {
        uds.path = uds_path_node->DefaultValue;
        LOG_INFO << "[ConfigManager] UDS Path: " << uds.path;
    }

    // 8. Load Provider Config
    auto provider_protocols_node = tree->findNode("Server.Config.Provider.SupportedProtocols");
    if (provider_protocols_node && !provider_protocols_node->DefaultValue.empty()) {
        // Use parse_json_array_string for string[] types
        provider.supportedProtocols = parse_json_array_string(provider_protocols_node->DefaultValue);
        LOG_INFO << "[ConfigManager] Provider Supported Protocols: " << provider_protocols_node->DefaultValue;
    }

    auto provider_grpc_node = tree->findNode("Server.Config.Provider.Grpc.Address");
    if (provider_grpc_node && !provider_grpc_node->DefaultValue.empty()) {
        provider.grpcAddress = provider_grpc_node->DefaultValue;
        LOG_INFO << "[ConfigManager] Provider gRPC Address: " << provider.grpcAddress;
    }

    auto provider_uds_node = tree->findNode("Server.Config.Provider.Uds.Path");
    if (provider_uds_node && !provider_uds_node->DefaultValue.empty()) {
        provider.udsPath = provider_uds_node->DefaultValue;
        LOG_INFO << "[ConfigManager] Provider UDS Path: " << provider.udsPath;
    }
}
