#include <iostream>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <thread>
#include <chrono>
#include <cstring>

int main() {
    std::string socket_path = "../../config/var/tmp/vissv3/udsMgr.sock";

    int client_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (client_fd < 0) {
        std::cerr << "Failed to create socket.\n";
        return 1;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path.c_str(), sizeof(addr.sun_path) - 1);

    if (connect(client_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        std::cerr << "Failed to connect to " << socket_path << "\n";
        close(client_fd);
        return 1;
    }

    std::cout << "Connected to VISS UDS Server at " << socket_path << "\n";

    std::string request = "{\"action\":\"get\", \"path\":\"Vehicle.Speed\", \"requestId\":\"test_uds_123\"}\n";
    if (send(client_fd, request.c_str(), request.length(), 0) < 0) {
        std::cerr << "Send failed.\n";
        close(client_fd);
        return 1;
    }
    std::cout << "Sent: " << request;

    char buffer[4096];
    ssize_t bytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes > 0) {
        buffer[bytes] = '\0';
        std::cout << "Received: " << buffer << "\n";
    }

    close(client_fd);
    return 0;
}
