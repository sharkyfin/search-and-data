#pragma once

#include "pointers/PointerOperations.hpp"
#include "pointers/UniquePtr.hpp"
#include "core/Functional.hpp"
#include "sets/ISet.hpp"
#include "trees/BinaryTree.hpp"

template <class T, class Compare = Less<T>>
class BinaryTreeSet : public ISet<T> {
private:
    BinaryTree<T, Compare> tree;

public:
    BinaryTreeSet(Compare compare = Compare())
        : tree(Move(compare)) {}

    int GetCount() const override {
        return tree.GetCount();
    }

    bool Add(const T& value) override {
        return tree.Add(value);
    }

    bool Remove(const T& value) override {
        return tree.Remove(value);
    }

    bool Contains(const T& value) const override {
        return tree.Find(value) != nullptr;
    }

    UniquePtr<ISet<T>> Union(const ISet<T>& other) const override {
        auto result = MakeUnique<BinaryTreeSet<T, Compare>>(tree.GetCompare());
        const ISet<T>* sources[]{this, &other};
        for (const ISet<T>* source : sources) {
            auto enumerator = source->GetEnumerator();
            while (enumerator->MoveNext()) {
                result->Add(enumerator->CurrentRef());
            }
        }
        return result;
    }

    UniquePtr<ISet<T>> Intersection(const ISet<T>& other) const override {
        auto result = MakeUnique<BinaryTreeSet<T, Compare>>(tree.GetCompare());
        auto enumerator = GetEnumerator();
        while (enumerator->MoveNext()) {
            const T& value = enumerator->CurrentRef();
            if (other.Contains(value)) {
                result->Add(value);
            }
        }
        return result;
    }

    UniquePtr<ISet<T>> Difference(const ISet<T>& other) const override {
        auto result = MakeUnique<BinaryTreeSet<T, Compare>>(tree.GetCompare());
        auto enumerator = GetEnumerator();
        while (enumerator->MoveNext()) {
            const T& value = enumerator->CurrentRef();
            if (!other.Contains(value)) {
                result->Add(value);
            }
        }
        return result;
    }

    bool IsValid() const {
        return tree.IsValid();
    }

    int GetHeight() const {
        return tree.GetHeight();
    }

    UniquePtr<IEnumerator<T>> GetEnumerator() const override {
        return tree.GetEnumerator();
    }
};
