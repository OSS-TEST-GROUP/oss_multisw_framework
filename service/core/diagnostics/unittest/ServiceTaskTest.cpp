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

TEST_F(ServiceTaskTest, Execute_NullFunction_TC002) {
    ServiceTask task_no_func(456, "task_no_func", ServiceTaskType::INTERVAL, nullptr);
    std::shared_ptr<RequestBase> req = std::make_shared<RequestBase>();
    EXPECT_NO_THROW(task_no_func.Execute(req));
}

TEST_F(ServiceTaskTest, Execute_UpdatesExecutedTime_TC003) {
    auto before = std::chrono::steady_clock::now();
    std::shared_ptr<RequestBase> req = std::make_shared<RequestBase>();
    ins->Execute(req);
    auto after = std::chrono::steady_clock::now();
    auto executed = ins->GetLastExecutedTime();
    
    EXPECT_GE(executed, before);
    EXPECT_LE(executed, after);
}

TEST_F(ServiceTaskTest, Execute_NullRequest_TC004) {
    EXPECT_FALSE(executor_called_);
    ins->Execute(nullptr);
    EXPECT_TRUE(executor_called_);
    EXPECT_EQ(nullptr, request_base_adr_);
}

TEST_F(ServiceTaskTest, DefaultConstructor_TC005) {
    ServiceTask default_task;
    EXPECT_EQ(0, default_task.GetTaskId());
    EXPECT_EQ("", default_task.GetName());
}

TEST_F(ServiceTaskTest, TaskType_ONCE_TC006) {
    ServiceTask once_task(100, "once", ServiceTaskType::ONCE, nullptr);
    EXPECT_EQ(ServiceTaskType::ONCE, once_task.GetType());
}

TEST_F(ServiceTaskTest, TaskType_PERIODIC_TC007) {
    ServiceTask periodic_task(200, "periodic", ServiceTaskType::INTERVAL, nullptr);
    EXPECT_EQ(ServiceTaskType::INTERVAL, periodic_task.GetType());
}

TEST_F(ServiceTaskTest, ZeroInterval_TC008) {
    ins->SetInterval(0ms);
    EXPECT_EQ(0ms, ins->GetInterval());
}

TEST_F(ServiceTaskTest, LargeInterval_TC009) {
    auto large_interval = std::chrono::hours(24);
    ins->SetInterval(large_interval);
    EXPECT_EQ(large_interval, ins->GetInterval());
}

TEST_F(ServiceTaskTest, MultipleExecutions_TC010) {
    int execution_count = 0;
    ServiceTask multi_exec_task(789, "multi", ServiceTaskType::INTERVAL, [&](std::shared_ptr<RequestBase> req) {
        execution_count++;
    });
    
    std::shared_ptr<RequestBase> req = std::make_shared<RequestBase>();
    multi_exec_task.Execute(req);
    EXPECT_EQ(1, execution_count);
    
    multi_exec_task.Execute(req);
    EXPECT_EQ(2, execution_count);
    
    multi_exec_task.Execute(req);
    EXPECT_EQ(3, execution_count);
}

TEST_F(ServiceTaskTest, EmptyTaskName_TC011) {
    ServiceTask empty_name_task(999, "", ServiceTaskType::ONCE, nullptr);
    EXPECT_EQ("", empty_name_task.GetName());
}

TEST_F(ServiceTaskTest, LongTaskName_TC012) {
    std::string long_name(500, 'x');
    ServiceTask long_name_task(1000, long_name, ServiceTaskType::INTERVAL, nullptr);
    EXPECT_EQ(long_name, long_name_task.GetName());
}

TEST_F(ServiceTaskTest, SpecialCharactersInName_TC013) {
    ServiceTask special_task(1001, "task-name_123.test!@#", ServiceTaskType::ONCE, nullptr);
    EXPECT_EQ("task-name_123.test!@#", special_task.GetName());
}

TEST_F(ServiceTaskTest, NegativeTaskId_TC014) {
    ServiceTask negative_id_task(-100, "negative", ServiceTaskType::ONCE, nullptr);
    EXPECT_EQ(-100, negative_id_task.GetTaskId());
}

TEST_F(ServiceTaskTest, ZeroTaskId_TC015) {
    ServiceTask zero_id_task(0, "zero", ServiceTaskType::ONCE, nullptr);
    EXPECT_EQ(0, zero_id_task.GetTaskId());
}

TEST_F(ServiceTaskTest, NextExecutionTime_FutureTime_TC016) {
    auto future_time = std::chrono::steady_clock::now() + std::chrono::hours(1);
    ins->SetNextExecutionTime(future_time);
    EXPECT_EQ(future_time, ins->GetNextExecutionTime());
}

TEST_F(ServiceTaskTest, NextExecutionTime_PastTime_TC017) {
    auto past_time = std::chrono::steady_clock::now() - std::chrono::hours(1);
    ins->SetNextExecutionTime(past_time);
    EXPECT_EQ(past_time, ins->GetNextExecutionTime());
}

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
