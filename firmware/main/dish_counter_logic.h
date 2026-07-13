#pragma once

#include <cstddef>

struct DishClassificationConfig {
    int minimumDistance;
    int maximumDistance;
    std::size_t requiredConsecutiveSamples;
};

class DishClassifier {
public:
    explicit DishClassifier(DishClassificationConfig config);

    // Returns true exactly once when the configured sample streak is reached.
    bool recordSample(int distance);
    void reset();

    bool classified() const;
    std::size_t consecutiveSamples() const;

private:
    DishClassificationConfig config_;
    std::size_t consecutiveSamples_ = 0;
    bool classified_ = false;
};

class ScalarKalmanFilter {
public:
    explicit ScalarKalmanFilter(float measuredError = 0.1F);

    float update(float measurement);
    void reset(float initialEstimate = 0.0F, float initialError = 1.0F);

private:
    float measuredError_;
    float estimatedError_ = 1.0F;
    float currentEstimate_ = 0.0F;
};
