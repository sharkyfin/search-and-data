#pragma once

#include <stdexcept>

#include "pointers/PointerOperations.hpp"
#include "pointers/UniquePtr.hpp"

template <class T>
class ForwardList {
private:
    struct Node {
        T value;
        UniquePtr<Node> next;

        Node(T val)
            : value(Move(val)) {}
    };

    UniquePtr<Node> head;
    int count{0};

public:
    class Iterator {
    private:
        Node* current{nullptr};

    public:
        Iterator(Node* node) : current(node) {}

        T& operator*() const { return current->value; }

        Iterator& operator++() {
            if (current) current = current->next.get();
            return *this;
        }

        bool operator!=(const Iterator& other) const noexcept {
            return current != other.current;
        }
    };

    class ConstIterator {
    private:
        const Node* current{nullptr};

    public:
        ConstIterator() = default;
        ConstIterator(const Node* node) : current(node) {}

        const T& operator*() const { return current->value; }

        ConstIterator& operator++() {
            if (current) current = current->next.get();
            return *this;
        }

        bool operator!=(const ConstIterator& other) const noexcept {
            return current != other.current;
        }
    };

    ForwardList() = default;

    ForwardList(const ForwardList& other) : ForwardList() {
        UniquePtr<Node>* tail = &head;
        for (const Node* curr = other.head.get(); curr != nullptr; curr = curr->next.get()) {
            *tail = MakeUnique<Node>(curr->value);
            tail = &((*tail)->next);
            ++count;
        }
    }

    ForwardList(ForwardList&& other) noexcept
        : head(Move(other.head)), count(other.count) {
        other.count = 0;
    }

    ForwardList& operator=(const ForwardList& other) {
        if (this != &other) {
            ForwardList copy(other);
            Swap(copy);
        }
        return *this;
    }

    ForwardList& operator=(ForwardList&& other) noexcept {
        if (this != &other) {
            Clear();
            head = Move(other.head);
            count = other.count;
            other.count = 0;
        }
        return *this;
    }

    ~ForwardList() {
        Clear();
    }

    bool IsEmpty() const noexcept { return head == nullptr; }
    int GetCount() const noexcept { return count; }

    void PushFront(T value) {
        auto node = MakeUnique<Node>(Move(value));
        node->next = Move(head);
        head = Move(node);
        ++count;
    }

    void PopFront() {
        if (!head) {
            throw std::out_of_range("ForwardList is empty.");
        }
        auto next = Move(head->next);
        head = Move(next);
        --count;
    }

    template <class Predicate>
    bool RemoveFirst(Predicate pred) {
        UniquePtr<Node>* curr = &head;
        while (*curr) {
            if (pred((*curr)->value)) {
                auto next = Move((*curr)->next);
                *curr = Move(next);
                --count;
                return true;
            }
            curr = &((*curr)->next);
        }
        return false;
    }

    void Clear() noexcept {
        while (head) {
            auto next = Move(head->next);
            head = Move(next);
        }
        count = 0;
    }

    void Swap(ForwardList& other) noexcept {
        head.swap(other.head);
        ::Swap(count, other.count);
    }

    Iterator begin() noexcept { return Iterator(head.get()); }
    Iterator end() noexcept { return Iterator(nullptr); }

    ConstIterator begin() const noexcept { return ConstIterator(head.get()); }
    ConstIterator end() const noexcept { return ConstIterator(nullptr); }
};
