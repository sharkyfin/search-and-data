#include <algorithm>
#include <cmath>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <memory>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

#include "algorithms/Algorithm.hpp"
#include "benchmark/Benchmark.hpp"
#include "benchmark/CsvExporter.hpp"
#include "core/ForwardList.hpp"
#include "core/ICollectible.hpp"
#include "core/IEnumerable.hpp"
#include "dictionaries/HashTable.hpp"
#include "dictionaries/SortedDictionary.hpp"
#include "dictionaries/TreeDictionary.hpp"
#include "domain/Person.hpp"
#include "domain/PersonCsv.hpp"
#include "indexing/Index.hpp"
#include "GUI/PeopleModel.hpp"
#include "sequences/SortedArraySequence.hpp"
#include "sets/BinaryTreeSet.hpp"

template <class T, class... Args>
ArraySequence<T> MakeTestSequence(const Args&... values) {
    ArraySequence<T> result;
    (result.Append(values), ...);
    return result;
}

template <class T>
void ExpectEnumeration(const ICollectible<T>& collection, const Sequence<T>& expected) {
    const IEnumerable<T>& enumerable = collection;
    auto enumerator = enumerable.GetEnumerator();
    EXPECT_THROW(enumerator->CurrentRef(), std::logic_error);
    int count = 0;
    while (enumerator->MoveNext()) {
        ASSERT_LT(count, expected.GetCount());
        EXPECT_EQ(enumerator->CurrentRef(), expected.Get(count++));
    }
    EXPECT_EQ(count, expected.GetCount());
    EXPECT_EQ(count, collection.GetCount());
    EXPECT_FALSE(enumerator->MoveNext());
    EXPECT_THROW(enumerator->CurrentRef(), std::logic_error);
}

void ExpectDictionary(IDictionary<int, std::string>& dictionary) {
    dictionary.Add(2, "two");
    dictionary.Add(1, "one");
    dictionary.Add(3, "three");
    dictionary.Get(2) = "updated";
    const auto& view = dictionary;
    EXPECT_EQ(view.GetCount(), 3);
    EXPECT_EQ(view.Get(2), "updated");
    EXPECT_EQ(view.Find(9), nullptr);
    EXPECT_THROW(dictionary.Add(2, "duplicate"), std::invalid_argument);
    EXPECT_THROW(dictionary.Get(9), std::out_of_range);
    EXPECT_THROW(dictionary.Remove(9), std::out_of_range);
    auto entries = view.GetEnumerator();
    int count = 0;
    while (entries->MoveNext()) {
        const auto& entry = entries->CurrentRef();
        EXPECT_EQ(entry.value, view.Get(entry.key));
        ++count;
    }
    EXPECT_EQ(count, 3);
    for (int key : {2, 1, 3}) dictionary.Remove(key);
    EXPECT_TRUE(dictionary.IsEmpty());
    EXPECT_FALSE(dictionary.GetEnumerator()->MoveNext());
}

TEST(UniquePtr, Ownership) {
    UniquePtr<int> owner(new int(7));
    UniquePtr<int> moved(Move(owner));
    EXPECT_FALSE(owner);
    ASSERT_EQ(*moved, 7);
    int* raw = moved.release();
    EXPECT_FALSE(moved);
    delete raw;
    UniquePtr<int[]> array(new int[2]{10, 20});
    EXPECT_EQ(array[1], 20);
    array.reset();
    EXPECT_EQ(array.get(), nullptr);
}

TEST(ForwardList, BasicOperations) {
    ForwardList<int> list;
    EXPECT_THROW(list.PopFront(), std::out_of_range);
    for (int value = 3; value > 0; --value) list.PushFront(value);
    int expected = 1;
    for (int value : list) ASSERT_TRUE(value == expected++);
    ASSERT_TRUE(expected == 4 && list.GetCount() == 3);
    ForwardList<int> copy(list);
    list.PopFront();
    ASSERT_TRUE(copy.GetCount() == 3 && *copy.begin() == 1);
    list = Move(copy);
    ASSERT_TRUE(copy.IsEmpty() && list.GetCount() == 3);
    ASSERT_TRUE(list.RemoveFirst([](int value) { return value == 2; }));
    ASSERT_TRUE(list.GetCount() == 2 && !list.RemoveFirst([](int value) { return value == 9; }));
    list.Clear();
    EXPECT_TRUE(list.IsEmpty());
}

