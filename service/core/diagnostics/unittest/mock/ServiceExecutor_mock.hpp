#ifndef DIAGNOSTICS_UNITTEST_MOCK_SERVICE_TASK_MOCK_HPP_
#define DIAGNOSTICS_UNITTEST_MOCK_SERVICE_TASK_MOCK_HPP_

#include <gmock/gmock.h>
#include "executor/ServiceExecutor.hpp"

namespace diagnostic_manager {

class ServiceExecutorMock : public ServiceExecutor {
    MOCK_METHOD0(Initialize, bool());
    MOCK_METHOD0(Start, bool());
    MOCK_METHOD0(Stop, void());
    MOCK_METHOD1(SetExecutorNum, void(uint32_t));
    MOCK_METHOD3(AddTask, TaskID(const std::string&, ExecutorFunctionType, const RequestBase&));
    MOCK_METHOD3(AddTaskInterval, TaskID(const std::string&, ExecutorFunctionType, TaskInterval));
    MOCK_METHOD3(AddTaskOnce, TaskID(const std::string&, ExecutorFunctionType, TaskDelay));
    MOCK_METHOD1(RemoveTask, void(TaskID));
    MOCK_METHOD1(RemoveTask, void(const std::string&));
    MOCK_METHOD1(RequestTask, bool(std::shared_ptr<RequestBase> request_msg));
    MOCK_METHOD0(GetAllTaskNum, uint64_t());
    MOCK_METHOD0(GetRequestTaskNum, uint64_t());
    MOCK_METHOD0(GetIntervalTaskNum, uint64_t());
    MOCK_METHOD0(GetOnceTaskNum, uint64_t());
    MOCK_METHOD0(GetRequestNum, uint64_t());
    MOCK_METHOD1(RunScheduler, void(const uint32_t idx));
};

ServiceExecutorMock* Mock_ServiceExecutorMock;

ServiceExecutor::ServiceExecutor() {}
ServiceExecutor::~ServiceExecutor() {}

bool ServiceExecutor::Initialize() {
    return Mock_ServiceExecutorMock->Initialize();
}

bool ServiceExecutor::Start() {
    return Mock_ServiceExecutorMock->Start();
}

void ServiceExecutor::Stop() {
    Mock_ServiceExecutorMock->Stop();
}

void ServiceExecutor::SetExecutorNum(uint32_t num) {
    Mock_ServiceExecutorMock->SetExecutorNum(num);
}

TaskID ServiceExecutor::AddTask(const std::string& name, ExecutorFunctionType function, const RequestBase& request) {
    return Mock_ServiceExecutorMock->AddTask(name, function, request);
}

TaskID ServiceExecutor::AddTaskInterval(const std::string& name, ExecutorFunctionType function, TaskInterval interval) {
    return Mock_ServiceExecutorMock->AddTaskInterval(name, function, interval);
}

TaskID ServiceExecutor::AddTaskOnce(const std::string& name, ExecutorFunctionType function, TaskDelay delay) {
    return Mock_ServiceExecutorMock->AddTaskOnce(name, function, delay);
}

void ServiceExecutor::RemoveTask(TaskID task_id) {
    Mock_ServiceExecutorMock->RemoveTask(task_id);
}

void ServiceExecutor::RemoveTask(const std::string& name) {
    Mock_ServiceExecutorMock->RemoveTask(name);
}

bool ServiceExecutor::RequestTask(std::shared_ptr<RequestBase> request_msg) {
    return Mock_ServiceExecutorMock->RequestTask(request_msg);
}

uint64_t ServiceExecutor::GetAllTaskNum() {
    return Mock_ServiceExecutorMock->GetAllTaskNum();
}

uint64_t ServiceExecutor::GetRequestTaskNum() {
    return Mock_ServiceExecutorMock->GetRequestTaskNum();
}

uint64_t ServiceExecutor::GetIntervalTaskNum() {
    return Mock_ServiceExecutorMock->GetIntervalTaskNum();
}

uint64_t ServiceExecutor::GetOnceTaskNum() {
    return Mock_ServiceExecutorMock->GetOnceTaskNum();
}

uint64_t ServiceExecutor::GetRequestNum() {
    return Mock_ServiceExecutorMock->GetRequestNum();
}

void ServiceExecutor::RunScheduler(const uint32_t idx) {
    Mock_ServiceExecutorMock->RunScheduler(idx);
}

} // namespace diagnostic_manager

#endif
