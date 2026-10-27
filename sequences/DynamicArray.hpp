#pragma once

#include <stdexcept>

#include "pointers/PointerOperations.hpp"
#include "pointers/UniquePtr.hpp"
#include "core/Functional.hpp"
#include "algorithms/Algorithm.hpp"

template <class T>
class DynamicArray {
private:
    UniquePtr<T[]> buffer;
    int count{0};
    int capacity{0};

    void Grow(int minCapacity) {
        int newCap = capacity == 0 ? 8 : capacity * 2;
        if (newCap < minCapacity) {
            newCap = minCapacity;
        }
        Reserve(newCap);
    }

public:
    DynamicArray() = default;

    DynamicArray(const DynamicArray& other) {
        if (other.count > 0) {
            Reserve(other.count);
            for (int i = 0; i < other.count; ++i) {
                buffer[i] = other.buffer[i];
            }
            count = other.count;
        }
    }

    DynamicArray(DynamicArray&& other) noexcept
        : buffer(Move(other.buffer)), count(other.count), capacity(other.capacity) {
        other.count = 0;
        other.capacity = 0;
    }

    DynamicArray& operator=(const DynamicArray& other) {
        if (this != &other) {
            DynamicArray copy(other);
            Swap(copy);
        }
        return *this;
    }

    DynamicArray& operator=(DynamicArray&& other) noexcept {
        if (this != &other) {
            buffer = Move(other.buffer);
            count = other.count;
            capacity = other.capacity;
            other.count = 0;
            other.capacity = 0;
        }
        return *this;
    }

    int GetSize() const noexcept { return count; }
    int GetCapacity() const noexcept { return capacity; }
    bool IsEmpty() const noexcept { return count == 0; }

    const T& Get(int index) const {
        if (index < 0 || index >= count) {
            throw std::out_of_range("DynamicArray index out of range.");
        }
        return buffer[index];
    }

    T& Get(int index) {
        if (index < 0 || index >= count) {
            throw std::out_of_range("DynamicArray index out of range.");
        }
        return buffer[index];
    }

    const T& operator[](int index) const noexcept {
        return buffer[index];
    }

    T& operator[](int index) noexcept {
        return buffer[index];
    }

    T& Back() {
        if (count == 0) {
            throw std::out_of_range("DynamicArray is empty.");
        }
        return buffer[count - 1];
    }

    void Reserve(int newCapacity) {
        if (newCapacity < 0) throw std::invalid_argument("Negative capacity.");
        if (newCapacity <= capacity) {
            return;
        }
        UniquePtr<T[]> newBuffer(new T[newCapacity]);
        for (int i = 0; i < count; ++i) {
            newBuffer[i] = Move(buffer[i]);
        }
        buffer = Move(newBuffer);
        capacity = newCapacity;
    }

    void Resize(int newSize, const T& defaultValue = T()) {
        if (newSize < 0) throw std::invalid_argument("Negative size.");
        T fill = defaultValue;
        if (newSize > capacity) {
            Reserve(newSize);
        }
        for (int i = count; i < newSize; ++i) {
            buffer[i] = fill;
        }
        for (int i = newSize; i < count; ++i) buffer[i] = T();
        count = newSize;
    }

    void Append(T value) {
        if (count == capacity) {
            Grow(count + 1);
        }
        buffer[count++] = Move(value);
    }

    void PopBack() {
        if (count == 0) {
            throw std::out_of_range("DynamicArray is empty.");
        }
        --count;
        buffer[count] = T();
    }

    void InsertAt(int index, T value) {
        if (index < 0 || index > count) {
            throw std::out_of_range("Insertion index is outside the sequence.");
        }
        if (count == capacity) {
            Grow(count + 1);
        }
        for (int i = count; i > index; --i) {
            buffer[i] = Move(buffer[i - 1]);
        }
        buffer[index] = Move(value);
        ++count;
    }

    void RemoveAt(int index) {
        if (index < 0 || index >= count) {
            throw std::out_of_range("Removal index is outside the sequence.");
        }
        for (int i = index; i + 1 < count; ++i) {
            buffer[i] = Move(buffer[i + 1]);
        }
        PopBack();
    }

    void Clear() noexcept {
        buffer.reset();
        count = 0;
        capacity = 0;
    }

    void Swap(DynamicArray& other) noexcept {
        buffer.swap(other.buffer);
        ::Swap(count, other.count);
        ::Swap(capacity, other.capacity);
    }

    template <class Compare = Less<T>>
    void Sort(Compare compare = Compare()) {
        if (count > 1) {
            QuickSort(buffer.get(), buffer.get() + count, compare);
        }
    }

    T* begin() noexcept { return buffer.get(); }
    T* end() noexcept { return count == 0 ? buffer.get() : buffer.get() + count; }
};
