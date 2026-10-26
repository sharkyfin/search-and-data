#pragma once

#include "core/IEnumerable.hpp"

template <class T>
class ICollectible : public IEnumerable<T> {
public:
    ~ICollectible() override = default;
    virtual int GetCount() const = 0;

    bool IsEmpty() const {
        return GetCount() == 0;
    }
};
