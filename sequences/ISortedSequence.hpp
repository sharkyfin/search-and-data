#pragma once

#include "sequences/Sequence.hpp"

template <class T, class Key, class Less>
int LowerBound(const Sequence<T>& values, const Key& key, Less less) {
    int first = 0;
    int last = values.GetCount();
    while (first < last) {
        const int middle = first + (last - first) / 2;
        if (less(values.Get(middle), key)) {
            first = middle + 1;
        } else {
            last = middle;
        }
    }
    return first;
}

template <class T>
class ISortedSequence : public Sequence<T> {
public:
    virtual void Add(const T& value) = 0;
    virtual bool Remove(const T& value) = 0;
    virtual int IndexOf(const T& value) const = 0;
};
