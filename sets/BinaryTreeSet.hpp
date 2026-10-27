#pragma once

#include "sets/ISet.hpp"
#include "trees/BinaryTree.hpp"

template <class T, class Compare = Less<T>>
class BinaryTreeSet : public ISet<T> {
private:
    enum class Operation { Union, Intersection, Difference };
    BinaryTree<T, Compare> tree;

    BinaryTreeSet Combine(const ISet<T>& other, Operation operation) const;

public:
    BinaryTreeSet(Compare compare = Compare());

    int GetCount() const override;
    bool Add(const T& value) override;
    bool Remove(const T& value) override;
    bool Contains(const T& value) const override;
    bool IsValid() const;
    int GetHeight() const;
    void Clear();

    UniquePtr<ISet<T>> Union(const ISet<T>& other) const override;

    UniquePtr<ISet<T>> Intersection(const ISet<T>& other) const override;

    UniquePtr<ISet<T>> Difference(const ISet<T>& other) const override;

    BinaryTreeSet operator|(const ISet<T>& other) const;
    BinaryTreeSet operator&(const ISet<T>& other) const;
    BinaryTreeSet operator-(const ISet<T>& other) const;
    bool operator<=(const ISet<T>& other) const;
    bool operator==(const ISet<T>& other) const;
    bool operator!=(const ISet<T>& other) const;

    template <class Transform>
    BinaryTreeSet Map(Transform transform) const;

    template <class Predicate>
    BinaryTreeSet Where(Predicate predicate) const;

    template <class Accumulator, class Function>
    Accumulator Reduce(Accumulator initial, Function function) const;

    template <class Writer>
    std::string ToString(Writer write) const;

    std::string ToString() const;

    template <class Reader>
    static BinaryTreeSet FromString(const std::string& text, Reader read, Compare compare = Compare());

    static BinaryTreeSet FromString(const std::string& text, Compare compare = Compare());

    UniquePtr<IEnumerator<T>> GetEnumerator() const override;
};

#include "BinaryTreeSet.tpp"