TEST(Sequences, ArraysAndSorting) {
    DynamicArray<int> array;
    for (int i = 20; i > 0; --i) array.Append(i);
    array.Sort(Less<int>());
    EXPECT_EQ(array.Get(0), 1);
    EXPECT_EQ(array.Get(19), 20);
    auto copied = array;
    copied[0] = 99;
    EXPECT_EQ(array.Get(0), 1);
    ArraySequence<int> values;
    ExpectEnumeration(values, MakeTestSequence<int>());
    EXPECT_THROW(values.Get(0), std::out_of_range);
    values.Append(2);
    values.InsertAt(0, 1);
    values.InsertAt(2, 3);
    ArraySequence<int> copy(values);
    values.GetMutable(0) = 9;
    values.RemoveAt(1);
    ExpectEnumeration(copy, MakeTestSequence<int>(1, 2, 3));
    ExpectEnumeration(values, MakeTestSequence<int>(9, 3));
    EXPECT_THROW(values.RemoveAt(2), std::out_of_range);
    EXPECT_THROW(values.Get(2), std::out_of_range);
    DynamicArray<int> buffer;
    buffer.Append(7);
    const int* original = &buffer.Get(0);
    ArraySequence<int> transferred(Move(buffer));
    EXPECT_TRUE(buffer.IsEmpty());
    EXPECT_EQ(&transferred.Get(0), original);

    auto source = MakeTestSequence<int>(4, 1, 3, 1);
    SortedArraySequence<int> sorted(source);
    ExpectEnumeration(sorted, MakeTestSequence<int>(1, 1, 3, 4));
    ExpectEnumeration(source, MakeTestSequence<int>(4, 1, 3, 1));
    ASSERT_TRUE(sorted.IndexOf(1) == 0 && sorted.IndexOf(2) == -1);
    sorted.Add(0);
    sorted.Add(8);
    ASSERT_TRUE(sorted.Remove(1) && !sorted.Remove(9));
    ExpectEnumeration(sorted, MakeTestSequence<int>(0, 1, 3, 4, 8));

}

TEST(BinaryTreeSet, BalanceAndSetOperations) {
    const int orders[][3] = {
        {3, 2, 1},
        {1, 2, 3},
        {3, 1, 2},
        {1, 3, 2}
    };
    for (const auto& order : orders) {
        BinaryTreeSet<int> set;
        for (int value : order) {
            ASSERT_TRUE(set.Add(value) && set.IsValid());
        }
        EXPECT_EQ(set.GetHeight(), 2);
        EXPECT_FALSE(set.Add(2));
        ExpectEnumeration(set, MakeTestSequence<int>(1, 2, 3));
        const int removalOrder[]{2, 1, 3};
        for (int value : removalOrder) {
            ASSERT_TRUE(set.Remove(value) && set.IsValid());
        }
        ASSERT_TRUE(set.IsEmpty() && !set.Remove(0));
    }

    BinaryTreeSet<int> first;
    BinaryTreeSet<int> second;
    BinaryTreeSet<int> empty;
    const int a[]{1, 2, 3};
    const int b[]{3, 4};
    for (int value : a) first.Add(value);
    for (int value : b) second.Add(value);
    ExpectEnumeration(*first.Union(second), MakeTestSequence<int>(1, 2, 3, 4));
    ExpectEnumeration(*first.Intersection(second), MakeTestSequence<int>(3));
    ExpectEnumeration(*first.Difference(second), MakeTestSequence<int>(1, 2));
    ASSERT_TRUE(first.Difference(first)->IsEmpty() && first.Intersection(empty)->IsEmpty());
    auto united = first.Union(empty);
    united->Remove(1);
    ASSERT_TRUE(first.Contains(1) && first.GetCount() == 3 && second.GetCount() == 2);

    BinaryTreeSet<int, bool (*)(int, int)> descending([](int x, int y) { return x > y; });
    for (int value : a) descending.Add(value);
    ExpectEnumeration(descending, MakeTestSequence<int>(3, 2, 1));
    EXPECT_TRUE(descending.Contains(1) && descending.Contains(3));
    EXPECT_FALSE(descending.Contains(9));
    EXPECT_TRUE(descending.Remove(2));
    EXPECT_FALSE(descending.Remove(9));
    ExpectEnumeration(descending, MakeTestSequence<int>(3, 1));
    EXPECT_TRUE(descending.IsValid());
}

TEST(Dictionaries, BasicOperations) {
    SortedDictionary<int, std::string> sorted;
    TreeDictionary<int, std::string> tree;
    HashTable<int, std::string> hash;
    IDictionary<int, std::string>* dictionaries[]{&sorted, &tree, &hash};
    for (auto* dictionary : dictionaries) {
        ExpectDictionary(*dictionary);
    }
    EXPECT_TRUE(tree.IsValid());
}

