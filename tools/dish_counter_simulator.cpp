#include "dish_counter_defaults.h"
#include "dish_counter_logic.h"

#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

namespace {

std::string trim(const std::string &value)
{
    std::size_t first = 0;
    while (first < value.size() && std::isspace(static_cast<unsigned char>(value[first]))) {
        ++first;
    }

    std::size_t last = value.size();
    while (last > first && std::isspace(static_cast<unsigned char>(value[last - 1]))) {
        --last;
    }
    return value.substr(first, last - first);
}

DishClassifier makeClassifier()
{
    return DishClassifier({
        dish_counter_defaults::minimumDistance,
        dish_counter_defaults::maximumDistance,
        dish_counter_defaults::requiredConsecutiveSamples,
    });
}

class Simulation {
public:
    bool startTray(std::size_t lineNumber)
    {
        if (trayActive_) {
            std::cerr << "Line " << lineNumber << ": finish the active tray before starting another\n";
            return false;
        }

        ++trayNumber_;
        classifier_.reset();
        trayActive_ = true;
        std::cout << "Tray " << trayNumber_ << " started\n";
        return true;
    }

    bool recordSample(int rawDistance, std::size_t lineNumber)
    {
        if (!trayActive_) {
            std::cerr << "Line " << lineNumber << ": distance sample requires an active tray\n";
            return false;
        }

        if (rawDistance < 0) {
            classifier_.recordSample(rawDistance);
            std::cout << "  raw=timeout  streak=0\n";
            return true;
        }

        const int filteredDistance = static_cast<int>(
            filter_.update(static_cast<float>(rawDistance)));
        const bool countedNow = classifier_.recordSample(filteredDistance);
        std::cout << "  raw=" << std::setw(3) << rawDistance
                  << "  filtered=" << std::setw(3) << filteredDistance
                  << "  streak=" << classifier_.consecutiveSamples();

        if (countedNow) {
            ++totalCount_;
            std::cout << "  COUNT";
        }
        std::cout << '\n';
        return true;
    }

    bool finishTray(std::size_t lineNumber)
    {
        if (!trayActive_) {
            std::cerr << "Line " << lineNumber << ": no active tray to finish\n";
            return false;
        }

        std::cout << "Tray " << trayNumber_ << " result: "
                  << (classifier_.classified() ? "COUNTED" : "REJECTED") << "\n\n";
        trayActive_ = false;
        return true;
    }

    bool trayActive() const
    {
        return trayActive_;
    }

    unsigned totalCount() const
    {
        return totalCount_;
    }

private:
    DishClassifier classifier_ = makeClassifier();
    ScalarKalmanFilter filter_{dish_counter_defaults::kalmanMeasuredError};
    unsigned trayNumber_ = 0;
    unsigned totalCount_ = 0;
    bool trayActive_ = false;
};

bool parseDistance(const std::string &line, int &distance)
{
    std::size_t consumed = 0;
    try {
        distance = std::stoi(line, &consumed);
    } catch (...) {
        return false;
    }
    return consumed == line.size();
}

int runTrace(std::istream &trace)
{
    Simulation simulation;
    std::string rawLine;
    std::size_t lineNumber = 0;

    while (std::getline(trace, rawLine)) {
        ++lineNumber;
        const std::string line = trim(rawLine);
        if (line.empty() || line[0] == '#') {
            continue;
        }

        if (line == "tray") {
            if (!simulation.startTray(lineNumber)) {
                return EXIT_FAILURE;
            }
            continue;
        }

        if (line == "end") {
            if (!simulation.finishTray(lineNumber)) {
                return EXIT_FAILURE;
            }
            continue;
        }

        int distance = 0;
        if (!parseDistance(line, distance)) {
            std::cerr << "Line " << lineNumber << ": expected tray, end, or an integer distance\n";
            return EXIT_FAILURE;
        }
        if (!simulation.recordSample(distance, lineNumber)) {
            return EXIT_FAILURE;
        }
    }

    if (simulation.trayActive() && !simulation.finishTray(lineNumber + 1)) {
        return EXIT_FAILURE;
    }

    std::cout << "Simulation total: " << simulation.totalCount() << '\n';
    return EXIT_SUCCESS;
}

void printUsage(const char *program)
{
    std::cerr << "Usage: " << program << " TRACE_FILE\n";
}

} // namespace

int main(int argc, char **argv)
{
    if (argc != 2) {
        printUsage(argv[0]);
        return EXIT_FAILURE;
    }

    std::ifstream trace(argv[1]);
    if (!trace) {
        std::cerr << "Could not open trace file: " << argv[1] << '\n';
        return EXIT_FAILURE;
    }

    return runTrace(trace);
}
