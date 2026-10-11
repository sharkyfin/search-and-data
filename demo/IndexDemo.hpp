#pragma once

#include <iomanip>
#include <iostream>

#include "benchmark/Benchmark.hpp"
#include "benchmark/CsvExporter.hpp"
#include "indexing/IndexFactory.hpp"

inline void PrintRecords(const ICollectible<Person>& records) {
    auto entries = records.GetEnumerator();
    int position = 0;
    while (entries->MoveNext()) {
        const Person& person = entries->CurrentRef();
        std::cout << "  " << position++ << " | ID=" << person.id << " | "
                  << person.firstName << ' ' << person.lastName << " | " << person.birthYear << '\n';
    }
}

inline void RunIndexDemo() {
    ArraySequence<Person> people;
    people.Append(Person{9, "Анна", "Иванов", 2004});
    people.Append(Person{2, "Иван", "Петров", 2003});
    people.Append(Person{17, "Алексей", "Иванов", 2004});
    std::cout << "Исходные записи заданы в ArraySequence<Person>:\n";
    PrintRecords(people);

    const SourceKind sources[]{SourceKind::Array, SourceKind::TreeSet, SourceKind::Sorted,
        SourceKind::SortedDictionary, SourceKind::TreeDictionary, SourceKind::HashTable};
    const char* const sourceNames[]{"ArraySequence", "BinaryTreeSet", "SortedArraySequence",
        "SortedDictionary", "TreeDictionary", "HashTable"};
    using Positions = ArraySequence<RecordId>;
    std::cout << "\nКаждый источник создаётся из исходных записей. Индекс копирует его в снимок.\n";
    for (int i = 0; i < 6; ++i) {
        auto source = MakePersonSource(people, sources[i]);
        std::cout << "\nИсточник: " << sourceNames[i];
        if (i >= 3) std::cout << " (ID -> Person)";
        std::cout << "\nПорядок записей и позиции в снимке:\n";
        PrintRecords(*source);
        const auto showIndex = [&](IndexKind kind, auto dictionary) {
            Index<Person, std::string> index(*source, Move(dictionary), ByLastName);
            std::cout << "  Индекс: " << GetIndexName(kind) << " (фамилия -> позиции)\n";
            auto entries = index.GetEntriesEnumerator();
            while (entries->MoveNext()) {
                const auto& entry = entries->CurrentRef();
                std::cout << "    " << entry.key << " -> [";
                for (int j = 0; j < entry.value.GetCount(); ++j) {
                    if (j) std::cout << ", ";
                    std::cout << entry.value.Get(j).position;
                }
                std::cout << "]\n";
            }
            const auto matches = index.FindAll("Иванов");
            std::cout << "    Поиск Иванов: ID записей =";
            for (int j = 0; j < matches.GetCount(); ++j) {
                std::cout << ' ' << index.GetRecords().Get(matches.Get(j).position).id;
            }
            std::cout << '\n';
        };
        showIndex(IndexKind::Sorted, MakeUnique<SortedDictionary<std::string, Positions>>());
        showIndex(IndexKind::Tree, MakeUnique<TreeDictionary<std::string, Positions>>());
        showIndex(IndexKind::Hash, MakeUnique<HashTable<std::string, Positions>>());
    }

    const auto benchmarkPeople = GeneratePeople(10000, 42);
    const auto results = CompareIndexes<Person, int>(benchmarkPeople, ByBirthYear, 0);
    ExportMeasurements(results, "birth_year", "benchmark.csv");
    const auto& measurement = results.Get(0);
    std::cout << "\nБенчмарки индексации по году рождения\n"
              << measurement.records << " записей, " << measurement.keys << " ключ, "
              << measurement.queries << " запросов. Время в миллисекундах.\n\n"
              << std::left << std::setw(18) << "Index" << std::setw(12) << "Order"
              << std::right << std::setw(12) << "Build, ms" << std::setw(12) << "Query, ms" << '\n'
              << std::string(54, '-') << '\n'
              << std::fixed << std::setprecision(6);
    for (int i = 0; i < results.GetCount(); ++i) {
        const auto& result = results.Get(i);
        std::cout << std::left << std::setw(18) << GetIndexName(result.kind)
                  << std::setw(12) << result.order << std::right
                  << std::setw(12) << result.buildMilliseconds
                  << std::setw(12) << result.queryMilliseconds << '\n';
    }
    std::cout << "\nЗамеры сохранены в benchmark.csv\n";
}
