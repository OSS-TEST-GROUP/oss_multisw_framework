#ifndef IPROVIDER_H
#define IPROVIDER_H

#include <string>
#include <memory>
#include "Types.h"
#include "ThreadSafeQueue.h"
#include "VssTree.h"

class IProvider {
public:
    virtual ~IProvider() = default;

    // Provider를 시작합니다.
    virtual void start() = 0;

    // Provider를 중지합니다.
    virtual void stop() = 0;

    // Provider의 이름을 반환합니다.
    virtual std::string getName() const = 0;

    // 데이터 채널을 설정합니다.
    virtual void setDataChannel(const std::shared_ptr<ThreadSafeQueue<VehicleData>>& channel) = 0;

    // 외부로부터 값을 설정합니다. (Actuator 제어 등)
    virtual void setValue(const std::string& path, const std::any& value) = 0;

    // VSS 트리를 설정합니다.
    virtual void setVssTree(std::shared_ptr<viss::VSSTree> vss_tree) = 0;
};

#endif // IPROVIDER_H
