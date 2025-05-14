#ifndef DIAGNOSTICS_EXECUTOR_SERVICE_EXECUTOR_HPP_
#define DIAGNOSTICS_EXECUTOR_SERVICE_EXECUTOR_HPP_

#include <iostream>
#include <chrono>
#include <unordered_map>
#include <queue>
#include <thread>
#include <typeindex>

#include "ServiceTask.hpp"
#include "RequestBase.hpp"

namespace diagnostic_manager {

struct ComparatorIntervalQueue {
    bool operator()(const std::shared_ptr<ServiceTask> l, std::shared_ptr<ServiceTask> r) const;
};

class ServiceExecutor {
  public:
    ServiceExecutor();
    virtual ~ServiceExecutor();

    bool Initialize();

    bool Start();
    void Stop();

    void SetExecutorNum(uint32_t num);

    TaskID AddTask(const std::string& name, ExecutorFunctionType function, RequestBase& request);
    TaskID AddTaskInterval(const std::string& name, ExecutorFunctionType function, TaskInterval interval);
    TaskID AddTaskOnce(const std::string& name, ExecutorFunctionType function, TaskDelay delay);

    void RemoveTask(TaskID task_id);
    void RemoveTask(const std::string& name);

    bool RequestTask(std::shared_ptr<RequestBase> request_msg);

    uint64_t GetAllTaskNum();
    uint64_t GetRequestTaskNum();
    uint64_t GetIntervalTaskNum();
    uint64_t GetOnceTaskNum();
    uint64_t GetRequestNum();

  private:
    void RunScheduler(const uint32_t idx);
    
    bool running_;
    uint32_t executor_num_;

    std::unordered_map<int32_t, std::shared_ptr<ServiceTask>> service_task_list_;
    std::unordered_map<std::type_index, int32_t> task_request_map_;
    std::priority_queue<std::shared_ptr<ServiceTask>, std::vector<std::shared_ptr<ServiceTask>>, ComparatorIntervalQueue> task_interval_queue_;
    std::queue<std::shared_ptr<RequestBase>> task_request_queue_;

    std::vector<std::thread> thread_list_;
    std::atomic<uint64_t> running_threads_;
  
    std::mutex task_handling_lock_;
    std::mutex task_request_handling_lock_;
    std::condition_variable task_request_handling_cv_;

    std::atomic<uint64_t> next_task_id_;
};

} // namespace diagnostic_manager

#endif
