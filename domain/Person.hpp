#pragma once

#include <algorithm>
#include <random>
#include <string>

#include "sequences/ArraySequence.hpp"
#include "core/Pair.hpp"

struct Person {
    int id{0};
    std::string firstName;
    std::string lastName;
    int birthYear{0};

    bool operator<(const Person& other) const {
        return id < other.id;
    }
};

inline const Person& GetPerson(const Person& person) { return person; }
inline const Person& GetPerson(const Pair<int, Person>& entry) { return entry.value; }

inline std::string ByLastName(const Person& person) {
    return person.lastName;
}

inline int ByBirthYear(const Person& person) {
    return person.birthYear;
}

inline ArraySequence<Person> GeneratePeople(int count, unsigned int seed) {
    const char* const firstNames[]{
        "Alex", "Anna", "Daria", "Ivan", "Mark", "Maria", "Nikita", "Sofia"
    };
    const char* const lastNames[]{
        "Ivanov", "Petrov", "Sokolov", "Volkov", "Orlov", "Morozov", "Smirnov", "Belov"
    };
    std::mt19937 random(seed);
    std::uniform_int_distribution<int> name(0, 7);
    std::uniform_int_distribution<int> year(1960, 2010);
    const int maxGroup = std::max(1, count / 32) - 1;
    std::uniform_int_distribution<int> group(0, maxGroup);
    ArraySequence<Person> people;
    people.Reserve(count);
    for (int position = 0; position < count; ++position) {
        const std::string first = firstNames[name(random)];
        std::string last = lastNames[name(random)];
        const int suffix = group(random);
        if (suffix != 0) {
            last += " " + std::to_string(suffix);
        }
        people.Append(Person{position + 1, first, last, year(random)});
    }
    return people;
}
