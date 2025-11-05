#include "VssSubscriptionManager.h"
#include "Utils.h"
#include "Logger.h"
#include <iostream>
#include <algorithm>
#include <stdexcept>

#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

// ################### Filter Implementations ###################

// A simple time-based filter
class TimeBasedFilter : public viss::IFilter {
private:
    std::chrono::milliseconds m_period;
    mutable std::chrono::steady_clock::time_point m_last_triggered;

public:
    TimeBasedFilter(int period_ms) : m_period(period_ms), m_last_triggered(std::chrono::steady_clock::now()) {}

    bool shouldTrigger(const viss::DataPoint& current, const viss::DataPoint& previous) const override {
        auto now = std::chrono::steady_clock::now();
        if ((now - m_last_triggered) >= m_period) {
            m_last_triggered = now;
            return true;
        }
        return false;
    }

    viss::FilterVariant getType() const override { return viss::FilterVariant::TIMEBASED; }
};

// A simple change-based filter
class ChangeFilter : public viss::IFilter {
public:
    // For simplicity, we assume 'ne' (not equal) logic for any change.
    // A full implementation would parse logic-op and diff.
    bool shouldTrigger(const viss::DataPoint& current, const viss::DataPoint& previous) const override {
        // This is a simplified check. A real implementation would need to handle types properly.
        if (!previous.value.has_value()) return true; // Always trigger on the first data point
        return any_to_string(current.value) != any_to_string(previous.value);
    }
    viss::FilterVariant getType() const override { return viss::FilterVariant::CHANGE; }
};

// A filter for historical data queries.
class HistoryFilter : public viss::IFilter {
private:
    std::string m_timePeriod;

public:
    HistoryFilter(const std::string& timePeriod) : m_timePeriod(timePeriod) {}

    // History filters are for fetching data, not for event-based triggering.
    // This should always allow the data fetch to proceed.
    bool shouldTrigger(const viss::DataPoint& current, const viss::DataPoint& previous) const override {
        return true;
    }

    viss::FilterVariant getType() const override { return viss::FilterVariant::HISTORY; }

    const std::string& getTimePeriod() const { return m_timePeriod; }
};


// ################### VssSubscriptionManager Implementation ###################
VssSubscriptionManager::VssSubscriptionManager(std::shared_ptr<IStorage> ds, std::shared_ptr<ObjectPool<PooledResponseMap>> response_pool, std::shared_ptr<viss::VSSTree> vss_tree)
    : data_store(ds), response_pool_(response_pool), vss_tree_(vss_tree), active_(false) {}

VssSubscriptionManager::~VssSubscriptionManager() {
    stop();
}

// Parses the "filter" part of a subscribe request and creates filter objects.
std::vector<std::shared_ptr<viss::IFilter>> VssSubscriptionManager::parseFilters(const rapidjson::Value& filter_json) {
    std::vector<std::shared_ptr<viss::IFilter>> filters;

    const rapidjson::Value* expressions = &filter_json;
    rapidjson::Document doc;
    if (!filter_json.IsArray()) {
        doc.SetArray();
        doc.PushBack(rapidjson::Value(filter_json, doc.GetAllocator()), doc.GetAllocator());
        expressions = &doc;
    }

    for (const auto& exp : expressions->GetArray()) {
        if (!exp.IsObject() || !exp.HasMember("variant") || !exp["variant"].IsString()) {
            LOG_WARN << "Skipping invalid filter expression object.";
            continue;
        }

        std::string variant = exp["variant"].GetString();
        LOG_INFO << "Parsing filter variant: " << variant;

        if (variant == "timebased" && exp.HasMember("parameter") && exp["parameter"].IsObject()) {
            const auto& param = exp["parameter"];
            if (param.HasMember("period") && param["period"].IsString()) {
                try {
                    int period_ms = std::stoi(param["period"].GetString());
                    filters.push_back(std::make_shared<TimeBasedFilter>(period_ms));
                    LOG_INFO << "  - Created TimeBasedFilter with period: " << period_ms << "ms";
                } catch (const std::invalid_argument& ia) {
                    LOG_ERROR << "Invalid period for timebased filter: " << param["period"].GetString();
                }
            }
        } else if (variant == "change") {
            filters.push_back(std::make_shared<ChangeFilter>());
            LOG_INFO << "  - Created ChangeFilter.";
        } else if (variant == "history" && exp.HasMember("parameter") && exp["parameter"].IsString()) {
            std::string timePeriod = exp["parameter"].GetString();
            filters.push_back(std::make_shared<HistoryFilter>(timePeriod));
            LOG_INFO << "  - Created HistoryFilter with time period: " << timePeriod;
        } else {
            LOG_WARN << "Unsupported or malformed filter variant: " << variant;
        }
    }

    return filters;
}

std::string VssSubscriptionManager::addSubscription(std::shared_ptr<RequestData> request) {
    std::lock_guard<std::mutex> lock(mtx);
    std::string subscriptionId = std::to_string(nextSubscriptionId++);
    
    auto sub = std::make_shared<Subscription>();
    sub->subscriptionId = subscriptionId;
    sub->originalRequest = request;
    sub->active = true;

    // Determine trigger path and watched paths
    const auto& req_doc = *request->request_doc;
    std::string path_str = req_doc.HasMember("path") ? req_doc["path"].GetString() : "";
    sub->watchedPaths = vss_tree_->getPaths(path_str, request->request_doc.get());
    if (sub->watchedPaths.empty()) {
        // No valid paths found for this subscription.
        return ""; 
    }

    LOG_DEBUG << "Subscription " << subscriptionId << " watching " << sub->watchedPaths.size() << " path(s):";
    for(const auto& p : sub->watchedPaths) {
        LOG_DEBUG << "  - " << p;
    }
    // VISS rule: The first path is the trigger path.
    // When a request addresses multiple signals, the triggering condition SHALL be evaluated for only one signal, 
    // specifically the first signal in the paths parameter array.
    sub->triggerSignalPath = sub->watchedPaths[0];

    // Parse filters from the request document
    if (req_doc.HasMember("filter")) {
       sub->filters = parseFilters(req_doc["filter"]);
    }

    m_subscriptions[subscriptionId] = sub;
    m_triggerMap[sub->triggerSignalPath].push_back(sub);

    return subscriptionId;
}

