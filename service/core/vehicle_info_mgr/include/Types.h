#ifndef TYPES_H
#define TYPES_H

#include <string>
#include <any>

// A simple structure to hold vehicle data
struct VehicleData {
    std::string path;
    std::any value;
    long long timestamp;
};

#include <vector>

// Data structure for holding a value and its timestamp.
namespace viss {
    struct DataPoint {
        std::any value;
        long long timestamp;
    };
}

// Data structure for notifications sent to subscribers
struct NotificationData {
    std::string subscriptionId;
    std::string path;
    std::any value;
    long long timestamp;
};

#endif // TYPES_H
