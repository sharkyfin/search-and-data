#pragma once

#include <string>

#include "indexing/IndexTypes.hpp"

struct Measurement {
    IndexKind kind;
    std::string order;
    int records{0};
    int keys{0};
    int queries{0};
    double buildMilliseconds{0.0};
    double queryMilliseconds{0.0};
};
