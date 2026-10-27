#pragma once

#include <string>

struct TimeSummary {
    double median{0.0};
    double minimum{0.0};
    double maximum{0.0};
};

struct Measurement {
    std::string source;
    std::string field;
    std::string algorithm;
    std::string status{"ok"};
    int records{0};
    int keys{0};
    int queries{0};
    int repetitions{0};
    unsigned int seed{42};
    double sourceMilliseconds{0.0};
    TimeSummary build;
    TimeSummary search;
    TimeSummary total;
};
