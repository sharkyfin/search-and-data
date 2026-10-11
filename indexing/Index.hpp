#pragma once

#include <stdexcept>

#include "pointers/PointerOperations.hpp"
#include "pointers/UniquePtr.hpp"
#include "core/ICollectible.hpp"
#include "sequences/ArraySequence.hpp"
#include "dictionaries/IDictionary.hpp"
#include "indexing/RecordId.hpp"

template <class Record, class Key>
class Index {
private:
    ArraySequence<Record> records;
    UniquePtr<IDictionary<Key, ArraySequence<RecordId>>> entries;

public:
    template <class Selector>
    Index(const ICollectible<Record>& source,
          UniquePtr<IDictionary<Key, ArraySequence<RecordId>>> dictionary, Selector select)
        : records(source), entries(Move(dictionary)) {
        if (!entries || !entries->IsEmpty()) {
            throw std::invalid_argument("Индексу нужен пустой словарь.");
        }
        for (int position = 0; position < records.GetCount(); ++position) {
            const Key key = select(records.Get(position));
            ArraySequence<RecordId>* matches = entries->Find(key);
            if (matches) {
                matches->Append(RecordId{position});
            } else {
                ArraySequence<RecordId> positions;
                positions.Append(RecordId{position});
                entries->Add(key, positions);
            }
        }
    }

    const ArraySequence<Record>& GetRecords() const {
        return records;
    }

    int GetKeyCount() const {
        return entries->GetCount();
    }

    UniquePtr<IEnumerator<Pair<Key, ArraySequence<RecordId>>>> GetEntriesEnumerator() const {
        return entries->GetEnumerator();
    }

    ArraySequence<RecordId> FindAll(const Key& key) const {
        const IDictionary<Key, ArraySequence<RecordId>>& dictionary = *entries;
        const ArraySequence<RecordId>* matches = dictionary.Find(key);
        return matches ? *matches : ArraySequence<RecordId>();
    }
};
