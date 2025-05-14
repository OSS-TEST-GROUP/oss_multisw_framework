#include <gtest/gtest.h>

#include "executor/ServiceTask.cpp"

#include "mock/RequestBase_mock.hpp"

using namespace diagnostic_manager;
using namespace std::chrono_literals;

class ServiceTaskTest : public ::testing::Test {
protected:
    virtual void SetUp() {
        executor_called_ = false;
        request_base_adr_ = nullptr;

        ins = new ServiceTask(123, "task123", ServiceTaskType::ONCE, [&](std::shared_ptr<RequestBase> req) {
            std::cout << "Executor Function!" << std::endl;
            executor_called_ = true;
            request_base_adr_ = req;
        });
    }
    virtual void TearDown(){
        delete ins;
    }

    ServiceTask* ins;

    bool executor_called_;
    std::shared_ptr<RequestBase> request_base_adr_;
};

TEST_F(ServiceTaskTest, GetTaskId_TC001) {
    EXPECT_EQ(123, ins->GetTaskId());
}

TEST_F(ServiceTaskTest, GetName_TC001) {
    EXPECT_EQ("task123", ins->GetName());
}

TEST_F(ServiceTaskTest, GetType_TC001) {
    EXPECT_EQ(ServiceTaskType::ONCE, ins->GetType());
}

TEST_F(ServiceTaskTest, SetInterval_TC001) {
    std::chrono::milliseconds duration = 100ms;

    ins->SetInterval(duration);
    EXPECT_EQ(ins->interval_, duration);
}

TEST_F(ServiceTaskTest, GetInterval_TC001) {
    ins->interval_ = 100ms;

    EXPECT_EQ(100ms, ins->GetInterval());
}

TEST_F(ServiceTaskTest, GetLastExecutedTime_TC001) {
    auto current = std::chrono::steady_clock::now();
    ins->executed_time_ = current;

    EXPECT_EQ(current, ins->GetLastExecutedTime());
}

TEST_F(ServiceTaskTest, SetNextExecutionTime_TC001) {
    auto current = std::chrono::steady_clock::now();

    ins->SetNextExecutionTime(current);
    EXPECT_EQ(current, ins->next_execution_time_);
}

TEST_F(ServiceTaskTest, GetNextExecutionTime_TC001) {
    auto current = std::chrono::steady_clock::now();
    ins->next_execution_time_ = current;

    EXPECT_EQ(current, ins->GetNextExecutionTime());
}

TEST_F(ServiceTaskTest, Execute_TC001) {
    EXPECT_FALSE(executor_called_);

    std::shared_ptr<RequestBase> req = std::make_shared<RequestBase>();
    ins->Execute(req);
    
    EXPECT_TRUE(executor_called_);
}

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
