#pragma once

#include "driver/gpio.h"

#include <cstddef>
#include <cstdint>

namespace app_config {

namespace pins {

constexpr gpio_num_t irSensor = GPIO_NUM_17;
constexpr gpio_num_t distanceSensor = GPIO_NUM_23;

constexpr gpio_num_t lorawanSpiClock = GPIO_NUM_5;
constexpr gpio_num_t lorawanSpiMosi = GPIO_NUM_27;
constexpr gpio_num_t lorawanSpiMiso = GPIO_NUM_19;
constexpr gpio_num_t lorawanChipSelect = GPIO_NUM_18;
constexpr gpio_num_t lorawanReset = GPIO_NUM_14;
constexpr gpio_num_t lorawanDio0 = GPIO_NUM_26;
constexpr gpio_num_t lorawanDio1 = GPIO_NUM_35;

} // namespace pins

namespace timing {

constexpr std::uint32_t irDebounceMs = 50;
constexpr std::uint32_t measurementIntervalMs = 100;
constexpr std::uint32_t measurementWindowMs = 10000;
constexpr std::uint32_t idleLogIntervalMs = 20000;
constexpr std::uint32_t lorawanJoinRetryMs = 30000;

constexpr std::int64_t pulseWaitTimeoutUs = 3000;
constexpr std::int64_t maximumDistancePulseUs = 1850;

} // namespace timing

namespace classification {

constexpr int minimumDistance = 200;
constexpr int maximumDistance = 250;
constexpr std::size_t requiredConsecutiveSamples = 5;
constexpr float kalmanMeasuredError = 0.1F;

} // namespace classification

namespace distance_sensor {

constexpr std::int64_t pulseZeroOffsetUs = 1000;
constexpr int distanceScaleNumerator = 3;
constexpr int distanceScaleDenominator = 4;

} // namespace distance_sensor

namespace tasks {

constexpr std::uint32_t distanceMeasurementStackSize = 2048;
constexpr unsigned distanceMeasurementPriority = 4;
constexpr std::uint32_t trayMonitorStackSize = 2048;
constexpr unsigned trayMonitorPriority = 5;
constexpr std::uint32_t lorawanStackSize = 4096;
constexpr unsigned lorawanPriority = 3;

} // namespace tasks

namespace lorawan {

constexpr int spiDmaChannel = 1;

} // namespace lorawan

static_assert(classification::minimumDistance <= classification::maximumDistance);
static_assert(classification::requiredConsecutiveSamples > 0);
static_assert(distance_sensor::distanceScaleDenominator != 0);

} // namespace app_config
