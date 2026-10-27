#pragma once

#include <fstream>
#include <stdexcept>
#include <string>

#include "sequences/Sequence.hpp"
#include "benchmark/Measurement.hpp"

inline void ExportMeasurements(const Sequence<Measurement>& measurements,
                               const std::string& path) {
    std::ofstream output(path);
    if (!output) {
        throw std::runtime_error("Не удалось открыть CSV для записи.");
    }
    output << "source,algorithm,field,input_order,records,unique_keys,queries,repetitions,seed,source_ms,"
              "build_ms,build_min_ms,build_max_ms,query_ms,query_min_ms,query_max_ms,"
              "total_ms,total_min_ms,total_max_ms,status\n";
    output.precision(9);
    for (int index = 0; index < measurements.GetCount(); ++index) {
        const Measurement& row = measurements.Get(index);
        output << row.source << ',' << row.algorithm << ',' << row.field << ",Random,"
               << row.records << ',' << row.keys << ',' << row.queries << ',' << row.repetitions << ','
               << row.seed << ',' << row.sourceMilliseconds;
        if (row.status == "ok") {
            for (const auto& time : {row.build, row.search, row.total}) {
                output << ',' << time.median << ',' << time.minimum << ',' << time.maximum;
            }
        } else {
            output << ",,,,,,,,,";
        }
        output << ',' << row.status << '\n';
    }
    output.close();
    if (!output) {
        throw std::runtime_error("Не удалось завершить запись CSV.");
    }
}
