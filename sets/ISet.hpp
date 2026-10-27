#pragma once

#include "core/ICollectible.hpp"

template <class T>
class ISet : public ICollectible<T> {
public:
    virtual bool Add(const T& value) = 0;
    virtual bool Remove(const T& value) = 0;
    virtual bool Contains(const T& value) const = 0;
    virtual UniquePtr<ISet<T>> Union(const ISet<T>& other) const = 0;
    virtual UniquePtr<ISet<T>> Intersection(const ISet<T>& other) const = 0;
    virtual UniquePtr<ISet<T>> Difference(const ISet<T>& other) const = 0;

    bool IsSubsetOf(const ISet<T>& other) const;

    bool Equals(const ISet<T>& other) const;
};

#include "ISet.tpp"
