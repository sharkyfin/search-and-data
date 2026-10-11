#pragma once

#include "pointers/UniquePtr.hpp"
#include "core/IEnumerator.hpp"

template <class T>
class ICollectible {
public:
    virtual ~ICollectible() = default;
    virtual int GetCount() const = 0;
    // Ссылки на элементы действуют, пока коллекция существует и не изменяется.
    virtual UniquePtr<IEnumerator<T>> GetEnumerator() const = 0;

    bool IsEmpty() const {
        return GetCount() == 0;
    }
};
