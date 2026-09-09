#include <gtest/gtest.h>
#include "UdsProvider.h"
#include "ThreadSafeQueue.h"
#include "ConfigManager.h"
#include "VssTree.h"
#include "Utils.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include <string>

class UdsProviderTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_socket_path = "/tmp/test_uds_provider_" + std::to_string(getpid()) + "_" + std::to_string(rand() % 10000) + ".sock";
        unlink(test_socket_path.c_str());

        // Set config to use test socket path
        ConfigManager::getInstance().provider.udsPath = test_socket_path;

        data_queue = std::make_shared<ThreadSafeQueue<VehicleData>>();
    }

    void TearDown() override {
        stopMockServer();
        unlink(test_socket_path.c_str());
    }

    void startMockServer(std::function<void(int client_fd)> client_handler) {
        server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
        ASSERT_GE(server_fd, 0);

        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, test_socket_path.c_str(), sizeof(addr.sun_path) - 1);

        ASSERT_EQ(bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)), 0);
        ASSERT_EQ(listen(server_fd, 5), 0);

        server_running = true;
        server_thread = std::thread([this, client_handler]() {
            while (server_running) {
                fd_set read_fds;
                FD_ZERO(&read_fds);
                FD_SET(server_fd, &read_fds);

                struct timeval tv;
                tv.tv_sec = 0;
                tv.tv_usec = 100000; // 100ms

                int ret = select(server_fd + 1, &read_fds, nullptr, nullptr, &tv);
                if (ret > 0 && FD_ISSET(server_fd, &read_fds)) {
                    int client_sock = accept(server_fd, nullptr, nullptr);
                    if (client_sock >= 0) {
                        client_handler(client_sock);
                        close(client_sock);
                    }
                }
            }
        });
    }

    void stopMockServer() {
        server_running = false;
        if (server_thread.joinable()) {
            server_thread.join();
        }
        if (server_fd >= 0) {
            close(server_fd);
            server_fd = -1;
        }
    }

    std::string test_socket_path;
    std::shared_ptr<ThreadSafeQueue<VehicleData>> data_queue;
    int server_fd = -1;
    std::atomic<bool> server_running{false};
    std::thread server_thread;
};

TEST_F(UdsProviderTest, BasicGettersAndSetters) {
    UdsProvider provider;
    EXPECT_EQ(provider.getName(), "UdsProvider");

    provider.setDataChannel(data_queue);
    provider.setValue("Vehicle.Speed", 100);

    auto vss_tree = std::make_shared<viss::VSSTree>("../config/viss.him");
    provider.setVssTree(vss_tree);

    // Stop when not started should be safe
    provider.stop();
}

TEST_F(UdsProviderTest, FallbackSocketPathWhenConfigEmpty) {
    ConfigManager::getInstance().provider.udsPath = "";
    UdsProvider provider;
    EXPECT_EQ(provider.getName(), "UdsProvider");
    provider.stop();
}

