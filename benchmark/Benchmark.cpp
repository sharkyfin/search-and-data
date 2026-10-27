#include "benchmark/Benchmark.hpp"

#include <chrono>
#include <random>
#include <stdexcept>

#include "domain/PersonSource.hpp"
#include "indexing/Index.hpp"

namespace {
using Clock = std::chrono::steady_clock;
constexpr const char* IndexNames[]{"HashTable", "TreeDictionary", "SortedDictionary"};

struct TimeLimitExceeded {};

double ElapsedMilliseconds(Clock::time_point start) {
    return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

void CheckTimeLimit(Clock::time_point start, double seconds) {
    if (ElapsedMilliseconds(start) >= seconds * 1000.0) throw TimeLimitExceeded{};
}

TimeSummary SummarizeTimes(ArraySequence<double> times) {
    times.Sort();
    const int middle = times.GetCount() / 2;
    const double median = times.GetCount() % 2 == 0
        ? (times.Get(middle - 1) + times.Get(middle)) / 2.0 : times.Get(middle);
    return {median, times.GetFirst(), times.GetLast()};
}

template <class Key, class Selector>
ArraySequence<Key> MakeQueries(const Sequence<Person>& people, Selector select,
                               const Key& missing, int count) {
    ArraySequence<Key> queries;
    queries.Reserve(count);
    int personIndex = 0;
    for (int i = 0; i < count; ++i) {
        if (i % 4 == 0) queries.Append(missing);
        else queries.Append(select(people.Get(personIndex++ % people.GetCount())));
    }
    return queries;
}

template <class Record, class Key>
UniquePtr<IDictionary<Key, ArraySequence<const Record*>>> MakeIndexDictionary(int choice) {
    if (choice == 0) return MakeUnique<HashTable<Key, ArraySequence<const Record*>>>();
    if (choice == 1) return MakeUnique<TreeDictionary<Key, ArraySequence<const Record*>>>();
    return MakeUnique<SortedDictionary<Key, ArraySequence<const Record*>>>();
}

template <class Record, class Key, class Selector>
Measurement MeasureIndex(const IEnumerable<Record>& source, const Sequence<Key>& queries,
                         Selector select, int dictionaryChoice, double timeLimit) {
    Measurement result;
    result.status = "build_timeout";
    auto start = Clock::now();
    int processedRecords = 0;
    try {
        Index<Record, Key> index(source, MakeIndexDictionary<Record, Key>(dictionaryChoice),
            [&](const Record& record) {
                if (processedRecords++ % 1024 == 0) CheckTimeLimit(start, timeLimit);
                return select(record);
            });
        result.build.median = ElapsedMilliseconds(start);
        result.keys = index.GetKeyCount();
        CheckTimeLimit(start, timeLimit);

        result.status = "query_timeout";
        start = Clock::now();
        // Читаем размер каждого результата, чтобы поиск выполнялся и в Release.
        volatile int foundCount = 0;
        for (int i = 0; i < queries.GetCount(); ++i) {
            if (i % 1024 == 0) CheckTimeLimit(start, timeLimit);
            foundCount = index.FindAll(queries.Get(i)).GetCount();
        }
        (void)foundCount;
        result.search.median = ElapsedMilliseconds(start);
        CheckTimeLimit(start, timeLimit);
        result.status = "ok";
    } catch (const TimeLimitExceeded&) {
        // Незавершённый замер не участвует в сводке и выборе победителя.
    }
    return result;
}

template <class Record, class Key, class Selector>
ArraySequence<Measurement> CompareIndexes(const IEnumerable<Record>& source,
    const Sequence<Key>& queries, Selector select, const BenchmarkOptions& options) {
    ArraySequence<Measurement> results;
    ArraySequence<double> buildTimes[3], searchTimes[3], totalTimes[3];
    for (const char* name : IndexNames) {
        Measurement row;
        row.algorithm = name;
        results.Append(row);
    }
    // Первый запуск — прогрев. Затем меняем порядок индексов в каждом повторении.
    for (int run = 0; run <= options.repetitions; ++run) {
        for (int position = 0; position < 3; ++position) {
            const int choice = (run + position) % 3;
            auto& row = results.GetMutable(choice);
            if (row.status != "ok") continue;
            const auto sample = MeasureIndex(source, queries, select, choice, options.timeLimitSeconds);
            row.status = sample.status;
            row.keys = sample.keys;
            if (run == 0 || sample.status != "ok") continue;
            buildTimes[choice].Append(sample.build.median);
            searchTimes[choice].Append(sample.search.median);
            totalTimes[choice].Append(sample.build.median + sample.search.median);
            ++row.repetitions;
        }
    }
    for (int choice = 0; choice < 3; ++choice) {
        auto& row = results.GetMutable(choice);
        if (row.status != "ok") continue;
        row.build = SummarizeTimes(buildTimes[choice]);
        row.search = SummarizeTimes(searchTimes[choice]);
        row.total = SummarizeTimes(totalTimes[choice]);
    }
    return results;
}

template <class Record>
ArraySequence<Measurement> BenchmarkSource(const IEnumerable<Record>& source,
    const Sequence<int>& years, const Sequence<std::string>& names, const BenchmarkOptions& options) {
    auto results = CompareIndexes(source, years,
        [](const Record& record) { return GetPerson(record).birthYear; }, options);
    for (int i = 0; i < results.GetCount(); ++i) results.GetMutable(i).field = "birthYear";
    auto byName = CompareIndexes(source, names,
        [](const Record& record) { return GetPerson(record).lastName; }, options);
    for (int i = 0; i < byName.GetCount(); ++i) {
        byName.GetMutable(i).field = "lastName";
        results.Append(byName.Get(i));
    }
    return results;
}
} // namespace

ArraySequence<Measurement> RunBenchmarks(int sourceChoice, const BenchmarkOptions& options) {
    if (sourceChoice < 1 || sourceChoice > 6 || options.records < 1 || options.records > 1000000
        || options.queries < 1 || options.repetitions < 1 || options.repetitions > 20
        || !(options.timeLimitSeconds >= 0 && options.timeLimitSeconds <= 3600)) {
        throw std::invalid_argument("Неверные параметры бенчмарка.");
    }
    auto people = GeneratePeople(options.records, options.seed);
    std::mt19937 random(options.seed);
    for (int i = people.GetCount() - 1; i > 0; --i) {
        std::uniform_int_distribution<int> position(0, i);
        Swap(people.GetMutable(i), people.GetMutable(position(random)));
    }
    const auto years = MakeQueries(people, ByBirthYear, 0, options.queries);
    const auto names = MakeQueries(people, ByLastName, std::string("Missing"), options.queries);
    ArraySequence<Measurement> results;
    const auto start = Clock::now();
    double sourceMilliseconds = 0;
    try {
        UsePeopleSource(sourceChoice, [&](auto add) {
            for (int i = 0; i < people.GetCount(); ++i) {
                if (i % 1024 == 0) CheckTimeLimit(start, options.timeLimitSeconds);
                add(people.Get(i));
            }
            sourceMilliseconds = ElapsedMilliseconds(start);
            CheckTimeLimit(start, options.timeLimitSeconds);
        }, [&](const auto& source) { results = BenchmarkSource(source, years, names, options); });
    } catch (const TimeLimitExceeded&) {
        sourceMilliseconds = ElapsedMilliseconds(start);
        for (const char* field : {"birthYear", "lastName"}) {
            for (const char* name : IndexNames) {
                Measurement row;
                row.field = field;
                row.algorithm = name;
                row.status = "source_timeout";
                results.Append(row);
            }
        }
    }
    for (int i = 0; i < results.GetCount(); ++i) {
        auto& row = results.GetMutable(i);
        row.source = PeopleSourceNames[sourceChoice - 1];
        row.records = options.records;
        row.queries = options.queries;
        row.seed = options.seed;
        row.sourceMilliseconds = sourceMilliseconds;
        if (row.status == "ok") {
            row.total.median += sourceMilliseconds;
            row.total.minimum += sourceMilliseconds;
            row.total.maximum += sourceMilliseconds;
        }
    }
    return results;
}
