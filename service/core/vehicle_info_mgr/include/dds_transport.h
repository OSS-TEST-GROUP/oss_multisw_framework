#ifndef DDS_TRANSPORT_H
#define DDS_TRANSPORT_H

#include <memory>
#include "ITransport.h"
#include "ObjectPool.h"

// Forward declarations
struct DDSTransportImpl;

class DDSTransport : public ITransport {
public:
    DDSTransport(std::shared_ptr<ObjectPool<RequestData>> pool, std::shared_ptr<ThreadSafeQueue<std::shared_ptr<RequestData>>> queue);
    ~DDSTransport();
    
    void start() override;
    void stop() override;

private:
    DDSTransportImpl* p_impl;
};

#endif // DDS_TRANSPORT_H
