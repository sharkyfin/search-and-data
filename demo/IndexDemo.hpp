#pragma once

#include <chrono>
#include <exception>
#include <iomanip>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>

#include "algorithms/Algorithm.hpp"
#include "benchmark/Benchmark.hpp"
#include "benchmark/CsvExporter.hpp"
#include "core/IEnumerable.hpp"
#include "domain/Person.hpp"
#include "domain/PersonCsv.hpp"
#include "sequences/SortedArraySequence.hpp"
#include "sets/BinaryTreeSet.hpp"

template <class Record>
void PrintRecords(const IEnumerable<Record>& records) {
    auto entries = records.GetEnumerator();
    int printed = 0;
    while (printed < 20 && entries->MoveNext()) {
        const Person& person = GetPerson(entries->CurrentRef());
        std::cout << "  ID=" << person.id << " | "
                  << person.firstName << ' ' << person.lastName << " | " << person.birthYear << '\n';
        ++printed;
    }
    if (entries->MoveNext()) std::cout << "  ...\n";
}

template <class Record>
void ShowSource(const char* name, const IEnumerable<Record>& source, const std::string& key = "Иванов") {
    std::cout << "\nИсточник: " << name << '\n';
    PrintRecords(source);
    const auto showIndex = [&](const char* name, auto dictionary) {
        Index<Record, std::string> index(source, Move(dictionary), [](const Record& record) {
            return GetPerson(record).lastName;
        });
        std::cout << "  Индекс: " << name << " (фамилия -> записи)\n";
        auto entries = index.GetEntriesEnumerator();
        int printed = 0;
        while (printed < 20 && entries->MoveNext()) {
            const auto& entry = entries->CurrentRef();
            std::cout << "    " << entry.key << " -> [";
            for (int j = 0; j < entry.value.GetCount() && j < 20; ++j) {
                if (j) std::cout << ", ";
                std::cout << GetPerson(*entry.value.Get(j)).id;
            }
            if (entry.value.GetCount() > 20) std::cout << ", ...";
            std::cout << "]\n";
            ++printed;
        }
        if (entries->MoveNext()) std::cout << "    ...\n";
        const auto& matches = index.FindAll(key);
        std::cout << "    Поиск " << key << ": найдено " << matches.GetCount() << ", ID =";
        for (int j = 0; j < matches.GetCount() && j < 20; ++j) {
            std::cout << ' ' << GetPerson(*matches.Get(j)).id;
        }
        if (matches.GetCount() > 20) std::cout << " ...";
        std::cout << '\n';
    };
    showIndex("SortedDictionary", MakeUnique<SortedDictionary<std::string, ArraySequence<const Record*>>>());
    showIndex("TreeDictionary", MakeUnique<TreeDictionary<std::string, ArraySequence<const Record*>>>());
    showIndex("HashTable", MakeUnique<HashTable<std::string, ArraySequence<const Record*>>>());
}

