#pragma once

#include <random>

#include "pointers/PointerOperations.hpp"

template <class T, class Compare>
void InsertionSort(T* first, T* last, Compare comp) {
    for (T* i = first + 1; i < last; ++i) {
        T key = Move(*i);
        T* j = i;
        while (j > first && comp(key, *(j - 1))) {
            *j = Move(*(j - 1));
            --j;
        }
        *j = Move(key);
    }
}

template <class T, class Compare>
void QuickSort(T* first, T* last, Compare comp) {
    while (last - first > 16) {
        T* mid = first + (last - first) / 2;
        if (comp(*mid, *first)) Swap(*mid, *first);
        if (comp(*(last - 1), *first)) Swap(*(last - 1), *first);
        if (comp(*(last - 1), *mid)) Swap(*(last - 1), *mid);

        T* pivotPos = last - 2;
        Swap(*mid, *pivotPos);
        const T& pivot = *pivotPos;

        T* i = first;
        T* j = last - 2;
        while (true) {
            while (comp(*(++i), pivot)) {}
            while (j > first && comp(pivot, *(--j))) {}
            if (i >= j) break;
            Swap(*i, *j);
        }
        Swap(*i, *pivotPos);

        if (i - first < last - (i + 1)) {
            QuickSort(first, i, comp);
            first = i + 1;
        } else {
            QuickSort(i + 1, last, comp);
            last = i;
        }
    }
    InsertionSort(first, last, comp);
}

template <class T>
void Reverse(T* first, T* last) {
    if (first >= last) return;
    --last;
    while (first < last) {
        Swap(*first, *last);
        ++first;
        --last;
    }
}

template <class T, class URBG>
void Shuffle(T* first, T* last, URBG& g) {
    const int n = static_cast<int>(last - first);
    for (int i = n - 1; i > 0; --i) {
        std::uniform_int_distribution<int> dist(0, i);
        const int j = dist(g);
        Swap(first[i], first[j]);
    }
}
