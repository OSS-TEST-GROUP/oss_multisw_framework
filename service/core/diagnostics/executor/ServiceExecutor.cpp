#include "ServiceExecutor.hpp"

namespace diagnostic_manager {

bool ComparatorIntervalQueue::operator()(const std::shared_ptr<ServiceTask> l, std::shared_ptr<ServiceTask> r) const {
    return (l->GetNextExecutionTime() > r->GetNextExecutionTime()); 
}

ServiceExecutor::ServiceExecutor() : running_(false), executor_num_(1U), running_threads_(0U), next_task_id_(1U) {}
ServiceExecutor::~ServiceExecutor() {}

bool ServiceExecutor::Initialize() {
    return true;
}

bool ServiceExecutor::Start() {
    if (running_) {
        return true;
    }

    running_ = true;
    for (uint32_t i = 0; i < executor_num_; i++) {
        thread_list_.emplace_back(&ServiceExecutor::RunScheduler, this, i);
    }

    return true;
}

void ServiceExecutor::Stop() {
    if (running_ == false) {
        return;
    }

    running_ = false;
    while (running_threads_.load() > 0) {
        task_request_handling_cv_.notify_all();
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
    for (uint32_t i = 0; i < executor_num_; i++) {
        thread_list_[i].join();
    }
    thread_list_.clear();
}

void ServiceExecutor::SetExecutorNum(uint32_t num) {
    if (running_) {
        std::cerr << "The executor num cannot be set while running." << std::endl;
    }
    executor_num_ = num;
}

TaskID ServiceExecutor::AddTask(const std::string& name, ExecutorFunctionType function, RequestBase& request) {
    TaskID task_id = next_task_id_.fetch_add(1);
    std::type_index req_type(typeid(request));

    std::shared_ptr<ServiceTask> task = std::make_shared<ServiceTask>(task_id, name, ServiceTaskType::REQUEST, function);

    {
        std::lock_guard<std::mutex> lock(task_handling_lock_);
        service_task_list_[task_id] = task;
        task_request_map_[req_type] = task_id;
    }
    return task_id;
}

TaskID ServiceExecutor::AddTaskInterval(const std::string& name, ExecutorFunctionType function, TaskInterval interval) {
    TaskID task_id = next_task_id_.fetch_add(1);
    std::shared_ptr<ServiceTask> task = std::make_shared<ServiceTask>(task_id, name, ServiceTaskType::INTERVAL, function);
    task->SetInterval(interval);
    task->SetNextExecutionTime(std::chrono::steady_clock::now());

    {
        std::lock_guard<std::mutex> lock(task_handling_lock_);
        service_task_list_[task_id] = task;
    }
    {
        std::lock_guard<std::mutex> lock(task_request_handling_lock_);
        task_interval_queue_.push(task);
        task_request_handling_cv_.notify_one();
    }
    return task_id;
}

TaskID ServiceExecutor::AddTaskOnce(const std::string& name, ExecutorFunctionType function, TaskDelay delay) {
    return 0;
}

void ServiceExecutor::RemoveTask(TaskID task_id) {
    std::lock_guard<std::mutex> lock(task_handling_lock_);

    auto iter = service_task_list_.find(task_id);
    if (iter == service_task_list_.end()) {
        return;
    }
    switch (iter->second->GetType()) {
        case ServiceTaskType::REQUEST: {
            for(auto& iter : task_request_map_) {
                if (iter.second == task_id) {
                    task_request_map_.erase(iter.first);
                    break;
                }
            }
        }
        case ServiceTaskType::UNKNOWN:
        case ServiceTaskType::INTERVAL:
        case ServiceTaskType::ONCE:
        default: {
            // Do nothing
        }
    }

    service_task_list_.erase(task_id);
}

void ServiceExecutor::RemoveTask(const std::string& name) {
    TaskID task_id = 0L;
    {
        std::lock_guard<std::mutex> lock(task_handling_lock_);
        for (auto& iter : service_task_list_) {
            if (iter.second->GetName() == name) {
                task_id = iter.first;
            }
        }
    }
    if (task_id != 0L) {
        RemoveTask(task_id);
    }
}

bool ServiceExecutor::RequestTask(std::shared_ptr<RequestBase> request_msg) {
    std::type_index req_type(typeid(*request_msg));

    {
        std::lock_guard<std::mutex> lock(task_handling_lock_);
        auto iter = task_request_map_.find(req_type);
        if (iter == task_request_map_.end()) {
            return false;
        }
    }
    {
        std::lock_guard<std::mutex> lock(task_request_handling_lock_);
        task_request_queue_.push(request_msg);
        task_request_handling_cv_.notify_one();
    }

    return true;
}

uint64_t ServiceExecutor::GetAllTaskNum() {
    return service_task_list_.size();
}

uint64_t ServiceExecutor::GetRequestTaskNum() {
    uint64_t task_num = 0;

    std::lock_guard<std::mutex> lock(task_handling_lock_);
    for (auto& iter : service_task_list_) {
        if (iter.second->GetType() == ServiceTaskType::REQUEST) {
            ++task_num;
        }
    }
    return task_num;
}

uint64_t ServiceExecutor::GetIntervalTaskNum() {
    uint64_t task_num = 0;

    std::lock_guard<std::mutex> lock(task_handling_lock_);
    for (auto& iter : service_task_list_) {
        if (iter.second->GetType() == ServiceTaskType::INTERVAL) {
            ++task_num;
        }
    }
    return task_num;
}

uint64_t ServiceExecutor::GetOnceTaskNum() {
    uint64_t task_num = 0;

    std::lock_guard<std::mutex> lock(task_handling_lock_);
    for (auto& iter : service_task_list_) {
        if (iter.second->GetType() == ServiceTaskType::ONCE) {
            ++task_num;
        }
    }
    return task_num;
}

uint64_t ServiceExecutor::GetRequestNum() {
    return task_request_queue_.size();
}

void ServiceExecutor::RunScheduler(const uint32_t idx) {
    std::unique_lock lk_request(task_request_handling_lock_, std::defer_lock);
    std::shared_ptr<ServiceTask> task;
    std::shared_ptr<RequestBase> request;
    std::optional<TaskTime> next_wakeup_time;

    running_threads_.fetch_add(1U);
    std::cout << "Starting the executor[" << idx << "]" << std::endl;

    while (running_) {
        // RequestTask handling
        lk_request.lock();
        if (!task_request_queue_.empty()) {
            request = task_request_queue_.front();
            task_request_queue_.pop();
        }
        lk_request.unlock();
        if (request) {
            std::type_index req_type(typeid(*request));
            {
                std::lock_guard<std::mutex> lk_task(task_handling_lock_);
                try {
                    TaskID task_id = task_request_map_.at(req_type);
                    task = service_task_list_.at(task_id);
                } catch (const std::out_of_range& ex) {
                    // No handling
                }
            }
            if (task) {
                task->Execute(request);
                task.reset();
            }
            request.reset();
        }

        // IntervalTask handling
        TaskTime current = std::chrono::steady_clock::now();

        lk_request.lock();
        if (!task_interval_queue_.empty()) {
            task = task_interval_queue_.top();

            if (service_task_list_.count(task->GetTaskId()) == 0) {
                task_interval_queue_.pop();
                task.reset();
                std::cout << "Removed task." << std::endl;
            } else {
                TaskTime execution_time = task->GetNextExecutionTime();
                if (current < execution_time) {
                    next_wakeup_time.emplace(execution_time);
                    task.reset();
                } else{
                    task_interval_queue_.pop();
                    if (!task_interval_queue_.empty()) {
                        next_wakeup_time.emplace(task_interval_queue_.top()->GetNextExecutionTime());
                    }
                }
            }
        }
        lk_request.unlock();
        if (task) {
            task->Execute(nullptr);

            TaskTime next_exec_time = current + task->GetInterval();
            task->SetNextExecutionTime(next_exec_time);
            if (next_wakeup_time.has_value()) {
                TaskTime earliest_next_time = std::min(next_exec_time, next_wakeup_time.value());
                next_wakeup_time.emplace(earliest_next_time);
            } else {
                next_wakeup_time.emplace(next_exec_time);
            }

            lk_request.lock();
            task_interval_queue_.push(task);
            lk_request.unlock();
            task.reset();
        }

        // Executor waiting handling
        lk_request.lock();
        if (task_request_queue_.empty()) {
            if (next_wakeup_time.has_value()) {
                task_request_handling_cv_.wait_until(lk_request, next_wakeup_time.value());
                next_wakeup_time.reset();
            } else {
                task_request_handling_cv_.wait(lk_request);
            }
        }
        lk_request.unlock();
    }

    std::cout << "Terminating the executor[" << idx << "]" << std::endl;
    running_threads_.fetch_sub(1U);
}

} // namespace diagnostic_manager