inline int RunIndexDemo() {
    std::string path;
    std::cout << "CSV-файл (Enter — демо и бенчмарки): ";
    if (!std::getline(std::cin, path)) return 0;
    if (!path.empty()) {
        try {
            const char* names[]{"ArraySequence", "BinaryTreeSet", "SortedArraySequence",
                "SortedDictionary (ID -> Person)", "TreeDictionary (ID -> Person)", "HashTable (ID -> Person)"};
            for (int i = 0; i < 6; ++i) std::cout << i + 1 << ". " << names[i] << '\n';
            std::cout << "Источник (Enter — 1): ";
            std::string choice;
            if (!std::getline(std::cin, choice)) return 0;
            if (choice.empty()) choice = "1";
            if (choice.size() != 1 || choice[0] < '1' || choice[0] > '6') {
                throw std::invalid_argument("Выберите источник от 1 до 6.");
            }
            const auto showCsv = [&](auto& people, auto add) {
                ReadPeopleCsv(path, add);
                std::cout << "Загружено записей: " << people.GetCount() << '\n';
                std::string key;
                std::cout << "Фамилия (Enter — Иванов): ";
                if (!std::getline(std::cin, key)) return;
                ShowSource(names[choice[0] - '1'], people, key.empty() ? "Иванов" : key);
            };
            if (choice == "1") {
                ArraySequence<Person> people;
                showCsv(people, [&](Person person) { people.Append(Move(person)); });
            } else if (choice == "2") {
                BinaryTreeSet<Person> people;
                showCsv(people, [&](const Person& person) { people.Add(person); });
            } else if (choice == "3") {
                SortedArraySequence<Person> people;
                showCsv(people, [&](const Person& person) { people.Add(person); });
            } else {
                UniquePtr<IDictionary<int, Person>> people;
                if (choice == "4") people = MakeUnique<SortedDictionary<int, Person>>();
                else if (choice == "5") people = MakeUnique<TreeDictionary<int, Person>>();
                else people = MakeUnique<HashTable<int, Person>>();
                showCsv(*people, [&](const Person& person) { people->Add(person.id, person); });
            }
            return 0;
        } catch (const std::exception& error) {
            std::cerr << "Ошибка: " << error.what() << '\n';
            return 1;
        }
    }
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
    const char* const fields[]{"birthYear", "lastName"};
    const char* const orders[]{"Ascending", "Descending", "Random"};
    const char* const sourceNames[]{"ArraySequence", "BinaryTreeSet", "SortedArraySequence",
        "SortedDictionary", "TreeDictionary", "HashTable"};
    ArraySequence<Measurement> measurements;
    std::cout << "\nБенчмарки: 6 источников, 3 индекса, 100 000 запросов. Время в мс.\n"
              << "Лимит: 5 с на заполнение источника и 5 с на прогон каждого индекса.\n"
              << "Input order — порядок заполнения источника по выбранному полю.\n"
              << std::fixed << std::setprecision(3);
    for (int size : sizes) {
        auto benchmarkPeople = GeneratePeople(size, 42);
        for (int field = 0; field < 2; ++field) {
            benchmarkPeople.Sort([&](const Person& a, const Person& b) {
                if (field == 0 && a.birthYear != b.birthYear) return a.birthYear < b.birthYear;
                if (field == 1 && a.lastName != b.lastName) return a.lastName < b.lastName;
                return a.id < b.id;
            });
            std::cout << "\n" << fields[field] << ": " << size << " записей\n"
                      << std::left << std::setw(22) << "Source" << std::setw(18) << "Index"
                      << std::setw(12) << "Input order" << std::right << std::setw(8) << "Keys"
                      << std::setw(12) << "Build, ms" << std::setw(12) << "Query, ms"
                      << "  Status\n";
            for (int order = 0; order < 3; ++order) {
                DynamicArray<Person> arranged;
                arranged.Reserve(size);
                for (int i = 0; i < size; ++i) arranged.Append(benchmarkPeople.Get(i));
                if (order == 1) Reverse(arranged.begin(), arranged.end());
                if (order == 2) {
                    std::mt19937 random(42);
                    Shuffle(arranged.begin(), arranged.end(), random);
                }
                ArraySequence<Person> array(Move(arranged));
                ArraySequence<Measurement> skipped;
                for (const char* algorithm : BenchmarkIndexNames) {
                    Measurement row{};
                    row.algorithm = algorithm;
                    row.records = size;
                    row.status = "source_timeout";
                    skipped.Append(row);
                }
                const auto printResults = [&](const char* name, ArraySequence<Measurement> results) {
                    for (int i = 0; i < results.GetCount(); ++i) {
                        auto& row = results.GetMutable(i);
                        row.source = name;
                        row.field = fields[field];
                        row.order = orders[order];
                        std::cout << std::left << std::setw(22) << row.source << std::setw(18) << row.algorithm
                                  << std::setw(12) << row.order << std::right << std::setw(8) << row.keys;
                        if (row.status == "source_timeout") std::cout << std::setw(12) << "-";
                        else std::cout << std::setw(12) << row.buildMilliseconds;
                        if (row.status == "source_timeout" || row.status == "build_timeout") std::cout << std::setw(12) << "-";
                        else std::cout << std::setw(12) << row.queryMilliseconds;
                        std::cout << "  " << row.status << '\n' << std::flush;
                        measurements.Append(row);
                    }
                };
                {
                    BinaryTreeSet<Person> treeSet;
                    SortedArraySequence<Person> sortedArray;
                    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
                    for (int i = 0; i < size && std::chrono::steady_clock::now() < deadline; ++i) {
                        treeSet.Add(array.Get(i));
                    }
                    deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
                    for (int i = 0; i < size && std::chrono::steady_clock::now() < deadline; ++i) {
                        sortedArray.Add(array.Get(i));
                    }
                    const ICollectible<Person>* sources[]{&array, &treeSet, &sortedArray};
                    for (int source = 0; source < 3; ++source) {
                        auto results = skipped;
                        if (sources[source]->GetCount() == size) {
                            if (field == 0) results = CompareIndexes(*sources[source], ByBirthYear, 0);
                            else results = CompareIndexes(*sources[source], ByLastName, std::string("Missing"));
                        }
                        printResults(sourceNames[source], Move(results));
                    }
                }
                {
                    SortedDictionary<int, Person> sortedDictionary;
                    TreeDictionary<int, Person> treeDictionary;
                    HashTable<int, Person> hashTable;
                    IDictionary<int, Person>* sources[]{&sortedDictionary, &treeDictionary, &hashTable};
                    for (int source = 0; source < 3; ++source) {
                        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
                        for (int i = 0; i < size && std::chrono::steady_clock::now() < deadline; ++i) {
                            const auto& person = array.Get(i);
                            sources[source]->Add(person.id, person);
                        }
                        auto results = skipped;
                        if (sources[source]->GetCount() == size) {
                            if (field == 0) results = CompareIndexes(*sources[source], [](const auto& entry) {
                                return entry.value.birthYear;
                            }, 0);
                            else results = CompareIndexes(*sources[source], [](const auto& entry) {
                                return entry.value.lastName;
                            }, std::string("Missing"));
                        }
                        printResults(sourceNames[source + 3], Move(results));
                    }
                }
            }
        }
    }
    ExportMeasurements(measurements, "benchmark.csv");
    std::cout << "\nCSV: benchmark.csv. ok = проверено, timeout = незавершённый прогон.\n";
    return 0;
}
