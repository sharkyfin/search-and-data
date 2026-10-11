#pragma once

#include <stdexcept>

#include "pointers/PointerOperations.hpp"
#include "pointers/UniquePtr.hpp"
#include "core/ICollectible.hpp"
#include "sequences/ArraySequence.hpp"
#include "dictionaries/IDictionary.hpp"

template <class Record, class Key>
class Index {
private:
    UniquePtr<IDictionary<Key, ArraySequence<const Record*>>> entries;

public:
    template <class Selector>
    Index(const ICollectible<Record>& source,
          UniquePtr<IDictionary<Key, ArraySequence<const Record*>>> dictionary, Selector select)
        : entries(Move(dictionary)) {
        if (!entries || !entries->IsEmpty()) {
            throw std::invalid_argument("Индексу нужен пустой словарь.");
        }
        auto enumerator = source.GetEnumerator();
        while (enumerator->MoveNext()) {
            const Record* record = &enumerator->CurrentRef();
            const Key key = select(*record);
            auto* matches = entries->Find(key);
            if (!matches) {
                entries->Add(key, {});
                matches = entries->Find(key);
            }
            matches->Append(record);
        }
    }

    template <class Selector>
    Index(const ICollectible<Record>&&,
          UniquePtr<IDictionary<Key, ArraySequence<const Record*>>>, Selector) = delete;

    int GetKeyCount() const {
        return entries->GetCount();
    }

    UniquePtr<IEnumerator<Pair<Key, ArraySequence<const Record*>>>> GetEntriesEnumerator() const {
        return entries->GetEnumerator();
    }

    const ArraySequence<const Record*>& FindAll(const Key& key) const {
        const IDictionary<Key, ArraySequence<const Record*>>& dictionary = *entries;
        const auto* matches = dictionary.Find(key);
        static const ArraySequence<const Record*> empty;
        return matches ? *matches : empty;
    }
};