TEST(HashTable, CollisionsAndOwnership) {
    struct ConstantHash { int operator()(int) const { return 0; } };
    HashTable<int, std::string, ConstantHash> table;
    for (int key = 0; key < 32; ++key) table.Add(key, std::to_string(key));
    EXPECT_GT(table.GetCapacity(), 8);
    table.Add(32, table.Get(0));
    EXPECT_EQ(table.Get(32), "0");
    table.Remove(32);
    auto copy = table;
    copy.Get(1) = "copy";
    EXPECT_EQ(table.Get(1), "1");
    HashTable<int, std::string, ConstantHash> moved(Move(copy));
    ASSERT_TRUE(copy.IsEmpty() && moved.Get(1) == "copy");
    for (int key = 0; key < 32; ++key) {
        EXPECT_EQ(table.Get(key), std::to_string(key));
        table.Remove(key);
    }
    ASSERT_TRUE(table.IsEmpty() && table.GetCapacity() == 8);
    table.Add(-1, "negative");
    EXPECT_EQ(table.Get(-1), "negative");
}

template <class Record, class Key, class Selector>
void ExpectIndex(const Index<Record, Key>& index, const IEnumerable<Record>& source,
                 const Key& key, Selector select) {
    const auto& found = index.FindAll(key);
    auto records = source.GetEnumerator();
    int match = 0;
    while (records->MoveNext()) {
        const Record& record = records->CurrentRef();
        if (select(record) == key) {
            ASSERT_LT(match, found.GetCount());
            EXPECT_EQ(found.Get(match++), &record);
        }
    }
    EXPECT_EQ(match, found.GetCount());
}

template <class Record>
void CheckSource(const ICollectible<Record>& source) {
    const IEnumerable<Record>& enumerable = source;
    ArraySequence<const Record*> addresses;
    auto records = source.GetEnumerator();
    while (records->MoveNext()) addresses.Append(&records->CurrentRef());
    records.reset();
    const auto select = [](const Record& record) { return GetPerson(record).birthYear; };
    const auto check = [&](auto dictionary) {
        Index<Record, int> index(enumerable, Move(dictionary), select);
        EXPECT_TRUE(index.FindAll(1900).IsEmpty());
        auto entries = index.GetEntriesEnumerator();
        int total = 0;
        while (entries->MoveNext()) {
            const auto& entry = entries->CurrentRef();
            ExpectIndex(index, enumerable, entry.key, select);
            total += entry.value.GetCount();
        }
        EXPECT_EQ(total, source.GetCount());
    };
    check(MakeUnique<SortedDictionary<int, ArraySequence<const Record*>>>());
    check(MakeUnique<TreeDictionary<int, ArraySequence<const Record*>>>());
    check(MakeUnique<HashTable<int, ArraySequence<const Record*>>>());
    ArraySequence<const Record*> remaining;
    records = source.GetEnumerator();
    while (records->MoveNext()) remaining.Append(&records->CurrentRef());
    ExpectEnumeration(remaining, addresses);
}

TEST(Index, SourcesAndKeys) {
    const auto load = [](auto& source, auto add) {
        std::istringstream input("id,firstName,lastName,birthYear\n"
            "9,A,Ivanov,2004\n2,B,Petrov,2003\n17,C,Ivanov,2004\n");
        ReadPeopleCsv(input, add);
        EXPECT_EQ(source.GetCount(), 3);
        CheckSource(source);
    };
    ArraySequence<Person> people;
    load(people, [&](Person person) { people.Append(Move(person)); });
    BinaryTreeSet<Person> set;
    load(set, [&](const Person& person) { set.Add(person); });
    SortedArraySequence<Person> sorted;
    load(sorted, [&](const Person& person) { sorted.Add(person); });
    SortedDictionary<int, Person> sortedDictionary;
    TreeDictionary<int, Person> treeDictionary;
    HashTable<int, Person> hashTable;
    IDictionary<int, Person>* dictionaries[]{&sortedDictionary, &treeDictionary, &hashTable};
    for (auto* dictionary : dictionaries) {
        load(*dictionary, [&](const Person& person) { dictionary->Add(person.id, person); });
    }
    EXPECT_EQ(people.GetFirst().id, 9);
    EXPECT_EQ(sorted.GetFirst().id, 2);
}

