#pragma once

#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

#include "core/ForwardList.hpp"
#include "core/Functional.hpp"
#include "core/ICollectible.hpp"
#include "core/IEnumerable.hpp"
#include "core/Text.hpp"

enum class Traversal { KLP, KPL, LPK, LKP, PLK, PKL, BreadthFirst };

template <class T, class Compare = Less<T>>
class BinaryTree : public ICollectible<T> {
private:
    struct Node {
        T value;
        UniquePtr<Node> left;
        UniquePtr<Node> right;
        int height{1};

        Node(T val);
    };

    UniquePtr<Node> root;
    int count{0};
    Compare compare;
    bool allowDuplicates{false};
    int version{0};

    static int Height(const UniquePtr<Node>& node) noexcept;

    static void Update(Node& node) noexcept;

    static void RotateLeft(UniquePtr<Node>& node);

    static void RotateRight(UniquePtr<Node>& node);

    static void Balance(UniquePtr<Node>& node);

    void Invalidate() noexcept;

    template <class U>
    bool Insert(UniquePtr<Node>& node, U&& value);

    static UniquePtr<Node> ExtractMinimum(UniquePtr<Node>& node);

    static UniquePtr<Node> ExtractMaximum(UniquePtr<Node>& node);

    template <class Key>
    bool Erase(UniquePtr<Node>& node, const Key& key);

    template <class Key>
    Node* FindNode(const Key& key) const;

    static UniquePtr<Node> Clone(const Node* node, int& copied);

    int Check(const Node* node, const T* lower, const T* upper, int& visited) const;

    bool Same(const Node* first, const Node* second) const;

    bool ContainsNode(const Node* node, const Node* candidate) const;

    static const T& Follow(const Node* node, const std::string& path);

    class Enumerator : public IEnumerator<T> {
    private:
        struct Frame { const Node* node; int stage; };
        const BinaryTree& owner;
        int version;
        Traversal order;
        ForwardList<Frame> pending;
        const Node* current{nullptr};

        void CheckVersion() const;

    public:
        Enumerator(const BinaryTree& owner, Traversal order);

        bool MoveNext() override;

        const T& CurrentRef() const override;
    };

public:
    BinaryTree(Compare compare = Compare(), bool allowDuplicates = false);

    BinaryTree(const BinaryTree& other);

    BinaryTree(BinaryTree&& other);

    BinaryTree& operator=(const BinaryTree& other);

    BinaryTree& operator=(BinaryTree&& other);

    void Swap(BinaryTree& other);

    int GetCount() const override;
    int GetHeight() const noexcept;
    const Compare& GetCompare() const noexcept;

    bool IsValid() const;

    template <class U>
    bool Add(U&& value);

    template <class Key>
    bool Remove(const Key& key);

    template <class Key>
    T* Find(const Key& key);

    template <class Key>
    const T* Find(const Key& key) const;

    void Clear() noexcept;

    const T& Maximum() const;

    T PopMaximum();

    UniquePtr<IEnumerator<T>> GetEnumerator() const override;

    UniquePtr<IEnumerator<T>> GetEnumerator(Traversal order) const;

    template <class Transform>
    BinaryTree Map(Transform transform, Traversal order = Traversal::LKP) const;

    template <class Predicate>
    BinaryTree Where(Predicate predicate, Traversal order = Traversal::LKP) const;

    template <class Accumulator, class Function>
    Accumulator Reduce(Accumulator initial, Function function, Traversal order = Traversal::LKP) const;

    BinaryTree Merge(const IEnumerable<T>& other) const;

    BinaryTree ExtractSubtree(const T& value) const;

    bool ContainsSubtree(const BinaryTree& other) const;

    const T& GetByPath(const std::string& path) const;
    const T& GetByPath(const T& start, const std::string& path) const;

    template <class Writer>
    std::string ToString(Writer write) const;

    std::string ToString() const;

    template <class Reader>
    static BinaryTree FromString(const std::string& text, Reader read,
        Compare compare = Compare(), bool allowDuplicates = false);

    static BinaryTree FromString(const std::string& text, Compare compare = Compare(), bool allowDuplicates = false);
};

#include "BinaryTree.tpp"
