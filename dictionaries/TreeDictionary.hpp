#pragma once

#include <stdexcept>

#include "pointers/UniquePtr.hpp"
#include "dictionaries/IDictionary.hpp"
#include "trees/BinaryTree.hpp"

template <class K, class V>
class TreeDictionary : public IDictionary<K, V> {
private:
    struct PairLess {
        bool operator()(const Pair<K, V>& first, const Pair<K, V>& second) const noexcept {
            return first.key < second.key;
        }
    };

    BinaryTree<Pair<K, V>, PairLess> tree;

    static int Order(const K& key, const Pair<K, V>& entry) noexcept {
        return key < entry.key ? -1 : (entry.key < key ? 1 : 0);
    }

public:
    TreeDictionary() = default;

    int GetCount() const override {
        return tree.GetCount();
    }

    void Add(const K& key, const V& value) override {
        if (!tree.Add(Pair<K, V>{key, value})) {
            throw std::invalid_argument("Key already exists.");
        }
    }

    void Remove(const K& key) override {
        if (!tree.RemoveBy(key, Order)) {
            throw std::out_of_range("Key was not found.");
        }
    }

    V* Find(const K& key) override {
        Pair<K, V>* entry = tree.FindBy(key, Order);
        return entry ? &entry->value : nullptr;
    }

    const V* Find(const K& key) const override {
        const Pair<K, V>* entry = tree.FindBy(key, Order);
        return entry ? &entry->value : nullptr;
    }

    bool IsValid() const {
        return tree.IsValid();
    }

    UniquePtr<IEnumerator<Pair<K, V>>> GetEnumerator() const override {
        return tree.GetEnumerator();
    }
};
