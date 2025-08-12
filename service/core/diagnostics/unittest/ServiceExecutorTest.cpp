#include <gtest/gtest.h>

#include "executor/ServiceExecutor.cpp"

#include "mock/ServiceTask_mock.hpp"
#include "mock/RequestBase_mock.hpp"
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
using ::testing::Invoke;

using namespace diagnostic_manager;
using namespace std::chrono_literals;

class ServiceExecutorTest : public ::testing::Test {
protected:
    virtual void SetUp() {
        Mock_ServiceTaskMock = new ServiceTaskMock;

        ins = new ServiceExecutor();
    }
    virtual void TearDown(){
        delete ins;

        delete Mock_ServiceTaskMock;
    }

    ServiceExecutor* ins;
};

TEST_F(ServiceExecutorTest, Initialize_TC001) {

}

TEST_F(ServiceExecutorTest, StartStop_TC001) {
    constexpr uint32_t TEST_THREADS = 3U;
    EXPECT_FALSE(ins->running_);

    ins->SetExecutorNum(TEST_THREADS);

    EXPECT_TRUE(ins->Start());
    EXPECT_EQ(TEST_THREADS, ins->thread_list_.size());

    std::this_thread::sleep_for(std::chrono::microseconds(100));
    
    ins->Stop();
    EXPECT_EQ(0U, ins->thread_list_.size());
}


TEST_F(ServiceExecutorTest, SetExecutorNum_TC001) {
    EXPECT_EQ(1U, ins->executor_num_);

    ins->SetExecutorNum(10U);
    EXPECT_EQ(10U, ins->executor_num_);
}

TEST_F(ServiceExecutorTest, AddTask_TC001) {
    EXPECT_EQ(0U, ins->service_task_list_.size());
    EXPECT_EQ(0U, ins->task_request_map_.size());

    class TestRequest : public RequestBase {};
    TestRequest test_req;
    ExecutorFunctionType test_func = [](std::shared_ptr<RequestBase> req) {};

    EXPECT_CALL(*Mock_ServiceTaskMock, Constructor(ins->next_task_id_.load(), "TestTask1", ServiceTaskType::REQUEST, _)).WillOnce(Return());

    TaskID task_id = ins->AddTask("TestTask1", test_func, test_req);
    EXPECT_TRUE(task_id >= 1);
    
    std::shared_ptr task = ins->service_task_list_[task_id];
    EXPECT_EQ(2U, ins->next_task_id_.load());
    EXPECT_EQ(1U, ins->service_task_list_.size());
    EXPECT_EQ(task_id, ins->task_request_map_[std::type_index(typeid(test_req))]);
    
}

TEST_F(ServiceExecutorTest, AddTaskInterval_TC001) {
    EXPECT_EQ(0U, ins->service_task_list_.size());
    EXPECT_EQ(0U, ins->task_request_queue_.size());

    ExecutorFunctionType test_func1 = [](std::shared_ptr<RequestBase> req) {};

    EXPECT_CALL(*Mock_ServiceTaskMock, Constructor(ins->next_task_id_.load(), "TestInterval1", ServiceTaskType::INTERVAL, _)).WillOnce(Return());
    EXPECT_CALL(*Mock_ServiceTaskMock, SetInterval(1000ms)).WillOnce(Return());
    EXPECT_CALL(*Mock_ServiceTaskMock, SetNextExecutionTime(_)).WillOnce(Return());

    TaskID task_id = ins->AddTaskInterval("TestInterval1", test_func1, 1000ms);
    EXPECT_TRUE(task_id >= 1);

    std::shared_ptr task = ins->service_task_list_[task_id];
    EXPECT_EQ(2U, ins->next_task_id_.load());
    EXPECT_EQ(1U, ins->service_task_list_.size());
    EXPECT_EQ(task, ins->task_interval_queue_.top());

}

