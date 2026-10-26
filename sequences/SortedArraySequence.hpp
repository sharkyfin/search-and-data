#pragma once

#include "pointers/PointerOperations.hpp"
#include "pointers/UniquePtr.hpp"
#include "core/Functional.hpp"
#include "sequences/ArraySequence.hpp"
#include "sequences/ISortedSequence.hpp"

template <class T, class Compare = Less<T>>
class SortedArraySequence : public ISortedSequence<T> {
private:
    ArraySequence<T> values;
    Compare compare;

public:
    SortedArraySequence() = default;

    SortedArraySequence(const ICollectible<T>& source, Compare compare = Compare())
        : values(source), compare(Move(compare)) {
        values.Sort(this->compare);
    }

    int GetCount() const override {
        return values.GetCount();
    }

    const T& Get(int index) const override {
        return values.Get(index);
    }

    void Add(const T& value) override {
        values.InsertAt(LowerBound(values, value, compare), value);
    }

    int IndexOf(const T& value) const override {
        const int index = LowerBound(values, value, compare);
        if (index == values.GetCount() || compare(value, values.Get(index))) {
            return -1;
        }
        return index;
    }

    bool Remove(const T& value) override {
        const int index = IndexOf(value);
        if (index < 0) {
            return false;
        }
        values.RemoveAt(index);
        return true;
    }

    UniquePtr<IEnumerator<T>> GetEnumerator() const override {
        return values.GetEnumerator();
    }
};
