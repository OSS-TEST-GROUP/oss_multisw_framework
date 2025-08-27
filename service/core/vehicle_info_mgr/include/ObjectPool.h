#ifndef OBJECT_POOL_H
#define OBJECT_POOL_H

#include <memory>
#include <mutex>
#include <vector>
#include <functional>
#include <iostream>
#include <typeinfo>

template <typename T>
class ObjectPool : public std::enable_shared_from_this<ObjectPool<T>> {
public:
    using Ptr = std::shared_ptr<T>;

    ObjectPool() = default;
    virtual ~ObjectPool() {
        // Properly clean up raw pointers in the pool
        for (T* ptr : pool_) {
            delete ptr;
        }
    }

    // Disable copy and move constructors/assignments
    ObjectPool(const ObjectPool&) = delete;
    ObjectPool& operator=(const ObjectPool&) = delete;
    ObjectPool(ObjectPool&&) = delete;
    ObjectPool& operator=(ObjectPool&&) = delete;

    Ptr acquire() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (pool_.empty()) {
            std::cout << "[ObjectPool<" << typeid(T).name() << ">] Pool is empty. Creating a new object." << std::endl;
            // 1. std::shared_ptr<T>를 생성합니다. 이 스마트 포인터는 'new T()'로 생성된 새로운 객체를 관리합니다.
            // 2. 두 번째 인자로 "커스텀 Deleter"라는 람다 함수를 전달합니다.
            //    - C++의 std::shared_ptr는 자신이 관리하는 객체의 참조 카운트가 0이 되면,
            //      기본적으로는 'delete'를 호출하여 메모리를 해제합니다.
            //    - 하지만 커스텀 Deleter를 제공하면, 메모리 해제 대신 우리가 지정한 동작을 수행하게 할 수 있습니다.
            // 3. 이 커스텀 Deleter는 람다 함수 `[self = this->shared_from_this()](T* ptr) { self->release(ptr); }` 입니다.
            //    - `[self = ...]` : C++17의 "캡처 초기화" 기능입니다. 람다 함수가 생성될 때 'ObjectPool' 자신의 shared_ptr를 'self'라는 이름으로 캡처(저장)해 둡니다.
            //                        이는 람다 함수가 호출되는 시점(객체가 소멸될 때)에도 'ObjectPool' 인스턴스가 유효함을 보장하기 위함입니다.
            //    - `(T* ptr)` : Deleter는 삭제할 객체의 raw 포인터를 인자로 받습니다.
            //    - `{ self->release(ptr); }` : 참조 카운트가 0이 되면, `delete ptr;` 대신 `self->release(ptr);` 함수를 호출합니다.
            //                           즉, 객체를 메모리에서 해제하는 대신, 우리 ObjectPool의 `release` 함수를 호출하여 풀에 다시 반납하도록 합니다.
            return Ptr(new T(), [self = this->shared_from_this()](T* ptr) {
                self->release(ptr);
            });
        } else {
            std::cout << "[ObjectPool<" << typeid(T).name() << ">] Reusing an object from the pool. Pool size was: " << pool_.size() << std::endl;
            // Reuse an object from the pool
            T* ptr = pool_.back();
            pool_.pop_back();
            return Ptr(ptr, [self = this->shared_from_this()](T* p) {
                self->release(p);
            });
        }
    }

private:
    void release(T* ptr) {
        ptr->reset(); // Reset object state
        {
            std::lock_guard<std::mutex> lock(mutex_);
            pool_.push_back(ptr);
            std::cout << "[ObjectPool<" << typeid(T).name() << ">] Object released to the pool. Pool size is now: " << pool_.size() << std::endl;
        }
    }

    std::mutex mutex_;
    std::vector<T*> pool_; // Store raw pointers to avoid recursive shared_ptr deleters
};

#endif // OBJECT_POOL_H
