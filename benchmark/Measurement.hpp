#pragma once

#include <string>

struct Measurement {
    std::string field;
    std::string algorithm;
    std::string order;
    std::string status;
    int records{0};
    int keys{0};
    int queries{0};
    double buildMilliseconds{0.0};
    double queryMilliseconds{0.0};
};
