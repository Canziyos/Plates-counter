#include "dish_counter_logic.h"

#include <cmath>
#include <cstdlib>
#include <iostream>

namespace {

int failures = 0;

void expect(bool condition, const char *message)
{
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

DishClassifier makeClassifier()
{
    return DishClassifier({200, 250, 5});
}

void testCountsAfterRequiredStreak()
{
    DishClassifier classifier = makeClassifier();
    for (int sample = 0; sample < 4; ++sample) {
        expect(!classifier.recordSample(225), "must not classify before five samples");
    }

    expect(classifier.recordSample(225), "fifth consecutive sample must classify");
    expect(classifier.classified(), "classifier must remember the result");
    expect(!classifier.recordSample(225), "one window must classify only once");
}

void testInvalidSampleResetsStreak()
{
    DishClassifier classifier = makeClassifier();
    classifier.recordSample(225);
    classifier.recordSample(225);
    classifier.recordSample(-1);

    expect(classifier.consecutiveSamples() == 0, "sensor timeout must reset the streak");
    for (int sample = 0; sample < 4; ++sample) {
        expect(!classifier.recordSample(225), "streak after timeout must start from zero");
    }
    expect(classifier.recordSample(225), "a fresh five-sample streak must classify");
}

void testRangeBoundariesAreInclusive()
{
    DishClassifier classifier = makeClassifier();
    const int boundarySamples[] = {200, 250, 200, 250};
    for (const int distance : boundarySamples) {
        expect(!classifier.recordSample(distance), "range boundaries must continue the streak");
    }
    expect(classifier.recordSample(200), "five boundary samples must classify");

    classifier.reset();
    classifier.recordSample(225);
    expect(!classifier.recordSample(199), "below-range sample must not classify");
    expect(classifier.consecutiveSamples() == 0, "below-range sample must reset the streak");

    classifier.recordSample(225);
    expect(!classifier.recordSample(251), "above-range sample must not classify");
    expect(classifier.consecutiveSamples() == 0, "above-range sample must reset the streak");
}

void testResetStartsNewWindow()
{
    DishClassifier classifier = makeClassifier();
    for (int sample = 0; sample < 5; ++sample) {
        classifier.recordSample(225);
    }
    classifier.reset();

    expect(!classifier.classified(), "reset must clear the classification result");
    expect(classifier.consecutiveSamples() == 0, "reset must clear the streak");
}

void testKalmanFilterConvergesAndResets()
{
    ScalarKalmanFilter filter;
    const float first = filter.update(240.0F);
    const float second = filter.update(240.0F);

    expect(first > 0.0F && first < 240.0F, "first estimate must move toward the measurement");
    expect(second > first && second < 240.0F, "repeated estimate must converge toward the measurement");

    filter.reset();
    const float afterReset = filter.update(240.0F);
    expect(std::fabs(afterReset - first) < 0.001F, "reset must restore the initial filter state");
}

} // namespace

int main()
{
    testCountsAfterRequiredStreak();
    testInvalidSampleResetsStreak();
    testRangeBoundariesAreInclusive();
    testResetStartsNewWindow();
    testKalmanFilterConvergesAndResets();

    if (failures != 0) {
        std::cerr << failures << " assertion(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "All dish-counter logic tests passed\n";
    return EXIT_SUCCESS;
}
