#ifndef DIAGNOSTICS_EXECUTOR_SERVICE_TASK_HPP_
#define DIAGNOSTICS_EXECUTOR_SERVICE_TASK_HPP_

#include <iostream>
#include <chrono>
#include <memory>
#include <functional>

#include "RequestBase.hpp"

namespace diagnostic_manager {

enum class ServiceTaskType {
    UNKNOWN,
    INTERVAL,
    ONCE,
    REQUEST,
};

using TaskID = int32_t;
using TaskTime = std::chrono::steady_clock::time_point;
using TaskInterval = std::chrono::milliseconds;
using TaskDelay = std::chrono::milliseconds;
using ExecutorFunctionType = std::function<void(std::shared_ptr<RequestBase>)>;

class ServiceTask {
  public:
    ServiceTask();
    ServiceTask(const TaskID task_id, const std::string& name, const ServiceTaskType type, ExecutorFunctionType function);
    virtual ~ServiceTask();

    TaskID GetTaskId();
    std::string GetName();
    ServiceTaskType GetType();

    void SetInterval(TaskInterval duration);
    TaskInterval GetInterval();
    TaskTime GetLastExecutedTime();
    void SetNextExecutionTime(TaskTime time);
    TaskTime GetNextExecutionTime();

    void Execute(std::shared_ptr<RequestBase> request);

  private:
    TaskID task_id_;
    std::string name_;
    ServiceTaskType type_;
    ExecutorFunctionType function_;
    TaskInterval interval_;
    TaskTime executed_time_;
    TaskTime next_execution_time_;
};

} // namespace diagnostic_manager

#endif
