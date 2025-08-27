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

// A rapidjson Document wrapper with a reset method for object pooling
struct PooledResponseMap : public rapidjson::Document {
    void reset() {
        // Reset the document to a null object, ready for reuse.
        this->SetObject();
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
        std::cout << "RequestData {" << std::endl;
        std::cout << "  requestId: " << requestId << std::endl;
        
        if (request_doc) {
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            request_doc->Accept(writer);
            std::cout << "  request_doc: " << buffer.GetString() << std::endl;
        } else {
            std::cout << "  request_doc: (null)" << std::endl;
        }
        std::cout << "}" << std::endl;
    }

    void reset() {
        requestId.clear();
        if (request_doc) {
            request_doc->SetNull();
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