TEST_F(ServiceExecutorTest, ComparatorIntervalQueue_TC001) {
    ExecutorFunctionType test_func1 = [](std::shared_ptr<RequestBase> req) {};
    ExecutorFunctionType test_func2 = [](std::shared_ptr<RequestBase> req) {};
    ExecutorFunctionType test_func3 = [](std::shared_ptr<RequestBase> req) {};
    ExecutorFunctionType test_func4 = [](std::shared_ptr<RequestBase> req) {};

    std::shared_ptr<ServiceTask> task1 = std::make_shared<ServiceTask>(1U, "Task1", ServiceTaskType::INTERVAL, test_func1);
    std::shared_ptr<ServiceTask> task2 = std::make_shared<ServiceTask>(2U, "Task2", ServiceTaskType::INTERVAL, test_func2);
    std::shared_ptr<ServiceTask> task3 = std::make_shared<ServiceTask>(3U, "Task3", ServiceTaskType::INTERVAL, test_func3);
    std::shared_ptr<ServiceTask> task4 = std::make_shared<ServiceTask>(4U, "Task4", ServiceTaskType::INTERVAL, test_func4);
    Mock_ServiceTaskMock->use_internal_next_execution_time_ = true;
    std::chrono::steady_clock::time_point current = std::chrono::steady_clock::now();
    task1->SetNextExecutionTime(current+100ms);
    task2->SetNextExecutionTime(current+50ms);
    task3->SetNextExecutionTime(current+1000ms);
    task4->SetNextExecutionTime(current+700ms);


    EXPECT_CALL(*Mock_ServiceTaskMock, Constructor(_, _, _, _)).WillRepeatedly(Return());

    ins->task_interval_queue_.push(task1);
    ins->task_interval_queue_.push(task2);
    ins->task_interval_queue_.push(task3);
    ins->task_interval_queue_.push(task4);

    EXPECT_EQ(task2, ins->task_interval_queue_.top()); ins->task_interval_queue_.pop();
    EXPECT_EQ(task1, ins->task_interval_queue_.top()); ins->task_interval_queue_.pop();
    EXPECT_EQ(task4, ins->task_interval_queue_.top()); ins->task_interval_queue_.pop();
    EXPECT_EQ(task3, ins->task_interval_queue_.top()); ins->task_interval_queue_.pop();
}

TEST_F(ServiceExecutorTest, AddTaskOnce_TC001) {
}

TEST_F(ServiceExecutorTest, RemoveTask_TC001_RequestTask) {
    EXPECT_EQ(0U, ins->service_task_list_.size());
    EXPECT_EQ(0U, ins->task_request_map_.size());

    // Prepare for the test
    class TestRequest : public RequestBase {};
    TestRequest test_req;
    ExecutorFunctionType test_func = [](std::shared_ptr<RequestBase> req) {};

    EXPECT_CALL(*Mock_ServiceTaskMock, Constructor(ins->next_task_id_.load(), "TestTask1", ServiceTaskType::REQUEST, _)).WillOnce(Return());
    EXPECT_CALL(*Mock_ServiceTaskMock, GetType()).WillOnce(Return(ServiceTaskType::REQUEST));

    TaskID task_id = ins->AddTask("TestTask1", test_func, test_req);
    EXPECT_TRUE(task_id >= 1);
    EXPECT_EQ(1U, ins->service_task_list_.size());

    // Test
    ins->RemoveTask(task_id);
    EXPECT_EQ(0U, ins->service_task_list_.size());
    EXPECT_EQ(0U, ins->task_request_map_.size());
}

TEST_F(ServiceExecutorTest, RemoveTaskByName_TC001) {
    EXPECT_EQ(0U, ins->service_task_list_.size());
    EXPECT_EQ(0U, ins->task_request_map_.size());

    // Prepare for the test
    class TestRequest : public RequestBase {};
    TestRequest test_req;
    ExecutorFunctionType test_func = [](std::shared_ptr<RequestBase> req) {};

    EXPECT_CALL(*Mock_ServiceTaskMock, Constructor(ins->next_task_id_.load(), "TestTask1", ServiceTaskType::REQUEST, _)).WillOnce(Return());
    EXPECT_CALL(*Mock_ServiceTaskMock, GetType()).WillRepeatedly(Return(ServiceTaskType::REQUEST));
    EXPECT_CALL(*Mock_ServiceTaskMock, GetName()).WillRepeatedly(Return("TestTask1"));

    TaskID task_id = ins->AddTask("TestTask1", test_func, test_req);
    EXPECT_TRUE(task_id >= 1);
    EXPECT_EQ(1U, ins->service_task_list_.size());

    // Test
    ins->RemoveTask("TestTask1");
    EXPECT_EQ(0U, ins->service_task_list_.size());
    EXPECT_EQ(0U, ins->task_request_map_.size());
}

