#include <cmath>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <memory>
#include <type_traits>
#include <stdexcept>
#include <string>
#include <utility>

#include "benchmark/Benchmark.hpp"
#include "benchmark/CsvExporter.hpp"
#include "core/ForwardList.hpp"
#include "domain/Person.hpp"
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
    auto enumerator = collection.GetEnumerator();
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
void ExpectIndex(const Index<Record, Key>& index, const ICollectible<Record>& source,
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
    ArraySequence<const Record*> addresses;
    auto records = source.GetEnumerator();
    while (records->MoveNext()) addresses.Append(&records->CurrentRef());
    records.reset();
    const auto select = [](const Record& record) { return GetPerson(record).birthYear; };
    const auto check = [&](auto dictionary) {
        Index<Record, int> index(source, Move(dictionary), select);
        EXPECT_TRUE(index.FindAll(1900).IsEmpty());
        auto entries = index.GetEntriesEnumerator();
        int total = 0;
        while (entries->MoveNext()) {
            const auto& entry = entries->CurrentRef();
            ExpectIndex(index, source, entry.key, select);
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
    const auto people = MakeTestSequence<Person>(
        Person{9, "A", "Ivanov", 2004}, Person{2, "B", "Petrov", 2003},
        Person{17, "C", "Ivanov", 2004});
    BinaryTreeSet<Person> set;
    const SortedArraySequence<Person> sorted(people);
    SortedDictionary<int, Person> sortedDictionary;
    TreeDictionary<int, Person> treeDictionary;
    HashTable<int, Person> hashTable;
    auto records = people.GetEnumerator();
    while (records->MoveNext()) {
        const Person& person = records->CurrentRef();
        ASSERT_TRUE(set.Add(person));
        sortedDictionary.Add(person.id, person);
        treeDictionary.Add(person.id, person);
        hashTable.Add(person.id, person);
    }
    CheckSource(people);
    CheckSource(set);
    CheckSource(sorted);
    CheckSource(sortedDictionary);
    CheckSource(treeDictionary);
    CheckSource(hashTable);
    EXPECT_EQ(people.GetFirst().id, 9);
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
    static_assert(std::is_same_v<decltype(index.FindAll(2004)), const ArraySequence<const Person*>&>);
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
    const auto measurements = CompareIndexes<Person, std::pair<std::string, int>>(people, select,
        std::pair<std::string, int>{"Missing", 0}, ConstantHash());
    ASSERT_EQ(measurements.GetCount(), 9);
    for (int i = 0; i < measurements.GetCount(); ++i) {
        EXPECT_EQ(measurements.Get(i).keys, 3);
    }
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
    static_assert(!std::is_copy_constructible_v<Record>);
    static_assert(!std::is_constructible_v<Index<Record, int>, ArraySequence<Record>&&,
        UniquePtr<IDictionary<int, ArraySequence<const Record*>>>, int (*)(const Record&)>);
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

TEST(Benchmark, MeasurementsAndCsv) {
    const auto people = GeneratePeople(1000, 42);
    auto result = CompareIndexes<Person, int>(people, ByBirthYear, 0);
    auto names = CompareIndexes<Person, std::string>(people, ByLastName, "Missing");
    for (int i = 0; i < result.GetCount(); ++i) {
        EXPECT_EQ(result.Get(i).keys, 51);
        EXPECT_GT(names.Get(i).keys, 200);
        result.GetMutable(i).field = "birthYear";
        names.GetMutable(i).field = "lastName";
    }
    for (int i = 0; i < names.GetCount(); ++i) result.Append(names.Get(i));
    ASSERT_EQ(result.GetCount(), 18);
    const char* const algorithms[]{"HashTable", "TreeDictionary", "SortedDictionary"};
    const char* const orders[]{"Ascending", "Descending", "Random"};
    for (int i = 0; i < result.GetCount(); ++i) {
        const auto& row = result.Get(i);
        EXPECT_EQ(row.algorithm, algorithms[i % 3]);
        EXPECT_EQ(row.order, orders[(i / 3) % 3]);
        EXPECT_EQ(row.status, "ok");
        EXPECT_EQ(row.records, 1000);
        EXPECT_EQ(row.queries, 100000);
        EXPECT_TRUE(std::isfinite(row.buildMilliseconds) && row.buildMilliseconds >= 0);
        EXPECT_TRUE(std::isfinite(row.queryMilliseconds) && row.queryMilliseconds >= 0);
    }
    const auto limited = CompareIndexes<Person, int>(people, ByBirthYear, 0, Hash<int>(), 0.0);
    ASSERT_EQ(limited.GetCount(), 9);
    for (int i = 0; i < limited.GetCount(); ++i) {
        EXPECT_EQ(limited.Get(i).status, "build_timeout");
        EXPECT_EQ(limited.Get(i).queries, 0);
        result.Append(limited.Get(i));
    }
    const auto path = std::filesystem::temp_directory_path() / "search-data-test-results.csv";
    ExportMeasurements(result, path.string());
    std::ifstream input(path);
    std::string line;
    std::getline(input, line);
    EXPECT_EQ(line, "algorithm,field,order,records,unique_keys,queries,shuffle_seed,build_ms,query_ms,status");
    int lines = 0;
    while (std::getline(input, line)) ++lines;
    input.close();
    std::filesystem::remove(path);
    EXPECT_EQ(lines, 27);
    const ArraySequence<Person> empty;
    const auto emptyResults = CompareIndexes<Person, int>(empty, ByBirthYear, 0);
    EXPECT_EQ(emptyResults.GetCount(), 9);
    EXPECT_EQ(emptyResults.Get(0).keys, 0);
    EXPECT_EQ(emptyResults.Get(0).status, "ok");
}
