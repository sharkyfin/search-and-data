#pragma once

#include <stdexcept>

#include "domain/Person.hpp"
#include "dictionaries/HashTable.hpp"
#include "dictionaries/SortedDictionary.hpp"
#include "dictionaries/TreeDictionary.hpp"
#include "sequences/SortedArraySequence.hpp"
#include "sets/BinaryTreeSet.hpp"

inline constexpr const char* PeopleSourceNames[]{
    "ArraySequence", "BinaryTreeSet", "SortedArraySequence",
    "SortedDictionary", "TreeDictionary", "HashTable"
};

// Источник живёт до завершения action, поэтому указатели индексов остаются действительными.
template <class Load, class Action>
void UsePeopleSource(int choice, Load load, Action action) {
    if (choice < 1 || choice > 6) throw std::invalid_argument("Выберите источник от 1 до 6.");
    if (choice == 1) {
        ArraySequence<Person> people;
        load([&](const Person& person) { people.Append(person); });
        action(static_cast<const ICollectible<Person>&>(people));
    } else if (choice == 2) {
        BinaryTreeSet<Person> people;
        load([&](const Person& person) { people.Add(person); });
        action(static_cast<const ICollectible<Person>&>(people));
    } else if (choice == 3) {
        SortedArraySequence<Person> people;
        load([&](const Person& person) { people.Add(person); });
        action(static_cast<const ICollectible<Person>&>(people));
    } else {
        UniquePtr<IDictionary<int, Person>> people;
        if (choice == 4) people = MakeUnique<SortedDictionary<int, Person>>();
        else if (choice == 5) people = MakeUnique<TreeDictionary<int, Person>>();
        else people = MakeUnique<HashTable<int, Person>>();
        load([&](const Person& person) { people->Add(person.id, person); });
        action(static_cast<const ICollectible<Pair<int, Person>>&>(*people));
    }
}
