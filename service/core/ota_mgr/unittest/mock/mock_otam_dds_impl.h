#ifndef MOCK_OTAM_DDS_IMPL_H_
#define MOCK_OTAM_DDS_IMPL_H_

#include "otam_common/dds_abstraction.h"
#include <gmock/gmock.h>
#include <memory>
#include <string>

/**
 * @file mock_otam_dds_impl.h
 * @brief Mock implementation of DDS abstraction for unit testing
 * @author OTA Manager Team
 * @date 2024
 */

/**
 * @class MockOtamDdsImpl
 * @brief Mock implementation of DdsAbstraction for unit testing
 * 
 * This mock class allows unit tests to verify DDS interactions without
 * requiring actual DDS infrastructure. It uses Google Mock framework
 * to provide mock implementations of all DDS operations.
 */
class MockOtamDdsImpl : public DdsAbstraction {
public:
    /**
     * @brief Default constructor
     */
    MockOtamDdsImpl() = default;

    /**
     * @brief Virtual destructor
     */
    ~MockOtamDdsImpl() override = default;

    // Mock methods corresponding to DdsAbstraction interface
    MOCK_METHOD(void, Create, (const std::string& topic, std::shared_ptr<void> type, std::shared_ptr<void> listener), (override));
    MOCK_METHOD(void, SetQos, (uint32_t qos), (override));
    MOCK_METHOD(void, Write, (std::shared_ptr<void> message), (override));
    MOCK_METHOD(void, Read, (std::shared_ptr<void> message), (override));
};

#endif // MOCK_OTAM_DDS_IMPL_H_