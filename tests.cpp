#include <cmath>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <stdexcept>
#include <string>
#include <utility>

#include "benchmark/Benchmark.hpp"
#include "benchmark/CsvExporter.hpp"
#include "core/ForwardList.hpp"
#include "indexing/IndexFactory.hpp"

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
    ASSERT_TRUE(dictionary.IsEmpty());
    dictionary.Add(2, "two");
    dictionary.Add(1, "one");
    dictionary.Add(3, "three");
    dictionary.Get(2) = "updated";
    const auto& view = dictionary;
    ASSERT_TRUE(view.GetCount() == 3 && view.Get(2) == "updated");
    ASSERT_TRUE(!view.ContainsKey(9) && view.Find(9) == nullptr);
    EXPECT_THROW(dictionary.Add(2, "duplicate"), std::invalid_argument);
    EXPECT_THROW(dictionary.Get(9), std::out_of_range);
    EXPECT_THROW(dictionary.Remove(9), std::out_of_range);
    auto entries = view.GetEnumerator();
    int count = 0;
    while (entries->MoveNext()) {
        const auto& entry = entries->CurrentRef();
        ASSERT_TRUE(entry.value == view.Get(entry.key));
        ++count;
    }
    EXPECT_EQ(count, 3);
    dictionary.Remove(2);
    ASSERT_TRUE(!dictionary.ContainsKey(2) && dictionary.GetCount() == 2);
    dictionary.Remove(1);
    dictionary.Remove(3);
    ASSERT_TRUE(dictionary.IsEmpty() && !dictionary.GetEnumerator()->MoveNext());
}

TEST(UniquePtr, Ownership) {
    UniquePtr<int> owner;
    EXPECT_FALSE(owner);
    owner.reset(new int(7));
    ASSERT_TRUE(static_cast<bool>(owner) && *owner == 7);

    UniquePtr<int> moved(Move(owner));
    ASSERT_TRUE(!owner && moved && *moved == 7);

    int* raw = moved.release();
    ASSERT_TRUE(!moved && raw && *raw == 7);
    delete raw;

    UniquePtr<int[]> array(new int[3]{10, 20, 30});
    ASSERT_TRUE(array[0] == 10 && array[1] == 20 && array[2] == 30);
    array[1] = 99;
    EXPECT_EQ(array[1], 99);
}

