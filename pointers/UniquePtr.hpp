#pragma once

#include <cstddef>

#include "DefaultDelete.hpp"
#include "PointerOperations.hpp"

template <class T, class Deleter = DefaultDelete<T>>
class UniquePtr {
private:
    T* ptr = nullptr;
    Deleter deleter{};

public:
    UniquePtr(T* pointer = nullptr) noexcept : ptr(pointer) {}

    UniquePtr(const UniquePtr&) = delete;

    UniquePtr(UniquePtr&& other) noexcept
        : ptr(other.release()), deleter(Move(other.deleter)) {}

    template <class U, class OtherDeleter>
    UniquePtr(UniquePtr<U, OtherDeleter>&& other) noexcept
        : ptr(other.release()), deleter(Move(other.get_deleter())) {}

    template <class U, class OtherDeleter>
    UniquePtr(UniquePtr<U[], OtherDeleter>&&) = delete;

    ~UniquePtr() {
        if (ptr) {
            deleter(ptr);
        }
    }

    UniquePtr& operator=(const UniquePtr&) = delete;

    UniquePtr& operator=(UniquePtr&& other) noexcept {
        if (this != &other) {
            reset(other.release());
            deleter = Move(other.deleter);
        }
        return *this;
    }

    T* get() const noexcept { return ptr; }

    Deleter& get_deleter() noexcept { return deleter; }

    operator bool() const noexcept { return ptr != nullptr; }

    T& operator*() const { return *ptr; }

    T* operator->() const noexcept { return ptr; }

    T* release() noexcept {
        T* old = ptr;
        ptr = nullptr;
        return old;
    }

    void reset(T* newPtr = nullptr) noexcept {
        if (ptr != newPtr) {
            T* old = ptr;
            ptr = newPtr;
            if (old) {
                deleter(old);
            }
        }
    }

    void swap(UniquePtr& other) noexcept {
        Swap(ptr, other.ptr);
        Swap(deleter, other.deleter);
    }
};

template <class T, class Deleter>
class UniquePtr<T[], Deleter> {
private:
    T* ptr = nullptr;
    Deleter deleter{};

public:
    UniquePtr(T* pointer = nullptr) noexcept : ptr(pointer) {}

    template <class U>
    UniquePtr(U*) = delete;

    UniquePtr(const UniquePtr&) = delete;

    UniquePtr(UniquePtr&& other) noexcept
        : ptr(other.release()), deleter(Move(other.deleter)) {}

    ~UniquePtr() {
        if (ptr) {
            deleter(ptr);
        }
    }

    UniquePtr& operator=(const UniquePtr&) = delete;

    UniquePtr& operator=(UniquePtr&& other) noexcept {
        if (this != &other) {
            reset(other.release());
            deleter = Move(other.deleter);
        }
        return *this;
    }

    T* get() const noexcept { return ptr; }

    T& operator[](int index) const { return ptr[index]; }

    T* release() noexcept {
        T* old = ptr;
        ptr = nullptr;
        return old;
    }

    void reset(T* newPtr = nullptr) noexcept {
        if (ptr != newPtr) {
            T* old = ptr;
            ptr = newPtr;
            if (old) {
                deleter(old);
            }
        }
    }

    template <class U>
    void reset(U*) = delete;

    void swap(UniquePtr& other) noexcept {
        Swap(ptr, other.ptr);
        Swap(deleter, other.deleter);
    }
};

template <class T, class Deleter>
bool operator==(const UniquePtr<T, Deleter>& ptr, std::nullptr_t) noexcept {
    return ptr.get() == nullptr;
}

template <class T, class... Args>
UniquePtr<T> MakeUnique(Args&&... args) {
    return UniquePtr<T>(new T(static_cast<Args&&>(args)...));
}
