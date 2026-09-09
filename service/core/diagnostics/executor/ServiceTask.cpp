#include "ServiceTask.hpp"

namespace diagnostic_manager {

ServiceTask::ServiceTask()
        : task_id_(0), type_(ServiceTaskType::UNKNOWN) {}
ServiceTask::ServiceTask(const TaskID task_id, const std::string& name, const ServiceTaskType type, ExecutorFunctionType function)
        : task_id_(task_id), name_(name), type_(type), function_(function) {}

ServiceTask::~ServiceTask() {}

TaskID ServiceTask::GetTaskId() {
    return task_id_;
}

std::string ServiceTask::GetName() {
    return name_;
}

ServiceTaskType ServiceTask::GetType() {
    return type_;
}

void ServiceTask::SetInterval(TaskInterval duration) {
    interval_ = duration;
}

TaskInterval ServiceTask::GetInterval() {
    return interval_;
}

TaskTime ServiceTask::GetLastExecutedTime() {
    return executed_time_;
}

void ServiceTask::SetNextExecutionTime(TaskTime time) {
    next_execution_time_ = time;
}

TaskTime ServiceTask::GetNextExecutionTime() {
    return next_execution_time_;
}

void ServiceTask::Execute(std::shared_ptr<RequestBase> request) {
    executed_time_ = std::chrono::steady_clock::now();

    if (function_) {
        function_(request);
    }
}

} // namespace diagnostic_manager
