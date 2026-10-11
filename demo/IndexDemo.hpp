#pragma once

#include <iomanip>
#include <iostream>
#include <type_traits>

#include "benchmark/Benchmark.hpp"
#include "benchmark/CsvExporter.hpp"
#include "domain/Person.hpp"
#include "sequences/SortedArraySequence.hpp"
#include "sets/BinaryTreeSet.hpp"

template <class Record>
void PrintRecords(const ICollectible<Record>& records) {
    auto entries = records.GetEnumerator();
    while (entries->MoveNext()) {
        const Person& person = GetPerson(entries->CurrentRef());
        std::cout << "  ID=" << person.id << " | "
                  << person.firstName << ' ' << person.lastName << " | " << person.birthYear << '\n';
    }
}

template <class Record>
void ShowSource(const char* name, const ICollectible<Record>& source) {
    std::cout << "\nИсточник: " << name << '\n';
    PrintRecords(source);
    const auto showIndex = [&](const char* name, auto dictionary) {
        Index<Record, std::string> index(source, Move(dictionary), [](const Record& record) {
            return GetPerson(record).lastName;
        });
        std::cout << "  Индекс: " << name << " (фамилия -> записи)\n";
        auto entries = index.GetEntriesEnumerator();
        while (entries->MoveNext()) {
            const auto& entry = entries->CurrentRef();
            std::cout << "    " << entry.key << " -> [";
            for (int j = 0; j < entry.value.GetCount(); ++j) {
                if (j) std::cout << ", ";
                std::cout << GetPerson(*entry.value.Get(j)).id;
            }
            std::cout << "]\n";
        }
        const auto& matches = index.FindAll("Иванов");
        std::cout << "    Поиск Иванов: ID записей =";
        for (int j = 0; j < matches.GetCount(); ++j) {
            std::cout << ' ' << GetPerson(*matches.Get(j)).id;
        }
        std::cout << '\n';
    };
    showIndex("SortedDictionary", MakeUnique<SortedDictionary<std::string, ArraySequence<const Record*>>>());
    showIndex("TreeDictionary", MakeUnique<TreeDictionary<std::string, ArraySequence<const Record*>>>());
    showIndex("HashTable", MakeUnique<HashTable<std::string, ArraySequence<const Record*>>>());
}

inline void RunIndexDemo() {
    ArraySequence<Person> people;
    people.Append(Person{9, "Анна", "Иванов", 2004});
    people.Append(Person{2, "Иван", "Петров", 2003});
    people.Append(Person{17, "Алексей", "Иванов", 2004});
    BinaryTreeSet<Person> set;
    const SortedArraySequence<Person> sorted(people);
    SortedDictionary<int, Person> sortedDictionary;
    TreeDictionary<int, Person> treeDictionary;
    HashTable<int, Person> hashTable;
    auto records = people.GetEnumerator();
    while (records->MoveNext()) {
        const Person& person = records->CurrentRef();
        set.Add(person);
        sortedDictionary.Add(person.id, person);
        treeDictionary.Add(person.id, person);
        hashTable.Add(person.id, person);
    }
    ShowSource("ArraySequence", people);
    ShowSource("BinaryTreeSet", set);
    ShowSource("SortedArraySequence", sorted);
    ShowSource("SortedDictionary (ID -> Person)", sortedDictionary);
    ShowSource("TreeDictionary (ID -> Person)", treeDictionary);
    ShowSource("HashTable (ID -> Person)", hashTable);

    const int sizes[]{1000, 10000, 100000, 1000000};
    ArraySequence<Measurement> measurements;
    std::cout << "\nБенчмарки: 100 000 запросов, лимит 5 с на прогон. Время в мс.\n"
              << std::fixed << std::setprecision(3);
    for (int size : sizes) {
        const auto benchmarkPeople = GeneratePeople(size, 42);
        const auto run = [&](const char* field, auto select, const auto& missingKey) {
            auto results = CompareIndexes<Person, std::decay_t<decltype(missingKey)>>(
                benchmarkPeople, select, missingKey);
            std::cout << "\n" << field << ": " << size << " записей, "
                      << results.Get(0).keys << " ключей\n"
                      << std::left << std::setw(18) << "Index" << std::setw(12) << "Order"
                      << std::right << std::setw(12) << "Build, ms" << std::setw(12) << "Query, ms"
                      << "  Status\n";
            for (int i = 0; i < results.GetCount(); ++i) {
                auto& row = results.GetMutable(i);
                row.field = field;
                std::cout << std::left << std::setw(18) << row.algorithm << std::setw(12) << row.order
                          << std::right << std::setw(12) << row.buildMilliseconds;
                if (row.status == "build_timeout") std::cout << std::setw(12) << "-";
                else std::cout << std::setw(12) << row.queryMilliseconds;
                std::cout << "  " << row.status << '\n';
                measurements.Append(row);
            }
            std::cout << std::flush;
        };
        run("birthYear", ByBirthYear, 0);
        run("lastName", ByLastName, std::string("Missing"));
    }
    ExportMeasurements(measurements, "benchmark.csv");
    std::cout << "\nCSV: benchmark.csv. ok = проверено, timeout = незавершённый прогон.\n";
}