TEST_F(ServiceExecutorTest, RequestTask_TC001) {
    EXPECT_EQ(0U, ins->task_request_queue_.size());

    // Prepare for the task
    class TestRequest : public RequestBase {};
    TestRequest test_req;
    ExecutorFunctionType test_func = [](std::shared_ptr<RequestBase> req) {};

    EXPECT_CALL(*Mock_ServiceTaskMock, Constructor(ins->next_task_id_.load(), "TestTask1", ServiceTaskType::REQUEST, _)).WillOnce(Return());

    TaskID task_id = ins->AddTask("TestTask1", test_func, test_req);
    EXPECT_TRUE(task_id > 0);

    // Test
    std::shared_ptr<TestRequest> req = std::make_shared<TestRequest>();
    EXPECT_TRUE(ins->RequestTask(req));
    EXPECT_EQ(req, ins->task_request_queue_.front());
}

TEST_F(ServiceExecutorTest, RequestTask_TC002_NotRegisteredTask) {
    EXPECT_EQ(0U, ins->task_request_queue_.size());

    // Prepare for the task
    class TestRequest : public RequestBase {};

    // Test
    std::shared_ptr<TestRequest> req = std::make_shared<TestRequest>();
    EXPECT_FALSE(ins->RequestTask(req));
    EXPECT_EQ(0U, ins->task_request_queue_.size());
}

TEST_F(ServiceExecutorTest, GetAllTaskNum_TC001) {
    std::shared_ptr<ServiceTask> task1 = std::make_shared<ServiceTask>();
    std::shared_ptr<ServiceTask> task2 = std::make_shared<ServiceTask>();
    std::shared_ptr<ServiceTask> task3 = std::make_shared<ServiceTask>();
    std::shared_ptr<ServiceTask> task4 = std::make_shared<ServiceTask>();
    std::shared_ptr<ServiceTask> task5 = std::make_shared<ServiceTask>();

    ins->service_task_list_[1U] = task1;
    ins->service_task_list_[2U] = task2;
    ins->service_task_list_[3U] = task3;
    ins->service_task_list_[4U] = task4;
    ins->service_task_list_[5U] = task5;

    EXPECT_EQ(5U, ins->GetAllTaskNum());
}

TEST_F(ServiceExecutorTest, GetRequestTaskNum_TC001) {
    std::shared_ptr<ServiceTask> task1 = std::make_shared<ServiceTask>();
    std::shared_ptr<ServiceTask> task2 = std::make_shared<ServiceTask>();
    std::shared_ptr<ServiceTask> task3 = std::make_shared<ServiceTask>();
    std::shared_ptr<ServiceTask> task4 = std::make_shared<ServiceTask>();
    std::shared_ptr<ServiceTask> task5 = std::make_shared<ServiceTask>();

    ins->service_task_list_[1U] = task1;
    ins->service_task_list_[2U] = task2;
    ins->service_task_list_[3U] = task3;
    ins->service_task_list_[4U] = task4;
    ins->service_task_list_[5U] = task5;

    EXPECT_CALL(*Mock_ServiceTaskMock, GetType()).WillOnce(Return(ServiceTaskType::REQUEST))
                                                  .WillOnce(Return(ServiceTaskType::REQUEST))
                                                  .WillOnce(Return(ServiceTaskType::INTERVAL))
                                                  .WillOnce(Return(ServiceTaskType::ONCE))
                                                  .WillOnce(Return(ServiceTaskType::REQUEST));

    EXPECT_EQ(3U, ins->GetRequestTaskNum());
}

TEST_F(ServiceExecutorTest, GetIntervalTaskNum_TC001) {
    std::shared_ptr<ServiceTask> task1 = std::make_shared<ServiceTask>();
    std::shared_ptr<ServiceTask> task2 = std::make_shared<ServiceTask>();
    std::shared_ptr<ServiceTask> task3 = std::make_shared<ServiceTask>();
    std::shared_ptr<ServiceTask> task4 = std::make_shared<ServiceTask>();
    std::shared_ptr<ServiceTask> task5 = std::make_shared<ServiceTask>();

    ins->service_task_list_[1U] = task1;
    ins->service_task_list_[2U] = task2;
    ins->service_task_list_[3U] = task3;
    ins->service_task_list_[4U] = task4;
    ins->service_task_list_[5U] = task5;

    EXPECT_CALL(*Mock_ServiceTaskMock, GetType()).WillOnce(Return(ServiceTaskType::INTERVAL))
                                                  .WillOnce(Return(ServiceTaskType::REQUEST))
                                                  .WillOnce(Return(ServiceTaskType::INTERVAL))
                                                  .WillOnce(Return(ServiceTaskType::ONCE))
                                                  .WillOnce(Return(ServiceTaskType::REQUEST));

    EXPECT_EQ(2U, ins->GetIntervalTaskNum());
}

