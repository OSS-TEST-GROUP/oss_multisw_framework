#ifndef DIAGNOSTICS_UNITTEST_MOCK_SERVICE_TASK_MOCK_HPP_
#define DIAGNOSTICS_UNITTEST_MOCK_SERVICE_TASK_MOCK_HPP_

#include <gmock/gmock.h>
#include "executor/ServiceTask.hpp"

namespace diagnostic_manager {

class ServiceTaskMock : public ServiceTask {
    MOCK_METHOD4(Constructor, void(const TaskID task_id, const std::string& name, const ServiceTaskType type, ExecutorFunctionType function));
    MOCK_METHOD0(GetTaskId, TaskID());
    MOCK_METHOD0(GetName, std::string());
    MOCK_METHOD0(GetType, ServiceTaskType());
    MOCK_METHOD1(SetInterval, void(TaskInterval));
    MOCK_METHOD0(GetInterval, TaskInterval());
    MOCK_METHOD0(GetLastExecutedTime, std::chrono::steady_clock::time_point());
    MOCK_METHOD1(SetNextExecutionTime, void(TaskTime));
    MOCK_METHOD0(GetNextExecutionTime, TaskTime());
    MOCK_METHOD1(Execute, void(std::shared_ptr<RequestBase>));

    bool use_internal_task_id_ = false;
    bool use_internal_interval_ = false;
    bool use_internal_next_execution_time_ = false;
    bool use_internal_execute_ = false;
};

ServiceTaskMock* Mock_ServiceTaskMock;

ServiceTask::ServiceTask() {}
ServiceTask::ServiceTask(const TaskID task_id, const std::string& name, const ServiceTaskType type, ExecutorFunctionType function) {
    Mock_ServiceTaskMock->Constructor(task_id, name, type, function);
    task_id_ = task_id;
    function_ = function;
}
ServiceTask::~ServiceTask() {}

TaskID ServiceTask::GetTaskId() {
    if (Mock_ServiceTaskMock->use_internal_task_id_) {
        return task_id_;
    } else {
        return Mock_ServiceTaskMock->GetTaskId();
    }
}

std::string ServiceTask::GetName() {
    return Mock_ServiceTaskMock->GetName();
}

ServiceTaskType ServiceTask::GetType() {
    return Mock_ServiceTaskMock->GetType();
}

void ServiceTask::SetInterval(TaskInterval duration) {\
    if (Mock_ServiceTaskMock->use_internal_interval_) {
        interval_ = duration;
    } else {
        Mock_ServiceTaskMock->SetInterval(duration);
    }
}

TaskInterval ServiceTask::GetInterval() {
    if (Mock_ServiceTaskMock->use_internal_interval_) {
        return interval_;
    } else {
        return Mock_ServiceTaskMock->GetInterval();
    }
}

std::chrono::steady_clock::time_point ServiceTask::GetLastExecutedTime() {
    return Mock_ServiceTaskMock->GetLastExecutedTime();
}

void ServiceTask::SetNextExecutionTime(TaskTime time) {
    if (Mock_ServiceTaskMock->use_internal_next_execution_time_) {
        next_execution_time_ = time;
    } else {
        Mock_ServiceTaskMock->SetNextExecutionTime(time);
    }
}

TaskTime ServiceTask::GetNextExecutionTime() {
    if (Mock_ServiceTaskMock->use_internal_next_execution_time_) {
        return next_execution_time_;
    } else {
        return Mock_ServiceTaskMock->GetNextExecutionTime();
    }
}

void ServiceTask::Execute(std::shared_ptr<RequestBase> request) {
    if (Mock_ServiceTaskMock->use_internal_execute_) {
        function_(request);
    } else {
        Mock_ServiceTaskMock->Execute(request);
    }
}

} // namespace diagnostic_manager

#endif 