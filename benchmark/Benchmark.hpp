#pragma once

#include <chrono>
#include <random>
#include <string>

#include "algorithms/Algorithm.hpp"
#include "core/Functional.hpp"
#include "core/ICollectible.hpp"
#include "sequences/DynamicArray.hpp"
#include "sequences/ArraySequence.hpp"
#include "indexing/Index.hpp"
#include "dictionaries/SortedDictionary.hpp"
#include "dictionaries/TreeDictionary.hpp"
#include "dictionaries/HashTable.hpp"
#include "indexing/IndexTypes.hpp"
#include "benchmark/Measurement.hpp"

template <class Record, class Key, class Selector>
ArraySequence<Measurement> CompareIndexes(
    const ICollectible<Record>& source, Selector select, const Key& missingKey) {
    DynamicArray<Record> input;
    auto enumerator = source.GetEnumerator();
    while (enumerator->MoveNext()) {
        input.Append(enumerator->CurrentRef());
    }
    input.Sort([&](const Record& first, const Record& second) {
        return select(first) < select(second);
    });

    const char* const orders[]{"Ascending", "Descending", "Random"};
    using Positions = ArraySequence<RecordId>;
    ArraySequence<Measurement> measurements;

    for (int order = 0; order < 3; ++order) {
        DynamicArray<Record> arranged = input;
        if (order == 1) {
            Reverse(arranged.begin(), arranged.end());
        } else if (order == 2) {
            std::mt19937 random(42);
            Shuffle(arranged.begin(), arranged.end(), random);
        }
        ArraySequence<Record> records;
        for (int i = 0; i < arranged.GetSize(); ++i) {
            records.Append(arranged[i]);
        }
        ArraySequence<Key> queries;
        for (int query = 0; query < 512; ++query) {
            const Key key = records.IsEmpty() || query % 4 == 0
                ? missingKey : select(records.Get((query * 17) % records.GetCount()));
            queries.Append(key);
        }
        const auto measure = [&](IndexKind kind, auto makeDictionary) {
            Measurement row{};
            row.kind = kind;
            row.order = orders[order];
            row.records = records.GetCount();
            row.queries = queries.GetCount();
            const auto buildStart = std::chrono::steady_clock::now();
            Index<Record, Key> index(records, makeDictionary(), select);
            const auto buildEnd = std::chrono::steady_clock::now();
            row.keys = index.GetKeyCount();
            row.buildMilliseconds = std::chrono::duration<double, std::milli>(buildEnd - buildStart).count();
            volatile int resultCount = 0;
            const auto queryStart = std::chrono::steady_clock::now();
            for (int query = 0; query < queries.GetCount(); ++query) {
                resultCount = index.FindAll(queries.Get(query)).GetCount();
            }
            (void)resultCount;
            const auto queryEnd = std::chrono::steady_clock::now();
            row.queryMilliseconds = std::chrono::duration<double, std::milli>(queryEnd - queryStart).count();
            measurements.Append(row);
        };
        measure(IndexKind::Sorted, [] { return MakeUnique<SortedDictionary<Key, Positions>>(); });
        measure(IndexKind::Tree, [] { return MakeUnique<TreeDictionary<Key, Positions>>(); });
        measure(IndexKind::Hash, [] { return MakeUnique<HashTable<Key, Positions>>(); });
    }
    return measurements;
}
