#pragma once

#include <stdexcept>

#include "core/ICollectible.hpp"
#include "core/Pair.hpp"

template <class K, class V>
class IDictionary : public ICollectible<Pair<K, V>> {
public:
    virtual void Add(const K& key, const V& value) = 0;
    virtual void Remove(const K& key) = 0;
    virtual V* Find(const K& key) = 0;
    virtual const V* Find(const K& key) const = 0;

    bool ContainsKey(const K& key) const {
        return Find(key) != nullptr;
    }

    V& Get(const K& key) {
        V* value = Find(key);
        if (!value) {
            throw std::out_of_range("Key was not found.");
        }
        return *value;
    }

    const V& Get(const K& key) const {
        const V* value = Find(key);
        if (!value) {
            throw std::out_of_range("Key was not found.");
        }
        return *value;
    }
};
