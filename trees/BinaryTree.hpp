#pragma once

#include <stdexcept>

#include "pointers/PointerOperations.hpp"
#include "pointers/UniquePtr.hpp"
#include "core/Functional.hpp"
#include "core/IEnumerator.hpp"
#include "sequences/DynamicArray.hpp"

template <class T, class Compare = Less<T>>
class BinaryTree {
private:
    struct Node {
        T value;
        UniquePtr<Node> left;
        UniquePtr<Node> right;
        int height{1};

        Node(T val)
            : value(Move(val)) {}
    };

    UniquePtr<Node> root;
    int count{0};
    Compare compare;

    static int Height(const UniquePtr<Node>& node) noexcept {
        return node ? node->height : 0;
    }

    static void Update(Node& node) noexcept {
        const int lh = Height(node.left);
        const int rh = Height(node.right);
        node.height = 1 + (lh > rh ? lh : rh);
    }

    static void RotateLeft(UniquePtr<Node>& node) {
        auto next = Move(node->right);
        node->right = Move(next->left);
        Update(*node);
        next->left = Move(node);
        Update(*next);
        node = Move(next);
    }

    static void RotateRight(UniquePtr<Node>& node) {
        auto next = Move(node->left);
        node->left = Move(next->right);
        Update(*node);
        next->right = Move(node);
        Update(*next);
        node = Move(next);
    }

    static void Balance(UniquePtr<Node>& node) {
        if (!node) {
            return;
        }
        const int lh = Height(node->left);
        const int rh = Height(node->right);
        node->height = 1 + (lh > rh ? lh : rh);
        const int difference = lh - rh;
        if (difference > 1) {
            if (Height(node->left->left) < Height(node->left->right)) {
                RotateLeft(node->left);
            }
            RotateRight(node);
        } else if (difference < -1) {
            if (Height(node->right->right) < Height(node->right->left)) {
                RotateRight(node->right);
            }
            RotateLeft(node);
        }
    }

    template <class U>
    bool Insert(UniquePtr<Node>& node, U&& value) {
        if (!node) {
            node = MakeUnique<Node>(static_cast<U&&>(value));
            return true;
        }
        bool inserted = false;
        if (compare(value, node->value)) {
            inserted = Insert(node->left, static_cast<U&&>(value));
        } else if (compare(node->value, value)) {
            inserted = Insert(node->right, static_cast<U&&>(value));
        } else {
            return false;
        }
        Balance(node);
        return inserted;
    }

    static UniquePtr<Node> ExtractMinimum(UniquePtr<Node>& node) {
        if (!node->left) {
            auto minimum = Move(node);
            node = Move(minimum->right);
            return minimum;
        }
        auto minimum = ExtractMinimum(node->left);
        Balance(node);
        return minimum;
    }

    template <class Key, class Order>
    bool Erase(UniquePtr<Node>& node, const Key& key, const Order& order) {
        if (!node) {
            return false;
        }
        const int direction = order(key, node->value);
        bool removed = true;
        if (direction < 0) {
            removed = Erase(node->left, key, order);
        } else if (direction > 0) {
            removed = Erase(node->right, key, order);
        } else if (!node->left) {
            auto next = Move(node->right);
            node = Move(next);
        } else if (!node->right) {
            auto next = Move(node->left);
            node = Move(next);
        } else {
            auto next = ExtractMinimum(node->right);
            next->left = Move(node->left);
            next->right = Move(node->right);
            node = Move(next);
        }
        Balance(node);
        return removed;
    }

    template <class Key, class Order>
    Node* FindNode(const Key& key, const Order& order) const {
        Node* node = root.get();
        while (node) {
            const int direction = order(key, node->value);
            if (direction == 0) {
                return node;
            }
            node = direction < 0 ? node->left.get() : node->right.get();
        }
        return nullptr;
    }

    int Check(const Node* node, const T* lower, const T* upper, int& visited) const {
        if (!node) {
            return 0;
        }
        if ((lower && !compare(*lower, node->value)) || (upper && !compare(node->value, *upper))) {
            return -1;
        }
        ++visited;
        const int left = Check(node->left.get(), lower, &node->value, visited);
        const int right = Check(node->right.get(), &node->value, upper, visited);
        const int diff = left > right ? left - right : right - left;
        const int maxH = left > right ? left : right;
        if (left < 0 || right < 0 || diff > 1 || node->height != 1 + maxH) {
            return -1;
        }
        return node->height;
    }

    class Enumerator : public IEnumerator<T> {
    private:
        DynamicArray<const Node*> pending;
        const Node* current{nullptr};

        void PushLeft(const Node* node) {
            while (node) {
                pending.Append(node);
                node = node->left.get();
            }
        }

    public:
        Enumerator(const Node* root) {
            PushLeft(root);
        }

        bool MoveNext() override {
            if (pending.IsEmpty()) {
                current = nullptr;
                return false;
            }
            current = pending.Back();
            pending.PopBack();
            PushLeft(current->right.get());
            return true;
        }

        const T& CurrentRef() const override {
            if (!current) {
                throw std::logic_error("Enumerator has no current element.");
            }
            return current->value;
        }
    };

    struct DefaultOrder {
        const Compare& compare;

        int operator()(const T& first, const T& second) const noexcept {
            return compare(first, second) ? -1 : (compare(second, first) ? 1 : 0);
        }
    };

public:
    BinaryTree(Compare compare = Compare())
        : compare(Move(compare)) {}

    BinaryTree(const BinaryTree&) = delete;
    BinaryTree& operator=(const BinaryTree&) = delete;

    int GetCount() const noexcept {
        return count;
    }

    int GetHeight() const noexcept {
        return Height(root);
    }

    const Compare& GetCompare() const noexcept {
        return compare;
    }

    bool IsValid() const {
        int visited = 0;
        return Check(root.get(), nullptr, nullptr, visited) >= 0 && visited == count;
    }

    template <class U>
    bool Add(U&& value) {
        if (!Insert(root, static_cast<U&&>(value))) {
            return false;
        }
        ++count;
        return true;
    }

    bool Remove(const T& value) {
        return RemoveBy(value, DefaultOrder{compare});
    }

    T* Find(const T& value) {
        return FindBy(value, DefaultOrder{compare});
    }

    const T* Find(const T& value) const {
        return FindBy(value, DefaultOrder{compare});
    }

    template <class Key, class Order>
    bool RemoveBy(const Key& key, const Order& order) {
        if (!Erase(root, key, order)) {
            return false;
        }
        --count;
        return true;
    }

    template <class Key, class Order>
    T* FindBy(const Key& key, const Order& order) {
        Node* node = FindNode(key, order);
        return node ? &node->value : nullptr;
    }

    template <class Key, class Order>
    const T* FindBy(const Key& key, const Order& order) const {
        const Node* node = FindNode(key, order);
        return node ? &node->value : nullptr;
    }

    UniquePtr<IEnumerator<T>> GetEnumerator() const {
        return MakeUnique<Enumerator>(root.get());
    }
};
