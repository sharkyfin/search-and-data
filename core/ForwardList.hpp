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
    Node* tail{nullptr};
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
        for (const T& value : other) PushBack(value);
    }

    ForwardList(ForwardList&& other) noexcept
        : head(Move(other.head)), tail(other.tail), count(other.count) {
        other.tail = nullptr;
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
            tail = other.tail;
            count = other.count;
            other.tail = nullptr;
            other.count = 0;
        }
        return *this;
    }

    ~ForwardList() {
        Clear();
    }

    bool IsEmpty() const noexcept { return head.get() == nullptr; }
    int GetCount() const noexcept { return count; }

    void PushFront(T value) {
        auto node = MakeUnique<Node>(Move(value));
        if (!head) tail = node.get();
        node->next = Move(head);
        head = Move(node);
        ++count;
    }

    void PushBack(T value) {
        auto node = MakeUnique<Node>(Move(value));
        Node* last = node.get();
        if (tail) tail->next = Move(node);
        else head = Move(node);
        tail = last;
        ++count;
    }

    void PopFront() {
        if (!head) {
            throw std::out_of_range("ForwardList is empty.");
        }
        auto next = Move(head->next);
        head = Move(next);
        --count;
        if (!head) tail = nullptr;
    }

    template <class Predicate>
    bool RemoveFirst(Predicate pred) {
        UniquePtr<Node>* curr = &head;
        Node* previous = nullptr;
        while (*curr) {
            if (pred((*curr)->value)) {
                if (curr->get() == tail) tail = previous;
                auto next = Move((*curr)->next);
                *curr = Move(next);
                --count;
                return true;
            }
            previous = curr->get();
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
        tail = nullptr;
    }

    void Swap(ForwardList& other) noexcept {
        head.swap(other.head);
        ::Swap(tail, other.tail);
        ::Swap(count, other.count);
    }

    Iterator begin() noexcept { return Iterator(head.get()); }
    Iterator end() noexcept { return Iterator(nullptr); }

    ConstIterator begin() const noexcept { return ConstIterator(head.get()); }
    ConstIterator end() const noexcept { return ConstIterator(nullptr); }
};
