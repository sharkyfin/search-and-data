#pragma once

#include <stdexcept>

#include "pointers/UniquePtr.hpp"
#include "dictionaries/IDictionary.hpp"
#include "sequences/ArraySequence.hpp"
#include "sequences/ISortedSequence.hpp"

template <class K, class V>
class SortedDictionary : public IDictionary<K, V> {
private:
    ArraySequence<Pair<K, V>> entries;

    int Locate(const K& key) const {
        return LowerBound(entries, key, [](const Pair<K, V>& entry, const K& candidate) {
            return entry.key < candidate;
        });
    }

    bool Matches(int index, const K& key) const {
        return index < entries.GetCount() && !(key < entries.Get(index).key);
    }

public:
    int GetCount() const override {
        return entries.GetCount();
    }

    void Add(const K& key, const V& value) override {
        const int index = Locate(key);
        if (Matches(index, key)) {
            throw std::invalid_argument("Key already exists.");
        }
        entries.InsertAt(index, Pair<K, V>{key, value});
    }

    void Remove(const K& key) override {
        const int index = Locate(key);
        if (!Matches(index, key)) {
            throw std::out_of_range("Key was not found.");
        }
        entries.RemoveAt(index);
    }

    V* Find(const K& key) override {
        const int index = Locate(key);
        return Matches(index, key) ? &entries.GetMutable(index).value : nullptr;
    }

    const V* Find(const K& key) const override {
        const int index = Locate(key);
        return Matches(index, key) ? &entries.Get(index).value : nullptr;
    }

    UniquePtr<IEnumerator<Pair<K, V>>> GetEnumerator() const override {
        return entries.GetEnumerator();
    }
};
