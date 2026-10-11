#pragma once

#include <stdexcept>

#include "pointers/UniquePtr.hpp"
#include "core/IEnumerator.hpp"
#include "sequences/Sequence.hpp"
#include "sequences/DynamicArray.hpp"

template <class T>
class ArraySequence : public Sequence<T> {
private:
    DynamicArray<T> values;

    class Enumerator : public IEnumerator<T> {
    private:
        const DynamicArray<T>& values;
        int position{0};
        bool hasCurrent{false};

    public:
        Enumerator(const DynamicArray<T>& values)
            : values(values) {}

        bool MoveNext() override {
            hasCurrent = position < values.GetSize();
            if (hasCurrent) {
                ++position;
            }
            return hasCurrent;
        }

        const T& CurrentRef() const override {
            if (!hasCurrent) {
                throw std::logic_error("Enumerator has no current element.");
            }
            return values.Get(position - 1);
        }
    };

public:
    ArraySequence() = default;

    ArraySequence(DynamicArray<T>&& source) : values(Move(source)) {}

    ArraySequence(const ICollectible<T>& source) {
        values.Reserve(source.GetCount());
        auto enumerator = source.GetEnumerator();
        while (enumerator->MoveNext()) {
            values.Append(enumerator->CurrentRef());
        }
    }

    int GetCount() const override {
        return values.GetSize();
    }

    const T& Get(int index) const override {
        return values.Get(index);
    }

    T& GetMutable(int index) {
        return values.Get(index);
    }

    void Reserve(int capacity) {
        values.Reserve(capacity);
    }

    void Append(T value) {
        values.Append(Move(value));
    }

    void InsertAt(int index, T value) {
        values.InsertAt(index, Move(value));
    }

    void RemoveAt(int index) {
        values.RemoveAt(index);
    }

    void Clear() {
        values.Clear();
    }

    template <class Compare = Less<T>>
    void Sort(Compare compare = Compare()) {
        values.Sort(compare);
    }

    UniquePtr<IEnumerator<T>> GetEnumerator() const override {
        return MakeUnique<Enumerator>(values);
    }
};
