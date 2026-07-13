#include "dish_counter_logic.h"

DishClassifier::DishClassifier(DishClassificationConfig config)
    : config_(config)
{
}

bool DishClassifier::recordSample(int distance)
{
    if (classified_) {
        return false;
    }

    const bool inRange = distance >= config_.minimumDistance
        && distance <= config_.maximumDistance;
    if (!inRange) {
        consecutiveSamples_ = 0;
        return false;
    }

    ++consecutiveSamples_;
    if (consecutiveSamples_ < config_.requiredConsecutiveSamples) {
        return false;
    }

    classified_ = true;
    return true;
}

void DishClassifier::reset()
{
    consecutiveSamples_ = 0;
    classified_ = false;
}

bool DishClassifier::classified() const
{
    return classified_;
}

std::size_t DishClassifier::consecutiveSamples() const
{
    return consecutiveSamples_;
}

ScalarKalmanFilter::ScalarKalmanFilter(float measuredError)
    : measuredError_(measuredError)
{
}

float ScalarKalmanFilter::update(float measurement)
{
    const float predictedError = estimatedError_ + measuredError_;
    const float gain = predictedError / (predictedError + measuredError_);
    currentEstimate_ += gain * (measurement - currentEstimate_);
    estimatedError_ = (1.0F - gain) * predictedError;
    return currentEstimate_;
}

void ScalarKalmanFilter::reset(float initialEstimate, float initialError)
{
    currentEstimate_ = initialEstimate;
    estimatedError_ = initialError;
}
