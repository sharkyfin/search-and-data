#pragma once

#include <stdexcept>

#include "pointers/PointerOperations.hpp"
#include "pointers/UniquePtr.hpp"
#include "core/Functional.hpp"
#include "core/IEnumerator.hpp"
#include "dictionaries/IDictionary.hpp"
#include "sequences/DynamicArray.hpp"
#include "core/ForwardList.hpp"

template <class K, class V, class Hash = Hash<K>>
class HashTable : public IDictionary<K, V> {
private:
    DynamicArray<ForwardList<Pair<K, V>>> buckets;
    int count{0};
    Hash hash;

    int GetBucketIndex(const K& key, int capacity) const noexcept {
        const int h = hash(key);
        const int idx = h % capacity;
        return idx >= 0 ? idx : -idx;
    }

    void Rehash(int capacity) {
        DynamicArray<ForwardList<Pair<K, V>>> next;
        next.Resize(capacity);
        for (int i = 0; i < buckets.GetSize(); ++i) {
            for (const auto& entry : buckets[i]) {
                next[GetBucketIndex(entry.key, capacity)].PushFront(entry);
            }
        }
        buckets.Swap(next);
    }

    class Enumerator : public IEnumerator<Pair<K, V>> {
    private:
        const DynamicArray<ForwardList<Pair<K, V>>>& buckets;
        int bucket{0};
        typename ForwardList<Pair<K, V>>::ConstIterator position;
        bool hasCurrent{false};

    public:
        Enumerator(const DynamicArray<ForwardList<Pair<K, V>>>& buckets)
            : buckets(buckets) {}

        bool MoveNext() override {
            if (hasCurrent) {
                ++position;
                if (position != buckets[bucket].end()) {
                    return true;
                }
                ++bucket;
            }
            hasCurrent = false;
            while (bucket < buckets.GetSize()) {
                position = buckets[bucket].begin();
                if (position != buckets[bucket].end()) {
                    hasCurrent = true;
                    return true;
                }
                ++bucket;
            }
            return false;
        }

        const Pair<K, V>& CurrentRef() const override {
            if (!hasCurrent) {
                throw std::logic_error("Enumerator has no current element.");
            }
            return *position;
        }
    };

public:
    HashTable(Hash hash = Hash())
        : hash(Move(hash)) {
        buckets.Resize(8);
    }

    HashTable(const HashTable&) = default;
    HashTable& operator=(const HashTable&) = default;

    HashTable(HashTable&& other) noexcept
        : HashTable(other.hash) {
        buckets = Move(other.buckets);
        count = other.count;
        other.count = 0;
    }

    HashTable& operator=(HashTable&& other) noexcept {
        if (this != &other) {
            HashTable next(Move(other));
            buckets.Swap(next.buckets);
            count = next.count;
            hash = Move(next.hash);
            next.count = 0;
        }
        return *this;
    }

    int GetCount() const override {
        return count;
    }

    int GetCapacity() const {
        return buckets.GetSize();
    }

    V* Find(const K& key) override {
        if (buckets.IsEmpty()) return nullptr;
        for (auto& entry : buckets[GetBucketIndex(key, buckets.GetSize())]) {
            if (entry.key == key) {
                return &entry.value;
            }
        }
        return nullptr;
    }

    const V* Find(const K& key) const override {
        if (buckets.IsEmpty()) return nullptr;
        for (const auto& entry : buckets[GetBucketIndex(key, buckets.GetSize())]) {
            if (entry.key == key) {
                return &entry.value;
            }
        }
        return nullptr;
    }

    void Add(const K& key, const V& value) override {
        if (this->ContainsKey(key)) {
            throw std::invalid_argument("Key already exists.");
        }
        if (buckets.IsEmpty()) {
            buckets.Resize(8);
        }
        Pair<K, V> entry{key, value};
        if (count == buckets.GetSize()) {
            Rehash(buckets.GetSize() * 2);
        }
        const int bucketIndex = GetBucketIndex(entry.key, buckets.GetSize());
        buckets[bucketIndex].PushFront(Move(entry));
        ++count;
    }

    void Remove(const K& key) override {
        if (buckets.IsEmpty()) {
            throw std::out_of_range("Key was not found.");
        }
        auto& bucket = buckets[GetBucketIndex(key, buckets.GetSize())];
        bool removed = bucket.RemoveFirst([&](const Pair<K, V>& entry) {
            return entry.key == key;
        });
        if (removed) {
            --count;
            if (buckets.GetSize() > 8 && count <= buckets.GetSize() / 4) {
                Rehash(buckets.GetSize() / 2);
            }
            return;
        }
        throw std::out_of_range("Key was not found.");
    }

    UniquePtr<IEnumerator<Pair<K, V>>> GetEnumerator() const override {
        return MakeUnique<Enumerator>(buckets);
    }
};
