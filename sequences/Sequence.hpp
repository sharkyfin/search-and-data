#pragma once

#include <stdexcept>

#include "core/ICollectible.hpp"

template <class T>
class Sequence : public ICollectible<T> {
public:
    virtual const T& Get(int index) const = 0;

    const T& GetFirst() const {
        return Get(0);
    }

    const T& GetLast() const {
        if (this->IsEmpty()) {
            throw std::out_of_range("Sequence is empty.");
        }
        return Get(this->GetCount() - 1);
    }
};