TEST(DynamicArray, BasicOperations) {
    DynamicArray<int> arr;
    ASSERT_TRUE(arr.IsEmpty() && arr.GetSize() == 0);
    for (int i = 0; i < 20; ++i) {
        arr.Append(20 - i);
    }
    ASSERT_TRUE(arr.GetSize() == 20 && arr.GetCapacity() >= 20);
    arr.Sort(Less<int>());
    for (int i = 0; i < 20; ++i) {
        EXPECT_EQ(arr[i], i + 1);
    }
    auto copy = arr;
    copy[0] = 99;
    EXPECT_EQ(arr[0], 1);
    DynamicArray<int> moved(Move(copy));
    ASSERT_TRUE(copy.IsEmpty() && moved[0] == 99);
    EXPECT_THROW(arr.Get(20), std::out_of_range);
    arr.InsertAt(0, 0);
    ASSERT_TRUE(arr[0] == 0 && arr.GetSize() == 21);
    arr.RemoveAt(0);
    ASSERT_TRUE(arr[0] == 1 && arr.GetSize() == 20);
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

TEST(ArraySequence, BasicOperations) {
    ArraySequence<int> values;
    ExpectEnumeration(values, MakeTestSequence<int>());
    EXPECT_THROW(values.GetFirst(), std::out_of_range);
    EXPECT_THROW(values.GetLast(), std::out_of_range);
    EXPECT_THROW(values.InsertAt(1, 0), std::out_of_range);
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
}

TEST(SortedArraySequence, OrderingAndSearch) {
    auto source = MakeTestSequence<int>(4, 1, 3, 1);
    SortedArraySequence<int> sorted(source);
    ExpectEnumeration(sorted, MakeTestSequence<int>(1, 1, 3, 4));
    ExpectEnumeration(source, MakeTestSequence<int>(4, 1, 3, 1));
    ASSERT_TRUE(sorted.IndexOf(1) == 0 && sorted.IndexOf(2) == -1);
    sorted.Add(0);
    sorted.Add(8);
    ASSERT_TRUE(sorted.Remove(1) && !sorted.Remove(9));
    ExpectEnumeration(sorted, MakeTestSequence<int>(0, 1, 3, 4, 8));
    const auto greater = [](int first, int second) { return first > second; };
    SortedArraySequence<int, decltype(greater)> descending(source, greater);
    descending.Add(5);
    ASSERT_TRUE(descending.IndexOf(5) == 0 && descending.IndexOf(0) == -1);
    ExpectEnumeration(descending, MakeTestSequence<int>(5, 4, 3, 1, 1));
}

TEST(BinaryTreeSet, RotationsAndRemoval) {
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
}

TEST(BinaryTreeSet, SetOperations) {
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

    struct Order {
        bool descending{false};
        bool operator()(int a, int b) const { return descending ? a > b : a < b; }
    };
    BinaryTreeSet<int, Order> descending(Order{true});
    for (int value : a) descending.Add(value);
    ExpectEnumeration(*descending.Union(second), MakeTestSequence<int>(4, 3, 2, 1));
    ExpectEnumeration(*descending.Intersection(first), MakeTestSequence<int>(3, 2, 1));
    ExpectEnumeration(*descending.Difference(second), MakeTestSequence<int>(2, 1));
}

TEST(SortedDictionary, BasicOperations) {
    SortedDictionary<int, std::string> dictionary;
    ExpectDictionary(dictionary);
}

TEST(TreeDictionary, BasicOperations) {
    TreeDictionary<int, std::string> dictionary;
    ExpectDictionary(dictionary);
    EXPECT_TRUE(dictionary.IsValid());
}

TEST(HashTable, BasicOperations) {
    HashTable<int, std::string> dictionary;
    ExpectDictionary(dictionary);
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

template <class Key, class Selector>
void ExpectIndex(const Index<Person, Key>& index, const Key& key, int count, Selector select) {
    const auto found = index.FindAll(key);
    ASSERT_EQ(found.GetCount(), count);
    int previous = -1;
    for (int i = 0; i < found.GetCount(); ++i) {
        const int position = found.Get(i).position;
        ASSERT_GT(position, previous);
        EXPECT_EQ(select(index.GetRecords().Get(position)), key);
        previous = position;
    }
}

TEST(Index, SourcesAndKeys) {
    ArraySequence<Person> people;
    people.Append(Person{3, "Anna", "Ivanov", 2004});
    people.Append(Person{1, "Ivan", "Petrov", 2003});
    people.Append(Person{2, "Alex", "Ivanov", 2004});
    const SourceKind sources[]{SourceKind::Array, SourceKind::TreeSet, SourceKind::Sorted,
        SourceKind::SortedDictionary, SourceKind::TreeDictionary, SourceKind::HashTable};
    using Positions = ArraySequence<RecordId>;
    for (SourceKind source : sources) {
        auto records = MakePersonSource(people, source);
        const auto check = [&](IndexKind kind, auto makeNames, auto makeYears) {
            SCOPED_TRACE(testing::Message() << "source=" << static_cast<int>(source)
                                            << ", dictionary=" << static_cast<int>(kind));
            Index<Person, std::string> names(*records, makeNames(), ByLastName);
            Index<Person, int> years(*records, makeYears(), ByBirthYear);
            const auto byId = [](const Person& person) { return person.id; };
            Index<Person, int> ids(*records, makeYears(), byId);
            ASSERT_TRUE(names.GetKeyCount() == 2 && years.GetKeyCount() == 2 && ids.GetKeyCount() == 3);
            ExpectIndex(names, std::string("Ivanov"), 2, ByLastName);
            ExpectIndex(years, 2004, 2, ByBirthYear);
            ExpectIndex(ids, 2, 1, byId);
            ASSERT_TRUE(names.FindAll("missing").IsEmpty() && years.FindAll(1900).IsEmpty() && ids.FindAll(99).IsEmpty());
            auto entries = names.GetEntriesEnumerator();
            int positions = 0;
            while (entries->MoveNext()) {
                const auto& entry = entries->CurrentRef();
                for (int i = 0; i < entry.value.GetCount(); ++i) {
                    ASSERT_TRUE(names.GetRecords().Get(entry.value.Get(i).position).lastName == entry.key);
                    ++positions;
                }
            }
            EXPECT_EQ(positions, people.GetCount());
            ArraySequence<Person> empty;
            auto emptySource = MakePersonSource(empty, source);
            Index<Person, int> emptyIndex(*emptySource, makeYears(), ByBirthYear);
            ASSERT_TRUE(emptyIndex.GetKeyCount() == 0 && emptyIndex.FindAll(2000).IsEmpty());
            EXPECT_EQ(people.GetFirst().id, 3);
        };
        check(IndexKind::Sorted, [] { return MakeUnique<SortedDictionary<std::string, Positions>>(); },
              [] { return MakeUnique<SortedDictionary<int, Positions>>(); });
        check(IndexKind::Tree, [] { return MakeUnique<TreeDictionary<std::string, Positions>>(); },
              [] { return MakeUnique<TreeDictionary<int, Positions>>(); });
        check(IndexKind::Hash, [] { return MakeUnique<HashTable<std::string, Positions>>(); },
              [] { return MakeUnique<HashTable<int, Positions>>(); });
    }
    Index<Person, std::string> snapshot(people,
        MakeUnique<TreeDictionary<std::string, Positions>>(), ByLastName);
    people.GetMutable(0).lastName = "Changed";
    people.Clear();
    ASSERT_TRUE(snapshot.GetRecords().GetCount() == 3 && snapshot.FindAll("Ivanov").GetCount() == 2);
}

TEST(Index, EmptyAndDuplicateKeys) {
    ArraySequence<Person> empty;
    using Positions = ArraySequence<RecordId>;
    const auto check = [&](auto makeDictionary) {
        Index<Person, int> index(empty, makeDictionary(), ByBirthYear);
        ASSERT_TRUE(index.GetKeyCount() == 0 && index.FindAll(2000).IsEmpty());
        auto equal = MakeTestSequence<int>(7, 7, 7);
        Index<int, int> duplicates(equal, makeDictionary(), [](int value) { return value; });
        ExpectEnumeration(duplicates.FindAll(7), MakeTestSequence<RecordId>(RecordId{0}, RecordId{1}, RecordId{2}));
    };
    check([] { return MakeUnique<SortedDictionary<int, Positions>>(); });
    check([] { return MakeUnique<TreeDictionary<int, Positions>>(); });
    check([] { return MakeUnique<HashTable<int, Positions>>(); });
    ArraySequence<Person> people;
    people.Append(Person{1, "A", "B", 2000});
    people.Append(Person{1, "C", "D", 2001});
    const SourceKind uniqueSources[]{SourceKind::TreeSet, SourceKind::SortedDictionary,
        SourceKind::TreeDictionary, SourceKind::HashTable};
    for (SourceKind source : uniqueSources) {
        EXPECT_THROW(MakePersonSource(people, source), std::invalid_argument);
    }
}

TEST(Index, CompositeKeys) {
    using Key = std::pair<std::string, int>;
    using Positions = ArraySequence<RecordId>;
    ArraySequence<Person> people;
    people.Append(Person{1, "A", "Ivanov", 2004});
    people.Append(Person{2, "B", "Ivanov", 2003});
    people.Append(Person{3, "C", "Petrov", 2004});
    people.Append(Person{4, "D", "Ivanov", 2004});
    const auto select = [](const Person& person) {
        return std::make_pair(person.lastName, person.birthYear);
    };
    const auto check = [&](auto dictionary) {
        Index<Person, Key> index(people, Move(dictionary), select);
        EXPECT_EQ(index.GetKeyCount(), 3);
        ExpectIndex(index, Key{"Ivanov", 2004}, 2, select);
        ExpectIndex(index, Key{"Ivanov", 2003}, 1, select);
        ExpectIndex(index, Key{"Petrov", 2004}, 1, select);
        EXPECT_TRUE(index.FindAll(Key{"Ivanov", 1999}).IsEmpty());
        EXPECT_TRUE(index.FindAll(Key{"Missing", 2004}).IsEmpty());
    };
    check(MakeUnique<SortedDictionary<Key, Positions>>());
    check(MakeUnique<TreeDictionary<Key, Positions>>());
    struct ConstantHash { int operator()(const Key&) const { return 0; } };
    check(MakeUnique<HashTable<Key, Positions, ConstantHash>>());
}

TEST(Index, DictionaryOwnership) {
    using Positions = ArraySequence<RecordId>;
    const auto people = MakeTestSequence<Person>(Person{1, "A", "B", 2004});
    auto dictionary = MakeUnique<SortedDictionary<int, Positions>>();
    const auto* original = dictionary.get();
    Index<Person, int> index(people, Move(dictionary), ByBirthYear);
    EXPECT_FALSE(dictionary);
    EXPECT_EQ(original->Get(2004).Get(0).position, 0);
    EXPECT_THROW((Index<Person, int>(people, nullptr, ByBirthYear)), std::invalid_argument);
    auto populated = MakeUnique<SortedDictionary<int, Positions>>();
    populated->Add(2004, MakeTestSequence<RecordId>(RecordId{99}));
    EXPECT_THROW((Index<Person, int>(people, Move(populated), ByBirthYear)), std::invalid_argument);
}

TEST(Benchmark, MeasurementsAndCsv) {
    const auto people = GeneratePeople(48, 42);
    const auto result = CompareIndexes<Person, int>(people, ByBirthYear, 0);
    EXPECT_EQ(result.GetCount(), 9);
    for (int row = 0; row < result.GetCount(); ++row) {
        const Measurement& measurement = result.Get(row);
        ASSERT_TRUE(measurement.records == 48 && measurement.queries == 512);
        ASSERT_TRUE(std::isfinite(measurement.buildMilliseconds) && measurement.buildMilliseconds >= 0);
        ASSERT_TRUE(std::isfinite(measurement.queryMilliseconds) && measurement.queryMilliseconds >= 0);
    }
    const auto path = std::filesystem::temp_directory_path() / "search-data-test-results.csv";
    ExportMeasurements(result, "birth_year", path.string());
    std::ifstream input(path);
    std::string line;
    std::getline(input, line);
    EXPECT_EQ(line, "algorithm,field,order,records,unique_keys,queries,shuffle_seed,build_ms,query_ms");
    int lines = 1;
    while (std::getline(input, line)) {
        ++lines;
    }
    input.close();
    std::filesystem::remove(path);
    EXPECT_EQ(lines, 10);
    ArraySequence<Person> empty;
    EXPECT_EQ((CompareIndexes<Person, int>(empty, ByBirthYear, 0).GetCount()), 9);
}
