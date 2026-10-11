#pragma once

#include <stdexcept>

enum class IndexKind { Sorted, Tree, Hash };
enum class SourceKind { Array, TreeSet, Sorted, SortedDictionary, TreeDictionary, HashTable };

inline const char* GetIndexName(IndexKind kind) {
    switch (kind) {
        case IndexKind::Sorted: return "SortedDictionary";
        case IndexKind::Tree: return "TreeDictionary";
        case IndexKind::Hash: return "HashTable";
    }
    throw std::invalid_argument("Неизвестный словарь индекса.");
}
