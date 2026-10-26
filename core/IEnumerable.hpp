#pragma once

#include "pointers/UniquePtr.hpp"
#include "core/IEnumerator.hpp"

template <class T>
class IEnumerable {
public:
    virtual ~IEnumerable() = default;
    virtual UniquePtr<IEnumerator<T>> GetEnumerator() const = 0;
};
