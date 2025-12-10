#ifndef ISTORAGE_H
#define ISTORAGE_H

#include "Types.h"
#include <string>
#include <vector>
#include <deque>
#include <memory>
#include <map>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>

class IStorage {
public:
    virtual ~IStorage() = default;

    virtual void setValue(const VehicleData& data) = 0;
    virtual std::vector<viss::DataPoint> getValues(const std::string& path, const std::string& timePeriod = "") = 0;
};

// Manages VSS signal data in memory, implementing the IStorage interface
class VssDataStore : public IStorage {
public:
    VssDataStore();
    ~VssDataStore();

    void setValue(const VehicleData& data) override;
    std::vector<viss::DataPoint> getValues(const std::string& path, const std::string& timePeriod = "") override;

private:
    void archiveLoop();

    // Store only the latest data point for each path for fast access.
    std::map<std::string, viss::DataPoint> m_latest_store;
    // Store a history of data points for each path.
    std::map<std::string, std::deque<viss::DataPoint>> m_history_store;
    
    mutable std::mutex mtx;
    std::thread m_archiver_thread;
    std::atomic<bool> m_active{false};
    std::condition_variable m_cv;
    const size_t MAX_HISTORY_SIZE = 100;
};


#endif // ISTORAGE_H
