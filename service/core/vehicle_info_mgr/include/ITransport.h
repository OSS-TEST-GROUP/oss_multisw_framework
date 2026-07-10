#ifndef ITRANSPORT_H
#define ITRANSPORT_H

#include <string>
#include <any>
#include <map>
#include <memory>
#include <vector>
#include "ThreadSafeQueue.h"

// Data structure for requests passed through the central queue
#include <iostream>
#include "Types.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "rapidjson/document.h"
#include "Logger.h"

// A rapidjson Document wrapper with a reset method for object pooling
struct PooledResponseMap : public rapidjson::Document {
    void reset() {
        // Reset the document to a null object, ready for reuse.
        this->SetObject();
        // rapidjson's MemoryPoolAllocator is a bump allocator: Free() is a
        // no-op and memory is reclaimed only by Clear(). SetObject() alone
        // resets the DOM value but leaves every previously allocated chunk in
        // place, so a pooled Document's allocator grows monotonically with
        // each reuse — on the ~1s/subscription notification hot path this was
        // an unbounded leak. Clear() releases the chunks; the empty object
        // holds no allocator memory, so clearing here is safe.
        this->GetAllocator().Clear();
    }
};

struct RequestData {
    // Identity
    std::string requestId;

    // Request payload
    std::unique_ptr<rapidjson::Document> request_doc;
    
    // Channel for the response
    std::shared_ptr<ThreadSafeQueue<std::shared_ptr<PooledResponseMap>>> responseChan;

    RequestData() : request_doc(std::make_unique<rapidjson::Document>()) {}

    void print() const {
        LOG_INFO << "RequestData {";
        LOG_INFO << "  requestId: " << requestId;
        
        if (request_doc) {
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            request_doc->Accept(writer);
            LOG_INFO << "  request_doc: " << buffer.GetString();
        } else {
            LOG_INFO << "  request_doc: (null)";
        }
        LOG_INFO << "}";
    }

    void reset() {
        requestId.clear();
        if (request_doc) {
            request_doc->SetNull();
            // Same MemoryPoolAllocator semantics as PooledResponseMap::reset():
            // SetNull() drops the value but not the allocator chunks, so pooled
            // RequestData accumulated every parsed request DOM across reuses.
            request_doc->GetAllocator().Clear();
        }
        responseChan.reset();
    }
};

// Common interface for all transport layers (gRPC, MQTT, WS, etc.)
class ITransport {
public:
    virtual ~ITransport() = default;
    virtual void start() = 0;
    virtual void stop() = 0;
};

#endif // ITRANSPORT_H
