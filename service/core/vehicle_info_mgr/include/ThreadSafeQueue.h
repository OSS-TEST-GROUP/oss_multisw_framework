#ifndef THREAD_SAFE_QUEUE_H
#define THREAD_SAFE_QUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>

template <typename T>
class ThreadSafeQueue {
public:
    ThreadSafeQueue() : valid_(true) {}

    template<typename U>
    void push(U&& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!valid_) return;
        queue_.push(std::forward<U>(value));
        cond_.notify_one();
    }

    bool wait_and_pop(T& value) {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this] { return !queue_.empty() || !valid_; });
        if (!valid_ && queue_.empty()) {
            return false;
        }
        value = std::move(queue_.front());
        queue_.pop();
        return true;
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    void invalidate() {
        std::lock_guard<std::mutex> lock(mutex_);
        valid_ = false;
        cond_.notify_all();
    }

private:
    mutable std::mutex mutex_;
    std::queue<T> queue_;
    std::condition_variable cond_;
    std::atomic<bool> valid_;
};

#endif // THREAD_SAFE_QUEUE_H
