#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include "VssTree.h"

// Protocol-specific configuration structures
struct HttpConfig {
    int port = 7443;
    std::string certPath;
    std::string keyPath;
};

struct MqttConfig {
    int port = 8883;
    std::string primaryTopic = "viss/primary";
    std::string protobufTopic = "viss/protobuf";
};

struct WebSocketConfig {
    int port = 6443;
    std::string certPath;
    std::string keyPath;
};

struct GrpcConfig {
    int port = 5443;
    std::string providerAddr;
};

struct DdsConfig {
    uint32_t domainId = 0;
    std::string topicPrefix = "viss";
};

struct UdsConfig {
    std::string path = "var/tmp/vissv3/udsMgr.sock";
};

struct ProviderConfig {
    std::vector<std::string> supportedProtocols = {"Grpc", "Uds"};
    std::string grpcAddress = "localhost:50051";
    std::string udsPath = "var/tmp/vissv3/provider.sock";
};

/**
 * Singleton class to manage system-wide configuration settings.
 * Organized by protocol.
 */
class ConfigManager {
public:
    static ConfigManager& getInstance();

    // Deleted copy constructor and assignment operator for Singleton
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    /**
     * Initializes configuration values from a loaded VSS tree.
     */
    void initializeFromTree(std::shared_ptr<viss::VSSTree> tree);

    // Global settings
    std::string getHimPath() const { return him_path_; }
    void setHimPath(const std::string& path) { him_path_ = path; }

    const std::vector<std::string>& getSupportedProtocols() const { return supported_protocols_; }
    void setSupportedProtocols(const std::vector<std::string>& protocols) { supported_protocols_ = protocols; }

    std::string getHimDirectory() const { return him_directory_; }
    void setHimDirectory(const std::string& dir) { him_directory_ = dir; }

    // Protocol grouped settings
    HttpConfig http;
    MqttConfig mqtt;
    WebSocketConfig websocket;
    GrpcConfig grpc;
    DdsConfig dds;
    UdsConfig uds;
    ProviderConfig provider;

private:
    ConfigManager() = default;

    std::string him_path_;
    std::string him_directory_;
    std::vector<std::string> supported_protocols_;
};

#endif // CONFIG_MANAGER_H