TEST(Index, EmptyAndDuplicateKeys) {
    ArraySequence<Person> empty;
    const auto check = [&](auto makeDictionary) {
        Index<Person, int> index(empty, makeDictionary(), ByBirthYear);
        ASSERT_TRUE(index.GetKeyCount() == 0 && index.FindAll(2000).IsEmpty());
        const auto equal = MakeTestSequence<Person>(Person{1, "A", "B", 7},
            Person{2, "C", "D", 7}, Person{3, "E", "F", 7});
        Index<Person, int> duplicates(equal, makeDictionary(), ByBirthYear);
        EXPECT_EQ(duplicates.FindAll(7).GetCount(), 3);
        ExpectIndex(duplicates, equal, 7, ByBirthYear);
    };
    check([] { return MakeUnique<SortedDictionary<int, ArraySequence<const Person*>>>(); });
    check([] { return MakeUnique<TreeDictionary<int, ArraySequence<const Person*>>>(); });
    check([] { return MakeUnique<HashTable<int, ArraySequence<const Person*>>>(); });
    const auto people = MakeTestSequence<Person>(Person{1, "A", "B", 2004});
    auto dictionary = MakeUnique<SortedDictionary<int, ArraySequence<const Person*>>>();
    const auto* original = dictionary.get();
    Index<Person, int> index(people, Move(dictionary), ByBirthYear);
    const ArraySequence<const Person*>& matches = index.FindAll(2004);
    EXPECT_EQ(&matches, &original->Get(2004));
    EXPECT_FALSE(dictionary);
    EXPECT_EQ(original->Get(2004).Get(0), &people.Get(0));
    EXPECT_THROW((Index<Person, int>(people, nullptr, ByBirthYear)), std::invalid_argument);
    auto populated = MakeUnique<SortedDictionary<int, ArraySequence<const Person*>>>();
    populated->Add(2004, MakeTestSequence<const Person*>(&people.Get(0)));
    EXPECT_THROW((Index<Person, int>(people, Move(populated), ByBirthYear)), std::invalid_argument);

    BinaryTreeSet<Person> unique;
    EXPECT_TRUE(unique.Add(Person{1, "A", "B", 2000}));
    EXPECT_FALSE(unique.Add(Person{1, "C", "D", 2001}));
}

TEST(Index, CompositeKeys) {
    ArraySequence<Person> people;
    people.Append(Person{1, "A", "Ivanov", 2004});
    people.Append(Person{2, "B", "Ivanov", 2003});
    people.Append(Person{3, "C", "Petrov", 2004});
    people.Append(Person{4, "D", "Ivanov", 2004});
    const auto select = [](const Person& person) {
        return std::make_pair(person.lastName, person.birthYear);
    };
    const auto check = [&](auto dictionary) {
        Index<Person, std::pair<std::string, int>> index(people, Move(dictionary), select);
        EXPECT_EQ(index.GetKeyCount(), 3);
        ExpectIndex(index, people, std::pair<std::string, int>{"Ivanov", 2004}, select);
        ExpectIndex(index, people, std::pair<std::string, int>{"Ivanov", 2003}, select);
        ExpectIndex(index, people, std::pair<std::string, int>{"Petrov", 2004}, select);
        EXPECT_TRUE(index.FindAll(std::pair<std::string, int>{"Ivanov", 1999}).IsEmpty());
        EXPECT_TRUE(index.FindAll(std::pair<std::string, int>{"Missing", 2004}).IsEmpty());
    };
    check(MakeUnique<SortedDictionary<std::pair<std::string, int>, ArraySequence<const Person*>>>());
    check(MakeUnique<TreeDictionary<std::pair<std::string, int>, ArraySequence<const Person*>>>());
    struct ConstantHash { int operator()(const std::pair<std::string, int>&) const { return 0; } };
    check(MakeUnique<HashTable<std::pair<std::string, int>, ArraySequence<const Person*>, ConstantHash>>());
}

TEST(Index, NonCopyableRecordsAndLifetime) {
    struct Payload {
        int* destroyed;
        ~Payload() { ++*destroyed; }
    };
    struct Record {
        int key{0};
        std::unique_ptr<Payload> payload;
    };
    int destroyed = 0;
    {
        ArraySequence<Record> source;
        source.Append(Record{7, std::unique_ptr<Payload>(new Payload{&destroyed})});
        const Record* original = &source.Get(0);
        {
            const auto select = [](const Record& record) { return record.key; };
            Index<Record, int> indexes[]{
                Index<Record, int>(source, MakeUnique<SortedDictionary<int, ArraySequence<const Record*>>>(), select),
                Index<Record, int>(source, MakeUnique<TreeDictionary<int, ArraySequence<const Record*>>>(), select),
                Index<Record, int>(source, MakeUnique<HashTable<int, ArraySequence<const Record*>>>(), select)
            };
            for (const auto& index : indexes) {
                ASSERT_EQ(index.FindAll(7).GetCount(), 1);
                EXPECT_EQ(index.FindAll(7).Get(0), original);
                EXPECT_NE(index.FindAll(7).Get(0)->payload.get(), nullptr);
                EXPECT_TRUE(index.FindAll(9).IsEmpty());
            }
        }
        EXPECT_EQ(destroyed, 0);
        EXPECT_EQ(source.Get(0).payload.get(), original->payload.get());
    }
    EXPECT_EQ(destroyed, 1);
}

