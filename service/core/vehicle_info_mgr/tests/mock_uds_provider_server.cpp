#include <iostream>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <thread>
#include <chrono>
#include <filesystem>
#include <cstring>

int main() {
    std::string socket_path = "/var/tmp/vissv2/provider.sock";

    std::string uds_dir = std::filesystem::path(socket_path).parent_path().string();
    if (!std::filesystem::exists(uds_dir)) {
        std::filesystem::create_directories(uds_dir);
    }

    if (std::filesystem::exists(socket_path)) {
        std::filesystem::remove(socket_path);
    }

    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Failed to create socket.\n";
        return 1;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path.c_str(), sizeof(addr.sun_path) - 1);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Bind failed.\n";
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 5) < 0) {
        std::cerr << "Listen failed.\n";
        close(server_fd);
        return 1;
    }

    std::cout << "Mock Provider listening on " << socket_path << "...\n";

    while (true) {
        int client_fd = accept(server_fd, nullptr, nullptr);
        if (client_fd < 0) {
            std::cerr << "Accept failed.\n";
            continue;
        }

        std::cout << "Client connected.\n";

        char buffer[1024];
        ssize_t bytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes > 0) {
            buffer[bytes] = '\0';
            std::cout << "Received request: " << buffer << "\n";
        }

        int count = 0;
        while (true) {
            std::string response = "{\"path\": \"Vehicle.Speed\", \"value\": " + std::to_string(60 + count % 10) + ", \"timestamp\": 123456789}\n";
            if (send(client_fd, response.c_str(), response.length(), MSG_NOSIGNAL) < 0) {
                std::cout << "Client disconnected.\n";
                break;
            }
            std::cout << "Sent: " << response;
            count++;
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
        close(client_fd);
    }

    close(server_fd);
    return 0;
}