bool VssSubscriptionManager::removeSubscription(const std::string& subscriptionId) {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = m_subscriptions.find(subscriptionId);
    if (it != m_subscriptions.end()) {
        auto sub = it->second;
        sub->active = false;

        // Remove from trigger map
        auto& subs_for_trigger = m_triggerMap[sub->triggerSignalPath];
        subs_for_trigger.erase(
            std::remove_if(subs_for_trigger.begin(), subs_for_trigger.end(),
                           [&](const auto& s) { return s->subscriptionId == subscriptionId; }),
            subs_for_trigger.end());
        
        if (subs_for_trigger.empty()) {
            m_triggerMap.erase(sub->triggerSignalPath);
        }
        
        m_subscriptions.erase(it);
        return true;
    }
    return false;
}

std::map<std::string, std::shared_ptr<VssSubscriptionManager::Subscription>> VssSubscriptionManager::getAllSubscriptions() {
    std::lock_guard<std::mutex> lock(mtx);
    return m_subscriptions;
}

void VssSubscriptionManager::run() {
    active_ = true;
    publisher_thread_ = std::thread([this]() {
        while (active_.load()) {
            std::map<std::string, std::shared_ptr<Subscription>> subscriptions;
            {
                std::lock_guard<std::mutex> lock(mtx);
                subscriptions = m_subscriptions;
            }

            for (const auto& pair : subscriptions) {
                const auto& sub = pair.second;
                if (!sub->active.load()) continue;

                bool all_filters_passed = false;
                std::string trigger_path = sub->triggerSignalPath;
                
                // The trigger logic is always based on the latest value.
                auto latest_values = data_store->getValues(trigger_path);
                viss::DataPoint current_dp = latest_values.empty() ? viss::DataPoint{std::any(), 0} : latest_values[0];
                viss::DataPoint previous_dp = sub->lastState.count(trigger_path) ? sub->lastState.at(trigger_path) : viss::DataPoint{std::any(), 0};

                if (sub->filters.empty()) {
                    all_filters_passed = true; // No filters, always trigger
                } else {
                    for (const auto& filter : sub->filters) {
                        if (filter->shouldTrigger(current_dp, previous_dp)) {
                            all_filters_passed = true;
                            break; 
                        }
                    }
                }
                
                if (all_filters_passed) {
                    // Update the state for the trigger path
                    sub->lastState[trigger_path] = current_dp;

                    // Prepare and send notification
                    auto notification = response_pool_->acquire();
                    notification->SetObject();
                    rapidjson::Document::AllocatorType& allocator = notification->GetAllocator();

                    notification->AddMember("action", "subscription", allocator);
                    notification->AddMember("subscriptionId", rapidjson::Value(sub->subscriptionId.c_str(), allocator), allocator);
                    
                    // Find if a history filter exists to get the time period
                    std::string timePeriod = "";
                    for (const auto& f : sub->filters) {
                        if (f->getType() == viss::FilterVariant::HISTORY) {
                            timePeriod = static_cast<HistoryFilter*>(f.get())->getTimePeriod();
                            break;
                        }
                    }

                    rapidjson::Value data_array(rapidjson::kArrayType);
                    for (const auto& p : sub->watchedPaths) {
                        auto points = data_store->getValues(p, timePeriod);
                        if (points.empty()) continue;

                        rapidjson::Value dp_list(rapidjson::kArrayType);
                        for (const auto& point : points) {
                            rapidjson::Value dp_obj(rapidjson::kObjectType);
                            dp_obj.AddMember("value", any_to_json_value(point.value, allocator), allocator);
                            dp_obj.AddMember("ts", rapidjson::Value(formatTimestamp(point.timestamp).c_str(), allocator), allocator);
                            dp_list.PushBack(dp_obj, allocator);
                        }

                        rapidjson::Value data_obj(rapidjson::kObjectType);
                        data_obj.AddMember("path", rapidjson::Value(p.c_str(), allocator), allocator);

                        if (dp_list.Size() == 1) {
                            data_obj.AddMember("dp", dp_list[0], allocator);
                        } else {
                            data_obj.AddMember("dp", dp_list, allocator);
                        }
                        
                        data_array.PushBack(data_obj, allocator);
                    }

                    if (data_array.Size() == 1) {
                        notification->AddMember("data", data_array[0], allocator);
                    } else {
                        notification->AddMember("data", data_array, allocator);
                    }
                    
                    notification->AddMember("ts", rapidjson::Value(getCurrentTimestamp().c_str(), allocator), allocator);

                    if (sub->originalRequest->responseChan) {
                        sub->originalRequest->responseChan->push(notification);
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });
}

void VssSubscriptionManager::stop() {
    LOG_INFO << "VssSubscriptionManager::stop() called.";
    active_ = false;
    if (publisher_thread_.joinable()) {
        publisher_thread_.join();
    }
    LOG_INFO << "VssSubscriptionManager::stop() completed.";
}
