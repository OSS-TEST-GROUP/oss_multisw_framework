#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <thread>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>

#include "transmitter/dds/DDSProtocol.cpp"
#include "transmitter/dds/DDSDynamicIDLParser.cpp"
#include "transmitter/MessageProtocol.cpp"
#include "transmitter/Message.cpp"
#include "diagnostic/VariantData.cpp"
#include "mock/MockLogger.hpp"

using ::testing::Return;
using ::testing::_;
using ::testing::A;
using ::testing::ReturnRef;
using ::testing::Values;
using ::testing::SetArgPointee;
using ::testing::SetArrayArgument;
using ::testing::SaveArg;
using ::testing::SaveArgPointee;
using ::testing::DoAll;
using ::testing::Args;
using ::testing::AllOf;
using ::testing::AtLeast;
using ::testing::Combine;

using namespace diagnostic_manager;

class DDSProtocolTest : public ::testing::Test {
protected:
    virtual void SetUp(){
        ins = new DDSProtocol;
        Mock_Logger = new MockLogger();
    }
    virtual void TearDown(){
        stopAllProcess();
        process_list.clear();
        delete ins;
        delete Mock_Logger;
        Mock_Logger = nullptr;
    }

    pid_t startProcess(char* args[]) {
        pid_t pid = fork();
        if (pid < 0) {
            return -1;
        } else if (pid == 0) {
            execv(args[0], args);
        }
        process_list.insert(pid);
        return pid;
    }
    int32_t stopProcess(pid_t pid) {
    if (pid <= 0) return -1;

    if (kill(pid, SIGTERM) == -1 && errno != ESRCH) {
        perror("kill failed");
        return -1;
    }

    int32_t status = 0;
    for (int i = 0; i < 20; ++i) {
        pid_t result = waitpid(pid, &status, WNOHANG);
        if (result == pid) break;
        if (result == -1 && errno == ECHILD) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (kill(pid, 0) == 0) {
        kill(pid, SIGKILL);
        waitpid(pid, &status, 0);
    }

    return status;
//        kill(pid, SIGTERM);
//
//        int32_t status;
//        waitpid(pid, &status, 0);
//
//        process_list.erase(pid);
//        return status;
    }
    void stopAllProcess() {
        for (auto iter : process_list) {
            stopProcess(iter);
        }
        process_list.clear();
    }
    std::set<pid_t> process_list;


    DDSProtocol* ins;
};

TEST_F(DDSProtocolTest, InitializeAndFinalize_TC001) {
    EXPECT_TRUE(ins->Initialize());
    ins->Finalize();
}

TEST_F(DDSProtocolTest, GetProtocolType_TC001) {
    EXPECT_EQ(MessageProtocolType::DDS, ins->GetProtocolType());
}

TEST_F(DDSProtocolTest, PublishMessage_TC001) {

}

TEST_F(DDSProtocolTest, SubscribeMessage_TC001) {
    constexpr uint32_t MAX_WAIT_COUNT = 100;
    EXPECT_TRUE(ins->Initialize());
    ins->SubscribeMessage("/HelloWorldTopic");

    char* args[] = {"test/HelloWorldPublisher", 0};
    pid_t pid = startProcess(args);
    if (pid == -1) {
        std::cerr << "Failed to start test publisher" << std::endl;
        FAIL();
    }

    uint32_t wait_count = 0;
    while (wait_count < MAX_WAIT_COUNT) {
        if (ins->discovered_topic_.size() > 0) {
            break;
        }

        wait_count++;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    EXPECT_EQ(1, ins->discovered_topic_.size());

	auto it = ins->discovered_topic_.find("/HelloWorldTopic");
	ASSERT_TRUE(it != ins->discovered_topic_.end());
	ASSERT_TRUE(it->second != nullptr) << "TopicInformation shared_ptr is null";
	EXPECT_TRUE(it->second->topic != nullptr);
	EXPECT_TRUE(it->second->reader != nullptr);
    //EXPECT_TRUE(ins->discovered_topic_["/HelloWorldTopic"]->topic != nullptr);
    //EXPECT_TRUE(ins->discovered_topic_["/HelloWorldTopic"]->reader != nullptr);

    stopProcess(pid);

    ins->Finalize();
}

TEST_F(DDSProtocolTest, UnsubscribeMessage_TC001) {
    constexpr uint32_t MAX_WAIT_COUNT = 100;
    EXPECT_TRUE(ins->Initialize());
    ins->SubscribeMessage("/HelloWorldTopic");

    char* args[] = {"test/HelloWorldPublisher", 0};
    pid_t pid = startProcess(args);
    if (pid == -1) {
        std::cerr << "Failed to start test publisher" << std::endl;
        FAIL();
    }

    uint32_t wait_count = 0;
    while (wait_count < MAX_WAIT_COUNT) {
        if (ins->discovered_topic_.size() > 0) {
            break;
        }

        wait_count++;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    EXPECT_EQ(1, ins->discovered_topic_.size());

    ins->UnsubscribeMessage("/HelloWorldTopic");

	auto it = ins->discovered_topic_.find("/HelloWorldTopic");
	ASSERT_TRUE(it != ins->discovered_topic_.end());
	ASSERT_TRUE(it->second != nullptr) << "TopicInformation shared_ptr is null";
	EXPECT_TRUE(it->second->topic == nullptr);
	EXPECT_TRUE(it->second->reader == nullptr);
    //EXPECT_TRUE(ins->discovered_topic_["/HelloWorldTopic"]->topic == nullptr);
    //EXPECT_TRUE(ins->discovered_topic_["/HelloWorldTopic"]->reader == nullptr);
    

    stopProcess(pid);

    ins->Finalize();
}

TEST_F(DDSProtocolTest, SerializeMessage_TC001) {

}

TEST_F(DDSProtocolTest, DeserializeMessage_TC001) {
//    constexpr uint32_t MAX_WAIT_COUNT = 100;
//    std::shared_ptr<Message> received_message;
//
//    ins->SetForwardCallback([&] (std::shared_ptr<Message> msg) {
//        received_message = msg;
//    });

    using namespace std::chrono_literals;

    std::mutex m;
    std::condition_variable cv;
    std::shared_ptr<Message> received_message;

    ins->SetForwardCallback([&](std::shared_ptr<Message> msg) {
        {
            std::lock_guard<std::mutex> lk(m);
            received_message = std::move(msg);
        }
        cv.notify_one();
    });

    constexpr uint32_t MAX_WAIT_COUNT = 100;

    EXPECT_TRUE(ins->Initialize());
    ins->SubscribeMessage("/HelloWorldTopic");

    //char* args[] = {"test/HelloWorldPublisher", 0};
    //pid_t pid = startProcess(args);
    //if (pid == -1) {
    //    std::cerr << "Failed to start test publisher" << std::endl;
    //    FAIL();
    //}


    //uint32_t wait_count = 0;
    //while (wait_count < MAX_WAIT_COUNT) {
    //    if (received_message != nullptr) {
    //        break;
    //    }

    //    wait_count++;
    //    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    //}

    //ins->UnsubscribeMessage("/HelloWorldTopic");
    //stopProcess(pid);

    char* args[] = { (char*)"test/HelloWorldPublisher", nullptr };
    pid_t pid = startProcess(args);
    ASSERT_NE(pid, -1) << "Failed to start test publisher";

    {
        std::unique_lock<std::mutex> lk(m);
        bool ok = cv.wait_for(lk, 50ms * MAX_WAIT_COUNT, [&]{
            return (bool)received_message;
        });
        ASSERT_TRUE(ok) << "Timed out waiting for received_message";
    }

    EXPECT_EQ("/HelloWorldTopic", received_message->GetName());
    EXPECT_EQ(MessageProtocolType::DDS, received_message->GetProtocolType());
    EXPECT_EQ("DataTypeTest", received_message->GetTypeName());
    
    std::shared_ptr<VariantData> data = received_message->GetData();
    ASSERT_TRUE(data) << "Data is null";
    EXPECT_EQ(11, data->GetValue<char>("v_char8"));
    EXPECT_EQ(12, data->GetValue<wchar_t>("v_char16"));
    EXPECT_EQ(13, data->GetValue<uint8_t>("v_uint8"));
    EXPECT_EQ(14, data->GetValue<int16_t>("v_int16"));
    EXPECT_EQ(15, data->GetValue<uint16_t>("v_uint16"));
    EXPECT_EQ(16, data->GetValue<int32_t>("v_int32"));
    EXPECT_EQ(17, data->GetValue<uint32_t>("v_uint32"));
    EXPECT_EQ(18, data->GetValue<int64_t>("v_int64"));
    EXPECT_EQ(19, data->GetValue<uint64_t>("v_uint64"));
    EXPECT_EQ(123.45f, data->GetValue<float>("v_float"));
    EXPECT_EQ(234.56, data->GetValue<double>("v_double"));
    EXPECT_EQ(true, data->GetValue<bool>("v_bool"));
    EXPECT_EQ("Test String", data->GetValue<std::string>("v_string"));
    
    EXPECT_EQ(1234, data->GetValue<VariantData>("v_structure").GetValue<int32_t>("id"));
    EXPECT_EQ("HelloPinky", data->GetValue<VariantData>("v_structure").GetValue<std::string>("name"));

    EXPECT_EQ(std::vector<char>({1,2}), data->GetValue<std::vector<char>>("seq_char8"));
    EXPECT_EQ(std::vector<wchar_t>({1,2}), data->GetValue<std::vector<wchar_t>>("seq_char16"));
    EXPECT_EQ(std::vector<uint8_t>({1,2}), data->GetValue<std::vector<uint8_t>>("seq_uint8"));
    EXPECT_EQ(std::vector<int16_t>({1,2}), data->GetValue<std::vector<int16_t>>("seq_int16"));
    EXPECT_EQ(std::vector<uint16_t>({1,2}), data->GetValue<std::vector<uint16_t>>("seq_uint16"));
    EXPECT_EQ(std::vector<int32_t>({1,2}), data->GetValue<std::vector<int32_t>>("seq_int32"));
    EXPECT_EQ(std::vector<uint32_t>({1,2}), data->GetValue<std::vector<uint32_t>>("seq_uint32"));
    EXPECT_EQ(std::vector<int64_t>({1,2}), data->GetValue<std::vector<int64_t>>("seq_int64"));
    EXPECT_EQ(std::vector<uint64_t>({1,2}), data->GetValue<std::vector<uint64_t>>("seq_uint64"));
    EXPECT_EQ(std::vector<float>({1.2,2.3}), data->GetValue<std::vector<float>>("seq_float"));
    EXPECT_EQ(std::vector<double>({1.23,2.34}), data->GetValue<std::vector<double>>("seq_double"));
    EXPECT_EQ(std::vector<bool>({false, true}), data->GetValue<std::vector<bool>>("seq_bool"));
    EXPECT_EQ(std::vector<std::string>({"abc", "def"}), data->GetValue<std::vector<std::string>>("seq_string"));

    ins->UnsubscribeMessage("/HelloWorldTopic");
    stopProcess(pid);

    ins->SetForwardCallback(nullptr);
    ins->Finalize();
}

TEST_F(DDSProtocolTest, DeserializeMessage_TC002_dynamic_idl_parse) {
    constexpr uint32_t MAX_WAIT_COUNT = 100;
    std::shared_ptr<Message> received_message;

    ins->SetForwardCallback([&] (std::shared_ptr<Message> msg) {
        received_message = msg;
    });

    EXPECT_TRUE(ins->Initialize());
    ins->SubscribeMessage("/HelloWorldTopicSimple");

    char* args[] = {"test/HelloWorldPublisherNoDynamic", 0};
    pid_t pid = startProcess(args);
    if (pid == -1) {
        std::cerr << "Failed to start test publisher" << std::endl;
        FAIL();
    }


    uint32_t wait_count = 0;
    while (wait_count < MAX_WAIT_COUNT) {
        if (received_message != nullptr) {
            break;
        }

        wait_count++;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    ins->UnsubscribeMessage("/HelloWorldTopicSimple");
    stopProcess(pid);

    ASSERT_TRUE(wait_count != MAX_WAIT_COUNT);

    EXPECT_EQ("/HelloWorldTopicSimple", received_message->GetName());
    EXPECT_EQ(MessageProtocolType::DDS, received_message->GetProtocolType());
    EXPECT_EQ("DataTypeTestSimple", received_message->GetTypeName());
    
    std::shared_ptr<VariantData> data = received_message->GetData();
    EXPECT_EQ(11, data->GetValue<int32_t>("id"));
    EXPECT_EQ("HelloWorld1234!", data->GetValue<std::string>("name"));

    ins->Finalize();
}

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
