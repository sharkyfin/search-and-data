#pragma once

#include <chrono>
#include <random>
#include <stdexcept>

#include "algorithms/Algorithm.hpp"
#include "core/Functional.hpp"
#include "core/ICollectible.hpp"
#include "sequences/DynamicArray.hpp"
#include "sequences/ArraySequence.hpp"
#include "indexing/Index.hpp"
#include "dictionaries/SortedDictionary.hpp"
#include "dictionaries/TreeDictionary.hpp"
#include "dictionaries/HashTable.hpp"
#include "benchmark/Measurement.hpp"

template <class Record, class Key, class Selector, class Hasher = Hash<Key>>
ArraySequence<Measurement> CompareIndexes(
    const ICollectible<Record>& source, Selector select, const Key& missingKey,
    Hasher hash = Hasher(), double limitSeconds = 5.0) {
    DynamicArray<Record> input;
    auto enumerator = source.GetEnumerator();
    while (enumerator->MoveNext()) input.Append(enumerator->CurrentRef());
    input.Sort([&](const Record& a, const Record& b) { return select(a) < select(b); });
    int keys = 0;
    for (int i = 0; i < input.GetSize(); ++i) {
        if (i == 0 || select(input.Get(i - 1)) < select(input.Get(i))) ++keys;
    }
    ArraySequence<Key> queries;
    queries.Reserve(100000);
    for (int i = 0; i < 100000; ++i) {
        queries.Append(input.IsEmpty() || i % 4 == 0
            ? missingKey : select(input.Get((i * 17) % input.GetSize())));
    }

    const char* const orders[]{"Ascending", "Descending", "Random"};
    ArraySequence<Measurement> measurements;
    for (int order = 0; order < 3; ++order) {
        DynamicArray<Record> arranged = input;
        if (order == 1) Reverse(arranged.begin(), arranged.end());
        if (order == 2) {
            std::mt19937 random(42);
            Shuffle(arranged.begin(), arranged.end(), random);
        }
        const ArraySequence<Record> records(Move(arranged));
        UniquePtr<Index<Record, Key>> reference;
        const auto measure = [&](const char* algorithm, auto makeDictionary) {
            Measurement row{};
            row.algorithm = algorithm;
            row.order = orders[order];
            row.records = records.GetCount();
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
                auto index = MakeUnique<Index<Record, Key>>(records, makeDictionary(),
                    [&](const Record& record) {
                        if (visited++ % 1024 == 0) checkTime();
                        return select(record);
                    });
                row.buildMilliseconds = elapsed();
                checkTime();
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
                // Один полный проход по группам: проверка не входит во время поиска.
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
        };
        // Первый завершённый индекс служит эталоном для остальных на том же источнике.
        measure("HashTable", [&] { return MakeUnique<HashTable<Key, ArraySequence<const Record*>, Hasher>>(hash); });
        measure("TreeDictionary", [] { return MakeUnique<TreeDictionary<Key, ArraySequence<const Record*>>>(); });
        measure("SortedDictionary", [] { return MakeUnique<SortedDictionary<Key, ArraySequence<const Record*>>>(); });
    }
    return measurements;
}
