#pragma once

#include <chrono>
#include <stdexcept>

#include "core/Functional.hpp"
#include "core/IEnumerable.hpp"
#include "sequences/DynamicArray.hpp"
#include "sequences/ArraySequence.hpp"
#include "indexing/Index.hpp"
#include "dictionaries/SortedDictionary.hpp"
#include "dictionaries/TreeDictionary.hpp"
#include "dictionaries/HashTable.hpp"
#include "benchmark/Measurement.hpp"

inline constexpr const char* BenchmarkIndexNames[]{"HashTable", "TreeDictionary", "SortedDictionary"};

template <class Record, class Key, class Selector, class Hasher = Hash<Key>>
ArraySequence<Measurement> CompareIndexes(
    const IEnumerable<Record>& source, Selector select, const Key& missingKey,
    Hasher hash = Hasher(), double limitSeconds = 5.0) {
    DynamicArray<Key> input;
    auto enumerator = source.GetEnumerator();
    while (enumerator->MoveNext()) input.Append(select(enumerator->CurrentRef()));
    input.Sort();
    int keys = 0;
    for (int i = 0; i < input.GetSize(); ++i) {
        if (i == 0 || input.Get(i - 1) < input.Get(i)) ++keys;
        if (!(input.Get(i) < missingKey) && !(missingKey < input.Get(i))) {
            throw std::invalid_argument("Отсутствующий ключ встречается в источнике.");
        }
    }
    ArraySequence<Key> queries;
    queries.Reserve(100000);
    for (int i = 0; i < 100000; ++i) {
        queries.Append(input.IsEmpty() || i % 4 == 0
            ? missingKey : input.Get(static_cast<int>((static_cast<long long>(i) * 17) % input.GetSize())));
    }

    UniquePtr<IDictionary<Key, ArraySequence<const Record*>>> dictionaries[]{
        MakeUnique<HashTable<Key, ArraySequence<const Record*>, Hasher>>(hash),
        MakeUnique<TreeDictionary<Key, ArraySequence<const Record*>>>(),
        MakeUnique<SortedDictionary<Key, ArraySequence<const Record*>>>()
    };
    ArraySequence<Measurement> measurements;
    UniquePtr<Index<Record, Key>> reference;
    for (int algorithm = 0; algorithm < 3; ++algorithm) {
        Measurement row{};
        row.algorithm = BenchmarkIndexNames[algorithm];
        row.records = input.GetSize();
        row.keys = keys;
        row.status = "build_timeout";
        auto phaseStart = std::chrono::steady_clock::now();
        const auto deadline = phaseStart + std::chrono::duration<double>(limitSeconds);
        bool timedOut = false;
        const auto checkTime = [&] {
            if (std::chrono::steady_clock::now() >= deadline) {
                timedOut = true;
                throw std::runtime_error("Превышен лимит времени бенчмарка.");
            }
        };
        const auto elapsed = [&] {
            return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - phaseStart).count();
        };
        try {
            int visited = 0;
            checkTime();
            auto index = MakeUnique<Index<Record, Key>>(source, Move(dictionaries[algorithm]),
                [&](const Record& record) {
                    if (visited++ % 1024 == 0) checkTime();
                    return select(record);
                });
            row.buildMilliseconds = elapsed();
            checkTime();
            if (visited != input.GetSize()) {
                throw std::runtime_error("Число записей источника изменилось во время бенчмарка.");
            }
            phaseStart = std::chrono::steady_clock::now();
            row.status = "query_timeout";
            volatile int resultCount = 0;
            for (; row.queries < queries.GetCount(); ++row.queries) {
                if (row.queries % 1024 == 0) checkTime();
                resultCount = index->FindAll(queries.Get(row.queries)).GetCount();
            }
            (void)resultCount;
            row.queryMilliseconds = elapsed();
            checkTime();
            row.status = "verification_timeout";
            if (index->GetKeyCount() != keys || !index->FindAll(missingKey).IsEmpty()) {
                throw std::runtime_error("Неверное число ключей или результат поиска отсутствующего ключа.");
            }
            if (reference) {
                auto entries = reference->GetEntriesEnumerator();
                int compared = 0;
                while (entries->MoveNext()) {
                    const auto& entry = entries->CurrentRef();
                    const auto& actual = index->FindAll(entry.key);
                    if (actual.GetCount() != entry.value.GetCount()) {
                        throw std::runtime_error("Алгоритмы вернули разные результаты.");
                    }
                    for (int i = 0; i < actual.GetCount(); ++i) {
                        if (compared++ % 1024 == 0) checkTime();
                        if (actual.Get(i) != entry.value.Get(i)) {
                            throw std::runtime_error("Алгоритмы вернули разные записи.");
                        }
                    }
                }
            }
            checkTime();
            if (!reference) reference = Move(index);
            row.status = "ok";
        } catch (const std::runtime_error&) {
            if (!timedOut) throw;
            if (row.status == "build_timeout") row.buildMilliseconds = elapsed();
            if (row.status == "query_timeout") row.queryMilliseconds = elapsed();
        }
        measurements.Append(row);
    }
    return measurements;
}
