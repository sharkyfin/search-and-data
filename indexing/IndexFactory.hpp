#pragma once

#include <stdexcept>

#include "pointers/UniquePtr.hpp"
#include "core/ICollectible.hpp"
#include "sequences/ArraySequence.hpp"
#include "sequences/SortedArraySequence.hpp"
#include "sets/BinaryTreeSet.hpp"
#include "dictionaries/SortedDictionary.hpp"
#include "dictionaries/TreeDictionary.hpp"
#include "dictionaries/HashTable.hpp"
#include "indexing/IndexTypes.hpp"
#include "domain/Person.hpp"

template <class K, class V>
class DictionaryValues : public ICollectible<V> {
private:
    UniquePtr<IDictionary<K, V>> dictionary;

    class Enumerator : public IEnumerator<V> {
    private:
        UniquePtr<IEnumerator<Pair<K, V>>> entries;

    public:
        Enumerator(UniquePtr<IEnumerator<Pair<K, V>>> entries)
            : entries(Move(entries)) {}

        bool MoveNext() override { return entries->MoveNext(); }
        const V& CurrentRef() const override { return entries->CurrentRef().value; }
    };

public:
    DictionaryValues(UniquePtr<IDictionary<K, V>> dictionary)
        : dictionary(Move(dictionary)) {}

    int GetCount() const override { return dictionary->GetCount(); }

    UniquePtr<IEnumerator<V>> GetEnumerator() const override {
        return MakeUnique<Enumerator>(dictionary->GetEnumerator());
    }
};

inline UniquePtr<ICollectible<Person>> MakePersonSource(
    const ICollectible<Person>& people, SourceKind source) {
    switch (source) {
        case SourceKind::Array:
            return MakeUnique<ArraySequence<Person>>(people);
        case SourceKind::TreeSet: {
            auto set = MakeUnique<BinaryTreeSet<Person>>();
            auto enumerator = people.GetEnumerator();
            while (enumerator->MoveNext()) {
                if (!set->Add(enumerator->CurrentRef())) {
                    throw std::invalid_argument("ID записей должны быть уникальными.");
                }
            }
            return set;
        }
        case SourceKind::Sorted: {
            return MakeUnique<SortedArraySequence<Person>>(people);
        }
        case SourceKind::SortedDictionary:
        case SourceKind::TreeDictionary:
        case SourceKind::HashTable: {
            UniquePtr<IDictionary<int, Person>> dictionary;
            if (source == SourceKind::SortedDictionary) {
                dictionary = MakeUnique<SortedDictionary<int, Person>>();
            } else if (source == SourceKind::TreeDictionary) {
                dictionary = MakeUnique<TreeDictionary<int, Person>>();
            } else {
                dictionary = MakeUnique<HashTable<int, Person>>();
            }
            auto enumerator = people.GetEnumerator();
            while (enumerator->MoveNext()) {
                const Person& person = enumerator->CurrentRef();
                if (dictionary->ContainsKey(person.id)) {
                    throw std::invalid_argument("ID записей должны быть уникальными.");
                }
                dictionary->Add(person.id, person);
            }
            return MakeUnique<DictionaryValues<int, Person>>(Move(dictionary));
        }
    }
    throw std::invalid_argument("Неизвестный источник записей.");
}
