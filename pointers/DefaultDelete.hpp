#pragma once

template <class T>
class DefaultDelete {
public:
    DefaultDelete() = default;

    template <class U>
    DefaultDelete(const DefaultDelete<U>&) noexcept {}

    void operator()(T* pointer) const noexcept {
        delete pointer;
    }
};

template <class T>
class DefaultDelete<T[]> {
public:
    void operator()(T* pointer) const noexcept {
        delete[] pointer;
    }

    template <class U>
    void operator()(U*) const = delete;
};
