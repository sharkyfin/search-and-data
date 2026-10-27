#pragma once

#include "domain/PersonSource.hpp"
#include "indexing/Index.hpp"

// Перебор значений словаря возвращает ссылки на Person внутри исходных пар.
class DictionaryPeople : public ICollectible<Person> {
private:
    UniquePtr<IDictionary<int, Person>> dictionary;

    class Enumerator : public IEnumerator<Person> {
    private:
        UniquePtr<IEnumerator<Pair<int, Person>>> entries;

    public:
        explicit Enumerator(UniquePtr<IEnumerator<Pair<int, Person>>> entries)
            : entries(Move(entries)) {}

        bool MoveNext() override { return entries->MoveNext(); }
        const Person& CurrentRef() const override { return entries->CurrentRef().value; }
    };

public:
    explicit DictionaryPeople(UniquePtr<IDictionary<int, Person>> dictionary)
        : dictionary(Move(dictionary)) {}

    int GetCount() const override { return dictionary->GetCount(); }
    UniquePtr<IEnumerator<Person>> GetEnumerator() const override {
        return MakeUnique<Enumerator>(dictionary->GetEnumerator());
    }
};

inline UniquePtr<ICollectible<Person>> CreatePeopleSource(const ArraySequence<Person>& records, int choice) {
    if (choice < 0 || choice > 5) throw std::invalid_argument("Неизвестный источник записей.");
    if (choice == 0) return MakeUnique<ArraySequence<Person>>(records);
    if (choice == 1) {
        auto people = MakeUnique<BinaryTreeSet<Person>>();
        for (int i = 0; i < records.GetCount(); ++i) people->Add(records.Get(i));
        return people;
    }
    if (choice == 2) return MakeUnique<SortedArraySequence<Person>>(records);
    UniquePtr<IDictionary<int, Person>> dictionary;
    if (choice == 3) dictionary = MakeUnique<SortedDictionary<int, Person>>();
    else if (choice == 4) dictionary = MakeUnique<TreeDictionary<int, Person>>();
    else dictionary = MakeUnique<HashTable<int, Person>>();
    for (int i = 0; i < records.GetCount(); ++i) {
        const auto& person = records.Get(i);
        dictionary->Add(person.id, person);
    }
    return MakeUnique<DictionaryPeople>(Move(dictionary));
}

template <class Key>
UniquePtr<IDictionary<Key, ArraySequence<const Person*>>> CreateIndexDictionary(int choice) {
    if (choice == 0) return MakeUnique<SortedDictionary<Key, ArraySequence<const Person*>>>();
    if (choice == 1) return MakeUnique<TreeDictionary<Key, ArraySequence<const Person*>>>();
    if (choice == 2) return MakeUnique<HashTable<Key, ArraySequence<const Person*>>>();
    throw std::invalid_argument("Неизвестный словарь индекса.");
}

class PeopleModel {
private:
    // Источник объявлен первым: он уничтожается после индексов и всех представлений.
    UniquePtr<ICollectible<Person>> source;
    ArraySequence<const Person*> records;
    UniquePtr<Index<Person, std::string>> names;
    UniquePtr<Index<Person, int>> numbers;

public:
    PeopleModel(const ArraySequence<Person>& people, int sourceChoice, int dictionaryChoice, int field) {
        if (field < 0 || field > 2) throw std::invalid_argument("Неизвестное поле индекса.");
        source = CreatePeopleSource(people, sourceChoice);
        auto entries = source->GetEnumerator();
        while (entries->MoveNext()) records.Append(&entries->CurrentRef());
        if (field == 0) {
            names = MakeUnique<Index<Person, std::string>>(*source,
                CreateIndexDictionary<std::string>(dictionaryChoice), ByLastName);
        } else {
            numbers = MakeUnique<Index<Person, int>>(*source, CreateIndexDictionary<int>(dictionaryChoice),
                [field](const Person& person) { return field == 1 ? person.birthYear : person.id; });
        }
    }

    const ArraySequence<const Person*>& GetRecords() const { return records; }
    const Index<Person, std::string>* GetNameIndex() const { return names.get(); }
    const Index<Person, int>* GetNumberIndex() const { return numbers.get(); }
    int GetKeyCount() const { return names ? names->GetKeyCount() : numbers->GetKeyCount(); }
};