TEST_F(ServiceExecutorTest, GetOnceTaskNum_TC001) {
    std::shared_ptr<ServiceTask> task1 = std::make_shared<ServiceTask>();
    std::shared_ptr<ServiceTask> task2 = std::make_shared<ServiceTask>();
    std::shared_ptr<ServiceTask> task3 = std::make_shared<ServiceTask>();
    std::shared_ptr<ServiceTask> task4 = std::make_shared<ServiceTask>();
    std::shared_ptr<ServiceTask> task5 = std::make_shared<ServiceTask>();

    ins->service_task_list_[1U] = task1;
    ins->service_task_list_[2U] = task2;
    ins->service_task_list_[3U] = task3;
    ins->service_task_list_[4U] = task4;
    ins->service_task_list_[5U] = task5;

    EXPECT_CALL(*Mock_ServiceTaskMock, GetType()).WillOnce(Return(ServiceTaskType::ONCE))
                                                 .WillOnce(Return(ServiceTaskType::REQUEST))
                                                 .WillOnce(Return(ServiceTaskType::INTERVAL))
                                                 .WillOnce(Return(ServiceTaskType::ONCE))
                                                 .WillOnce(Return(ServiceTaskType::ONCE));

    EXPECT_EQ(3U, ins->GetOnceTaskNum());
}

TEST_F(ServiceExecutorTest, GetRequestNum_TC001) {
}