TEST(Csv, ParsingAndIndexing) {
    std::istringstream input("id,firstName,lastName,birthYear\r\n"
        "9,Анна Мария,Иванов,2004\r\n17,Алексей,Иванов,2004\r\n"
        "2,Иван Иваныч,Петров,2003\r\n\r\n");
    ArraySequence<Person> people;
    ReadPeopleCsv(input, [&](Person person) { people.Append(Move(person)); });
    ASSERT_EQ(people.GetCount(), 3);
    EXPECT_EQ(people.Get(0).firstName, "Анна Мария");
    EXPECT_EQ(people.Get(2).firstName, "Иван Иваныч");
    EXPECT_EQ(people.Get(2).id, 2);
    CheckSource(people);
    const auto check = [&](auto dictionary) {
        Index<Person, std::string> index(people, Move(dictionary), ByLastName);
        EXPECT_EQ(index.FindAll("Иванов").GetCount(), 2);
        ExpectIndex(index, people, std::string("Иванов"), ByLastName);
    };
    check(MakeUnique<SortedDictionary<std::string, ArraySequence<const Person*>>>());
    check(MakeUnique<TreeDictionary<std::string, ArraySequence<const Person*>>>());
    check(MakeUnique<HashTable<std::string, ArraySequence<const Person*>>>());
    std::istringstream empty("id,firstName,lastName,birthYear\r\n");
    ReadPeopleCsv(empty, [](const Person&) { FAIL() << "CSV должен быть пустым."; });
}

