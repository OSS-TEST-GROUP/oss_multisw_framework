#ifndef GRPC_PROVIDER_H
#define GRPC_PROVIDER_H

#include "IProvider.h"
#include "ThreadSafeQueue.h"
#include <grpcpp/grpcpp.h>
#include "VISSv3.0.grpc.pb.h"
#include <thread>
#include <atomic>
#include <functional>

class GrpcProvider : public IProvider {
private:
    // Forward declaration of the nested Subscription class
    class Subscription;

public:
    GrpcProvider();
    // Constructor for testing with a mock stub and channel
    GrpcProvider(std::unique_ptr<grpcProtobufMessages::VISS::StubInterface> mock_stub, std::shared_ptr<grpc::ChannelInterface> mock_channel);
    virtual ~GrpcProvider();

    void start() override;
    void stop() override;
    std::string getName() const override;
    void setDataChannel(const std::shared_ptr<ThreadSafeQueue<VehicleData>>& channel) override;
    void setValue(const std::string& path, const std::any& value) override;
    void setVssTree(std::shared_ptr<viss::VSSTree> vss_tree) override;
    void subscribe(const std::string& path) override;
    void unsubscribe(const std::string& path) override;

private:
    void state_monitor_loop();
    void cleanup_thread_main();
    void cleanup_all_subscriptions();

    std::string m_name;
    std::string m_server_address;
    std::shared_ptr<grpc::ChannelInterface> m_channel;
    std::shared_ptr<ThreadSafeQueue<VehicleData>> m_data_channel;
    std::unique_ptr<grpcProtobufMessages::VISS::StubInterface> m_stub;

    grpc::CompletionQueue m_cq;
    std::thread m_state_monitor_thread;
    std::thread m_cleanup_thread;
    std::atomic<bool> m_monitoring_active{false};
    std::atomic<bool> m_is_stopping{false};

    // Mutex to protect access to subscription maps
    std::mutex m_subscription_mutex;
    // Main map for managing subscription objects
    std::map<std::string, std::shared_ptr<Subscription>> m_subscriptions;
    // Helper map for quick lookup from path to subscription ID
    std::map<std::string, std::string> m_path_to_subscription_id;
    
    // Queue for handling unexpected subscription terminations
    ThreadSafeQueue<std::string> m_cleanup_queue;

    // Private nested class to manage each subscription
    class Subscription {
    public:
        Subscription(
            std::string path,
            std::string id,
            std::unique_ptr<grpc::ClientContext> context,
            std::unique_ptr<grpc::ClientReaderInterface<grpcProtobufMessages::SubscribeStreamMessage>> stream,
            std::function<void(const VehicleData&)> data_handler,
            std::function<void(const std::string&)> cleanup_handler
        );
        ~Subscription();

        void start();
        void stop();
        //bool is_active() const;
        //std::string get_id() const;
        std::string get_path() const;


    private:
        void run();

        std::string m_path;
        std::string m_id;
        std::atomic<bool> m_is_active;

        std::unique_ptr<grpc::ClientContext> m_context;
        std::unique_ptr<grpc::ClientReaderInterface<grpcProtobufMessages::SubscribeStreamMessage>> m_stream;
        
        std::thread m_thread;

        std::function<void(const VehicleData&)> m_data_handler;
        std::function<void(const std::string&)> m_cleanup_handler;
    };
};

#endif // GRPC_PROVIDER_H
