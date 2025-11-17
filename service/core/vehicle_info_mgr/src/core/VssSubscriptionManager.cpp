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


// ################### VssSubscriptionManager::Subscription Implementation ###################

VssSubscriptionManager::Subscription::Subscription(
    std::string id,
    std::shared_ptr<RequestData> request,
    std::shared_ptr<viss::VSSTree> vss_tree,
    std::shared_ptr<IStorage> data_store,
    std::shared_ptr<ObjectPool<PooledResponseMap>> response_pool)
    : m_subscriptionId(std::move(id)),
      m_originalRequest(std::move(request)),
      m_active(true),
      m_data_store(std::move(data_store)),
      m_response_pool(std::move(response_pool)) {
    
    const auto& req_doc = *m_originalRequest->request_doc;
    std::string path_str = req_doc.HasMember("path") ? req_doc["path"].GetString() : "";
    m_watchedPaths = vss_tree->getPaths(path_str, &req_doc);

    if (!m_watchedPaths.empty()) {
        m_triggerSignalPath = m_watchedPaths[0];
    }

    if (req_doc.HasMember("filter")) {
        m_filters = parseFilters(req_doc["filter"]);
    }
    LOG_INFO << "Created Subscription ID: " << m_subscriptionId << " for trigger path: " << m_triggerSignalPath;
}

VssSubscriptionManager::Subscription::~Subscription() {
    if (m_originalRequest && m_originalRequest->responseChan) {
        LOG_INFO << "Sending termination signal for subscription ID: " << m_subscriptionId;
        try {
            auto termination_signal = m_response_pool->acquire();
            termination_signal->SetObject();
            rapidjson::Document::AllocatorType& allocator = termination_signal->GetAllocator();
            termination_signal->AddMember("action", "subscription", allocator);
            termination_signal->AddMember("subscriptionId", rapidjson::Value(m_subscriptionId.c_str(), allocator), allocator);
            termination_signal->AddMember("status", "terminated", allocator);
            m_originalRequest->responseChan->push(termination_signal);
        } catch (const std::exception& e) {
            LOG_ERROR << "Exception in Subscription destructor for ID " << m_subscriptionId << ": " << e.what();
        }
    }
    LOG_INFO << "Destroyed Subscription ID: " << m_subscriptionId;
}

void VssSubscriptionManager::Subscription::process() {
    if (!m_active.load()) {
        return;
    }

    auto latest_values = m_data_store->getValues(m_triggerSignalPath);
    viss::DataPoint current_dp = latest_values.empty() ? viss::DataPoint{std::any(), 0} : latest_values[0];
    viss::DataPoint previous_dp = m_lastState.count(m_triggerSignalPath) ? m_lastState.at(m_triggerSignalPath) : viss::DataPoint{std::any(), 0};

    bool all_filters_passed = false;
    if (m_filters.empty()) {
        all_filters_passed = true;
    } else {
        for (const auto& filter : m_filters) {
            if (filter->shouldTrigger(current_dp, previous_dp)) {
                all_filters_passed = true;
                break;
            }
        }
    }

    if (all_filters_passed) {
        m_lastState[m_triggerSignalPath] = current_dp;

        auto notification = m_response_pool->acquire();
        notification->SetObject();
        rapidjson::Document::AllocatorType& allocator = notification->GetAllocator();

        notification->AddMember("action", "subscription", allocator);
        notification->AddMember("subscriptionId", rapidjson::Value(m_subscriptionId.c_str(), allocator), allocator);

        std::string timePeriod = "";
        for (const auto& f : m_filters) {
            if (f->getType() == viss::FilterVariant::HISTORY) {
                timePeriod = static_cast<HistoryFilter*>(f.get())->getTimePeriod();
                break;
            }
        }

        rapidjson::Value data_array(rapidjson::kArrayType);
        for (const auto& p : m_watchedPaths) {
            auto points = m_data_store->getValues(p, timePeriod);
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

        if (m_originalRequest && m_originalRequest->responseChan) {
            m_originalRequest->responseChan->push(notification);
        }
    }
}

bool VssSubscriptionManager::Subscription::isActive() const {
    return m_active.load();
}

void VssSubscriptionManager::Subscription::stop() {
    m_active = false;
}

const std::string& VssSubscriptionManager::Subscription::getTriggerPath() const {
    return m_triggerSignalPath;
}

const std::string& VssSubscriptionManager::Subscription::getSubscriptionId() const {
    return m_subscriptionId;
}

std::vector<std::shared_ptr<viss::IFilter>> VssSubscriptionManager::Subscription::parseFilters(const rapidjson::Value& filter_json) {
    std::vector<std::shared_ptr<viss::IFilter>> filters;
    const rapidjson::Value* expressions = &filter_json;

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


// ################### VssSubscriptionManager Implementation ###################
VssSubscriptionManager::VssSubscriptionManager(std::shared_ptr<IStorage> ds, std::shared_ptr<ObjectPool<PooledResponseMap>> response_pool, std::shared_ptr<viss::VSSTree> vss_tree)
    : data_store(ds), response_pool_(response_pool), vss_tree_(vss_tree), active_(false) {}

VssSubscriptionManager::~VssSubscriptionManager() {
    stop();
}

std::string VssSubscriptionManager::addSubscription(std::shared_ptr<RequestData> request) {
    std::lock_guard<std::mutex> lock(mtx);
    std::string subscriptionId = std::to_string(nextSubscriptionId++);
    
    auto sub = std::make_shared<Subscription>(subscriptionId, request, vss_tree_, data_store, response_pool_);

    if (sub->getTriggerPath().empty()) {
        LOG_WARN << "Could not create subscription. No valid paths found for request: " << request->requestId;
        return "";
    }
    
    m_subscriptions[subscriptionId] = sub;
    m_triggerMap[sub->getTriggerPath()].push_back(sub);

    return subscriptionId;
}

bool VssSubscriptionManager::removeSubscription(const std::string& subscriptionId) {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = m_subscriptions.find(subscriptionId);
    if (it != m_subscriptions.end()) {
        auto sub = it->second;
        
        std::string trigger_path_copy = sub->getTriggerPath();

        if (m_triggerMap.count(trigger_path_copy)) {
            auto& subs_for_trigger = m_triggerMap.at(trigger_path_copy);
            subs_for_trigger.erase(
                std::remove_if(subs_for_trigger.begin(), subs_for_trigger.end(),
                               [&](const auto& s) { return s->getSubscriptionId() == subscriptionId; }),
                subs_for_trigger.end());
            
            if (subs_for_trigger.empty()) {
                m_triggerMap.erase(trigger_path_copy);
            }
        }
        
        m_subscriptions.erase(it); // This will trigger the Subscription's destructor
        LOG_INFO << "Removed subscription ID: " << subscriptionId;
        return true;
    }
    LOG_WARN << "Attempted to remove non-existent subscription ID: " << subscriptionId;
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
            std::map<std::string, std::shared_ptr<Subscription>> subscriptions_copy;
            {
                std::lock_guard<std::mutex> lock(mtx);
                subscriptions_copy = m_subscriptions;
            }

            for (const auto& pair : subscriptions_copy) {
                if (pair.second) {
                    pair.second->process();
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
    
    // Clear all subscriptions, which will trigger their destructors
    std::lock_guard<std::mutex> lock(mtx);
    m_subscriptions.clear();
    m_triggerMap.clear();

    LOG_INFO << "VssSubscriptionManager::stop() completed.";
}
