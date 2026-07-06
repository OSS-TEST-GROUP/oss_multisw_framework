#include "UdsProvider.h"
#include "Logger.h"
#include "ConfigManager.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <iostream>
#include <chrono>
#include "rapidjson/document.h"

UdsProvider::UdsProvider() : active(false), client_fd(-1) {
    socket_path = ConfigManager::getInstance().provider.udsPath;
    if (socket_path.empty()) {
        // Fallback or warning if config doesn't have it
        LOG_WARN << "[UdsProvider] Provider UDS Path is empty in config! Using default: /var/tmp/vissv2/provider.sock";
        socket_path = "/var/tmp/vissv2/provider.sock";
    }
}

UdsProvider::~UdsProvider() {
    stop();
}

void UdsProvider::start() {
    active = true;
    provider_thread = std::thread(&UdsProvider::runLoop, this);
    LOG_INFO << "UdsProvider started. Target UDS path: " << socket_path;
}

void UdsProvider::stop() {
    if (!active.exchange(false)) {
        return; // Already stopped
    }
    LOG_INFO << "UdsProvider::stop() called.";
    if (client_fd >= 0) {
        close(client_fd);
        client_fd = -1;
    }
    if (provider_thread.joinable()) {
        provider_thread.join();
    }
    LOG_INFO << "UdsProvider::stop() completed.";
}

std::string UdsProvider::getName() const {
    return "UdsProvider";
}

void UdsProvider::setDataChannel(const std::shared_ptr<ThreadSafeQueue<VehicleData>>& channel) {
    data_channel = channel;
}

void UdsProvider::setValue(const std::string& path, const std::any& value) {
    LOG_INFO << "UdsProvider: setValue called for path " << path;
    // For now, this provider only consumes data. 
    // Implementing set request forwarding to external provider via UDS would go here.
}

void UdsProvider::setVssTree(std::shared_ptr<viss::VSSTree> vss_tree) {
    vss_tree_ = vss_tree;
}

void UdsProvider::reconnectAndHandle() {
    client_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (client_fd < 0) {
        LOG_ERROR << "[UdsProvider] Failed to create socket";
        std::this_thread::sleep_for(std::chrono::seconds(2));
        return;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path.c_str(), sizeof(addr.sun_path) - 1);

    //LOG_INFO << "[UdsProvider] Connecting to UDS server at " << socket_path << "...";
    if (connect(client_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        //LOG_WARN << "[UdsProvider] Failed to connect. Server might be down. Retrying in 2 seconds...";
        close(client_fd);
        client_fd = -1;
        std::this_thread::sleep_for(std::chrono::seconds(2));
        return;
    }

    LOG_INFO << "[UdsProvider] Successfully connected to UDS server.";
    
    // Send subscription string with newline terminator for line-buffered servers
    std::string sub_msg = "Vehicle.*\n"; 
    if (send(client_fd, sub_msg.c_str(), sub_msg.length(), MSG_NOSIGNAL) < 0) {
        LOG_ERROR << "[UdsProvider] Failed to send subscription request.";
        close(client_fd);
        client_fd = -1;
        return;
    }
    LOG_INFO << "[UdsProvider] Sent subscription request: " << "Vehicle.*";

    // Read loop
    char buffer[4096];
    std::string data_buffer;

    while (active.load()) {
        ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            data_buffer += buffer;

            size_t pos = 0;
            while ((pos = data_buffer.find('\n')) != std::string::npos) {
                std::string line = data_buffer.substr(0, pos);
                data_buffer.erase(0, pos + 1);
                
                if (line.empty() || line == "\r") continue;

                rapidjson::Document doc;
                doc.Parse(line.c_str());
                if (!doc.HasParseError() && doc.IsObject()) {
                    if (doc.HasMember("path") && doc.HasMember("value") && doc.HasMember("timestamp")) {
                        VehicleData data;
                        data.path = doc["path"].GetString();
                        
                        // Parse value to string representation
                        if (doc["value"].IsString()) {
                            data.value = std::string(doc["value"].GetString());
                        } else if (doc["value"].IsInt()) {
                            data.value = std::to_string(doc["value"].GetInt());
                        } else if (doc["value"].IsDouble()) {
                            data.value = std::to_string(doc["value"].GetDouble());
                        } else if (doc["value"].IsBool()) {
                            data.value = std::string(doc["value"].GetBool() ? "true" : "false");
                        }
                        
                        // Parse timestamp
                        if (doc["timestamp"].IsInt64()) {
                            data.timestamp = doc["timestamp"].GetInt64();
                        } else if (doc["timestamp"].IsString()) {
                            try {
                                data.timestamp = std::stoll(doc["timestamp"].GetString());
                            } catch (...) {
                                data.timestamp = 0;
                            }
                        }

                        if (data_channel) {
                            data_channel->push(data);
                        }
                    } else {
                        LOG_WARN << "[UdsProvider] JSON missing required fields (path, value, timestamp): " << line;
                    }
                } else {
                    LOG_WARN << "[UdsProvider] Failed to parse JSON or not an object: " << line;
                }
            }
        } else if (bytes_read == 0) {
            LOG_INFO << "[UdsProvider] Connection closed by server.";
            break;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                continue;
            }
            LOG_ERROR << "[UdsProvider] Read error: " << strerror(errno);
            break;
        }
    }

    if (client_fd >= 0) {
        close(client_fd);
        client_fd = -1;
    }
}

void UdsProvider::runLoop() {
    while (active.load()) {
        reconnectAndHandle();
    }
}
