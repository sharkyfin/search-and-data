#pragma once

#include "benchmark/Measurement.hpp"
#include "sequences/ArraySequence.hpp"

struct BenchmarkOptions {
    int records{10000};
    int queries{100000};
    int repetitions{5};
    unsigned int seed{42};
    double timeLimitSeconds{5.0};
};

ArraySequence<Measurement> RunBenchmarks(int sourceChoice, const BenchmarkOptions& options = {});
