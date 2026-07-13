#pragma once

#include <cstddef>

namespace dish_counter_defaults {

constexpr int minimumDistance = 200;
constexpr int maximumDistance = 250;
constexpr std::size_t requiredConsecutiveSamples = 5;
constexpr float kalmanMeasuredError = 0.1F;

static_assert(minimumDistance <= maximumDistance);
static_assert(requiredConsecutiveSamples > 0);

} // namespace dish_counter_defaults
