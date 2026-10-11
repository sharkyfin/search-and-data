#pragma once

#include <fstream>
#include <stdexcept>
#include <string>

#include "sequences/Sequence.hpp"
#include "benchmark/Measurement.hpp"
#include "indexing/IndexTypes.hpp"

inline void ExportMeasurements(const Sequence<Measurement>& measurements,
                               const std::string& field, const std::string& path) {
    std::ofstream output(path);
    if (!output) {
        throw std::runtime_error("Не удалось открыть CSV для записи.");
    }
    output << "algorithm,field,order,records,unique_keys,queries,shuffle_seed,build_ms,query_ms\n";
    output.precision(9);
    for (int index = 0; index < measurements.GetCount(); ++index) {
        const Measurement& row = measurements.Get(index);
        output << GetIndexName(row.kind) << ',' << field << ',' << row.order << ','
               << row.records << ',' << row.keys << ',' << row.queries << ",42,"
               << row.buildMilliseconds << ',' << row.queryMilliseconds << '\n';
    }
    output.close();
    if (!output) {
        throw std::runtime_error("Не удалось завершить запись CSV.");
    }
}