TEST(Csv, FilesAndInvalidInput) {
    const auto path = std::filesystem::temp_directory_path() /
        ("search-data-people-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".csv");
    {
        std::ofstream output(path, std::ios::binary);
        ASSERT_TRUE(output);
        output << "id,firstName,lastName,birthYear\r\n1,A,B,2004";
    }
    ArraySequence<Person> people;
    ReadPeopleCsv(path.string(), [&](Person person) { people.Append(Move(person)); });
    std::filesystem::remove(path);
    Index<Person, int> index(people, MakeUnique<HashTable<int, ArraySequence<const Person*>>>(), ByBirthYear);
    ASSERT_EQ(index.FindAll(2004).GetCount(), 1);
    EXPECT_EQ(index.FindAll(2004).Get(0), &people.Get(0));
    const auto ignore = [](const Person&) {};
    EXPECT_THROW(ReadPeopleCsv(path.string(), ignore), std::runtime_error);
    const char* invalid[]{"", "id,lastName,birthYear\n", "id,firstName,lastName,birthYear,id\n",
        "id;firstName;lastName;birthYear\n", "lastName,id,firstName,birthYear\n",
        "\xEF\xBB\xBFid,firstName,lastName,birthYear\n"};
    for (const char* text : invalid) {
        std::istringstream input(text);
        EXPECT_THROW(ReadPeopleCsv(input, ignore), std::invalid_argument);
    }
    const char* rows[]{"1,A,B\n", "1,A,B,2004,extra\n", "bad,A,B,2004\n", "1,A,B,2004bad\n", "1,\"A,B,2004\n",
        "1,\"A\"bad,B,2004\n", "1,A\"bad,B,2004\n", "1,A,B,2004\n1,C,D,2005\n"};
    for (const char* row : rows) {
        std::istringstream input(std::string("id,firstName,lastName,birthYear\n") + row);
        EXPECT_THROW(ReadPeopleCsv(input, ignore), std::invalid_argument);
    }
}

TEST(Benchmark, MeasurementsAndCsv) {
    BenchmarkOptions options;
    options.records = 1000;
    options.queries = 1000;
    options.repetitions = 3;
    const auto results = RunBenchmarks(1, options);
    ASSERT_EQ(results.GetCount(), 6);
    const char* algorithms[]{"HashTable", "TreeDictionary", "SortedDictionary"};
    for (int i = 0; i < results.GetCount(); ++i) {
        const auto& row = results.Get(i);
        EXPECT_EQ(row.source, "ArraySequence");
        EXPECT_EQ(row.algorithm, algorithms[i % 3]);
        EXPECT_EQ(row.field, i < 3 ? "birthYear" : "lastName");
        EXPECT_EQ(row.status, "ok");
        EXPECT_EQ(row.records, options.records);
        EXPECT_EQ(row.queries, options.queries);
        EXPECT_EQ(row.repetitions, options.repetitions);
        EXPECT_EQ(row.seed, options.seed);
        if (i < 3) EXPECT_EQ(row.keys, 51);
        else EXPECT_GT(row.keys, 200);
        EXPECT_GE(row.sourceMilliseconds, 0);
        for (const auto& time : {row.build, row.search, row.total}) {
            EXPECT_TRUE(std::isfinite(time.median));
            EXPECT_GE(time.minimum, 0);
            EXPECT_LE(time.minimum, time.median);
            EXPECT_LE(time.median, time.maximum);
        }
        EXPECT_GE(row.total.median, row.sourceMilliseconds);
    }
    const auto path = std::filesystem::temp_directory_path() / "search-data-test-results.csv";
    ExportMeasurements(results, path.string());
    std::ifstream input(path);
    std::string line;
    std::getline(input, line);
    EXPECT_EQ(line, "source,algorithm,field,input_order,records,unique_keys,queries,repetitions,seed,source_ms,"
                    "build_ms,build_min_ms,build_max_ms,query_ms,query_min_ms,query_max_ms,"
                    "total_ms,total_min_ms,total_max_ms,status");
    int lines = 0;
    while (std::getline(input, line)) {
        EXPECT_EQ(std::count(line.begin(), line.end(), ','), 19);
        ++lines;
    }
    input.close();
    std::filesystem::remove(path);
    EXPECT_EQ(lines, 6);
}

TEST(Benchmark, EverySourceUsesTheSameRecords) {
    BenchmarkOptions options;
    options.records = 128;
    options.queries = 64;
    options.repetitions = 1;
    options.seed = 17;
    const auto reference = RunBenchmarks(1, options);
    const char* names[]{"ArraySequence", "BinaryTreeSet", "SortedArraySequence",
        "SortedDictionary", "TreeDictionary", "HashTable"};
    for (int source = 1; source <= 6; ++source) {
        const auto results = RunBenchmarks(source, options);
        ASSERT_EQ(results.GetCount(), reference.GetCount());
        for (int i = 0; i < results.GetCount(); ++i) {
            const auto& row = results.Get(i);
            EXPECT_EQ(row.source, names[source - 1]);
            EXPECT_EQ(row.status, "ok");
            EXPECT_EQ(row.keys, reference.Get(i).keys);
            EXPECT_EQ(row.records, options.records);
            EXPECT_EQ(row.queries, options.queries);
            EXPECT_EQ(row.repetitions, 1);
            EXPECT_EQ(row.seed, 17U);
        }
    }
}

TEST(Benchmark, SourceTimeout) {
    BenchmarkOptions options;
    options.records = 10;
    options.timeLimitSeconds = 0;
    const auto results = RunBenchmarks(1, options);
    ASSERT_EQ(results.GetCount(), 6);
    for (int i = 0; i < results.GetCount(); ++i) {
        EXPECT_EQ(results.Get(i).status, "source_timeout");
        EXPECT_EQ(results.Get(i).repetitions, 0);
    }
}

TEST(Benchmark, InvalidParameters) {
    BenchmarkOptions options;
    EXPECT_THROW(RunBenchmarks(0, options), std::invalid_argument);
    EXPECT_THROW(RunBenchmarks(7, options), std::invalid_argument);
    options.records = 0;
    EXPECT_THROW(RunBenchmarks(1, options), std::invalid_argument);
    options.records = 100;
    options.queries = 0;
    EXPECT_THROW(RunBenchmarks(1, options), std::invalid_argument);
    options.queries = 10;
    options.repetitions = 0;
    EXPECT_THROW(RunBenchmarks(1, options), std::invalid_argument);
    options.repetitions = 1;
    options.timeLimitSeconds = std::nan("");
    EXPECT_THROW(RunBenchmarks(1, options), std::invalid_argument);
}

TEST(SharedStructures, CollectionInteroperability) {
    ArraySequence<int> array;
    SortedArraySequence<int> sorted;
    BinaryTree<int> tree;
    BinaryTreeSet<int> set;
    for (int value : {3, 1, 2}) {
        array.Append(value);
        sorted.Add(value);
        tree.Add(value);
        set.Add(value);
    }
    const ICollectible<int>* sources[]{&array, &sorted, &tree, &set};
    for (const ICollectible<int>* source : sources) {
        EXPECT_EQ(source->GetCount(), 3);
        EXPECT_FALSE(source->IsEmpty());
        const IEnumerable<int>& enumerable = *source;
        ArraySequence<int> copy(*source);
        auto entries = enumerable.GetEnumerator();
        int count = 0;
        while (entries->MoveNext()) EXPECT_EQ(entries->CurrentRef(), copy.Get(count++));
        EXPECT_EQ(count, source->GetCount());
        SortedArraySequence<int> ordered(*source);
        for (int index = 0; index < 3; ++index) EXPECT_EQ(ordered.Get(index), index + 1);
    }
}

TEST(SharedStructures, DictionaryContractsAndCopies) {
    SortedDictionary<int, std::string> sorted;
    TreeDictionary<int, std::string> tree;
    HashTable<int, std::string> hash;
    IDictionary<int, std::string>* dictionaries[]{&sorted, &tree, &hash};
    for (auto* dictionary : dictionaries) {
        dictionary->Add(2, "two");
        dictionary->Add(1, "one");
        dictionary->Get(2) = "updated";
        const IDictionary<int, std::string>& view = *dictionary;
        EXPECT_EQ(dictionary->Find(2), view.Find(2));
        EXPECT_EQ(view.Get(2), "updated");
        EXPECT_THROW(dictionary->Add(2, "duplicate"), std::invalid_argument);
        EXPECT_THROW(view.Get(9), std::out_of_range);
        const ICollectible<Pair<int, std::string>>& collection = view;
        ArraySequence<Pair<int, std::string>> entries(collection);
        ASSERT_EQ(entries.GetCount(), 2);
        auto enumerator = collection.GetEnumerator();
        while (enumerator->MoveNext()) {
            const auto& entry = enumerator->CurrentRef();
            EXPECT_EQ(&entry.value, view.Find(entry.key));
        }
        dictionary->Remove(1);
        EXPECT_FALSE(view.ContainsKey(1));
        EXPECT_THROW(dictionary->Remove(1), std::out_of_range);
    }
    // Mutable dictionary lookup must keep referring to the original AVL node.
    std::string* original = tree.Find(2);
    for (int key = 3; key < 100; ++key) tree.Add(key, std::to_string(key));
    EXPECT_EQ(tree.Find(2), original);
    auto copy = tree;
    copy.Get(2) = "copy";
    EXPECT_EQ(tree.Get(2), "updated");
    EXPECT_NE(copy.Find(2), original);
    TreeDictionary<int, std::string> moved(Move(copy));
    EXPECT_TRUE(copy.IsEmpty());
    copy.Add(1, "reused");
    EXPECT_EQ(moved.Get(2), "copy");
    EXPECT_TRUE(tree.IsValid());
    EXPECT_TRUE(moved.IsValid());

    HashTable<std::string, int> names;
    names.Add("Ivanov", 2);
    auto namesCopy = names;
    namesCopy.Get("Ivanov") = 3;
    EXPECT_EQ(names.Get("Ivanov"), 2);
}

TEST(SharedStructures, ExtendedTreeAndSetOperations) {
    BinaryTree<int> tree;
    for (int value : {4, 2, 6, 1, 3, 5, 7}) tree.Add(value);
    auto breadthFirst = tree.GetEnumerator(Traversal::BreadthFirst);
    for (int value : {4, 2, 6, 1, 3, 5, 7}) {
        ASSERT_TRUE(breadthFirst->MoveNext());
        EXPECT_EQ(breadthFirst->CurrentRef(), value);
    }
    EXPECT_FALSE(breadthFirst->MoveNext());
    auto subtree = tree.ExtractSubtree(2);
    EXPECT_TRUE(tree.ContainsSubtree(subtree));
    auto snapshot = tree;
    auto old = tree.GetEnumerator();
    EXPECT_EQ(tree.PopMaximum(), 7);
    EXPECT_THROW(old->MoveNext(), std::logic_error);
    EXPECT_EQ(snapshot.Maximum(), 7);
    EXPECT_EQ(tree.GetByPath(2, "R"), 3);
    EXPECT_EQ(tree.Where([](int value) { return value % 2 == 0; }).ToString(), "2 4 6");
    EXPECT_EQ(tree.Map([](int value) { return value * 2; }).Reduce(0,
        [](int value, int sum) { return sum + value; }), 42);

    auto first = BinaryTreeSet<int>::FromString("1 2 3");
    auto second = BinaryTreeSet<int>::FromString("3 4");
    EXPECT_EQ((first | second).ToString(), "1 2 3 4");
    EXPECT_EQ((first & second).ToString(), "3");
    EXPECT_EQ((first - second).ToString(), "1 2");
    EXPECT_TRUE((first & second).IsSubsetOf(first));
    auto setCopy = first;
    first.Clear();
    EXPECT_TRUE(first.IsEmpty());
    EXPECT_TRUE(setCopy.Equals(BinaryTreeSet<int>::FromString("1 2 3")));
}

TEST(SharedStructures, ArrayResourceLifetimeAndResize) {
    DynamicArray<std::shared_ptr<int>> owners;
    owners.Append(std::make_shared<int>(1));
    owners.Append(std::make_shared<int>(2));
    std::weak_ptr<int> removed = owners.Get(0);
    std::weak_ptr<int> last = owners.Get(1);
    owners.RemoveAt(0);
    EXPECT_TRUE(removed.expired());
    EXPECT_FALSE(last.expired());
    owners.PopBack();
    EXPECT_TRUE(last.expired());
    owners.Append(std::make_shared<int>(3));
    std::weak_ptr<int> cleared = owners.Get(0);
    owners.Clear();
    EXPECT_TRUE(cleared.expired());
    EXPECT_TRUE(owners.IsEmpty());

    DynamicArray<std::string> values;
    values.Append(std::string(128, 'x'));
    values.Resize(values.GetCapacity() + 1, values.Get(0));
    EXPECT_EQ(values.Back(), std::string(128, 'x'));
    values.Resize(1);
    EXPECT_EQ(values.GetSize(), 1);
    EXPECT_THROW(values.Resize(-1), std::invalid_argument);
    EXPECT_THROW(values.Reserve(-1), std::invalid_argument);
}

TEST(SharedStructures, ReverseAndShuffle) {
    DynamicArray<int> values;
    for (int value = 0; value < 100; ++value) values.Append(value);
    Reverse(values.begin(), values.end());
    for (int index = 0; index < values.GetSize(); ++index) EXPECT_EQ(values.Get(index), 99 - index);
    std::mt19937 random(42);
    Shuffle(values.begin(), values.end(), random);
    values.Sort();
    for (int index = 0; index < values.GetSize(); ++index) EXPECT_EQ(values.Get(index), index);
}

TEST(GUI, EverySourceDictionaryAndField) {
    const auto input = MakeTestSequence<Person>(Person{9, "Anna", "Ivanov", 2004},
        Person{2, "Ivan", "Petrov", 2003}, Person{17, "Alex", "Ivanov", 2004});
    for (int source = 0; source < 6; ++source) {
        for (int dictionary = 0; dictionary < 3; ++dictionary) {
            for (int field = 0; field < 3; ++field) {
                PeopleModel model(input, source, dictionary, field);
                const auto& records = model.GetRecords();
                ASSERT_EQ(records.GetCount(), 3);
                const ArraySequence<const Person*>* matches = nullptr;
                if (field == 0) matches = &model.GetNameIndex()->FindAll("Ivanov");
                else matches = &model.GetNumberIndex()->FindAll(field == 1 ? 2004 : 9);
                ASSERT_EQ(matches->GetCount(), field == 2 ? 1 : 2);
                for (int i = 0; i < matches->GetCount(); ++i) {
                    const Person* person = matches->Get(i);
                    EXPECT_EQ(person->lastName, "Ivanov");
                    bool belongsToSource = false;
                    for (int j = 0; j < records.GetCount(); ++j) {
                        if (records.Get(j) == person) belongsToSource = true;
                    }
                    EXPECT_TRUE(belongsToSource);
                    for (int j = 0; j < input.GetCount(); ++j) EXPECT_NE(person, &input.Get(j));
                }
                EXPECT_EQ(model.GetKeyCount(), field == 2 ? 3 : 2);
                if (source >= 1 && source <= 4) {
                    EXPECT_EQ(records.Get(0)->id, 2);
                    EXPECT_EQ(records.Get(1)->id, 9);
                    EXPECT_EQ(records.Get(2)->id, 17);
                }
            }
        }
    }
}

TEST(GUI, SourceLifetimeEmptyAndInvalidChoices) {
    UniquePtr<PeopleModel> model;
    {
        const auto input = MakeTestSequence<Person>(Person{1, "Anna", "Ivanov", 2004});
        model = MakeUnique<PeopleModel>(input, 5, 2, 0);
    }
    ASSERT_EQ(model->GetNameIndex()->FindAll("Ivanov").GetCount(), 1);
    EXPECT_EQ(model->GetNameIndex()->FindAll("Ivanov").Get(0), model->GetRecords().Get(0));
    EXPECT_EQ(model->GetRecords().Get(0)->firstName, "Anna");
    EXPECT_TRUE(model->GetNameIndex()->FindAll("Missing").IsEmpty());
    const ArraySequence<Person> empty;
    for (int source = 0; source < 6; ++source) {
        PeopleModel model(empty, source, 0, 0);
        EXPECT_TRUE(model.GetRecords().IsEmpty());
        EXPECT_EQ(model.GetKeyCount(), 0);
        EXPECT_TRUE(model.GetNameIndex()->FindAll("Ivanov").IsEmpty());
    }
    EXPECT_THROW(PeopleModel(empty, -1, 0, 0), std::invalid_argument);
    EXPECT_THROW(PeopleModel(empty, 6, 0, 0), std::invalid_argument);
    EXPECT_THROW(PeopleModel(empty, 0, 3, 0), std::invalid_argument);
    EXPECT_THROW(PeopleModel(empty, 0, 0, 3), std::invalid_argument);
}