TEST_F(ServiceExecutorTest, RunScheduler_TC001_RequestBase) {
    constexpr uint64_t TEST_EXE_NUM = 10U;
    constexpr uint64_t TEST_EXE_COUNT = 2U;
    std::mutex execute_count_lock;
    std::unordered_map<uint64_t, uint64_t> execute_count;

    // Prepare for the task
    class TestRequest : public RequestBase {
      public:
        TestRequest(uint32_t idx) : idx(idx) {}
        uint32_t idx;
    };
    TestRequest test_req(0);
    ExecutorFunctionType test_func = [&](std::shared_ptr<RequestBase> req) {
        std::shared_ptr<TestRequest> test_req = std::dynamic_pointer_cast<TestRequest>(req);
        std::lock_guard<std::mutex> lk(execute_count_lock);
        if (execute_count.count(test_req->idx) == 0) {
            execute_count[test_req->idx] = 1;
        } else {
            execute_count[test_req->idx]++;
        }
    };

    ExecutorFunctionType delivered_func;
    EXPECT_CALL(*Mock_ServiceTaskMock, Constructor(ins->next_task_id_.load(), "TestTask1", ServiceTaskType::REQUEST, _)).WillRepeatedly(
            Invoke([&](const TaskID task_id, const std::string name, const ServiceTaskType type, ExecutorFunctionType function) {
        delivered_func = function;
    }));
    EXPECT_CALL(*Mock_ServiceTaskMock, Execute(_)).WillRepeatedly(Invoke([&](std::shared_ptr<RequestBase> request) {
        delivered_func(request);
    }));

    TaskID task_id = ins->AddTask("TestTask1", test_func, test_req);
    EXPECT_TRUE(task_id > 0);

    // Test
    EXPECT_TRUE(ins->Start());
    for (uint64_t i = 0; i < TEST_EXE_NUM; ++i) {
        std::shared_ptr<TestRequest> req = std::make_shared<TestRequest>(i);
        for (uint64_t count = 0; count < TEST_EXE_COUNT; count++) {
            ins->RequestTask(req);
        }
    }
    while (ins->GetRequestNum() > 0U) {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
    ins->Stop();
    for (uint64_t i = 0; i < TEST_EXE_NUM; ++i) {
        EXPECT_EQ(execute_count[i], TEST_EXE_COUNT);
    }
}

TEST_F(ServiceExecutorTest, RunScheduler_TC002_RequestBaseParallel) {
    constexpr uint64_t TEST_EXE_NUM = 10000U;
    constexpr uint64_t TEST_EXE_COUNT = 5U;
    std::mutex execute_count_lock;
    std::unordered_map<uint64_t, uint64_t> execute_count;

    // Prepare for the task
    class TestRequest : public RequestBase {
      public:
        TestRequest(uint32_t idx) : idx(idx) {}
        uint32_t idx;
    };
    TestRequest test_req(0);
    ExecutorFunctionType test_func = [&](std::shared_ptr<RequestBase> req) {
        std::shared_ptr<TestRequest> test_req = std::dynamic_pointer_cast<TestRequest>(req);
        std::lock_guard<std::mutex> lk(execute_count_lock);
        if (execute_count.count(test_req->idx) == 0) {
            execute_count[test_req->idx] = 1;
        } else {
            execute_count[test_req->idx]++;
        }
    };

    ExecutorFunctionType delivered_func;
    EXPECT_CALL(*Mock_ServiceTaskMock, Constructor(ins->next_task_id_.load(), "TestTask1", ServiceTaskType::REQUEST, _)).WillRepeatedly(
            Invoke([&](const TaskID task_id, const std::string name, const ServiceTaskType type, ExecutorFunctionType function) {
        delivered_func = function;
    }));
    EXPECT_CALL(*Mock_ServiceTaskMock, Execute(_)).WillRepeatedly(Invoke([&](std::shared_ptr<RequestBase> request) {
        delivered_func(request);
    }));

    TaskID task_id = ins->AddTask("TestTask1", test_func, test_req);
    EXPECT_TRUE(task_id > 0);

    // Test
    ins->SetExecutorNum(5);
    EXPECT_TRUE(ins->Start());
    for (uint64_t i = 0; i < TEST_EXE_NUM; ++i) {
        std::shared_ptr<TestRequest> req = std::make_shared<TestRequest>(i);
        for (uint64_t count = 0; count < TEST_EXE_COUNT; count++) {
            ins->RequestTask(req);
        }
    }
    while (ins->GetRequestNum() > 0U) {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
    ins->Stop();
    for (uint64_t i = 0; i < TEST_EXE_NUM; ++i) {
        EXPECT_EQ(execute_count[i], TEST_EXE_COUNT);
    }
}

TEST_F(ServiceExecutorTest, RunScheduler_TC003_IntervalTask) {
    std::unordered_map<uint64_t, uint64_t> execute_count;

    std::vector<std::string> interval_pass_result({"1", "2", "3", "2", "2", "1"});
    std::vector<std::string> interval_result;

    // Prepare for the task
    ExecutorFunctionType test_func1 = [&](std::shared_ptr<RequestBase> req) { interval_result.push_back("1"); };
    ExecutorFunctionType test_func2 = [&](std::shared_ptr<RequestBase> req) { interval_result.push_back("2"); };
    ExecutorFunctionType test_func3 = [&](std::shared_ptr<RequestBase> req) { interval_result.push_back("3"); };

    Mock_ServiceTaskMock->use_internal_task_id_ = true;
    Mock_ServiceTaskMock->use_internal_interval_ = true;
    Mock_ServiceTaskMock->use_internal_next_execution_time_ = true;
    Mock_ServiceTaskMock->use_internal_execute_ = true;
    EXPECT_CALL(*Mock_ServiceTaskMock, Constructor(_, _, _, _)).WillRepeatedly(Return());

    TaskID task_id1 = ins->AddTaskInterval("TestInterval1", test_func1, 25ms);
    TaskID task_id2 = ins->AddTaskInterval("TestInterval2", test_func2, 10ms);
    TaskID task_id3 = ins->AddTaskInterval("TestInterval3", test_func3, 30ms);
    EXPECT_EQ(1U, task_id1);
    EXPECT_EQ(2U, task_id2);
    EXPECT_EQ(3U, task_id3);

    // Test
    EXPECT_TRUE(ins->Start());
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    ins->Stop();
    EXPECT_TRUE(interval_result.size() >= 6);
    for (uint64_t i = 0; i < std::min(interval_result.size(), interval_pass_result.size()); ++i) {
        EXPECT_EQ(interval_pass_result[i], interval_result[i]);
    }
}


int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
