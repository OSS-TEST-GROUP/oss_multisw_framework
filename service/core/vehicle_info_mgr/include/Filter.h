#ifndef __FILTER_H__
#define __FILTER_H__

#include "Types.h"
#include <memory>
#include <string>
#include <vector>

namespace viss {

// Defines the types of filters available, corresponding to VISS spec variants.
enum class FilterVariant {
    TIMEBASED,
    RANGE,
    CHANGE,
    CURVELOG,
    HISTORY,
    METADATA,
    UNKNOWN
};

/**
 * @brief An interface for all filter types.
 * Each concrete filter class will implement this interface to provide
 * specific trigger logic based on VISS filter variants.
 */
class IFilter {
public:
    virtual ~IFilter() = default;

    /**
     * @brief Determines whether a notification should be triggered based on the filter's logic.
     *
     * @param current The current data point of the signal being watched.
     * @param previous The previous data point of the signal.
     * @return true if the condition is met and a notification should be sent.
     * @return false otherwise.
     */
    virtual bool shouldTrigger(const DataPoint& current, const DataPoint& previous) const = 0;

    /**
     * @brief Get the type of the filter.
     *
     * @return The FilterVariant enum value.
     */
    virtual FilterVariant getType() const = 0;
};

} // namespace viss

#endif // __FILTER_H__