TEST_F(UdsProviderTest, ReceivesVariousDataTypes) {
    startMockServer([](int client_sock) {
        // Read subscription message
        char buf[256];
        ssize_t n = recv(client_sock, buf, sizeof(buf) - 1, 0);
        (void)n;

        // Send string value with int64 timestamp
        std::string msg1 = "{\"path\":\"Vehicle.Speed\",\"value\":\"120\",\"timestamp\":1600000000}\n";
        send(client_sock, msg1.c_str(), msg1.length(), 0);

        // Send int value with string timestamp
        std::string msg2 = "{\"path\":\"Vehicle.Powertrain.Transmission.CurrentGear\",\"value\":3,\"timestamp\":\"1600000001\"}\n";
        send(client_sock, msg2.c_str(), msg2.length(), 0);

        // Send double value with invalid string timestamp (fallback to 0)
        std::string msg3 = "{\"path\":\"Vehicle.CurrentLocation.Latitude\",\"value\":37.5665,\"timestamp\":\"invalid_ts\"}\n";
        send(client_sock, msg3.c_str(), msg3.length(), 0);

        // Send bool value
        std::string msg4 = "{\"path\":\"Vehicle.Cabin.Door.Row1.Left.IsOpen\",\"value\":true,\"timestamp\":1600000002}\n";
        send(client_sock, msg4.c_str(), msg4.length(), 0);

        // Send bool false value
        std::string msg5 = "{\"path\":\"Vehicle.Cabin.Door.Row1.Right.IsOpen\",\"value\":false,\"timestamp\":1600000003}\n";
        send(client_sock, msg5.c_str(), msg5.length(), 0);

        // Send invalid JSON, non-object JSON, and missing fields
        std::string msg6 = "not_a_json\n";
        send(client_sock, msg6.c_str(), msg6.length(), 0);

        std::string msg7 = "[1, 2, 3]\n";
        send(client_sock, msg7.c_str(), msg7.length(), 0);

        std::string msg8 = "{\"path\":\"Vehicle.MissingValue\"}\n";
        send(client_sock, msg8.c_str(), msg8.length(), 0);

        // Empty line
        std::string msg9 = "\r\n";
        send(client_sock, msg9.c_str(), msg9.length(), 0);

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    });

    UdsProvider provider;
    provider.setDataChannel(data_queue);
    provider.start();

    VehicleData d1, d2, d3, d4, d5;
    EXPECT_TRUE(data_queue->wait_and_pop(d1));
    EXPECT_EQ(d1.path, "Vehicle.Speed");
    EXPECT_EQ(any_to_string(d1.value), "120");
    EXPECT_EQ(d1.timestamp, 1600000000);

    EXPECT_TRUE(data_queue->wait_and_pop(d2));
    EXPECT_EQ(d2.path, "Vehicle.Powertrain.Transmission.CurrentGear");
    EXPECT_EQ(any_to_string(d2.value), "3");
    EXPECT_EQ(d2.timestamp, 1600000001);

    EXPECT_TRUE(data_queue->wait_and_pop(d3));
    EXPECT_EQ(d3.path, "Vehicle.CurrentLocation.Latitude");
    EXPECT_EQ(d3.timestamp, 0);

    EXPECT_TRUE(data_queue->wait_and_pop(d4));
    EXPECT_EQ(d4.path, "Vehicle.Cabin.Door.Row1.Left.IsOpen");
    EXPECT_EQ(any_to_string(d4.value), "true");

    EXPECT_TRUE(data_queue->wait_and_pop(d5));
    EXPECT_EQ(d5.path, "Vehicle.Cabin.Door.Row1.Right.IsOpen");
    EXPECT_EQ(any_to_string(d5.value), "false");

    provider.stop();
}

TEST_F(UdsProviderTest, ReconnectsWhenServerNotAvailable) {
    UdsProvider provider;
    provider.setDataChannel(data_queue);
    // Start provider when server is NOT running
    provider.start();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Now start the mock server
    startMockServer([](int client_sock) {
        char buf[256];
        recv(client_sock, buf, sizeof(buf) - 1, 0);

        std::string msg = "{\"path\":\"Vehicle.Speed\",\"value\":80,\"timestamp\":1600000010}\n";
        send(client_sock, msg.c_str(), msg.length(), 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    });

    VehicleData d;
    // Wait for reconnection and data reception
    EXPECT_TRUE(data_queue->wait_and_pop(d));
    EXPECT_EQ(d.path, "Vehicle.Speed");
    EXPECT_EQ(any_to_string(d.value), "80");

    provider.stop();
}

TEST_F(UdsProviderTest, HandlesServerDisconnectAndReconnect) {
    std::atomic<int> connection_count{0};
    startMockServer([&connection_count](int client_sock) {
        int count = ++connection_count;
        char buf[256];
        recv(client_sock, buf, sizeof(buf) - 1, 0);

        if (count == 1) {
            std::string msg = "{\"path\":\"Vehicle.Speed\",\"value\":50,\"timestamp\":100}\n";
            send(client_sock, msg.c_str(), msg.length(), 0);
            // Immediately close client socket to trigger EOF / disconnect
            close(client_sock);
        } else {
            std::string msg = "{\"path\":\"Vehicle.Speed\",\"value\":60,\"timestamp\":200}\n";
            send(client_sock, msg.c_str(), msg.length(), 0);
        }
    });

    UdsProvider provider;
    provider.setDataChannel(data_queue);
    provider.start();

    VehicleData d1, d2;
    EXPECT_TRUE(data_queue->wait_and_pop(d1));
    EXPECT_EQ(any_to_string(d1.value), "50");

    EXPECT_TRUE(data_queue->wait_and_pop(d2));
    EXPECT_EQ(any_to_string(d2.value), "60");

    provider.stop();
}
