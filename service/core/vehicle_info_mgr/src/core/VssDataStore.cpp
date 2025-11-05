#include "IStorage.h"
#include "Utils.h"
#include <chrono>
#include <thread>
#include <iostream>

VssDataStore::VssDataStore() : m_active(true) {
    m_archiver_thread = std::thread(&VssDataStore::archiveLoop, this);
}

VssDataStore::~VssDataStore() {
    m_active = false;
    if (m_archiver_thread.joinable()) {
        m_archiver_thread.join();
    }
}

void VssDataStore::setValue(const VehicleData& data) {
    std::lock_guard<std::mutex> lock(mtx);
    m_latest_store[data.path] = {data.value, data.timestamp};
}

std::vector<viss::DataPoint> VssDataStore::getValues(const std::string& path, const std::string& timePeriod) {
    std::lock_guard<std::mutex> lock(mtx);

    // If the path has never been seen in either store, return "not available".
    if (m_latest_store.find(path) == m_latest_store.end() && m_history_store.find(path) == m_history_store.end()) {
        return {
            viss::DataPoint{
                "viss-inline:Data-not-available",
                std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count()
            }
        };
    }
    
    if (timePeriod.empty()) {
        // No time period, return only the latest value if it exists.
        if (m_latest_store.count(path)) {
            return {m_latest_store.at(path)};
        }
    } else {
        long long durationMs = parseTimePeriod(timePeriod);
        if (durationMs <= 0) {
            // Invalid or no time period, return only the latest value
            if (m_latest_store.count(path)) {
                return {m_latest_store.at(path)};
            }
            return {};
        }

        auto now = std::chrono::system_clock::now();
        auto cutoff_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() - durationMs;

        std::vector<viss::DataPoint> results;
        if (m_history_store.count(path)) {
            const auto& history = m_history_store.at(path);
            for (const auto& point : history) {
                if (point.timestamp >= cutoff_time_ms) {
                    results.push_back(point);
                }
            }
        }
        if (m_latest_store.count(path)) {
             const auto& latest_point = m_latest_store.at(path);
             if (latest_point.timestamp >= cutoff_time_ms) {
                results.push_back(latest_point);
             }
        }
        return results;
    }
    
    return {}; // Return empty vector if not found
}

void VssDataStore::archiveLoop() {
    while (m_active) {
        std::this_thread::sleep_for(std::chrono::seconds(10));

        std::lock_guard<std::mutex> lock(mtx);
        if (!m_active) break;

        // Copy latest values to history
        for (const auto& pair : m_latest_store) {
            const std::string& path = pair.first;
            const viss::DataPoint& point = pair.second;

            std::cout << "Archiving data for path: " << path << std::endl;

            auto& history_deque = m_history_store[path];
            history_deque.push_back(point);

            // Ensure the deque does not exceed the max size
            if (history_deque.size() > MAX_HISTORY_SIZE) {
                history_deque.pop_front();
            }
        }
    }
}
